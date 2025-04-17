# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Set C++17 standard for the entire project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add definition required by OpenMesh
add_definitions(-D_USE_MATH_DEFINES)

# Add MSVC-specific compiler flags for Qt
if(MSVC)
  add_compile_options(/Zc:__cplusplus)
endif()

project ("OpenMeshViewer")

# Qt paths - use MSVC version since we're compiling with Visual Studio
if(MSVC)
  # Use MSVC-compatible Qt if available, or update this path to your MSVC Qt installation
  set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64/lib/cmake")
  # Alternatively, if you don't have MSVC Qt, you can install it or set the following:
  # set(Qt6_DIR "C:/Qt/6.8.2/msvc2019_64/lib/cmake/Qt6" CACHE PATH "Qt6_DIR")
else()
  # For MinGW compilation
  set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/mingw_64/lib/cmake")
endif()

set(OpenMesh_DIR "C:/Program Files/OpenMesh 11.0/share/OpenMesh/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

# 找到 OpenMesh（你需要提前 build 安装）
find_package(OpenMesh REQUIRED)

# Include sub-projects.
add_subdirectory ("OpenMeshViewer")