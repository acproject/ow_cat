cmake_minimum_required(VERSION 3.20)
project(ow_cat VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 启用vcpkg工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

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

# GTK依赖
if(UNIX AND NOT APPLE)
    pkg_check_modules(GTK4 REQUIRED gtk+-4.0)
elseif(WIN32)
    find_package(unofficial-gtk3 CONFIG REQUIRED)
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