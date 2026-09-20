#pragma once

#include "FPoolAllocator.h"


class FAllocator
{
public:

    [[nodiscard]]
    bool Init()
    {
        return PoolAllocator.Init(64, 1024);
    }

    void* Allocate(size_t Size)
    {
        if (Size <= PoolBlockSize)
        {
            return PoolAllocator.Allocate();
        }

        return std::malloc(Size);
    }

    void Free(void* Ptr)
    {
        if (PoolAllocator.Owns(Ptr))
        {
            PoolAllocator.Free(Ptr);
        }
        else
        {
            std::free(Ptr);
        }
    }

    void Shutdown()
    {
        PoolAllocator.Shutdown();
    }

private:
    static constexpr size_t PoolBlockSize = 64;

    static FPoolAllocator PoolAllocator;
};