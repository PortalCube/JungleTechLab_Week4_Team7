#pragma once
#include "Source/Runtime/Math/FVector.h"
#include "Source/Runtime/Math/FVector2.h"
#include "Source/Runtime/Math/FVector4.h"
#include "Source/Runtime/Core/TArray.h"
#include "Source/Runtime/Core/FString.h"
#include "Source/Runtime/Core/IntTypes.h"
#include "Source/Runtime/Rendering/Vertices.h"

struct FObjIndex
{
	int v = 0;
	int vt = 0;
	int vn = 0;
};

struct FMeshSection
{
	char MaterialName[64] = { 0 };
	uint32 StartIndex = 0;
	uint32 IndexCount = 0;

	void SetMateriaName(const FString& InName)
	{
		strncpy_s(MaterialName, sizeof(MaterialName), InName.c_str(), _TRUNCATE);
	}
};

struct FRawObjData
{
	TArray<FVector> Positions; // v
	TArray<FVector2> TexCoords; // vt
	TArray<FVector> Normals; // vn
	TArray<TArray<FObjIndex>> Faces; // f
	TArray<FMeshSection> Sections; // Mesh Section
};

#pragma pack(push, 1)
struct FMeshFileHeader
{
	uint32 VertexCount = 0;
	uint32 IndexCount = 0;
	uint32 SectionCount = 0;
};
#pragma pack(pop)

class FObjParser
{
public:
	static bool LoadObj(const char* InFilePath, FRawObjData& OutResult);
	static bool ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections);
	static bool SaveMeshToBinary(const char* OutFilePath, const TArray<FVertexData>& InVertices, TArray<uint32>& InIndices, TArray<FMeshSection>& InSections);
	static bool LoadMeshFromBinary(const char* InFilePath, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections);

	static FObjIndex ParseFaceToken(const FString& Token);
};