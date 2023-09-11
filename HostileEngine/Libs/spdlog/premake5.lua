project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "c++17"
    staticruntime "off"
    
    --targetdir ("bin/".."%{prj.name}")
    --objdir ("bin-int/".."%{prj.name}")

    disablewarnings {
        "4819",
    }

    includedirs
    {
        "spdlog/include",
    }

    files
    {
        "spdlog/src/**.cpp",
        "spdlog/src/**.h",
        "spdlog/include/**.h",
    }
    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "SPDLOG_COMPILED_LIB"
    }
    filter "system:windows"
        flags
        {
            "MultiProcessorCompile"
        }

        systemversion "latest"
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"