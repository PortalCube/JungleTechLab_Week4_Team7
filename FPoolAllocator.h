#pragma once



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
        if (InBlockSize < sizeof(FFreeBlock))
        {
            return false;
        }

        if (InBlockSize == 0 || InBlockCount == 0)
        {
            return false;
        }

        BlockSize = InBlockSize;
        BlockCount = InBlockCount;

        Memory = std::malloc(BlockSize * BlockCount);

        if (!Memory)
        {
            return false;
        }

        // Free List 초기화
        char* Current =
            static_cast<char*>(Memory);

        for (size_t i = 0; i < BlockCount - 1; ++i)
        {
            FFreeBlock* Block =
                reinterpret_cast<FFreeBlock*>(Current);

            Block->Next =
                reinterpret_cast<FFreeBlock*>(
                    Current + BlockSize
                    );

            Current += BlockSize;
        }

        // 마지막 Block
        FFreeBlock* LastBlock =
            reinterpret_cast<FFreeBlock*>(Current);

        LastBlock->Next = nullptr;

        FreeList = static_cast<FFreeBlock*>(Memory);

        return true;
    }

    void* Allocate()
    {
        if (!FreeList)
        {
            return nullptr;
        }

        FFreeBlock* Block = FreeList;

        FreeList = FreeList->Next;

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
    }

    void Shutdown()
    {
        std::free(Memory);

        Memory = nullptr;
        FreeList = nullptr;

        BlockSize = 0;
        BlockCount = 0;
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

private:
    void* Memory = nullptr;

    size_t BlockSize = 0;
    size_t BlockCount = 0;

    FFreeBlock* FreeList = nullptr;
};