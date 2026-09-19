#include "FResourceLoader.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Engine/FArchive.h"
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

TMap<FString, std::pair<UPipeline*, UPipelineDesc>> PipelineMap;
TMap<FString, std::pair<UMaterial*, UMaterialDesc>> MaterialMap;
TMap<FString, std::pair<UStaticMesh*, UStaticMeshDesc>> StaticMeshMap;
TMap<FString, std::pair<UFont*, UFontDesc>>	FontMap;
TMap<FString, std::pair<UTexture*, UTextureDesc>> TextureMap;

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
	{ "NBackone", ERasterizerCullMode::Back },
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
	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadDefaultStaticMeshAssets] 렌더러가 초기화되지 않았습니다.");
	}

	auto RegisterStaticMeshAsset = [&ResourceLibrary](const FString& ID, bool bCreated)
	{
		if (!bCreated)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 기본 메쉬 생성에 실패했습니다. {}",
				ID);
		}

		if (StaticMeshMap.contains(ID))
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 이미 ID가 존재합니다. {}",
				ID);
		}

		TSharedPtr<FMesh> Mesh = ResourceLibrary.GetMesh(FName(ID));
		if (Mesh == nullptr)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 등록된 FMesh를 찾지 못했습니다. {}",
				ID);
		}

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
		UStaticMeshDesc StaticMeshDesc{};
		StaticMeshDesc.ID = ID;
		StaticMeshDesc.Mesh = Mesh.get();

		StaticMeshMap[ID] = { StaticMesh, StaticMeshDesc };
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

	fs::path AssetPath{ FResourceLoader::AssetDirectoryPath };

	bool bIsExist = fs::exists(AssetPath);
	bool bIsDirectory = fs::is_directory(AssetPath);

	if (!bIsExist || !bIsDirectory)
	{
		return;
	}

	const auto& Iterator = fs::directory_iterator(AssetPath);

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

		try
		{
			json data = json::parse(File);
			FArchive Archive{ data };

			int Version = Archive.GetInt32("Version");

			if (Version != CurrentSchemaVersion)
			{
				UE_LOG("[FResourceLoader::LoadAssets] 파일의 버전이 불일치합니다. %s", Entry.path().c_str());
				continue;
			}

			FString Type = Archive.GetString("AssetType");

			if (Type == "Pipeline") { LoadPipelineAsset(Archive); }
			else if (Type == "Material") { LoadMaterialAsset(Archive); }
			else if (Type == "StaticMesh") { LoadStaticMeshAsset(Archive); }
			else if (Type == "Font") { LoadFontAsset(Archive); }
			else if (Type == "Texture") { LoadTextureAsset(Archive); }
			else
			{
				UE_LOG("[FResourceLoader::LoadAssets] 알 수 없는 AssetType %s", Entry.path().c_str());
				continue;
			}
		}
		catch (const json::parse_error& e)
		{
			UE_LOG("[FResourceLoader::LoadAssets] JSON 파일을 파싱하는데 실패했습니다. %s", Entry.path().c_str());
			continue;
		}
	}
}

void FResourceLoader::LoadPipelineAsset(FArchive& Archive)
{
	// 포인터만 생성..
	UPipeline* Pipeline = NewObject<UPipeline>();
	UPipelineDesc PipelineDesc{};

	FString ID = Archive.GetString("ID");

	if (PipelineMap.contains(ID))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 이미 ID가 존재합니다. {}", ID);
	}

	PipelineDesc.ID = ID;
	PipelineDesc.VertexShaderFilePath = Archive.GetString("VertexShaderFilePath");
	PipelineDesc.PixelShaderFilePath = Archive.GetString("PixelShaderFilePath");

	if (Archive.IsNull("Rasterizer"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Rasterizer' 필드가 없습니다. {}", ID);
	}

	FArchive RasterizerArchive = Archive.GetArchive("Rasterizer");
	PipelineDesc.Rasterizer.FillMode = Archive.GetEnum("FillMode", RasterizerFillModeMap);
	PipelineDesc.Rasterizer.CullMode = Archive.GetEnum("CullMode", RasterizerCullModeMap);
	PipelineDesc.Rasterizer.FrontFace = Archive.GetEnum("FrontFaceMode", RasterizerFrontFaceModeMap);
	PipelineDesc.Rasterizer.bUseMultisample = Archive.GetBool("Multisample");
	PipelineDesc.Rasterizer.bUseAntialiasedLine = Archive.GetBool("AntialiasedLine");

	if (Archive.IsNull("DepthStencil"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'DepthStencil' 필드가 없습니다. {}", ID);
	}

	FArchive DepthStencilArchive = Archive.GetArchive("DepthStencil");
	PipelineDesc.DepthStencil.bDepthEnable = Archive.GetBool("DepthEnable");
	PipelineDesc.DepthStencil.bStencilEnable = Archive.GetBool("StencilEnable");
	PipelineDesc.DepthStencil.DepthWrite = Archive.GetEnum("DepthWriteMode", DepthWriteModeMap);

	if (Archive.IsNull("Blend"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Blend' 필드가 없습니다. {}", ID);
	}

	FArchive BlendArchive = Archive.GetArchive("Blend");
	PipelineDesc.Blend.BlendMode = Archive.GetEnum("BlendMode", BlendModeMap);

	PipelineMap[ID] = { Pipeline, PipelineDesc };
}

void FResourceLoader::LoadMaterialAsset(FArchive& Archive)
{
	// 포인터만 생성..
	UMaterial* Material = NewObject<UMaterial>();
	UMaterialDesc MaterialDesc{};

	FString ID = Archive.GetString("ID");

	if (MaterialMap.contains(ID))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadMaterialAsset] 이미 ID가 존재합니다. {}", ID);
	}

	MaterialDesc.ID = ID;
	MaterialDesc.PipelineFilePath = Archive.GetString("PipelineFilePath");
	MaterialDesc.TextureFilePath = Archive.GetString("TextureFilePath");
	if (Archive.IsNull("TextureSampler"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadMaterialAsset] 'TextureSampler' 필드가 없습니다. {}", ID);
	}

	FArchive TextureSamplerArchive = Archive.GetArchive("TextureSampler");
	MaterialDesc.TextureSamplerDesc.FilterMode = TextureSamplerArchive.GetEnum("FilterMode", TextureSamplerFilterModeMap);
	MaterialDesc.TextureSamplerDesc.WrapMode = TextureSamplerArchive.GetEnum("WrapMode", TextureSamplerWrapModeMap);

	MaterialMap[ID] = { Material, MaterialDesc };
}

