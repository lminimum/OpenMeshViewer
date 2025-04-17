# CMakeList.txt : CMake project for OpenMeshViewer, include source and define
# project specific logic here.

# Tell CMake to run moc when necessary
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add source to this project's executable.
add_executable (OpenMeshViewer "OpenMeshViewer.cpp" "OpenMeshViewer.h")

# Set C++17 standard explicitly for this target
set_property(TARGET OpenMeshViewer PROPERTY CXX_STANDARD 17)
set_property(TARGET OpenMeshViewer PROPERTY CXX_STANDARD_REQUIRED ON)

# Link libraries to the target
target_link_libraries(OpenMeshViewer PRIVATE Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets OpenMeshCore OpenMeshTools)

# Deploy Qt DLLs to the executable directory
if(WIN32)
    # Find the directory containing Qt DLLs
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    
    # Determine if we're in Debug mode
    set(DEBUG_SUFFIX "$<$<CONFIG:Debug>:d>")
    
    # Add post-build commands to copy the required DLLs with conditional debug suffix
    add_custom_command(TARGET OpenMeshViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_bin_dir}/Qt6Widgets${DEBUG_SUFFIX}.dll"
            "${_qt_bin_dir}/Qt6Core${DEBUG_SUFFIX}.dll"
            "${_qt_bin_dir}/Qt6Gui${DEBUG_SUFFIX}.dll"
            "${_qt_bin_dir}/Qt6OpenGL${DEBUG_SUFFIX}.dll"
            "${_qt_bin_dir}/Qt6OpenGLWidgets${DEBUG_SUFFIX}.dll"
            $<TARGET_FILE_DIR:OpenMeshViewer>
    )
    
    # If using MinGW, you also need the libgcc and libstdc++ DLLs
    if(MINGW)
        add_custom_command(TARGET OpenMeshViewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_bin_dir}/libgcc_s_seh-1.dll"
                "${_qt_bin_dir}/libstdc++-6.dll"
                "${_qt_bin_dir}/libwinpthread-1.dll"
                $<TARGET_FILE_DIR:OpenMeshViewer>
        )
    endif()
    
    # Copy plugins to the plugins directory (required for some Qt features)
    set(PLUGINS_DIR "$<TARGET_FILE_DIR:OpenMeshViewer>/plugins")
    add_custom_command(TARGET OpenMeshViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGINS_DIR}/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_bin_dir}/../plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
            "${PLUGINS_DIR}/platforms/"
    )
    
    # Copy OpenMesh DLLs to the executable directory with conditional debug suffix
    set(OPENMESH_BIN_DIR "C:/Program Files/OpenMesh 11.0")
    add_custom_command(TARGET OpenMeshViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENMESH_BIN_DIR}/OpenMeshCore${DEBUG_SUFFIX}.dll"
            "${OPENMESH_BIN_DIR}/OpenMeshTools${DEBUG_SUFFIX}.dll"
            $<TARGET_FILE_DIR:OpenMeshViewer>
    )
endif()

