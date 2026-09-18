#pragma once

#include "Runtime/Core/TMap.h"

enum class EStatMemoryCategory
{
    UObject,
    Texture,
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
    void AddMemory( EStatMemoryCategory Category, size_t Size) 
    { 
        MemoryStats[Category] += Size; 
    }

    void RemoveMemory( EStatMemoryCategory Category, size_t Size)
    {
        auto It = MemoryStats.find(Category);

        if (It == MemoryStats.end())
        {
            return;
        }

        It->second -= Size;
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

private:
    FStatsManager() = default;

private:
    TMap<EStatMemoryCategory, size_t> MemoryStats;
};

