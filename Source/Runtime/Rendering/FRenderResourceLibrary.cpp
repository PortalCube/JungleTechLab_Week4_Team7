#include "FRenderResourceLibrary.h"
#include "Vertices.h"

#include "FRenderer.h"
#include "FTexture.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Geometry/Sphere.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Rendering/FRenderer.h"
#include <cmath>
#include <numbers>

#define STB_IMAGE_IMPLEMENTATION

#include "ThirdParty/stb/stb_image.h"


FRenderResourceLibrary &FRenderResourceLibrary::Get() {
  static FRenderResourceLibrary Instance;
  return Instance;
}

// 파이프라인 정보 엔트리
struct FPipelineEntry {
  EPipelineID Id;
  const wchar_t *VertexShader;
  const wchar_t *PixelShader;
  bool bDepthWrite = true;
  D3D11_CULL_MODE CullMode = D3D11_CULL_BACK;
  EBlendMode BlendMode = EBlendMode::Opaque;
  bool bIsInstancing = false;
};

// 기본 파이프라인 테이블
constexpr FPipelineEntry pipelineTable[] = {
    {
        .Id = EPipelineID::Simple_Solid,
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = EPipelineID::Textured,
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"TexturedPS.cso",
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = EPipelineID::Grid,
        .VertexShader = L"GridVS.cso",
        .PixelShader = L"GridPS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = EPipelineID::RotationGizmo,
        .VertexShader = L"RotationGizmoVS.cso",
        .PixelShader = L"RotationGizmoPS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
    {
        .Id = EPipelineID::Spotlight,
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"SpotlightPS.cso",
        .bDepthWrite = false,
        .CullMode = D3D11_CULL_NONE,
        .BlendMode = EBlendMode::Additive,
    },
    {
        .Id = EPipelineID::Text,
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"MsdfTextPS.cso",
        .bDepthWrite = false,
        .BlendMode = EBlendMode::Translucent,
    },
    {
        .Id = EPipelineID::Instance_Text,
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"MsdfTextPS.cso",
        .bDepthWrite = false,
        .BlendMode = EBlendMode::Translucent,
        .bIsInstancing = true,
    },
    {
        .Id = EPipelineID::Instance_Simple,
        .VertexShader = L"InstanceVS.cso",
        .PixelShader = L"ExamplePS.cso",
        .BlendMode = EBlendMode::Opaque,
        .bIsInstancing = true,
    },
    {
        .Id = EPipelineID::Gizmo,
        .VertexShader = L"ExampleVS.cso",
        .PixelShader = L"UnlightPS.cso",
        .BlendMode = EBlendMode::Opaque,
    },
};

// 머티리얼 정보 엔트리
struct FMaterialEntry {
  EMaterialID Id;
  EPipelineID PipelineID;
  const char *TextureName = nullptr;
};

// 기본 머티리얼 테이블
constexpr FMaterialEntry materialTable[] = {
    {
        .Id = EMaterialID::Simple,
        .PipelineID = EPipelineID::Simple_Solid,
    },
    {
        .Id = EMaterialID::Grid,
        .PipelineID = EPipelineID::Grid,
    },
    {
        .Id = EMaterialID::RotGizmo,
        .PipelineID = EPipelineID::RotationGizmo,
    },
    {
        .Id = EMaterialID::Spotlight,
        .PipelineID = EPipelineID::Spotlight,
    },
    {
        .Id = EMaterialID::Text,
        .PipelineID = EPipelineID::Text,
        .TextureName = "maplestorybold",
    },
    {
        .Id = EMaterialID::Textured,
        .PipelineID = EPipelineID::Textured,
        .TextureName = "transparent-test",
    },
    {
        .Id = EMaterialID::Billboard,
        .PipelineID = EPipelineID::Textured,
        .TextureName = "uv-test",
    },
    {
        .Id = EMaterialID::Instance_Text,
        .PipelineID = EPipelineID::Instance_Text,
        .TextureName = "maplestorybold",
    },
    {
        .Id = EMaterialID::Instance_Simple,
        .PipelineID = EPipelineID::Instance_Simple,
    },
    {
        .Id = EMaterialID::Gizmo,
        .PipelineID = EPipelineID::Gizmo,
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
    AllPipelineMap[EPipelineID::Simple_Solid] = SolidPipeline;
  }

  // 와이어프레임 파이프라인 생성 및 등록
  TSharedPtr<FRenderPipeline> WireframePipeline =
      Renderer.CreateRenderPipeline(Desc, EViewModeIndex::VMI_Wireframe);
  if (WireframePipeline) {
    AllPipelineMap[EPipelineID::Simple_Wireframe] = WireframePipeline;
  }

  return SolidPipeline != nullptr && WireframePipeline != nullptr;
}

bool FRenderResourceLibrary::InitializePipelines(FRenderer &Renderer) {
  // 솔리드 및 와이어프레임 파이프라인 개별 생성
  CreateSolidWireframePipeline(Renderer);

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
  if (!InitializePipelines(
          Renderer) // 파이프라인을 먼저 생성해야 뒤에 material 할당가능
      || !CreateCubeMesh(Renderer) ||
      !CreateCylinderMesh(Renderer, 1.0f, 24u, 1.0f, 1.0f) ||
      !CreateConeMesh(Renderer) || !CreateSpotlightConeMesh(Renderer) ||
      !CreateArrowMesh(Renderer) || !CreateCircleMesh(Renderer) ||
      !CreateRotationGizmoMesh(Renderer) || !CreateSquareArrowMesh(Renderer) ||
      !CreateGridMesh(Renderer) || !CreateSphereMesh(Renderer) ||
      !CreateLineMesh(Renderer) || !CreatePlaneMesh(Renderer) ||
      !CreateRectMesh(Renderer) || !CreateTextures(Renderer) ||
      !InitializeMaterials(Renderer) || !CreateInstancingArrayMap() ||
      !CreateEditTextures(Renderer)) {
    return false;
  }

  return true;
}

bool FRenderResourceLibrary::CreateCubeMesh(FRenderer &Renderer) {
  FMeshDesc MeshDesc{
      .VertexData = CubeVertices,
      .VertexDataSize = static_cast<uint32>(sizeof(CubeVertices)),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(std::size(CubeVertices)),
      .IndexData = CubeIndices,
      .IndexDataSize = static_cast<uint32>(sizeof(CubeIndices)),
      .IndexCount = static_cast<uint32>(std::size(CubeIndices)),
  };

  RegisterMesh(EMeshID::Cube, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Cube] != nullptr;
}

bool FRenderResourceLibrary::CreateCylinderMesh(FRenderer &Renderer,
                                                float Height, uint32 SliceCount,
                                                float TopRadius,
                                                float BottomRadius) {
  constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
  const float DTheta = TAU / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 4 + 2);
  Indices.reserve(SliceCount * 12);

  const float HalfH = Height * 0.5f;

  const uint32 TopCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, HalfH, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f,
                      0.0f, 0.0f, 1.0f});

  const uint32 TopRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({TopRadius * std::cos(Theta),
                        TopRadius * std::sin(Theta), HalfH, 0.0f, 0.0f, 1.0f,
                        1.0f, 0.5f + 0.5f * std::cos(Theta),
                        0.5f + 0.5f * std::sin(Theta), 0.0f, 0.0f, 1.0f});
  }

  const uint32 BottomCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, -HalfH, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f,
                      0.0f, 0.0f, -1.0f});

  const uint32 BottomRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.5f + 0.5f * std::cos(Theta),
                        0.5f + 0.5f * std::sin(Theta), 0.0f, 0.0f, -1.0f});
  }

  const uint32 SideTopStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({TopRadius * std::cos(Theta),
                        TopRadius * std::sin(Theta), HalfH, 0.0f, 0.0f, 1.0f,
                        1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        0.0f, std::cos(Theta), std::sin(Theta), 0.0f});
  }

  const uint32 SideBottomStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 0.0f, 0.0f,
                        1.0f, 1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        1.0f, std::cos(Theta), std::sin(Theta), 0.0f});
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(TopCenterIndex);
    Indices.push_back(TopRingStart + i);
    Indices.push_back(TopRingStart + Next);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(BottomCenterIndex);
    Indices.push_back(BottomRingStart + Next);
    Indices.push_back(BottomRingStart + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;

    const uint32 TL = SideTopStart + i;
    const uint32 TR = SideTopStart + Next;
    const uint32 BL = SideBottomStart + i;
    const uint32 BR = SideBottomStart + Next;

    Indices.push_back(BL);
    Indices.push_back(BR);
    Indices.push_back(TL);

    Indices.push_back(BR);
    Indices.push_back(TR);
    Indices.push_back(TL);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Cylinder, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Cylinder] != nullptr;
}

