workspace "HostileEngine"
    platforms {"Win64"}
    configurations  { "Debug", "Release" }  
    startproject "HostileEngine"  

group "Libs"
include "Libs/imgui"
include "Libs/spdlog"
group ""
--inc
IncludeDir={}
IncludeDir["IMGUI"]="Libs/imgui/imgui"
IncludeDir["SPDLOG"]="Libs/spdlog/spdlog/include"
IncludeDir["MONO"]="Libs/mono/mono/include"
--lib dir
LibraryDir = {}
LibraryDir["Mono_Debug"]="Libs/mono/mono/Debug"
LibraryDir["Mono_Release"]="Libs/mono/mono/Release"
--lib
Library = {}
Library["Mono_Lib"] = "mono-2.0-sgen.lib"

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
        "ImGui", "spdlog", "HostileEngine-ScriptCore"
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
        "%{IncludeDir.SPDLOG}",
        "%{IncludeDir.MONO}",
    }
    files {
        "HostileEngine/**.h",
        "HostileEngine/**.hpp",
        "HostileEngine/**.cpp"
    }
    defines{
        "_CRT_SECURE_NO_WARNINGS"
    }

    --copy mono runtime
    postbuildcommands {
        "{COPYDIR} \"../Libs/mono/runtime_bin/mono\" \"../HostileEngine/bin/Win64/%{cfg.buildcfg}/mono\"",
        "{COPY} \"../Libs/mono/runtime_bin/mono-2.0-sgen.dll\" \"../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",
        --copy our script core
        "{COPY} \"../HostileEngine-ScriptCore/bin/Win64/%{cfg.buildcfg}/HostileEngine-ScriptCore.dll\" \"../HostileEngine/bin/Win64/%{cfg.buildcfg}/\"",
    }

    
    filter "configurations:Debug"
        libdirs 
        {
            "%{LibraryDir.Mono_Debug}",
        }
        links
        {
            "%{Library.Mono_Lib}",
        }
        defines { "DEBUG" }
        symbols "On"
    filter "configurations:Release"
        libdirs 
        {
            "%{LibraryDir.Mono_Release}",
        }
        links
        {
            "%{Library.Mono_Lib}",
        }
        defines { "NDEBUG" }
        optimize "On"
group ""

group "Script"
project "HostileEngine-ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.8"
    location "HostileEngine-ScriptCore"
    files 
    {
        "HostileEngine-ScriptCore/**.cs",
    }

    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        optimize "Full"
        symbols "Off"
group ""