workspace "HostileEngine"
    platforms {"Win64"}
    configurations  { "Debug", "Release" }  
    startproject "HostileEngine"  

group "Libs"
include "Libs/imgui"
include "Libs/imnodes"
include "Libs/spdlog"
include "Libs/tracy"
include "Libs/imguizmo"
group ""
--inc
IncludeDir={}
IncludeDir["IMGUI"]="Libs/imgui/imgui"
IncludeDir["IMNODES"]="Libs/imnodes/imnodes"
IncludeDir["SPDLOG"]="Libs/spdlog/spdlog/include"
IncludeDir["MONO"]="Libs/mono/mono/include"
IncludeDir["TRACY"]="Libs/tracy/tracy/public/tracy"
IncludeDir["IMGUIZMO"]="Libs/imguizmo/imguizmo"
--lib dir
LibraryDir = {}
LibraryDir["Mono_Debug"]="Libs/mono/mono/Debug"
LibraryDir["Mono_Release"]="Libs/mono/mono/Release"
--lib
Library = {}
Library["Mono_Lib"] = "mono-2.0-sgen.lib"
Library["d3d12"] = "d3d12.lib"
Library["dxgi"] = "dxgi.lib"
--windows lib
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"

group "Executable"
project "HostileEngine"
    location "HostileEngine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "c++17"
    architecture "x86_64"
    staticruntime "off"
    
    
    flags
    {
        "MultiProcessorCompile"
    }

    links {
        "ImGui", 
        "ImNodes",
        "spdlog", 
        "HostileEngine-ScriptCore", 
        "HostileEngine-Compiler", 
        "Tracy",
        "ImGuizmo"
    }
    
    disablewarnings {
        "4819","4996","4005"
    }
    linkoptions {}

    pchheader "stdafx.h"
    pchsource "HostileEngine/src/stdafx.cpp"

    includedirs
    {
        "HostileEngine/",
        "HostileEngine/src",
        "%{IncludeDir.IMGUI}",
        "%{IncludeDir.IMNODES}",
        "%{IncludeDir.SPDLOG}",
        "%{IncludeDir.MONO}",
        "%{IncludeDir.TRACY}",
        "%{IncludeDir.IMGUIZMO}",
    }
    files {
        "HostileEngine/src/**.h",
        "HostileEngine/src/**.hpp",
        "HostileEngine/src/**.cpp"
    }
    defines{
        "_CRT_SECURE_NO_WARNINGS",
        "IMGUI_DEFINE_MATH_OPERATORS",
        "TRACY_ENABLE",
        "TRACY_ON_DEMAND"
    }
    --copy mono runtime
    postbuildcommands {
        "{COPYDIR} \"%{prj.location}/../Libs/mono/runtime_bin/mono\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/mono\"",
        --copy our script core
        "{COPY} \"%{prj.location}/../HostileEngine-ScriptCore/bin/Win64/%{cfg.buildcfg}/HostileEngine-ScriptCore.dll\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",
        "{COPY} \"%{prj.location}/../HostileEngine-Compiler/bin/Win64/%{cfg.buildcfg}/*.dll\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",

        "{COPYDIR} \"%{prj.location}/../Libs/tracy/profiler\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/profiler\"",
        "{COPYDIR} \"%{prj.location}/Content\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/Content\"",
        "{COPYDIR} \"%{prj.location}/Assets\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/Assets\""
    }
    
    vpaths { ["*"] = "HostileEngine/src" }

    filter "configurations:Debug"
        libdirs 
        {
            "%{LibraryDir.Mono_Debug}",
        }
        links
        {
            "%{Library.Mono_Lib}",
            "%{Library.d3d12}",
            "%{Library.dxgi}",
        }
        defines { "DEBUG" }
        symbols "On"

        postbuildcommands {
            "{COPY} \"%{prj.location}/../Libs/mono/runtime_bin/Debug/mono-2.0-sgen.dll\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",
        }

    filter "configurations:Release"
        libdirs 
        {
            "%{LibraryDir.Mono_Release}",
        }
        links
        {
            "%{Library.Mono_Lib}",
            "%{Library.d3d12}",
            "%{Library.dxgi}",
        }
        defines { "NDEBUG" }
        optimize "On"

        postbuildcommands {
            "{COPY} \"%{prj.location}/../Libs/mono/runtime_bin/Release/mono-2.0-sgen.dll\" \"%{prj.location}/../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",
        }
group ""

group "Script"
project "HostileEngine-ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.7.2"
    location "HostileEngine-ScriptCore"

    files 
    {
        "HostileEngine-ScriptCore/src/**.cs",
    }

    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        optimize "Full"
        symbols "Off"

project "HostileEngine-Compiler"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.7.2"
    location "HostileEngine-Compiler"

    files 
    {
        "HostileEngine-Compiler/src/**.cs",
    }

    nuget 
    { 
        "Microsoft.CodeAnalysis.Analyzers:3.3.4", 
        "Microsoft.CodeAnalysis.Common:4.7.0", 
        "Microsoft.CodeAnalysis.CSharp:4.7.0", 
        "System.Buffers:4.5.1", 
        "System.Collections.Immutable:7.0.0", 
        "System.Memory:4.5.5", 
        "System.Numerics.Vectors:4.5.0", 
        "System.Reflection.Metadata:7.0.0",
        "System.Runtime.CompilerServices.Unsafe:6.0.0",
        "System.Text.Encoding.CodePages:7.0.0",
        "System.Threading.Tasks.Extensions:4.5.4",



    }
    
    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"
    
    filter "configurations:Release"
        optimize "Full"
        symbols "Off"
group ""