bool FRenderResourceLibrary::CreateConeMesh(FRenderer &Renderer) {
  constexpr float BottomRadius = 0.5f;
  constexpr float Height = 1.0f;
  constexpr uint32 SliceCount = 48;
  constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
  const float DTheta = TAU / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  const float HalfH = Height * 0.5f;
  const float SlantLen =
      std::sqrt(Height * Height + BottomRadius * BottomRadius);
  const float NormalFactor = Height / SlantLen;
  const float NormalZ = BottomRadius / SlantLen;

  // 옆면 정점 생성
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    const float NextTheta = static_cast<float>(i + 1) * DTheta;
    const float MidTheta = (Theta + NextTheta) * 0.5f;

    const float ApexNx = NormalFactor * std::cos(MidTheta);
    const float ApexNy = NormalFactor * std::sin(MidTheta);

    const uint32 ApexIdx = static_cast<uint32>(Vertices.size());
    Vertices.push_back({0.0f, 0.0f, HalfH, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f,
                        ApexNx, ApexNy, NormalZ});

    const float BaseNx1 = NormalFactor * std::cos(Theta);
    const float BaseNy1 = NormalFactor * std::sin(Theta);
    const uint32 BaseIdx1 = static_cast<uint32>(Vertices.size());
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 1.0f, 1.0f,
                        1.0f, 1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        1.0f, BaseNx1, BaseNy1, NormalZ});

    const float BaseNx2 = NormalFactor * std::cos(NextTheta);
    const float BaseNy2 = NormalFactor * std::sin(NextTheta);
    const uint32 BaseIdx2 = static_cast<uint32>(Vertices.size());
    Vertices.push_back(
        {BottomRadius * std::cos(NextTheta), BottomRadius * std::sin(NextTheta),
         -HalfH, 1.0f, 1.0f, 1.0f, 1.0f,
         static_cast<float>(i + 1) / static_cast<float>(SliceCount), 1.0f,
         BaseNx2, BaseNy2, NormalZ});

    Indices.push_back(ApexIdx);
    Indices.push_back(BaseIdx2);
    Indices.push_back(BaseIdx1);
  }

  // 밑면 뚜껑 정점 생성
  const uint32 BottomCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, -HalfH, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.2f,
                      0.0f, 0.0f, -1.0f});

  const uint32 BottomRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    const float U = static_cast<float>(i) / static_cast<float>(SliceCount);
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 1.0f, 1.0f,
                        1.0f, 1.0f, U, 1.0f, 0.0f, 0.0f, -1.0f});
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(BottomCenterIndex);
    Indices.push_back(BottomRingStart + Next);
    Indices.push_back(BottomRingStart + i);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Cone, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Cone] != nullptr;
}

