cmake_minimum_required(VERSION 3.13)
project(mfcapture LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(PROJECT_NAME="mfcapture")

# Keep all downloaded third-party files inside the source tree's libs directory.
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(IMGUI_DIR "${LIBS_DIR}/ImGui")
set(INCLUDE_DIR "${LIBS_DIR}/include")
set(GL_DIR "${INCLUDE_DIR}/GL")
set(KHR_DIR "${INCLUDE_DIR}/KHR")

file(MAKE_DIRECTORY "${LIBS_DIR}" "${INCLUDE_DIR}" "${GL_DIR}" "${KHR_DIR}")

if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "Downloading Dear ImGui...")
    file(DOWNLOAD
        "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip"
        "${LIBS_DIR}/imgui.zip"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
        message(FATAL_ERROR "Dear ImGui download failed: ${DOWNLOAD_ERROR}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar -xf imgui.zip
        WORKING_DIRECTORY "${LIBS_DIR}"
    )
    file(RENAME "${LIBS_DIR}/imgui-1.92.8" "${IMGUI_DIR}")
    file(REMOVE "${LIBS_DIR}/imgui.zip")
endif()

if(NOT EXISTS "${IMGUI_DIR}/nfd.h")
    message(STATUS "Downloading Native File Dialog Extended...")
    file(DOWNLOAD
        "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.3.0.zip"
        "${LIBS_DIR}/nfd.zip"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
        message(FATAL_ERROR "Native File Dialog Extended download failed: ${DOWNLOAD_ERROR}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar -xf nfd.zip
        WORKING_DIRECTORY "${LIBS_DIR}"
    )
    set(NFD_SOURCE_DIR "${LIBS_DIR}/nativefiledialog-extended-1.3.0/src")
    file(COPY
        "${NFD_SOURCE_DIR}/include/nfd.h"
        "${NFD_SOURCE_DIR}/include/nfd.hpp"
        "${NFD_SOURCE_DIR}/nfd_win.cpp"
        "${NFD_SOURCE_DIR}/nfd_cocoa.m"
        "${NFD_SOURCE_DIR}/nfd_gtk.cpp"
        DESTINATION "${IMGUI_DIR}"
    )
    file(REMOVE_RECURSE "${LIBS_DIR}/nativefiledialog-extended-1.3.0")
    file(REMOVE "${LIBS_DIR}/nfd.zip")
endif()

if(NOT EXISTS "${INCLUDE_DIR}/picojson.h")
    message(STATUS "Downloading picojson...")
    file(DOWNLOAD
        "https://github.com/kazuho/picojson/archive/refs/tags/v1.3.0.zip"
        "${LIBS_DIR}/picojson.zip"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
        message(FATAL_ERROR "picojson download failed: ${DOWNLOAD_ERROR}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar -xf picojson.zip
        WORKING_DIRECTORY "${LIBS_DIR}"
    )
    file(COPY "${LIBS_DIR}/picojson-1.3.0/picojson.h" DESTINATION "${INCLUDE_DIR}")
    file(REMOVE_RECURSE "${LIBS_DIR}/picojson-1.3.0")
    file(REMOVE "${LIBS_DIR}/picojson.zip")
endif()

function(download_header HEADER_URL HEADER_PATH)
    if(NOT EXISTS "${HEADER_PATH}")
        file(DOWNLOAD "${HEADER_URL}" "${HEADER_PATH}" STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            message(FATAL_ERROR "Khronos header download failed: ${DOWNLOAD_ERROR}")
        endif()
    endif()
endfunction()

if(NOT EXISTS "${GL_DIR}/glcorearb.h")
    message(STATUS "Downloading OpenGL and KHR headers...")
    download_header("https://registry.khronos.org/OpenGL/api/GL/glcorearb.h" "${GL_DIR}/glcorearb.h")
    download_header("https://registry.khronos.org/OpenGL/api/GL/glext.h" "${GL_DIR}/glext.h")
    download_header("https://registry.khronos.org/OpenGL/api/GL/wglext.h" "${GL_DIR}/wglext.h")
    download_header("https://registry.khronos.org/OpenGL/api/GL/glxext.h" "${GL_DIR}/glxext.h")
    download_header("https://registry.khronos.org/EGL/api/KHR/khrplatform.h" "${KHR_DIR}/khrplatform.h")
endif()

if(WIN32)
    if(NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib")
        message(STATUS "Downloading GLFW for Windows...")
        file(DOWNLOAD
            "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip"
            "${LIBS_DIR}/glfw.zip"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            message(FATAL_ERROR "GLFW download failed: ${DOWNLOAD_ERROR}")
        endif()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar -xf glfw.zip
            WORKING_DIRECTORY "${LIBS_DIR}"
        )
        file(REMOVE "${LIBS_DIR}/glfw.zip")
    endif()

    if(NOT EXISTS "${LIBS_DIR}/opencv/build/include/opencv2/opencv.hpp")
        message(STATUS "Downloading OpenCV for Windows...")
        file(DOWNLOAD
            "https://github.com/opencv/opencv/releases/download/4.13.0/opencv-4.13.0-windows.exe"
            "${LIBS_DIR}/opencv.exe"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            message(FATAL_ERROR "OpenCV download failed: ${DOWNLOAD_ERROR}")
        endif()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar -xf opencv.exe
            WORKING_DIRECTORY "${LIBS_DIR}"
        )
        file(REMOVE "${LIBS_DIR}/opencv.exe")
    endif()
elseif(APPLE)
    if(NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.MACOS/lib-universal/libglfw3.a")
        message(STATUS "Downloading GLFW for macOS...")
        file(DOWNLOAD
            "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.MACOS.zip"
            "${LIBS_DIR}/glfw.zip"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            message(FATAL_ERROR "GLFW download failed: ${DOWNLOAD_ERROR}")
        endif()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar -xf glfw.zip
            WORKING_DIRECTORY "${LIBS_DIR}"
        )
        file(REMOVE "${LIBS_DIR}/glfw.zip")
    endif()

    if(NOT EXISTS "${LIBS_DIR}/opencv-4.13.0/CMakeLists.txt")
        message(STATUS "Downloading OpenCV sources for macOS...")
        file(DOWNLOAD
            "https://github.com/opencv/opencv/archive/refs/tags/4.13.0.zip"
            "${LIBS_DIR}/opencv.zip"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            message(FATAL_ERROR "OpenCV download failed: ${DOWNLOAD_ERROR}")
        endif()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar -xf opencv.zip
            WORKING_DIRECTORY "${LIBS_DIR}"
        )
        file(REMOVE "${LIBS_DIR}/opencv.zip")
    endif()
endif()

set(PROJECT_SOURCES
    Buffer.cpp
    Capture.cpp
    Config.cpp
    Expand.cpp
    Framebuffer.cpp
    GgApp.cpp
    Intrinsics.cpp
    Menu.cpp
    Preference.cpp
    Texture.cpp
    Undistortion.cpp
    gg.cpp
    main.cpp
    mfcapture.cpp
    libs/ImGui/imgui.cpp
    libs/ImGui/imgui_draw.cpp
    libs/ImGui/imgui_tables.cpp
    libs/ImGui/imgui_widgets.cpp
    libs/ImGui/backends/imgui_impl_glfw.cpp
    libs/ImGui/backends/imgui_impl_opengl3.cpp
)

set(PROJECT_HEADERS
    Buffer.h CamCv.h Camera.h CamImage.h CamMf.h Capture.h
    Config.h Expand.h Framebuffer.h GgApp.h Intrinsics.h Menu.h Mesh.h
    Preference.h Texture.h Undistortion.h gg.h opencv_link.h parseconfig.h
)

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)

set(ASSET_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/mfcapture_config.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mplus1-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/initial.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/gg.ico"
    "${CMAKE_CURRENT_SOURCE_DIR}/gg.icns"
)

if(WIN32)
    list(APPEND PROJECT_SOURCES CamMf.cpp libs/ImGui/nfd_win.cpp)
elseif(APPLE)
    enable_language(OBJC)
    list(APPEND PROJECT_SOURCES libs/ImGui/nfd_cocoa.m)
else()
    list(APPEND PROJECT_SOURCES libs/ImGui/nfd_gtk.cpp)
endif()

add_executable(mfcapture
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${SHADER_FILES}
    ${ASSET_FILES}
)

target_include_directories(mfcapture PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    "${INCLUDE_DIR}"
)

if(WIN32)
    target_compile_definitions(mfcapture PRIVATE _CRT_SECURE_NO_WARNINGS)

    set(OPENCV_DIR "${LIBS_DIR}/opencv")
    set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/build/include")
    set(OPENCV_LIB_DIR "${OPENCV_DIR}/build/x64/vc16/lib")
    set(OPENCV_BIN_DIR "${OPENCV_DIR}/build/x64/vc16/bin")
    set(GLFW_DIR "${LIBS_DIR}/glfw-3.4.bin.WIN64")

    target_include_directories(mfcapture PRIVATE
        "${OPENCV_INCLUDE_DIR}"
        "${GLFW_DIR}/include"
    )

    target_link_libraries(mfcapture PRIVATE
        debug "${OPENCV_LIB_DIR}/opencv_world4130d.lib"
        optimized "${OPENCV_LIB_DIR}/opencv_world4130.lib"
        "${GLFW_DIR}/lib-vc2022/glfw3.lib"
        opengl32 comctl32 ole32 uuid shell32 user32 gdi32 winmm shlwapi
        strmiids mf mfplat mfuuid mfreadwrite
    )

    add_custom_command(TARGET mfcapture POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${SHADER_FILES} ${ASSET_FILES} "$<TARGET_FILE_DIR:mfcapture>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${OPENCV_BIN_DIR}/opencv_world4130d.dll,${OPENCV_BIN_DIR}/opencv_world4130.dll>"
            "$<TARGET_FILE_DIR:mfcapture>"
        COMMENT "Copying assets and OpenCV runtime..."
        COMMAND_EXPAND_LISTS
    )

    set_target_properties(mfcapture PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:mfcapture>"
        VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:mfcapture>;%PATH%"
    )
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT mfcapture)
elseif(APPLE)
    set(GLFW_DIR "${LIBS_DIR}/glfw-3.4.bin.MACOS")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(GLFW_LIBRARY "${GLFW_DIR}/lib-arm64/libglfw3.a")
    else()
        set(GLFW_LIBRARY "${GLFW_DIR}/lib-x86_64/libglfw3.a")
    endif()
    if(NOT EXISTS "${GLFW_LIBRARY}")
        set(GLFW_LIBRARY "${GLFW_DIR}/lib-universal/libglfw3.a")
    endif()

    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_world ON CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_java OFF CACHE BOOL "" FORCE)
    add_subdirectory("${LIBS_DIR}/opencv-4.13.0" EXCLUDE_FROM_ALL)

    target_include_directories(mfcapture PRIVATE "${GLFW_DIR}/include")
    target_link_libraries(mfcapture PRIVATE
        opencv_world
        "${GLFW_LIBRARY}"
        "-framework CoreFoundation"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
        "-framework AppKit"
    )
else()
    find_package(OpenCV REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
    pkg_search_module(GTK3 REQUIRED gtk+-3.0)

    target_include_directories(mfcapture PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GTK3_INCLUDE_DIRS}
    )
    target_link_libraries(mfcapture PRIVATE
        ${OpenCV_LIBS}
        ${GLFW_LIBRARIES}
        ${GTK3_LIBRARIES}
        GL dl pthread
    )
endif()

source_group("Header Files" FILES ${PROJECT_HEADERS})
source_group("Shader Files" FILES ${SHADER_FILES})
source_group("Resource Files" FILES ${ASSET_FILES})
