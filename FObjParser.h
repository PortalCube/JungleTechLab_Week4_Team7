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

struct FRawObjData
{
	TArray<FVector> Positions; // v
	TArray<FVector2> TexCoords; // vt
	TArray<FVector> Normals; // vn
	TArray<TArray<FObjIndex>> Faces; // f
};

class FObjParser
{
public:
	static bool LoadObj(const char* InFilePath, FRawObjData& OutResult);
	static bool ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices);

	static FObjIndex ParseFaceToken(const FString& Token);
};