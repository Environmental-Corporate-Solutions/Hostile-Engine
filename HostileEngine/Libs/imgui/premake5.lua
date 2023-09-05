project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "c++17"
    staticruntime "on"
    
    --targetdir ("bin/".."%{prj.name}")
    --objdir ("bin-int/".."%{prj.name}")

    files
    {
        "imgui/imconfig.h",
		"imgui/imgui.h",
		"imgui/imgui.cpp",
		"imgui/imgui_draw.cpp",
		"imgui/imgui_internal.h",
        "imgui/imgui_tables.cpp",
		"imgui/imgui_widgets.cpp",
		"imgui/imstb_rectpack.h",
		"imgui/imstb_textedit.h",
		"imgui/imstb_truetype.h",
		"imgui/imgui_demo.cpp"
    }
    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
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