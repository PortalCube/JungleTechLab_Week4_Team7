#pragma once

#include "Runtime/Rendering/FRenderQueue.h"

#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Material/FMaterialInstance.h"

// 런타임 게임 로직에서 생성하는 렌더 정보
struct FRenderData
{
    UStaticMesh* Mesh;
    TArray<FMaterialInstance> Materials;
    FMatrix ModelMatrix;

    ERenderType Type = ERenderType::Primitive;
    TArray<FInstanceData> Instances;
};
