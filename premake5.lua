workspace "MyEngine"
    architecture "x86_64"
    configurations { "Debug", "Release", "Analysis" }
    platforms { "x86", "x64" }
    startproject "MyEngine"
    system "windows"
    systemversion "latest"
    location "."

    filter "platforms:x86"
        architecture "x86"

    filter "platforms:x64"
        architecture "x86_64"

    filter "action:vs2026"
        toolset "msc-v145"

    filter {}

externalproject "DirectXTK_Desktop_2026"
    location "Source/ThirdParty/DirectXTK"
    uuid "E0B52AE7-E160-4D32-BF3F-910B785E5A8E"
    kind "StaticLib"
    language "C++"
    configmap {
        ["Analysis"] = "Debug"
    }

project "MyEngine"
    uuid "05383B45-2B78-451C-9197-8B61474A12BC"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    characterset "Unicode"
    staticruntime "Off"

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp",
        "Shader/**.hlsl",
        "Shader/**.hlsli"
    }

    removefiles {
        "Source/ThirdParty/DirectXTK/**"
    }

    includedirs {
        ".",
        "Source",
        "Source/ThirdParty/DirectXTK",
        "Source/ThirdParty/DirectXTK/Inc",
        "Source/ThirdParty/DirectXTK/Src"
    }

    defines { "NOMINMAX", "_CONSOLE" }
    links {
        "DirectXTK_Desktop_2026",
        "user32",
        "d3d11",
        "dxgi",
        "d3dcompiler"
    }

    warnings "Default"
    buildoptions { "/utf-8", "/FS", "/MP" }
    linkoptions { "/DEBUG" }

    postbuildmessage "Copying textures to output directory..."
    postbuildcommands {
        '{COPYDIR} "%{wks.location}Resources/Textures" "%{cfg.targetdir}/Textures"',
        '{COPYDIR} "%{wks.location}Resources/Edit" "%{cfg.targetdir}/Edit"'
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "Full"
        symbols "On"
        linktimeoptimization "On"

    filter "configurations:Analysis"
        defines { "_DEBUG" }
        symbols "On"
        buildoptions { "/analyze" }

    filter { "configurations:Debug", "platforms:x64" }
        forceincludes { "Runtime/Core/Log.h" }
        prebuildmessage "Converting PNG textures to DDS..."
        prebuildcommands {
            'call "%{wks.location}ConvertTextures.bat"'
        }

    filter "platforms:x86"
        defines { "WIN32" }
        targetdir "Binaries/Win32/%{cfg.buildcfg}"
        objdir "Intermediate/%{prj.name}/Win32/%{cfg.buildcfg}"

    filter "platforms:x64"
        targetdir "Binaries/x64/%{cfg.buildcfg}"
        objdir "Intermediate/%{prj.name}/x64/%{cfg.buildcfg}"

    filter "files:Source/ThirdParty/Imgui/**.cpp"
        warnings "Off"

    filter "files:**VS.hlsl"
        shadertype "Vertex"
        shadermodel "5.0"
        shaderentry "MainVS"
        shaderobjectfileoutput "%{cfg.targetdir}/Shader/%{file.basename}.cso"

    filter "files:**PS.hlsl"
        shadertype "Pixel"
        shadermodel "5.0"
        shaderentry "MainPS"
        shaderobjectfileoutput "%{cfg.targetdir}/Shader/%{file.basename}.cso"

    -- These shaders use a lowercase entry point instead of MainVS/MainPS.
    filter {
        "files:Shader/RotationGizmoVS.hlsl or Shader/RotationGizmoPS.hlsl or Shader/UnlightPS.hlsl"
    }
        shaderentry "main"

    filter "files:**.hlsli"
        buildaction "None"

    filter {}