// 스포트라이트 전용 열린 원뿔 메쉬 생성
bool FRenderResourceLibrary::CreateSpotlightConeMesh(FRenderer &Renderer) {
  constexpr float BottomRadius = 0.5f;
  constexpr float Height = 1.0f;
  constexpr uint32 SliceCount = 48;
  constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
  const float DTheta = TAU / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  const float HalfH = Height * 0.5f;
  const float SlantLen =
      std::sqrt(Height * Height + BottomRadius * BottomRadius);
  const float NormalFactor = Height / SlantLen;
  const float NormalZ = BottomRadius / SlantLen;

  // 옆면 정점만 생성하고 밑면 뚜껑은 생성하지 않음
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    const float NextTheta = static_cast<float>(i + 1) * DTheta;
    const float MidTheta = (Theta + NextTheta) * 0.5f;

    const float ApexNx = NormalFactor * std::cos(MidTheta);
    const float ApexNy = NormalFactor * std::sin(MidTheta);

    const uint32 ApexIdx = static_cast<uint32>(Vertices.size());
    Vertices.push_back({0.0f, 0.0f, HalfH, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f,
                        ApexNx, ApexNy, NormalZ});

    const float BaseNx1 = NormalFactor * std::cos(Theta);
    const float BaseNy1 = NormalFactor * std::sin(Theta);
    const uint32 BaseIdx1 = static_cast<uint32>(Vertices.size());
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 1.0f, 1.0f,
                        1.0f, 1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        1.0f, BaseNx1, BaseNy1, NormalZ});

    const float BaseNx2 = NormalFactor * std::cos(NextTheta);
    const float BaseNy2 = NormalFactor * std::sin(NextTheta);
    const uint32 BaseIdx2 = static_cast<uint32>(Vertices.size());
    Vertices.push_back(
        {BottomRadius * std::cos(NextTheta), BottomRadius * std::sin(NextTheta),
         -HalfH, 1.0f, 1.0f, 1.0f, 1.0f,
         static_cast<float>(i + 1) / static_cast<float>(SliceCount), 1.0f,
         BaseNx2, BaseNy2, NormalZ});

    Indices.push_back(ApexIdx);
    Indices.push_back(BaseIdx2);
    Indices.push_back(BaseIdx1);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::SpotlightCone, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::SpotlightCone] != nullptr;
}

