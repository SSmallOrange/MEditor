-- 引入 Qt 模块
require "premake-qt/qt"
local qt = premake.extensions.qt

workspace "MEditor"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "MEditor"

    buildoptions { "/Zc:__cplusplus" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"
    filter {}

project "MEditor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin/int/%{cfg.buildcfg}")

    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.cpp",
        "src/**.ui",
        "src/**.qrc",
        "src/**.ts",
        "src/qss/**.qss"
    }

    includedirs {
        "src",
        "src/ui"
    }

    -- QT
    qt.enable()

    qtuseexternalinclude(true)

    local qtPath = os.getenv("QT6_DIR") or os.getenv("QT_DIR")
    if not qtPath then
        error("QT6_DIR (or QT_DIR) is NULL, please set Qt path")
    end

    qtpath(qtPath)
    qtmodules { "core", "gui", "widgets"}

    qtprefix "Qt6"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        qtsuffix "d"
        defines { "QT_DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { "QT_NO_DEBUG" }

    filter "configurations:*"
        postbuildcommands {
            '{MKDIR} "%{cfg.targetdir}/qss"',
            '{COPY} "src/qss/*.qss" "%{cfg.targetdir}/qss"',
        }

    filter {}
