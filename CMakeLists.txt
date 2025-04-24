cmake_minimum_required(VERSION 3.16)
project(OpenMeshViewer LANGUAGES CXX)

# ─── 全局编译选项 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_USE_MATH_DEFINES)          # 为 OpenMesh 启用数学常量
if(MSVC)                                      # MSVC 让 __cplusplus 返回真实值
    add_compile_options(/Zc:__cplusplus)
endif()

# ─── 第三方依赖路径 ───────────────────────────────────────────
# Qt
set(CMAKE_PREFIX_PATH "B:/Env/QT6/6.9.0/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Gui)

# OpenMesh
set(OpenMesh_DIR "B:/Env/OpenMesh 11.0/share/OpenMesh/cmake")
find_package(OpenMesh REQUIRED)
link_directories("B:/Env/OpenMesh 11.0/lib")  # 供导入库查找

# ─── 子工程 ──────────────────────────────────────────────────
add_subdirectory(src)
