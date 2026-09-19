#include "FResourceLoader.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TDeque.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UFont.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Asset/UTexture.h"
#include "Runtime/Mesh/FObjParser.h"
#include "Runtime/Mesh/MeshUtil.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObjectGlobals.h"

#include "Runtime/Material/FRasterizerDesc.h"
#include "Runtime/Material/FDepthStencilDesc.h"
#include "Runtime/Material/FBlendDesc.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <utility>

#pragma region StringEnumMap

TMap<FString, ERasterizerFillMode> RasterizerFillModeMap
{
	{ "Solid", ERasterizerFillMode::Solid },
	{ "Wireframe", ERasterizerFillMode::Wireframe },
};

TMap<FString, ERasterizerCullMode> RasterizerCullModeMap
{
	{ "None", ERasterizerCullMode::None },
	{ "Front", ERasterizerCullMode::Front },
	{ "Back", ERasterizerCullMode::Back },
};

TMap<FString, ERasterizerFrontFaceMode> RasterizerFrontFaceModeMap
{
	{ "CounterClockwise", ERasterizerFrontFaceMode::CounterClockwise },
	{ "Clockwise", ERasterizerFrontFaceMode::Clockwise },
};

TMap<FString, EDepthWriteMode> DepthWriteModeMap
{
	{ "Disable", EDepthWriteMode::Disable },
	{ "Enable", EDepthWriteMode::Enable },
};

TMap<FString, EBlendMode> BlendModeMap
{
	{ "Opaque", EBlendMode::Opaque },
	{ "Masked", EBlendMode::Masked },
	{ "Translucent", EBlendMode::Translucent },
	{ "Additive", EBlendMode::Additive },
	{ "PremultipliedAlpha", EBlendMode::PremultipliedAlpha },
};

TMap<FString, ETextureSamplerFilterMode> TextureSamplerFilterModeMap
{
	{ "Point", ETextureSamplerFilterMode::Point },
	{ "Bilinear", ETextureSamplerFilterMode::Bilinear },
	{ "Trilinear", ETextureSamplerFilterMode::Trilinear },
	{ "Anisotropic", ETextureSamplerFilterMode::Anisotropic },
};

TMap<FString, ETextureSamplerWrapMode> TextureSamplerWrapModeMap
{
	{ "Wrap", ETextureSamplerWrapMode::Wrap },
	{ "Mirror", ETextureSamplerWrapMode::Mirror },
	{ "Clamp", ETextureSamplerWrapMode::Clamp },
};

#pragma endregion