bool FRenderResourceLibrary::CreateArrowMesh(FRenderer &Renderer) {
  constexpr uint32 SliceCount = 16u;
  constexpr float ShaftLength = 0.75f;
  constexpr float ShaftRadius = 0.025f;
  constexpr float HeadRadius = 0.075f;
  constexpr float HeadLength = 0.25f;
  constexpr float DTheta =
      2.0f * std::numbers::pi_v<float> / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 5 + 3);
  Indices.reserve(SliceCount * 18);

  const uint32 ShaftBottomCenter = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f,
                      -1.0f, 0.0f, 0.0f});

  const uint32 ShaftBottomRing = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({0.0f, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, -1.0f, 0.0f, 0.0f});
  }

  const uint32 ShaftSideBottom = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({0.0f, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 ShaftSideTop = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 HeadBaseCenter = static_cast<uint32>(Vertices.size());
  Vertices.push_back({ShaftLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f,
                      0.5f, -1.0f, 0.0f, 0.0f});

  const uint32 HeadBaseRing = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, HeadRadius * std::cos(Theta),
                        HeadRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, -1.0f, 0.0f, 0.0f});
  }

  const uint32 HeadSideBase = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, HeadRadius * std::cos(Theta),
                        HeadRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 HeadTip = static_cast<uint32>(Vertices.size());
  Vertices.push_back({ShaftLength + HeadLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                      1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f});

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(ShaftBottomCenter);
    Indices.push_back(ShaftBottomRing + Next);
    Indices.push_back(ShaftBottomRing + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    const uint32 BL = ShaftSideBottom + i;
    const uint32 BR = ShaftSideBottom + Next;
    const uint32 TL = ShaftSideTop + i;
    const uint32 TR = ShaftSideTop + Next;

    Indices.push_back(BL);
    Indices.push_back(TL);
    Indices.push_back(BR);

    Indices.push_back(BR);
    Indices.push_back(TL);
    Indices.push_back(TR);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(HeadBaseCenter);
    Indices.push_back(HeadBaseRing + Next);
    Indices.push_back(HeadBaseRing + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(HeadSideBase + i);
    Indices.push_back(HeadTip);
    Indices.push_back(HeadSideBase + Next);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Arrow, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Arrow] != nullptr;
}

