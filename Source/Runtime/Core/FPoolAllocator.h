#pragma once

#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Core/Log.h"
#include <Windows.h>

class FPoolAllocator
{
private:
    struct FFreeBlock
    {
        FFreeBlock* Next = nullptr;
    };

public:
    [[nodiscard]]
    bool Init(size_t InBlockSize, size_t InBlockCount);
    void* Allocate();
    void Free(void* Ptr);
    void Shutdown();
    bool Owns(void* Ptr) const;

    size_t GetFreeBlockCount() const { return FreeBlockCount;} 
    size_t GetUsedBlockCount() const{ return BlockCount - FreeBlockCount;}
    size_t GetBlockCount() const { return BlockCount; }

private:
    void* Memory = nullptr;

    size_t BlockSize = 0;
    size_t BlockCount = 0;
    size_t FreeBlockCount = 0;
    FFreeBlock* FreeList = nullptr;
};