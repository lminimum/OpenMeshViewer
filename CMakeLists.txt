cmake_minimum_required(VERSION 3.30)
# tips:
#   & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --version
#   > cmake version 3.30.5-msvc23
project(OpenMeshViewer LANGUAGES CXX)

# ─── 全局编译选项 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
# tips: https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html#prop_tgt:CXX_STANDARD
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_USE_MATH_DEFINES)          # 为 OpenMesh 启用数学常量
if(MSVC)                                      # MSVC 让 __cplusplus 返回真实值
    add_compile_options(/Zc:__cplusplus)
endif()

# ─── 第三方依赖路径 ───────────────────────────────────────────
# Qt
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Gui)

# OpenMesh
set(OPENMESH_BIN "C:/Program Files/OpenMesh 11.0")
set(OpenMesh_DIR "${OPENMESH_BIN}/share/OpenMesh/cmake")
find_package(OpenMesh REQUIRED)
link_directories("${OPENMESH_BIN}/lib")  # 供导入库查找

# ─── 子工程 ──────────────────────────────────────────────────
add_subdirectory(src)
