#include "FRenderResourceLibrary.h"
#include "Runtime/Resource/FResourceLoader.h"

#include "FRenderer.h"
#include "FTexture.h"
#include <d3dcompiler.h>
#include "Runtime/Core/TArray.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Material/FBlendDesc.h"
#include "Runtime/Engine/FArchive.h"
#include "ThirdParty/Json/json.hpp"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

FRenderResourceLibrary &FRenderResourceLibrary::Get() {
  static FRenderResourceLibrary Instance;
  return Instance;
}

// 파이프라인 정보 엔트리
struct FPipelineEntry {
  FName Id;
  const wchar_t *VertexShader;
  const wchar_t *PixelShader;
  bool bDepthWrite = true;
  D3D11_CULL_MODE CullMode = D3D11_CULL_BACK;
  EBlendMode BlendMode = EBlendMode::Opaque;
  bool bIsInstancing = false;
};

// 기본 파이프라인 테이블
const FPipelineEntry pipelineTable[] = {
    {
        .Id = FName("Simple_Solid"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = FName("Simple_Line"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = FName("Grid"),
        .VertexShader = L"GridVS.cso",
        .PixelShader = L"GridPS.cso",
        .bDepthWrite = false,
        .CullMode = D3D11_CULL_NONE,
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = FName("Textured"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"TexturedPS.cso",
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = FName("Billboard"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"TexturedPS.cso",
        .bDepthWrite = true,
        //.CullMode = D3D11_CULL_NONE,
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = FName("RotationGizmo"),
        .VertexShader = L"RotationGizmoVS.cso",
        .PixelShader = L"RotationGizmoPS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = FName("Spotlight"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"SpotlightPS.cso",
        .bDepthWrite = false,
        .CullMode = D3D11_CULL_NONE,
        .BlendMode = EBlendMode::Additive,
    },
    {
        .Id = FName("Text"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"MsdfTextPS.cso",
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = FName("Instance_Text"),
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"MsdfTextPS.cso",
        .BlendMode = EBlendMode::Translucent,
        .bIsInstancing = true,
    },
    {
        .Id = FName("Instance_Simple"),
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
        .bIsInstancing = true,
    },
    {
        .Id = FName("Instance_Textured"),
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"TexturedPS.cso",
        .CullMode = D3D11_CULL_NONE,
        .BlendMode = EBlendMode::Translucent,
        .bIsInstancing = true,
    },
    {
        .Id = FName("Gizmo"),
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = FName("SelectedActor_Text"),
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"MsdfTextPS.cso",
        .bDepthWrite = false,
        .BlendMode = EBlendMode::Translucent,
        .bIsInstancing = true,
    },
};

// 머티리얼 정보 엔트리
struct FMaterialEntry {
  FName Id;
  FName PipelineID;
  const char *TextureName = nullptr;
};

// 기본 머티리얼 테이블
const FMaterialEntry materialTable[] = {
    {
        .Id = FName("Simple"),
        .PipelineID = FName("Simple_Solid"),
    },
    {
        .Id = FName("RotGizmo"),
        .PipelineID = FName("RotationGizmo"),
    },
    {
        .Id = FName("Spotlight"),
        .PipelineID = FName("Spotlight"),
    },
    {
        .Id = FName("Text"),
        .PipelineID = FName("Text"),
        .TextureName = "bazziotf",
    },
    {
        .Id = FName("Textured"),
        .PipelineID = FName("Textured"),
        .TextureName = "masteryi_head",
    },
    {
        .Id = FName("Billboard"),
        .PipelineID = FName("Billboard"),
        .TextureName = "uv-test",
    },
    {
        .Id = FName("Instance_Text_Bazzi"),
        .PipelineID = FName("Instance_Text"),
        .TextureName = "bazziotf",
    },
    {
        .Id = FName("Instance_Text_DNF"),
        .PipelineID = FName("Instance_Text"),
        .TextureName = "dnfbitbitv2",
    },
    {
        .Id = FName("Instance_Text_Maple"),
        .PipelineID = FName("Instance_Text"),
        .TextureName = "maplestorybold",
    },
    {
        .Id = FName("Instance_Simple"),
        .PipelineID = FName("Instance_Simple"),
    },
    {
        .Id = FName("Instance_Textured"),
        .PipelineID = FName("Instance_Textured"),
        .TextureName = "masteryi_head",
    },
    {
        .Id = FName("Gizmo"),
        .PipelineID = FName("Gizmo"),
    },
    {
        .Id = FName("Outline"),
        .PipelineID = FName("Outline"),
    },
    {
        .Id = FName("SelectedActor_Text"),
        .PipelineID = FName("SelectedActor_Text"),
        .TextureName = "bazziotf",
    },
};

bool FRenderResourceLibrary::CreateSolidWireframePipeline(FRenderer &Renderer) {
  const FWString Path = GetExecutableDirectory();
  const FWString VsPath = Path + L"/Shader/ExampleVS.cso";
  const FWString PsPath = Path + L"/Shader/ExamplePS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  FRenderPipelineDesc Desc = {
      .VertexShaderFileName = VsPath,
      .PixelShaderFileName = PsPath,
      .bEnableDepthTest = true,
  };

  // 솔리드 파이프라인 생성 및 등록
  TSharedPtr<FRenderPipeline> SolidPipeline =
      Renderer.CreateRenderPipeline(Desc, EViewModeIndex::VMI_Lit);
  if (SolidPipeline) {
    AllPipelineMap[FName("Simple_Solid")] = SolidPipeline;
  }

  // 와이어프레임 파이프라인 생성 및 등록
  TSharedPtr<FRenderPipeline> WireframePipeline =
      Renderer.CreateRenderPipeline(Desc, EViewModeIndex::VMI_Wireframe);
  if (WireframePipeline) {
    AllPipelineMap[FName("Simple_Wireframe")] = WireframePipeline;
  }

  return SolidPipeline != nullptr && WireframePipeline != nullptr;
}

bool FRenderResourceLibrary::CreateOutlinePipeline(FRenderer &Renderer) {
  ID3D11Device *Device = Renderer.GetDevice();
  if (!Device) {
    return false;
  }

  const FWString Path = GetExecutableDirectory();
  const FWString VsPath = Path + L"/Shader/ExampleVS.cso";
  const FWString PsPath = Path + L"/Shader/ExamplePS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  auto Pipeline = std::make_shared<FRenderPipeline>();

  // 버텍스 셰이더 로드 및 생성
  Microsoft::WRL::ComPtr<ID3DBlob> Blob;
  HRESULT Result = D3DReadFileToBlob(VsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateVertexShader(Blob->GetBufferPointer(),
                                      Blob->GetBufferSize(), nullptr,
                                      &Pipeline->VertexShader);
  if (FAILED(Result)) {
    return false;
  }

  // 입력 레이아웃 생성
  Result = Device->CreateInputLayout(FVertexLayouts::Layout,
                                     FVertexLayouts::NumElements,
                                     Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(),
                                     &Pipeline->InputLayout);
  if (FAILED(Result)) {
    return false;
  }

  // 픽셀 셰이더 로드 및 생성
  Result = D3DReadFileToBlob(PsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreatePixelShader(Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(), nullptr,
                                     &Pipeline->PixelShader);
  if (FAILED(Result)) {
    return false;
  }

  // 래스터라이저 상태 생성
  D3D11_RASTERIZER_DESC RasterizerDesc{
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_NONE,
      .FrontCounterClockwise = false,
  };
  Result = Device->CreateRasterizerState(&RasterizerDesc,
                                         &Pipeline->RasterizerState);
  if (FAILED(Result)) {
    return false;
  }

  // 스텐실 마스크 기록 설정
  D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};
  DepthStencilDesc.DepthEnable = FALSE;
  DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
  DepthStencilDesc.StencilEnable = TRUE;
  DepthStencilDesc.StencilReadMask = 0xFF;
  DepthStencilDesc.StencilWriteMask = 0xFF;
  DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
  DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  DepthStencilDesc.BackFace = DepthStencilDesc.FrontFace;
  Result = Device->CreateDepthStencilState(&DepthStencilDesc,
                                           &Pipeline->DepthStencilState);
  if (FAILED(Result)) {
    return false;
  }

  // 블렌드 상태 생성
  D3D11_BLEND_DESC BlendDesc{};
  BlendDesc.RenderTarget[0].BlendEnable = FALSE;
  BlendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
  Result = Device->CreateBlendState(&BlendDesc, &Pipeline->BlendState);
  if (FAILED(Result)) {
    return false;
  }

  // 샘플러 상태 생성
  D3D11_SAMPLER_DESC SamplerDesc{
      .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };
  Result = Device->CreateSamplerState(&SamplerDesc, &Pipeline->SamplerState);
  if (FAILED(Result)) {
    return false;
  }

  AllPipelineMap[FName("Outline")] = Pipeline;
  return true;
}

bool FRenderResourceLibrary::CreatePostProcessPipeline(FRenderer &Renderer) {
  ID3D11Device *Device = Renderer.GetDevice();
  if (!Device) {
    return false;
  }

  const FWString Path = GetExecutableDirectory();
  const FWString VsPath = Path + L"/Shader/ScreenQuadVS.cso";
  const FWString PsPath = Path + L"/Shader/OutlinePostProcessPS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  auto Pipeline = std::make_shared<FRenderPipeline>();

  // 버텍스 셰이더 로드 및 생성
  Microsoft::WRL::ComPtr<ID3DBlob> Blob;
  HRESULT Result = D3DReadFileToBlob(VsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateVertexShader(Blob->GetBufferPointer(),
                                      Blob->GetBufferSize(), nullptr,
                                      &Pipeline->VertexShader);
  if (FAILED(Result)) {
    return false;
  }

  // 픽셀 셰이더 로드 및 생성
  Result = D3DReadFileToBlob(PsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreatePixelShader(Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(), nullptr,
                                     &Pipeline->PixelShader);
  if (FAILED(Result)) {
    return false;
  }

  // 래스터라이저 상태 생성
  D3D11_RASTERIZER_DESC RasterizerDesc{
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_NONE,
      .FrontCounterClockwise = false,
  };
  Result = Device->CreateRasterizerState(&RasterizerDesc,
                                         &Pipeline->RasterizerState);
  if (FAILED(Result)) {
    return false;
  }

  // 깊이 스텐실 상태 생성
  D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{
      .DepthEnable = FALSE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
      .DepthFunc = D3D11_COMPARISON_ALWAYS,
      .StencilEnable = FALSE,
  };
  Result = Device->CreateDepthStencilState(&DepthStencilDesc,
                                           &Pipeline->DepthStencilState);
  if (FAILED(Result)) {
    return false;
  }

  // 블렌드 상태 생성
  D3D11_BLEND_DESC BlendDesc{};
  BlendDesc.RenderTarget[0].BlendEnable = FALSE;
  BlendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;
  Result = Device->CreateBlendState(&BlendDesc, &Pipeline->BlendState);
  if (FAILED(Result)) {
    return false;
  }

  // 샘플러 상태 생성
  D3D11_SAMPLER_DESC SamplerDesc{
      .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
      .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };
  Result = Device->CreateSamplerState(&SamplerDesc, &Pipeline->SamplerState);
  if (FAILED(Result)) {
    return false;
  }

  AllPipelineMap[FName("PostProcess")] = Pipeline;
  return true;
}

bool FRenderResourceLibrary::InitializePipelines(FRenderer &Renderer) {
  // 솔리드 및 와이어프레임 파이프라인 개별 생성
  CreateSolidWireframePipeline(Renderer);
  CreateOutlinePipeline(Renderer);
  CreatePostProcessPipeline(Renderer);

  const FWString Path = GetExecutableDirectory();

  for (const FPipelineEntry &Entry : pipelineTable) {
    if (AllPipelineMap.find(Entry.Id) != AllPipelineMap.end()) {
      continue;
    }

    const FWString VsPath = Path + L"/Shader/" + Entry.VertexShader;
    const FWString PsPath = Path + L"/Shader/" + Entry.PixelShader;

    if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
      continue;
    }

    FRenderPipelineDesc PipelineDesc = {
        .VertexShaderFileName = VsPath,
        .PixelShaderFileName = PsPath,
        .bEnableDepthTest = true,
        .bEnableDepthWrite = Entry.bDepthWrite,
        .CullMode = Entry.CullMode,
        .BlendMode = Entry.BlendMode,
        .bIsInstancing = Entry.bIsInstancing,
    };

    TSharedPtr<FRenderPipeline> Pipeline =
        Renderer.CreateRenderPipeline(PipelineDesc, EViewModeIndex::VMI_Lit);
    if (!Pipeline) {
      return false;
    }

    AllPipelineMap[Entry.Id] = Pipeline;
  }
  return true;
}

bool FRenderResourceLibrary::Initialize(FRenderer &Renderer) {
  RendererRef = &Renderer;
  if (!InitializePipelines(Renderer)) { // 파이프라인을 먼저 생성해야 뒤에 material 할당가능
    return false;
  }

  if (!InitializeMaterials(Renderer) || !CreateInstancingArrayMap()) {
    return false;
  }

  return true;
}

bool FRenderResourceLibrary::CreateInstancingArrayMap() {
  AllInstancingArrayMap.clear();
  // 기본 배치 키 등록
  AllInstancingArrayMap[{FName("Instance_Text"), FName("Rect")}] = {};
  AllInstancingArrayMap[{FName("Instance_Simple"), FName("Cube")}] = {};
  AllInstancingArrayMap[{FName("Instance_Textured"), FName("MasterYi")}] = {};
  AllInstancingArrayMap[{FName("SelectedActor_Text"), FName("Rect")}] = {};
  
  return true;
}

bool FRenderResourceLibrary::InitializeMaterials(FRenderer &Renderer) {
  for (const auto &Entry : materialTable) {
    TSharedPtr<FMaterial> Material = std::make_shared<FMaterial>();

    TSharedPtr<FRenderPipeline> Pipeline = GetPipeline(Entry.PipelineID);
    if (Pipeline) {
      Material->SetPipeLine(Pipeline);
    }

    if (Entry.TextureName) {
      Material->SetTexture(GetTexture(Entry.TextureName));
    }

    RegisterMaterial(Entry.Id, Material);
  }
  return true;
}

TSharedPtr<FMaterial> FRenderResourceLibrary::RegisterMaterial(const FName& Id, TSharedPtr<FMaterial> inMaterial) {
  if (inMaterial) {
    inMaterial->MaterialId = Id;
  }
  AllMaterialMap[Id] = inMaterial;
  return inMaterial;
}

bool FRenderResourceLibrary::CreateTextures(FRenderer &Renderer) 
{
  const std::filesystem::path ExeDir(GetExecutableDirectory());
  const std::filesystem::path ProjectRoot =
      ExeDir.parent_path().parent_path().parent_path();

  TArray<std::filesystem::path> SearchRoots = {
      ProjectRoot / L"Textures",
      std::filesystem::current_path() / L"Textures",
      ExeDir / L"Textures",
  };

  for (const auto &Root : SearchRoots) {
    std::error_code Ec;
    if (!std::filesystem::exists(Root, Ec)) {
      continue;
    }

    for (const auto &Entry :
         std::filesystem::recursive_directory_iterator(Root, Ec)) {
      if (!Entry.is_regular_file(Ec))
        continue;

      FWString Ext = Entry.path().extension().wstring();
      std::transform(Ext.begin(), Ext.end(), Ext.begin(), ::towlower);
      if (Ext != L".dds" && Ext != L".jpg" && Ext != L".jpeg")
        continue;

      // 확장자 제거
      FString KeyWide = Entry.path().stem().string();
      std::transform(KeyWide.begin(), KeyWide.end(), KeyWide.begin(),
                     ::tolower);
      FName TextureKey(KeyWide);

      // 이미 로드된 텍스처 건너뜀
      if (AllTextureMap.find(TextureKey) != AllTextureMap.end()) {
        continue;
      }
  
      TSharedPtr<FTexture> Texture = Renderer.CreateTexture(Entry.path().wstring().c_str());

      if (!Texture)
        continue;

      RegisterTexture(TextureKey, Texture);
    }
  }

  return true;
}

TSharedPtr<FMesh> FRenderResourceLibrary::GetOrCreateMesh(const FName &ID, const TArray<FVertexData> &vertices) {
  auto it = AllMeshMap.find(ID);
  if (it != AllMeshMap.end())
    return it->second;

  FMeshDesc Desc{.VertexData = vertices.data(),
                 .VertexDataSize =
                     static_cast<uint32>(sizeof(FVertexData) * vertices.size()),
                 .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
                 .VertexCount = static_cast<uint32>(vertices.size())};
  TSharedPtr<FMesh> newMesh =
      RendererRef ? RendererRef->CreateMesh(Desc) : nullptr;
  if (newMesh) {
    AllMeshMap[ID] = newMesh;
  }
  return newMesh;
}