project "Tracy"
    kind "StaticLib"
    language "C++"
    cppdialect "c++17"
    staticruntime "off"

    files
    {
        "**/TracyClient.cpp"
    }
    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "TRACY_ENABLE",
        "TRACY_ON_DEMAND"
    }
    includedirs
    {

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