bool FRenderResourceLibrary::CreateCircleMesh(FRenderer &Renderer) {
  constexpr uint32 SliceCount = 32u;
  constexpr float Radius = 1.0f;
  constexpr float Width = 0.07f;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 2);
  Indices.reserve(SliceCount * 6);

  for (uint32 i = 0; i < SliceCount; ++i) {
    float Theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                  static_cast<float>(SliceCount);
    float InnerRadius = Radius - Width * 0.5f;
    float OuterRadius = Radius + Width * 0.5f;

    Vertices.push_back({0.0f, InnerRadius * std::cos(Theta),
                        InnerRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f});
    Vertices.push_back({0.0f, OuterRadius * std::cos(Theta),
                        OuterRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        1.0f, 1.0f, 1.0f, 0.0f, 0.0f});

    uint32 InnerCurrent = 2 * i;
    uint32 OuterCurrent = 2 * i + 1;
    uint32 InnerNext = (2 * (i + 1)) % (SliceCount * 2);
    uint32 OuterNext = (2 * (i + 1) + 1) % (SliceCount * 2);

    Indices.push_back(InnerCurrent);
    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerNext);

    Indices.push_back(InnerNext);
    Indices.push_back(OuterCurrent);
    Indices.push_back(OuterNext);

    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerCurrent);
    Indices.push_back(InnerNext);

    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerNext);
    Indices.push_back(OuterNext);
  }

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Circle, Renderer.CreateMesh(Desc));
  return AllMeshMap[EMeshID::Circle] != nullptr;
}

bool FRenderResourceLibrary::CreateRotationGizmoMesh(FRenderer &Renderer) {
  constexpr uint32 SliceCount = 32u;
  constexpr float Radius = 1.0f;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 2);
  Indices.reserve(SliceCount * 6);

  for (uint32 i = 0; i < SliceCount; ++i) {
    float Theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                  static_cast<float>(SliceCount);

    Vertices.push_back({0.0f, Radius * std::cos(Theta),
                        Radius * std::sin(Theta), -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f, 0.0f});
    Vertices.push_back({0.0f, Radius * std::cos(Theta),
                        Radius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 0.0f, 0.0f});

    if (i == 0)
      continue;

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i + 1u);
    Indices.push_back(2u * i - 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i + 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i - 1u);
    Indices.push_back(2u * i + 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i - 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i + 1u);
    Indices.push_back(2u * i);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i + 1u);
  }
  Indices[0] = 2u * SliceCount - 2u;
  Indices[1] = 2u * SliceCount - 1u;
  Indices[3] = 2u * SliceCount - 2u;
  Indices[5] = 2u * SliceCount - 1u;
  Indices[6] = 2u * SliceCount - 2u;
  Indices[9] = 2u * SliceCount - 2u;

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::RotGizmo, Renderer.CreateMesh(Desc));
  return AllMeshMap[EMeshID::RotGizmo] != nullptr;
}

bool FRenderResourceLibrary::CreateSquareArrowMesh(FRenderer &Renderer) {
  constexpr float ShaftLength = 0.85f;
  constexpr float ShaftRadius = 0.025f;
  constexpr float ArrowLength = 1.0f;
  constexpr float TipSize = ArrowLength - ShaftLength;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(16u);
  Indices.reserve(72u);

  for (const auto &v : ColoredCubeVertices) {
    float ScaledX = v.x * ShaftLength;
    float ScaledY = v.y * ShaftRadius;
    float ScaledZ = v.z * ShaftRadius;
    Vertices.push_back({ScaledX + ShaftLength * 0.5f, ScaledY, ScaledZ, v.r,
                        v.g, v.b, v.a, v.u, v.v, v.nx, v.ny, v.nz});
  }

  for (const auto &Index : ColoredCubeIndices) {
    Indices.push_back(Index);
  }

  for (const auto &v : ColoredCubeVertices) {
    float ScaledX = v.x * TipSize;
    float ScaledY = v.y * TipSize;
    float ScaledZ = v.z * TipSize;
    Vertices.push_back({ScaledX + TipSize * 0.5f + ShaftLength, ScaledY,
                        ScaledZ, v.r, v.g, v.b, v.a, v.u, v.v, v.nx, v.ny,
                        v.nz});
  }

  for (const auto &Index : ColoredCubeIndices) {
    Indices.push_back(Index + 8u);
  }

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::SquareArrow, Renderer.CreateMesh(Desc));
  return AllMeshMap[EMeshID::SquareArrow] != nullptr;
}

bool FRenderResourceLibrary::CreateGridMesh(FRenderer &Renderer) {
  constexpr float HalfW = 10.0f;
  constexpr float HalfH = 10.0f;

  const TArray<FVertexData> Vertices = {
      {-HalfW, -HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
       1.0f},
      {HalfW, -HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
       1.0f},
      {HalfW, HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
       1.0f},
      {-HalfW, HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       1.0f},
  };
  const TArray<uint32> Indices = {0, 1, 2, 0, 2, 3};

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Grid, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Grid] != nullptr;
}