void FResourceLoader::LoadDefaultStaticMeshAssets()
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadDefaultStaticMeshAssets] 렌더러가 초기화되지 않았습니다.");
	}

	auto RegisterStaticMeshAsset = [&Registry, &ResourceLibrary](const FName& ID, bool bCreated)
	{
		if (!bCreated)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 기본 메쉬 생성에 실패했습니다. {}",
				ID.ToString());
		}

		TSharedPtr<FMesh> Mesh = ResourceLibrary.GetMesh(ID);
		if (Mesh == nullptr)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 등록된 FMesh를 찾지 못했습니다. {}",
				ID.ToString());
		}

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
		UStaticMeshDesc StaticMeshDesc{};
		StaticMeshDesc.ID = ID;
		StaticMeshDesc.Name = ID;
		StaticMeshDesc.Mesh = Mesh.get();

		StaticMesh->Load(StaticMeshDesc);
		Registry.Register(ID, StaticMesh);
	};

	RegisterStaticMeshAsset("Cube", MeshUtil::CreateCubeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Cylinder", MeshUtil::CreateCylinderMesh(*Renderer, ResourceLibrary, 1.0f, 24u, 1.0f, 1.0f));
	RegisterStaticMeshAsset("Cone", MeshUtil::CreateConeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("SpotlightCone", MeshUtil::CreateSpotlightConeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Arrow", MeshUtil::CreateArrowMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Circle", MeshUtil::CreateCircleMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("RotGizmo", MeshUtil::CreateRotationGizmoMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("SquareArrow", MeshUtil::CreateSquareArrowMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Grid", MeshUtil::CreateGridMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Sphere", MeshUtil::CreateSphereMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Line", MeshUtil::CreateLineMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Plane", MeshUtil::CreatePlaneMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("Rect", MeshUtil::CreateRectMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("MasterYi", MeshUtil::CreateMasterYiMesh(*Renderer, ResourceLibrary));
}

void FResourceLoader::LoadAssets()
{
	namespace fs = std::filesystem;
	using json = nlohmann::json;

	// 엔진 애셋을 먼저 로드
	LoadDefaultStaticMeshAssets();

	fs::path AssetPath{ FResourceLoader::AssetDirectoryPath };

	bool bIsExist = fs::exists(AssetPath);
	bool bIsDirectory = fs::is_directory(AssetPath);

	if (!bIsExist || !bIsDirectory)
	{
		return;
	}

	const auto& Iterator = fs::directory_iterator(AssetPath);

	TDeque<std::pair<FName, FArchive>> Deque;

	for (const auto& Entry : Iterator)
	{
		if (!Entry.is_regular_file()) { continue; }
		if (Entry.path().extension() != ".json") { continue; }

		std::ifstream File{ Entry.path() };
		if (!File.is_open())
		{
			UE_LOG("[FResourceLoader::LoadAssets] 파일을 여는데 실패했습니다. %s", Entry.path().c_str());
			continue;
		}

		json data;

		try
		{
			data = json::parse(File);
		}
		catch (const json::parse_error& e)
		{
			UE_LOG("[FResourceLoader::LoadAssets] JSON 파일을 파싱하는데 실패했습니다. %s", Entry.path().c_str());
			continue;
		}

		FArchive Archive{ data };
		const FString AssetID = Entry.path().lexically_relative(AssetPath).generic_string();
		Archive.SetString("AssetID", AssetID);

		int Version = Archive.GetInt32("Version");

		if (Version != CurrentSchemaVersion)
		{
			UE_LOG("[FResourceLoader::LoadAssets] 파일의 버전이 불일치합니다. %s", Entry.path().c_str());
			continue;
		}

		FString Type = Archive.GetString("AssetType");
		
		// TODO: 현재는 텍스쳐/파이프라인에 의존하는 애셋이 있어서 이렇게...
		// 나중에 약한 참조를 넣던 다른 로직을 쓰건 해결할 것
		if (Type == "Texture" || Type == "Pipeline")
		{
			Deque.emplace_front(Type, Archive);
		}
		else
		{
			Deque.emplace_back(Type, Archive);
		}
	}

	for (const auto& Item : Deque)
	{
		const FName& Type = Item.first;
		const FArchive& Archive = Item.second;
		const FName AssetID = Archive.GetString("AssetID");

		if (Type == "Pipeline")
		{
			LoadPipelineAsset(Archive, AssetID);
		}
		else if (Type == "Material")
		{
			LoadMaterialAsset(Archive, AssetID);
		}
		else if (Type == "StaticMesh")
		{
			LoadStaticMeshAsset(Archive, AssetID);
		}
		else if (Type == "Font")
		{
			LoadFontAsset(Archive, AssetID);
		}
		else if (Type == "Texture")
		{
			LoadTextureAsset(Archive, AssetID);
		}
		else
		{
			UE_LOG("[FResourceLoader::LoadAssets] 알 수 없는 AssetType %s", AssetID.c_str());
			continue;
		}
	}
}

void FResourceLoader::LoadPipelineAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	// 포인터만 생성..
	UPipeline* PipelineAsset = NewObject<UPipeline>();
	UPipelineDesc PipelineDesc{};
	FRenderPipelineDesc RenderPipelineDesc{};

	PipelineDesc.ID = ID;
	PipelineDesc.Name = Archive.GetString("Name");
	PipelineDesc.bIsInstancing = Archive.GetBool("Instancing");

	// FRenderPipelineDesc 생성
	const FString VertexShaderFilePath = Archive.GetString("VertexShaderFilePath");
	const FString PixelShaderFilePath = Archive.GetString("PixelShaderFilePath");
	RenderPipelineDesc.VertexShaderFilePath = VertexShaderFilePath;
	RenderPipelineDesc.PixelShaderFilePath = PixelShaderFilePath;
	RenderPipelineDesc.bIsInstancing = PipelineDesc.bIsInstancing;

	if (Archive.IsNull("Rasterizer"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Rasterizer' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive RasterizerArchive = Archive.GetArchive("Rasterizer");
	RenderPipelineDesc.Rasterizer.FillMode = RasterizerArchive.GetEnum("FillMode", RasterizerFillModeMap);
	RenderPipelineDesc.Rasterizer.CullMode = RasterizerArchive.GetEnum("CullMode", RasterizerCullModeMap);
	RenderPipelineDesc.Rasterizer.FrontFace = RasterizerArchive.GetEnum("FrontFaceMode", RasterizerFrontFaceModeMap);
	RenderPipelineDesc.Rasterizer.bUseMultisample = RasterizerArchive.GetBool("Multisample");
	RenderPipelineDesc.Rasterizer.bUseAntialiasedLine = RasterizerArchive.GetBool("AntialiasedLine");

	if (Archive.IsNull("DepthStencil"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'DepthStencil' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive DepthStencilArchive = Archive.GetArchive("DepthStencil");
	RenderPipelineDesc.DepthStencil.bDepthEnable = DepthStencilArchive.GetBool("DepthEnable");
	RenderPipelineDesc.DepthStencil.bStencilEnable = DepthStencilArchive.GetBool("StencilEnable");
	RenderPipelineDesc.DepthStencil.DepthWrite = DepthStencilArchive.GetEnum("DepthWriteMode", DepthWriteModeMap);

	if (Archive.IsNull("Blend"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Blend' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive BlendArchive = Archive.GetArchive("Blend");
	RenderPipelineDesc.Blend.BlendMode = BlendArchive.GetEnum("BlendMode", BlendModeMap);

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadPipelineAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	TSharedPtr<FRenderPipeline> Pipeline = Renderer->CreateRenderPipeline(RenderPipelineDesc);
	if (Pipeline == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadPipelineAsset] FRenderPipeline 생성에 실패했습니다. ID: {}",
			ID.ToString());
	}

	ResourceLibrary.RegisterPipeline(ID, Pipeline);
	PipelineDesc.Pipeline = Pipeline.get();

	PipelineAsset->Load(PipelineDesc);
	Registry.Register(ID, PipelineAsset);
}

void FResourceLoader::LoadMaterialAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	// 포인터만 생성..
	UMaterial* Material = NewObject<UMaterial>();
	UMaterialDesc MaterialDesc{};

	MaterialDesc.ID = ID;
	MaterialDesc.Name = Archive.GetString("Name");

	const FName UPipelineID = Archive.GetString("UPipelineID");
	const FName UTextureID = Archive.GetString("UTextureID");
	if (Archive.IsNull("TextureSampler"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadMaterialAsset] 'TextureSampler' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive TextureSamplerArchive = Archive.GetArchive("TextureSampler");
	MaterialDesc.TextureSamplerDesc.FilterMode = TextureSamplerArchive.GetEnum("FilterMode", TextureSamplerFilterModeMap);
	MaterialDesc.TextureSamplerDesc.WrapMode = TextureSamplerArchive.GetEnum("WrapMode", TextureSamplerWrapModeMap);

	MaterialDesc.Pipeline = Registry.Get<UPipeline>(UPipelineID);
	if (MaterialDesc.Pipeline == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadMaterialAsset] Pipeline을 찾을 수 없습니다. ID: {}, Pipeline: {}",
			ID.ToString(), UPipelineID);
	}

	MaterialDesc.Texture = Registry.Get<UTexture>(UTextureID);
	if (MaterialDesc.Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadMaterialAsset] Texture을 찾을 수 없습니다. ID: {}, Texture: {}",
			ID.ToString(), UTextureID);
	}

	Material->Load(MaterialDesc);
	Registry.Register(ID, Material);
}

void FResourceLoader::LoadStaticMeshAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	UStaticMeshDesc StaticMeshDesc{};

	StaticMeshDesc.ID = ID;
	StaticMeshDesc.Name = Archive.GetString("Name");
	FString MeshFilePath = Archive.GetString("MeshFilePath");

	FRawObjData RawObjData{};
	if (!FObjParser::LoadObj(MeshFilePath.c_str(), RawObjData))
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] OBJ 파일을 불러오는데 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			MeshFilePath);
	}

	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	if (!FObjParser::ConvertObjToVertex(RawObjData, Vertices, Indices, Sections))
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] OBJ 데이터를 정점 데이터로 변환하는데 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			MeshFilePath);
	}

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	FMeshDesc MeshDesc
	{
		.VertexData = Vertices.data(),
		.VertexDataSize = static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
		.VertexStride = static_cast<uint32>(sizeof(FVertexData)),
		.VertexCount = static_cast<uint32>(Vertices.size()),
		.IndexData = Indices.data(),
		.IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
		.IndexCount = static_cast<uint32>(Indices.size()),
	};

	TSharedPtr<FMesh> Mesh = Renderer->CreateMesh(MeshDesc);
	if (Mesh == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] FMesh 생성에 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			MeshFilePath);
	}

	ResourceLibrary.RegisterMesh(ID, Mesh);
	StaticMeshDesc.Mesh = Mesh.get();

	StaticMesh->Load(StaticMeshDesc);
	Registry.Register(ID, StaticMesh);
}

