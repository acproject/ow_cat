cmake_minimum_required(VERSION 3.20)
project(ow_cat VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 设置vcpkg工具链
set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_TARGET_TRIPLET "x64-windows")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 平台检测
if(WIN32)
    set(PLATFORM_NAME "windows")
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    set(PLATFORM_NAME "macos")
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX)
    set(PLATFORM_NAME "linux")
    add_definitions(-DPLATFORM_LINUX)
endif()

# 查找依赖包
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# GTK依赖 - 统一使用pkg-config方式
if(EXISTS "D:/vcpkg/installed/x64-windows")
    set(GTK4_FOUND TRUE)
    set(GTK4_VERSION "4.16.3")
    message(STATUS "GTK4 found via vcpkg at D:/vcpkg/installed/x64-windows")
    
    # Set GTK4 include directories
    set(GTK4_INCLUDE_DIRS
        "D:/vcpkg/installed/x64-windows/include/gtk-4.0"
        "D:/vcpkg/installed/x64-windows/include/glib-2.0"
        "D:/vcpkg/installed/x64-windows/lib/glib-2.0/include"
        "D:/vcpkg/installed/x64-windows/include/pango-1.0"
        "D:/vcpkg/installed/x64-windows/include/harfbuzz"
        "D:/vcpkg/installed/x64-windows/include/cairo"
        "D:/vcpkg/installed/x64-windows/include/gdk-pixbuf-2.0"
        "D:/vcpkg/installed/x64-windows/include/graphene-1.0"
        "D:/vcpkg/installed/x64-windows/lib/graphene-1.0/include"
    )
    
    # Set GTK4 library directories and libraries
    set(GTK4_LIBRARY_DIRS "D:/vcpkg/installed/x64-windows/lib")
    set(GTK4_LIBRARIES
        gtk-4 glib-2.0 gobject-2.0 gio-2.0 pango-1.0 pangocairo-1.0
        cairo cairo-gobject gdk_pixbuf-2.0 harfbuzz graphene-1.0
    )
else()
    set(GTK4_FOUND FALSE)
    message(STATUS "GTK4 not found - vcpkg installation not detected")
endif()

# 查找其他依赖包
find_package(boost_filesystem CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
find_package(boost_thread CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found")
endif()

# 包含子目录
add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/ui)
add_subdirectory(src/app)

# 主可执行文件
add_executable(ow_cat
    src/main.cpp
)

# 链接库
target_link_libraries(ow_cat
    PRIVATE
    ow_cat_core
    ow_cat_platform
    ow_cat_ui
    ow_cat_app
    Threads::Threads
)

# 设置包含目录
target_include_directories(ow_cat
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 编译选项
if(MSVC)
    target_compile_options(ow_cat PRIVATE /W4)
else()
    target_compile_options(ow_cat PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 安装规则
install(TARGETS ow_cat
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)