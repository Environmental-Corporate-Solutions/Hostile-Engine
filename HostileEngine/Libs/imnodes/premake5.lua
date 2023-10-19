project "ImNodes"
    kind "StaticLib"
    language "C++"
    cppdialect "c++17"
    staticruntime "off"
    
    --targetdir ("bin/".."%{prj.name}")
    --objdir ("bin-int/".."%{prj.name}")

    files 
    { 
        "imnodes/imnodes.h", 
        "imnodes/imnodes_internal.h", 
        "imnodes/imnodes.cpp" 
    }
    defines
    {
        "_CRT_SECURE_NO_WARNINGS", "IMGUI_DEFINE_MATH_OPERATORS"
    }
        includedirs
    {
        "%{prj.location}/../imgui/imgui",
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