void FResourceLoader::LoadFontAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UFont* FontAsset = NewObject<UFont>();

	const FName UTextureID = Archive.GetString("UTextureID");

	const FArchive GlyphDataArchive = Archive.GetArchive("GlyphData");
	TSharedPtr<FFont> Font = MakeShared<FFont>(GlyphDataArchive);

	UFontDesc FontDesc{};
	FontDesc.ID = ID;
	FontDesc.Name = Archive.GetString("Name");
	FontDesc.Font = Font.get();
	FontDesc.Texture = Registry.Get<UTexture>(UTextureID);
	if (FontDesc.Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadFontAsset] Texture을 찾을 수 없습니다. ID: {}, Texture: {}",
			ID.ToString(), UTextureID);
	}

	// TODO: Setter 지정
	FRenderResourceLibrary::Get().AllFontMap[ID] = Font;

	FontAsset->Load(FontDesc);
	Registry.Register(ID, FontAsset);
}

void FResourceLoader::LoadTextureAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UTexture* TextureAsset = NewObject<UTexture>();
	UTextureDesc TextureDesc{};


	TextureDesc.ID = ID;
	TextureDesc.Name = Archive.GetString("Name");
	FString RawTextureFilePath = Archive.GetString("RawTextureFilePath");

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	const std::filesystem::path RawTexturePath{ RawTextureFilePath };
	TSharedPtr<FTexture> Texture = Renderer->CreateTexture(RawTexturePath.wstring().c_str());
	if (Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] FTexture 생성에 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			RawTextureFilePath);
	}

	ResourceLibrary.RegisterTexture(ID, Texture);
	TextureDesc.Texture = Texture.get();

	TextureAsset->Load(TextureDesc);
	Registry.Register(ID, TextureAsset);
}