bool FRenderResourceLibrary::CreateSphereMesh(FRenderer &Renderer) {
  auto Vertices = CreateSphereVertices(0.5f, 20, 20, false);

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(Vertices.size()),
  };

  RegisterMesh(EMeshID::Sphere, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Sphere] != nullptr;
}

bool FRenderResourceLibrary::CreateLineMesh(FRenderer &Renderer) {
  FMeshDesc Desc{.VertexData = LineVertices,
                 .VertexDataSize = static_cast<uint32>(sizeof(LineVertices)),
                 .VertexStride = sizeof(FVertexData),
                 .VertexCount = static_cast<uint32>(std::size(LineVertices)),
                 .bIsLine = true};

  RegisterMesh(EMeshID::Line, Renderer.CreateMesh(Desc));
  return AllMeshMap[EMeshID::Line] != nullptr;
}

bool FRenderResourceLibrary::CreatePlaneMesh(FRenderer &Renderer) {
  FMeshDesc Desc{
      .VertexData = PlaneVertices,
      .VertexDataSize = static_cast<uint32>(sizeof(PlaneVertices)),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(std::size(PlaneVertices)),
  };

  RegisterMesh(EMeshID::Plane, Renderer.CreateMesh(Desc));
  return AllMeshMap[EMeshID::Plane] != nullptr;
}

bool FRenderResourceLibrary::CreateRectMesh(FRenderer &Renderer) {
  // 사각형 정점 배열
  const TArray<FVertexData> Vertices = {
      {0.0f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
       0.0f},
  };

  // 양면 인덱스 배열
  const TArray<uint32> Indices = {0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2};

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  RegisterMesh(EMeshID::Rect, Renderer.CreateMesh(MeshDesc));
  return AllMeshMap[EMeshID::Rect] != nullptr;
}

bool FRenderResourceLibrary::CreateInstancingArrayMap() {
  AllInstancingArrayMap.clear();
  // 기본 배치 키 등록
  AllInstancingArrayMap[{EMaterialID::Instance_Text, EMeshID::Rect}] = {};
  AllInstancingArrayMap[{EMaterialID::Instance_Simple, EMeshID::Cube}] = {};
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

bool FRenderResourceLibrary::CreateEditTextures(FRenderer& Renderer)
{
    const std::filesystem::path ExeDir(GetExecutableDirectory());
    const std::filesystem::path ProjectRoot =
        ExeDir.parent_path().parent_path().parent_path();

    TArray<std::filesystem::path> SearchRoots = {
        ProjectRoot / L"Edit",
        std::filesystem::current_path() / L"Edit",
        ExeDir / L"Edit",
    };

    for (const auto& Root : SearchRoots) {
        std::error_code Ec;
        if (!std::filesystem::exists(Root, Ec)) {
            continue;
        }

        for (const auto& Entry :
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

            // 이미 로드된 텍스처 건너뜀
            if (AllEditorTextureMap.find(KeyWide) != AllEditorTextureMap.end()) {
                continue;
            }

            TSharedPtr<FTexture> Texture = Renderer.CreateTexture(Entry.path().wstring().c_str());

            if (!Texture)
                continue;

            RegisterEditTexture(KeyWide, Texture);
        }
    }

    return true;
}

TSharedPtr<FMaterial> FRenderResourceLibrary::RegisterMaterial(EMaterialID Id, TSharedPtr<FMaterial> inMaterial) {
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

      // 이미 로드된 텍스처 건너뜀
      if (AllTextureMap.find(KeyWide) != AllTextureMap.end()) {
        continue;
      }
  
      TSharedPtr<FTexture> Texture = Renderer.CreateTexture(Entry.path().wstring().c_str());

      if (!Texture)
        continue;

      RegisterTexture(KeyWide, Texture);
    }
  }

  return true;
}

TSharedPtr<FMesh>
FRenderResourceLibrary::GetOrCreateMesh(const EMeshID &ID,
                                        const TArray<FVertexData> &vertices) {
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
