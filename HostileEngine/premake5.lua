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
--lib dir
LibraryDir = {}
--lib
Library = {}
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
        "ImGui", "spdlog"
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
    }
    files {
        "HostileEngine/**.h",
        "HostileEngine/**.hpp",
        "HostileEngine/**.cpp"
    }
    defines{
        "_CRT_SECURE_NO_WARNINGS"
    }
    
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
group ""