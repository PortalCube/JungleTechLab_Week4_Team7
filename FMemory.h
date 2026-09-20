#pragma once

#include "FAllocator.h"



struct FMemory
{
	enum AllocationHints
	{
		None = -1,
		Default,
		Temporary,
		SmallPool,

		Max
	};

public:
	static bool Init()
	{
		return Allocator.Init();
	}

	static void Shutdown()
	{
		Allocator.Shutdown();
	}

	static void* Malloc(size_t Size)
	{
		return Allocator.Allocate(Size);
	}


	static void Free(void* Ptr)
	{
		Allocator.Free(Ptr);
	}

private:
	static inline FAllocator Allocator;

};