#include "UTextComponent.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FArchive.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UTextComponent, UBillBoardComp)
UCLASS_META(UTextComponent, DisplayName, "Text")
UCLASS_META(UTextComponent, MeshName, "Text")

void UTextComponent::Register(UScene& Scene)
{
	Super::Register(Scene);

	FRenderResourceLibrary* Resources = Scene.GetRenderResourceLibrary();

	if (!Font) {
		Font = MakeShared<FFont>();

        FWString Path = GetExecutableDirectory() + L"/Fonts/MaplestoryBold.json";
        Font->Deserialize(Path);
	}

	if (!GetMaterial())
	{
		SetMaterial(Resources ? Resources->GetMaterial(EMaterialID::Text) : nullptr);
	}
	RebuildTextMesh();
}



void UTextComponent::RebuildTextMesh() {
    if (Text.empty()) return;

    TArray<FVertexData> Vertices;
    TArray<uint32> Indices;

    //FWString Text{ L"안   녕~ 하(Ha) 세 요yoyo" };    // 메시 임의 초기값
    TArray<uint32> IndexSet = { 0, 1, 2, 1, 3, 2 };

    float prevAdvance = 0.0f;
    UINT blankCnt = 0;
    for (uint16 i = 0; i < Text.length(); ++i)
    {
        const FCharacterInfo& CharInfo = Font->GetCharInfo(Text.at(i));
        if (Text.at(i) == L' ')
        {   // 공백일 경우 메시를 생성하지 않고 생성할 위치만 반영하기
            prevAdvance += CharInfo.advance;
            ++blankCnt;
            continue;
        }
        FVertexData tv[4]{};

        tv[0].x = 0.0f;
        tv[0].y = CharInfo.planeLeft + prevAdvance;
        tv[0].z = -CharInfo.planeTop;
        tv[0].u = CharInfo.u;
        tv[0].v = CharInfo.v;
        Vertices.push_back(tv[0]);

        tv[1].x = 0.0f;
        tv[1].y = CharInfo.planeRight + prevAdvance;
        tv[1].z = -CharInfo.planeTop;
        tv[1].u = CharInfo.u + CharInfo.width;
        tv[1].v = CharInfo.v;
        Vertices.push_back(tv[1]);

        tv[2].x = 0.0f;
        tv[2].y = CharInfo.planeLeft + prevAdvance;
        tv[2].z = -CharInfo.planeBottom;
        tv[2].u = CharInfo.u;
        tv[2].v = CharInfo.v + CharInfo.height;
        Vertices.push_back(tv[2]);

        tv[3].x = 0.0f;
        tv[3].y = CharInfo.planeRight + prevAdvance;
        tv[3].z = -CharInfo.planeBottom;
        tv[3].u = CharInfo.u + CharInfo.width;
        tv[3].v = CharInfo.v + CharInfo.height;
        Vertices.push_back(tv[3]);

        prevAdvance += CharInfo.advance;
        uint32 VertexOffset = (i - blankCnt) * 4;
        for (uint32 index : IndexSet)
        {
            Indices.push_back(index + VertexOffset);
        }
    }

    // 메시 빌드 추가하기
    FMeshDesc MeshData{
        .VertexData = Vertices.data(),
        .VertexDataSize = static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
        .VertexStride = sizeof(FVertexData),
        .VertexCount = static_cast<uint32>(Vertices.size()),
        .IndexData = Indices.data(),
        .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
        .IndexCount = static_cast<uint32>(Indices.size())
    };
    
    FRenderer* Renderer = FRenderResourceLibrary::Get().GetRenderer();
    if (!Renderer) {
        return;
    }

    // 기존 메쉬 버퍼 직접 갱신
    auto Mesh = GetMesh();
    if (Mesh) {
        Mesh->UpdateBuffers(Renderer->GetDevice(), Renderer->GetContext(), MeshData);
    } else {
        // 신규 메쉬 생성
        SetMesh(Renderer->CreateDynamicMesh(MeshData));
    }
}

void UTextComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetWString("Text", Text);
}

void UTextComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    if (!Archive.IsNull("Text"))
    {
        Text = Archive.GetWString("Text");
    }

    // 폰트 유효성 확인
    if (!Font)
    {
        Font = MakeShared<FFont>();
        FWString Path = GetExecutableDirectory() + L"/Fonts/MaplestoryBold.json";
        Font->Deserialize(Path);
    }

    // 텍스트 메쉬 재생성
    RebuildTextMesh();
}


