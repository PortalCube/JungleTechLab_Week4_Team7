#include "FObjParser.h"
#include <fstream>
#include <sstream>

bool FObjParser::LoadObj(const char* InFilePath, FRawObjData& OutResult)
{
    std::ifstream File(InFilePath);
    if (!File.is_open())
    {
        return false;
    }

    FString Line;
    while (std::getline(File, Line))
    {
        if (Line.empty() || Line[0] == '#')
        {
            continue;
        }

        std::stringstream ss(Line);
        FString Prefix;
        ss >> Prefix;

        if (Prefix == "v") // Position
        {
            FVector Pos;
            ss >> Pos.X >> Pos.Y >> Pos.Z;
            OutResult.Positions.push_back(Pos);
        }
        else if (Prefix == "vt") // Texture Coords
        {
            FVector2 Tex;
            ss >> Tex.X >> Tex.Y;
            OutResult.TexCoords.push_back(Tex);
        }
        else if (Prefix == "vn") // Normal
        {
            FVector Norm;
            ss >> Norm.X >> Norm.Y >> Norm.Z;
            OutResult.Normals.push_back(Norm);
        }
        else if (Prefix == "f") // Faces
        {
            TArray<FString> Tokens;
            FString Word;
            while (ss >> Word)
            {
                Tokens.push_back(Word);
            }

            if (Tokens.size() < 3) continue;

            TArray<FObjIndex> FaceIndices;
            for (const FString& T : Tokens)
            {
                FaceIndices.push_back(ParseFaceToken(T));
            }

            for (size_t i = 1; i + 1 < FaceIndices.size(); i++)
            {
                OutResult.Faces.push_back({ FaceIndices[0], FaceIndices[i], FaceIndices[i + 1] });
            }
        }
    }

    return true;
}

bool FObjParser::ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices)
{
    for (size_t i = 0; i < InObjData.Faces.size(); i++)
    {
        for (size_t j = 0; j < InObjData.Faces[i].size(); j++)
        {
            FVertexData Vertex{};

            int vIdx = InObjData.Faces[i][j].v - 1;
            int vtIdx = InObjData.Faces[i][j].vt - 1;
            int vnIdx = InObjData.Faces[i][j].vn - 1;

            if (vIdx >= 0 && vIdx < static_cast<int>(InObjData.Positions.size()))
            {
                Vertex.x = InObjData.Positions[vIdx].X;
                Vertex.y = InObjData.Positions[vIdx].Y;
                Vertex.z = InObjData.Positions[vIdx].Z;
            }

            if (vtIdx >= 0 && vtIdx < static_cast<int>(InObjData.TexCoords.size()))
            {
                Vertex.u = InObjData.TexCoords[vtIdx].X;
                Vertex.v = InObjData.TexCoords[vtIdx].Y;
            }

            if (vnIdx >= 0 && vnIdx < static_cast<int>(InObjData.Normals.size()))
            {
                Vertex.nx = InObjData.Normals[vnIdx].X;
                Vertex.ny = InObjData.Normals[vnIdx].Y;
                Vertex.nz = InObjData.Normals[vnIdx].Z;
            }

            OutIndices.push_back(static_cast<uint32>(OutVertices.size()));
            OutVertices.push_back(Vertex);
        }
    }

    return true;
}

FObjIndex FObjParser::ParseFaceToken(const FString& Token)
{
    FObjIndex Result;
    if (sscanf_s(Token.c_str(), "%d/%d/%d", &Result.v, &Result.vt, &Result.vn) == 3) return Result;
    if (sscanf_s(Token.c_str(), "%d//%d", &Result.v, &Result.vt) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d/%d", &Result.v, &Result.vt) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d", &Result.v) == 1) return Result;

    return Result;
}
