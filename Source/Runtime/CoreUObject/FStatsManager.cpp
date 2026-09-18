#include "Runtime/CoreUObject/FStatsManager.h"

#include <d3d11.h>
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")

void FStatsManager::Initialize(ID3D11Device* Device)
{
    if (!Device)
        return;

    Microsoft::WRL::ComPtr<IDXGIDevice> DxgiDevice;

    if (FAILED(Device->QueryInterface(
        IID_PPV_ARGS(&DxgiDevice))))
    {
        return;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> DxgiAdapter;

    if (FAILED(DxgiDevice->GetAdapter(&DxgiAdapter)))
    {
        return;
    }

    DxgiAdapter.As(&Adapter);
}

//  SYSTEM MEMORY
//  Total RAM       32.0 GB
//  Used RAM        13.6 GB
//  Available RAM   18.4 GB
//

size_t FStatsManager::GetProcessMemoryUsed() const
{
    PROCESS_MEMORY_COUNTERS_EX Counters{};

    if (!GetProcessMemoryInfo(
        GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&Counters),
        sizeof(Counters)))
    {
        return 0;
    }

    return static_cast<size_t>(Counters.WorkingSetSize);
}

size_t FStatsManager::GetSystemMemoryUsed() const
{
    MEMORYSTATUSEX MemoryStatus{};
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    if (!GlobalMemoryStatusEx(&MemoryStatus))
    {
        return 0;
    }

    return static_cast<size_t>(
        MemoryStatus.ullTotalPhys -
        MemoryStatus.ullAvailPhys);
}

size_t FStatsManager::GetSystemMemoryAvailable() const
{
    MEMORYSTATUSEX MemoryStatus{};
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    if (!GlobalMemoryStatusEx(&MemoryStatus))
    {
        return 0;
    }

    return static_cast<size_t>(MemoryStatus.ullAvailPhys);
}

size_t FStatsManager::GetGPUMemoryUsed() const
{
    if (!Adapter)
        return 0;

    DXGI_QUERY_VIDEO_MEMORY_INFO Info{};

    if (FAILED(Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &Info)))
    {
        return 0;
    }

    return static_cast<size_t>(Info.CurrentUsage);
}

size_t FStatsManager::GetGPUMemoryBudget() const
{
    if (!Adapter)
        return 0;

    DXGI_QUERY_VIDEO_MEMORY_INFO Info{};

    if (FAILED(Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &Info)))
    {
        return 0;
    }

    return static_cast<size_t>(Info.Budget);
}