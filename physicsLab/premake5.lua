workspace "PhysicsEngineWorkspace"
    platforms {"x86_64"}
    configurations { "Debug", "Release" }
    startproject "PhysicsEngine"
    location "." -- build directory
    cppdialect "C++17"
    
    -- Global defines
    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD", "_CRT_SECURE_NO_WARNINGS" }

    -- Include directories
    IncludeDir = {}
    IncludeDir["KHR"] = "PhysicsEngine/includes/KHR"
    IncludeDir["OPENGL"] = "PhysicsEngine/includes/opengl"
    IncludeDir["GLM"] = "PhysicsEngine/includes/opengl/glm"
    IncludeDir["PHYSICS"] = "PhysicsEngine/",

    includedirs 
    { 
        "PhysicsEngine/includes",
        "%{IncludeDir.KHR}",
        "%{IncludeDir.OPENGL}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.PHYSICS}"
    }

    -- Library directories and links
    libdirs { "PhysicsEngine/libraries" }
    links { "glfw3","opengl32" }

    -- Configuration Specific Settings
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

-- Projects
project "PhysicsEngine"
    location "PhysicsEngine"
    kind "ConsoleApp"
    language "C++"

    flags
    {
        "MultiProcessorCompile"
    }

    files {
        "PhysicsEngine/glad.c",
        -- General
        "PhysicsEngine/**.h",
        "PhysicsEngine/**.hpp",
        "PhysicsEngine/**.cpp",
        -- Engine
        "PhysicsEngine/engine/**.h",
        "PhysicsEngine/engine/**.cpp",
        -- Graphics
        "PhysicsEngine/graphics/**.h",
        "PhysicsEngine/graphics/**.cpp",
        -- Math
        "PhysicsEngine/math/**.h",
        "PhysicsEngine/math/**.cpp",
        -- Simulator
        "PhysicsEngine/simulator/**.h",
        "PhysicsEngine/simulator/**.cpp"
    }