void FResourceLoader::LoadStaticMeshAsset(FArchive& Archive)
{
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	UStaticMeshDesc StaticMeshDesc{};

	FString ID = Archive.GetString("ID");

	if (StaticMeshMap.contains(ID))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadStaticMeshAsset] 이미 ID가 존재합니다. {}", ID);
	}

	StaticMeshDesc.ID = ID;
	StaticMeshDesc.MeshFilePath = Archive.GetString("MeshFilePath");

	FRawObjData RawObjData{};
	if (!FObjParser::LoadObj(StaticMeshDesc.MeshFilePath.ToString().c_str(), RawObjData))
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] OBJ 파일을 불러오는데 실패했습니다. ID: {}, Path: {}",
			ID,
			StaticMeshDesc.MeshFilePath.ToString());
	}

	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	if (!FObjParser::ConvertObjToVertex(RawObjData, Vertices, Indices, Sections))
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] OBJ 데이터를 정점 데이터로 변환하는데 실패했습니다. ID: {}, Path: {}",
			ID,
			StaticMeshDesc.MeshFilePath.ToString());
	}

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID);
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
			ID,
			StaticMeshDesc.MeshFilePath.ToString());
	}

	ResourceLibrary.RegisterMesh(FName(ID), Mesh);
	StaticMeshDesc.Mesh = Mesh.get();

	StaticMeshMap[ID] = { StaticMesh, StaticMeshDesc };
}

void FResourceLoader::LoadFontAsset(FArchive& Archive)
{
	UFont* FontAsset = NewObject<UFont>();
	UFontDesc FontDesc{};

	FString ID = Archive.GetString("ID");

	if (FontMap.contains(ID))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadFontAsset] 이미 ID가 존재합니다. {}", ID);
	}

	FontDesc.ID = ID;
	FontDesc.TextureFilePath = Archive.GetString("TextureFilePath");

	const FArchive GlyphDataArchive = Archive.GetArchive("GlyphData");
	TSharedPtr<FFont> Font = MakeShared<FFont>(GlyphDataArchive);
	FontDesc.Font = Font.get();

	FRenderResourceLibrary::Get().AllFontMap[ID] = Font;

	FontMap[ID] = { FontAsset, FontDesc };
}

void FResourceLoader::LoadTextureAsset(FArchive& Archive)
{
	UTexture* TextureAsset = NewObject<UTexture>();
	UTextureDesc TextureDesc{};

	FString ID = Archive.GetString("ID");

	if (TextureMap.contains(ID))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadTextureAsset] 이미 ID가 존재합니다. {}", ID);
	}

	TextureDesc.ID = ID;
	TextureDesc.RawTextureFilePath = Archive.GetString("RawTextureFilePath");

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID);
	}

	const std::filesystem::path RawTexturePath{ TextureDesc.RawTextureFilePath.ToString() };
	TSharedPtr<FTexture> Texture = Renderer->CreateTexture(RawTexturePath.wstring().c_str());
	if (Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] FTexture 생성에 실패했습니다. ID: {}, Path: {}",
			ID,
			TextureDesc.RawTextureFilePath.ToString());
	}

	ResourceLibrary.RegisterTexture(FName(ID), Texture);
	TextureDesc.Texture = Texture.get();

	TextureMap[ID] = { TextureAsset, TextureDesc };
}
