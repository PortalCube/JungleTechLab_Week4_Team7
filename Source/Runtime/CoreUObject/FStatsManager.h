#pragma once

#include "Runtime/Core/TMap.h"
#include <dxgi1_4.h>

enum class EStatMemoryCategory
{
    UObject,
    Texture,
    VertexShader,
    PixelShader,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    RenderTarget,
    Editor,
};

class FStatsManager final
{
public:
    static FStatsManager& Get()
    {
        static FStatsManager Instance;
        return Instance;
    }

    FStatsManager(const FStatsManager&) = delete;
    FStatsManager& operator=(const FStatsManager&) = delete;

public:
    void Initialize(ID3D11Device* Device);

    void AddMemory( EStatMemoryCategory Category, size_t Size) 
    { 
        MemoryStats[Category] += Size; 
    }

    void RemoveMemory( EStatMemoryCategory Category, size_t Size)
    {
        /*auto It = MemoryStats.find(Category);

        if (It == MemoryStats.end())
        {
            return;
        }

        It->second -= Size;*/

        auto It = MemoryStats.find(Category);

        if (It == MemoryStats.end())
        {
            return;
        }

        // 여기서 브레이크
        size_t CurrentSize = It->second;

        if (CurrentSize < Size)
        {
            It->second = 0;
        }
        else
        {
            It->second = CurrentSize - Size;
        }
    }

    size_t GetMemory( EStatMemoryCategory Category) const
    {
        auto It = MemoryStats.find(Category);

        if (It == MemoryStats.end())
        {
            return 0;
        }

        return It->second;
    }

    size_t GetTotalMemory() const
    {
        size_t Total = 0;

        for (const auto& Pair : MemoryStats)
        {
            Total += Pair.second;
        }

        return Total;
    }

    size_t GetProcessMemoryUsed() const;
    size_t GetSystemMemoryUsed() const;
    size_t GetSystemMemoryAvailable() const;

    size_t GetGPUMemoryUsed() const;
    size_t GetGPUMemoryBudget() const;

    size_t GetVertexShaderMemoryUsed() const;
    size_t GetPixelShaderMemoryUsed() const;

    size_t GetTextureMemoryUsed() const;

private:
    FStatsManager() = default;

private:
    TMap<EStatMemoryCategory, size_t> MemoryStats;
    Microsoft::WRL::ComPtr<IDXGIAdapter3> Adapter;
};

