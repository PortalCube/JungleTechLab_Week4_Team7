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

    // Mesh Section
    FString CurrentMaterialName = "";
    uint32 CurrentStartindex = 0;  

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
        else if (Prefix == "usemtl") // Mesh section
        {
            FString NewMaterialName;
            ss >> NewMaterialName;

            uint32 NewStartIndex = static_cast<uint32>(OutResult.Faces.size());
            uint32 NewIndexCount = NewStartIndex - CurrentStartindex;

            if (NewIndexCount > 0)
            {
                FMeshSection NewMeshSection;
                NewMeshSection.SetMateriaName(CurrentMaterialName);
                NewMeshSection.StartIndex = CurrentStartindex;
                NewMeshSection.IndexCount = NewIndexCount;

                OutResult.Sections.push_back(NewMeshSection);

                CurrentStartindex = NewStartIndex;
            }

            CurrentMaterialName = NewMaterialName;            
        }
    }

    // Final mesh section
    uint32 FinalStartIndex = static_cast<uint32>(OutResult.Faces.size());
    uint32 FinalndexCount = FinalStartIndex - CurrentStartindex;

    if (FinalndexCount > 0)
    {
        FMeshSection FinalMeshSection;
        FinalMeshSection.SetMateriaName(CurrentMaterialName);
        FinalMeshSection.StartIndex = CurrentStartindex;
        FinalMeshSection.IndexCount = FinalndexCount;

        OutResult.Sections.push_back(FinalMeshSection);
    }

    return true;
}

bool FObjParser::ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections)
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

    OutSections = InObjData.Sections;

    return true;
}

bool FObjParser::SaveMeshToBinary(const char* OutFilePath, const TArray<FVertexData>& InVertices, TArray<uint32>& InIndices, TArray<FMeshSection>& InSections)
{
    std::ofstream File;
    File.open(OutFilePath, std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    Header.VertexCount = static_cast<uint32>(InVertices.size());
    Header.IndexCount = static_cast<uint32>(InIndices.size());
    Header.SectionCount = static_cast<uint32>(InSections.size());

    File.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

    const size_t VertexDataSize = sizeof(FVertexData) * InVertices.size();
    File.write(reinterpret_cast<const char*>(InVertices.data()), VertexDataSize);

    const size_t IndexDataSize = sizeof(uint32) * InIndices.size();
    File.write(reinterpret_cast<const char*>(InIndices.data()), IndexDataSize);

    const size_t SectionDataSize = sizeof(FMeshSection) * InSections.size();
    File.write(reinterpret_cast<const char*>(InSections.data()), SectionDataSize);

    File.close();
    return true;
}

bool FObjParser::LoadMeshFromBinary(const char* InFilePath, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections)
{
    std::ifstream File;
    File.open(InFilePath, std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    File.read(reinterpret_cast<char*>(&Header), sizeof(Header));

    OutVertices.resize(Header.VertexCount);
    const size_t VertexDataSize = sizeof(FVertexData) * Header.VertexCount;
    File.read(reinterpret_cast<char*>(OutVertices.data()), VertexDataSize);

    OutIndices.resize(Header.IndexCount);
    const size_t IndexDataSize = sizeof(uint32) * Header.IndexCount;
    File.read(reinterpret_cast<char*>(OutIndices.data()), IndexDataSize);

    OutSections.resize(Header.SectionCount);
    const size_t SectionDataSize = sizeof(FMeshSection) * Header.SectionCount;
    File.read(reinterpret_cast<char*>(OutSections.data()), SectionDataSize);

    File.close();
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
