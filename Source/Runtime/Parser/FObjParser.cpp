#include "FObjParser.h"
#include <filesystem>

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
            float x, y, z;
            ss >> x >> y >> z;
            Pos.X = -z; Pos.Y = x; Pos.Z = y;
            OutResult.Positions.push_back(Pos);
        }
        else if (Prefix == "vt") // Texture Coords
        {
            FVector2 Tex;
            float u, v;
            ss >> u >> v;
            Tex.X = u; Tex.Y = 1.0f - v;
            OutResult.TexCoords.push_back(Tex);
        }
        else if (Prefix == "vn") // Normal
        {
            FVector Norm;
            float x, y, z;
            ss >> x >> y >> z;
            Norm.X = -z; Norm.Y = x; Norm.Z = y;
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
                OutResult.Faces.push_back({ FaceIndices[0], FaceIndices[i + 1], FaceIndices[i] });
            }
        }
        else if (Prefix == "usemtl") // Mesh section
        {
            FString NewMaterialName;
            ss >> NewMaterialName;

            uint32 NewStartIndex = static_cast<uint32>(OutResult.Faces.size() * 3);
            uint32 NewIndexCount = NewStartIndex - CurrentStartindex;

            if (NewIndexCount > 0)
            {
                FMeshSection NewMeshSection;
                NewMeshSection.SectionName = CurrentMaterialName;
                NewMeshSection.StartIndex = CurrentStartindex;
                NewMeshSection.IndexCount = NewIndexCount;

                OutResult.Sections.push_back(NewMeshSection);

                CurrentStartindex = NewStartIndex;
            }

            CurrentMaterialName = NewMaterialName;            
        }
    }

    // Final mesh section
    uint32 FinalStartIndex = static_cast<uint32>(OutResult.Faces.size() * 3);
    uint32 FinalndexCount = FinalStartIndex - CurrentStartindex;

    if (FinalndexCount > 0)
    {
        FMeshSection FinalMeshSection;
        FinalMeshSection.SectionName = CurrentMaterialName;
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

bool FObjParser::SaveMeshToBinary(const char* OutFilePath, uint64 InSourceHash, const TArray<FVertexData>& InVertices, TArray<uint32>& InIndices, TArray<FMeshSection>& InSections)
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
    Header.SourceHash = InSourceHash;

    File.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

    const size_t VertexDataSize = sizeof(FVertexData) * InVertices.size();
    File.write(reinterpret_cast<const char*>(InVertices.data()), VertexDataSize);

    const size_t IndexDataSize = sizeof(uint32) * InIndices.size();
    File.write(reinterpret_cast<const char*>(InIndices.data()), IndexDataSize);

    for (const auto& Section : InSections)
    {
        uint32 NameLen = static_cast<uint32>(Section.SectionName.size());
        File.write(reinterpret_cast<const char*>(&NameLen), sizeof(uint32));

        File.write(Section.SectionName.data(), NameLen);

        File.write(reinterpret_cast<const char*>(&Section.StartIndex), sizeof(uint32));
        File.write(reinterpret_cast<const char*>(&Section.IndexCount), sizeof(uint32));
    }

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
    for (int i = 0; i < Header.SectionCount; i++)
    {
        uint32 NameLen = 0;
        File.read(reinterpret_cast<char*>(&NameLen), sizeof(uint32));

        OutSections[i].SectionName.resize(NameLen);
        File.read(&OutSections[i].SectionName[0], NameLen);

        File.read(reinterpret_cast<char*>(&OutSections[i].StartIndex), sizeof(uint32));
        File.read(reinterpret_cast<char*>(&OutSections[i].IndexCount) , sizeof(uint32));
    }    

    File.close();
    return true;
}

bool FObjParser::LoadMtl(const char* InFilePath, TArray<FMtlData>& OutResult)
{
    std::ifstream File(InFilePath);
    if (!File.is_open())
    {
        return false;
    }

    FMtlData CurrentMtl;
    bool bHasMat = false;

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

        if (Prefix == "newmtl")
        {
            if (bHasMat)
            {
                OutResult.push_back(CurrentMtl);
            }
            CurrentMtl = FMtlData();
            ss >> CurrentMtl.MaterialName;

            bHasMat = true;
        }
        else if (Prefix == "Kd")
        {
            ss >> CurrentMtl.Kd.X >> CurrentMtl.Kd.Y >> CurrentMtl.Kd.Z;
        }
        else if (Prefix == "map_Kd")
        {
            ss >> CurrentMtl.map_Kd;
        }
        else if (Prefix == "d")
        {
            ss >> CurrentMtl.d; 
        }
    }
    
    if (bHasMat)
    {
        OutResult.push_back(CurrentMtl);
    }

    return true;
}

bool FObjParser::ValidateBinary(const char* InBinFilePath, const char* InObjFilePath)
{
    std::ifstream BinFile;
    BinFile.open(InBinFilePath, std::ios::binary);
    if (!BinFile.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    BinFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));

    // Check Magic number
    if (Header.Magic != 0x4D455348)
    {
        return false;
    }

    uint64 ObjHash = ComputeFileHash(std::filesystem::path (InObjFilePath));

    return Header.SourceHash == ObjHash;
}

FObjIndex FObjParser::ParseFaceToken(const FString& Token)
{
    FObjIndex Result;
    if (sscanf_s(Token.c_str(), "%d/%d/%d", &Result.v, &Result.vt, &Result.vn) == 3) return Result;
    if (sscanf_s(Token.c_str(), "%d//%d", &Result.v, &Result.vn) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d/%d", &Result.v, &Result.vt) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d", &Result.v) == 1) return Result;

    return Result;
}

uint64 FObjParser::ComputeFileHash(const std::filesystem::path& FilePath)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        return 0;
    }

    constexpr uint64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64 FNV_PRIME = 1099511628211ULL;

    uint64 Hash = FNV_OFFSET_BASIS;
    char Buffer[4096]; // 4KB buffer

    while (File.read(Buffer, sizeof(Buffer)) || File.gcount() > 0)
    {
        std::streamsize BytesRead = File.gcount();
        for (std::streamsize i = 0; i < BytesRead; i++)
        {
            Hash ^= static_cast<uint8_t>(Buffer[i]);
            Hash *= FNV_PRIME;
        }

    }

    return Hash;
}
