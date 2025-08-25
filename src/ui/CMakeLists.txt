# UI层模块
# GTK4 configuration is inherited from parent CMakeLists.txt

# Check if GTK is available (configured in parent)
if(GTK4_FOUND)
    message(STATUS "GTK4 found: ${GTK4_VERSION}")
    set(OWCAT_USE_GTK ON)
else()
    message(WARNING "GTK4 not found, UI will be disabled")
    set(OWCAT_USE_GTK OFF)
endif()

set(UI_SOURCES
    # GTK UI implementation
    gtk_ui.cpp
    gtk_main_window.cpp
    gtk_candidate_window.cpp
    gtk_settings_dialog.cpp
    gtk_about_dialog.cpp
    gtk_status_icon.cpp
    gtk_theme_manager.cpp
)

set(UI_HEADERS
    # GTK UI headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/ui/gtk_ui.h
)

# 创建UI库
add_library(ow_cat_ui STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
)

# 设置包含目录
target_include_directories(ow_cat_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GTK4_INCLUDE_DIRS}
)

# GTK依赖配置 - 使用vcpkg配置的库
if(OWCAT_USE_GTK)
    target_link_directories(ow_cat_ui PRIVATE ${GTK4_LIBRARY_DIRS})
    target_link_libraries(ow_cat_ui
        PUBLIC
            ${GTK4_LIBRARIES}
    )
endif()

# 链接其他依赖
target_link_libraries(ow_cat_ui
    PUBLIC
    ow_cat_core
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# 定义预处理器宏
if(OWCAT_USE_GTK)
    target_compile_definitions(ow_cat_ui
        PRIVATE
            OWCAT_USE_GTK=1
            GTK_VERSION_MIN_REQUIRED=GTK_VERSION_4_0
            GTK_VERSION_MAX_ALLOWED=GTK_VERSION_4_12
    )
else()
    target_compile_definitions(ow_cat_ui
        PRIVATE
            OWCAT_USE_GTK=0
    )
endif()

# 平台特定设置
if(WIN32)
    target_compile_definitions(ow_cat_ui PRIVATE OWCAT_PLATFORM_WINDOWS=1)
    if(OWCAT_USE_GTK)
        target_compile_definitions(ow_cat_ui PRIVATE
            G_OS_WIN32=1
            GDK_WINDOWING_WIN32=1
        )
    endif()
elseif(APPLE)
    target_compile_definitions(ow_cat_ui PRIVATE OWCAT_PLATFORM_MACOS=1)
    if(OWCAT_USE_GTK)
        target_compile_definitions(ow_cat_ui PRIVATE
            GDK_WINDOWING_QUARTZ=1
        )
    endif()
elseif(UNIX)
    target_compile_definitions(ow_cat_ui PRIVATE OWCAT_PLATFORM_LINUX=1)
    if(OWCAT_USE_GTK)
        target_compile_definitions(ow_cat_ui PRIVATE
            GDK_WINDOWING_X11=1
        )
    endif()
endif()

# 编译选项
if(MSVC)
    target_compile_options(ow_cat_ui PRIVATE /W4)
else()
    target_compile_options(ow_cat_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug/Release特定设置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ow_cat_ui PRIVATE
        OWCAT_DEBUG=1
        G_ENABLE_DEBUG=1
    )
else()
    target_compile_definitions(ow_cat_ui PRIVATE
        OWCAT_RELEASE=1
        G_DISABLE_ASSERT=1
        G_DISABLE_CHECKS=1
    )
endif()

# 添加GTK资源文件（如果有的话）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
    # 这里可以添加GTK资源编译逻辑
endif()

# 打印配置摘要
if(OWCAT_USE_GTK)
    message(STATUS "UI Configuration:")
    message(STATUS "  GTK4 Version: ${GTK4_VERSION}")
    message(STATUS "  GTK4 Libraries: ${GTK4_LIBRARIES}")
    message(STATUS "  GTK4 Include Dirs: ${GTK4_INCLUDE_DIRS}")
    message(STATUS "  UI Library: ow_cat_ui (STATIC)")
    message(STATUS "  UI Sources: ${UI_SOURCES}")
else()
    message(STATUS "UI Configuration: DISABLED (GTK4 not found)")
endif()

# 导出配置给父CMakeLists.txt
set(OWCAT_USE_GTK ${OWCAT_USE_GTK} PARENT_SCOPE)