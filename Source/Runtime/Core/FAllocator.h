#pragma once

#include "Runtime/Core/FPoolAllocator.h"
#include <malloc.h>
#include <array>
#include "Runtime/Core/Log.h"

class FAllocator
{
public:

    [[nodiscard]]
    bool Init()
    {
        for (int i = 0; i < Pools.size(); ++i)
        {
            if (!Pools[i].Init(SizeClasses[i], 1024))
            {
                Shutdown();
                return false;
            }
            ++PoolCount;
        }

        return true;

        // return PoolAllocator.Init(PoolBlockSize, 1024);
    }

    void* Allocate(size_t Size)
    {
        /*if (Size <= PoolBlockSize)
        {
            return PoolAllocator.Allocate();
        }*/

        for (int i = 0; i < 5; ++i)
        {
            if (Size <= SizeClasses[i])
            {
                if (!Pools[i].Allocate())
                {
                    return std::malloc(Size);
                }
            }
        }
        UE_LOG("System Allocate!")
        return std::malloc(Size);
    }

    void Free(void* Ptr)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (Pools[i].Owns(Ptr))
            {
                Pools[i].Free(Ptr);
                return;
            }
        }
        std::free(Ptr);

        /*if (PoolAllocator.Owns(Ptr))
        {
            PoolAllocator.Free(Ptr);
        }
        else
        {
            std::free(Ptr);
        }*/
    }

    void* Allocate(size_t Size, size_t Alignment)
    {
        return _aligned_malloc(Size, Alignment);
    }

    void Free(void* Ptr, size_t Alignment)
    {
        _aligned_free(Ptr);
    }

    void Shutdown()
    {
        for (size_t i = 0; i < PoolCount; ++i)
        {
            FPoolAllocator& Pool = Pools[i];
        }
    }

private:
    //static constexpr size_t PoolBlockSize = 256;
    // inline static FPoolAllocator PoolAllocator;
    UINT PoolCount;
    inline static constexpr std::array<size_t, 5> SizeClasses = { 16, 32, 64, 128, 256 };
    std::array<FPoolAllocator, 5> Pools;

};