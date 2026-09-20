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
    bool Init(size_t InBlockSize, size_t InBlockCount)
    {
        if (InBlockSize == 0 || InBlockCount == 0)
        {
            return false;
        }

        if (InBlockSize < sizeof(FFreeBlock))
        {
            return false;
        }

        BlockSize = InBlockSize;
        BlockCount = InBlockCount;

        // Memory = std::malloc(BlockSize * BlockCount);

        const size_t TotalSize = BlockSize * BlockCount;

        Memory = VirtualAlloc(
            nullptr,
            TotalSize,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        if (!Memory)
        {
            BlockSize = 0;
            BlockCount = 0;
            return false;
        }

        // Free List 초기화
        char* Current = static_cast<char*>(Memory);

        for (size_t i = 0; i < BlockCount - 1; ++i)
        {
            FFreeBlock* Block = reinterpret_cast<FFreeBlock*>(Current);
            Block->Next = reinterpret_cast<FFreeBlock*>( Current + BlockSize );
            Current += BlockSize;
        }

        // 마지막 Block
        FFreeBlock* LastBlock = reinterpret_cast<FFreeBlock*>(Current);

        LastBlock->Next = nullptr;

        FreeList = static_cast<FFreeBlock*>(Memory);

        FreeBlockCount = BlockCount;

        FStatsManager::Get().AddMemory(
            EStatMemoryCategory::MemoryPool,
            BlockSize * BlockCount
        );

        FStatsManager::Get().AddMemory(
            EStatMemoryCategory::MemoryPoolUsed,
            0
        );

        FStatsManager::Get().AddMemory(
            EStatMemoryCategory::MemoryPoolFree,
            BlockSize * BlockCount
        );

        return true;
    }

    void* Allocate()
    {
        if (!FreeList)
        {
            return nullptr;
        }

        UE_LOG("Memory Pool Alocate!");

        FFreeBlock* Block = FreeList;

        FreeList = FreeList->Next;

        --FreeBlockCount;

        FStatsManager::Get().AddMemory(
            EStatMemoryCategory::MemoryPoolUsed,
            BlockSize
        );

        FStatsManager::Get().RemoveMemory(
            EStatMemoryCategory::MemoryPoolFree,
            BlockSize
        );

        return Block;
    }

    void Free(void* Ptr)
    {
        if (!Ptr)
        {
            return;
        }

        FFreeBlock* Block = static_cast<FFreeBlock*>(Ptr);

        Block->Next = FreeList;

        FreeList = Block;

        ++FreeBlockCount;

        FStatsManager::Get().RemoveMemory(
            EStatMemoryCategory::MemoryPoolUsed,
            BlockSize
        );

        FStatsManager::Get().AddMemory(
            EStatMemoryCategory::MemoryPoolFree,
            BlockSize
        );
    }

    void Shutdown()
    {
        if (Memory)
        {
            const size_t PoolSize = BlockSize * BlockCount;

            FStatsManager::Get().RemoveMemory(
                EStatMemoryCategory::MemoryPool,
                PoolSize
            );

            FStatsManager::Get().RemoveMemory(
                EStatMemoryCategory::MemoryPoolFree,
                BlockSize * FreeBlockCount
            );

            FStatsManager::Get().RemoveMemory(
                EStatMemoryCategory::MemoryPoolUsed,
                BlockSize * GetUsedBlockCount()
            );

            // std::free(Memory);
            VirtualFree(Memory, 0, MEM_RELEASE);
        }

        Memory = nullptr;
        FreeList = nullptr;

        BlockSize = 0;
        BlockCount = 0;
        FreeBlockCount = 0;
    }

    bool Owns(void* Ptr) const
    {
        if (!Memory || !Ptr)
        {
            return false;
        }

        uintptr_t Start = reinterpret_cast<uintptr_t>(Memory);

        uintptr_t End = Start + BlockSize * BlockCount;

        uintptr_t Address = reinterpret_cast<uintptr_t>(Ptr);

        if (Address < Start || Address >= End)
        {
            return false;
        }

        uintptr_t Offset = Address - Start;

        return (Offset % BlockSize) == 0;
    }

    size_t GetFreeBlockCount() const
    {
        return FreeBlockCount;
    }

    size_t GetUsedBlockCount() const
    {
        return BlockCount - FreeBlockCount;
    }

    size_t GetBlockCount() const
    {
        return BlockCount;
    }

private:
    void* Memory = nullptr;

    size_t BlockSize = 0;
    size_t BlockCount = 0;

    size_t FreeBlockCount = 0;

    FFreeBlock* FreeList = nullptr;
};