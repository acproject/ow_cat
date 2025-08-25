# 平台适配层模块
set(PLATFORM_COMMON_SOURCES
    platform_manager.cpp
)

set(PLATFORM_COMMON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/platform_manager.h
)

# 平台特定源文件
set(PLATFORM_SPECIFIC_SOURCES)
set(PLATFORM_SPECIFIC_HEADERS)
set(PLATFORM_SPECIFIC_LIBS)

# Windows平台
if(WIN32)
    list(APPEND PLATFORM_SPECIFIC_SOURCES
        windows/windows_candidate_window.cpp
        windows/windows_ime_adapter.cpp
        windows/windows_ime_message_handler.cpp
        windows/windows_system_integration.cpp
    )
    list(APPEND PLATFORM_SPECIFIC_LIBS
        user32
        imm32
        ole32
        oleaut32
        uuid
    )
endif()

# macOS平台
if(APPLE)
    list(APPEND PLATFORM_SPECIFIC_SOURCES
        macos/macos_ime.cpp
        macos/input_method_kit.cpp
    )
    list(APPEND PLATFORM_SPECIFIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/macos/macos_ime.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/macos/input_method_kit.h
    )
    find_library(CARBON_FRAMEWORK Carbon)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(INPUT_METHOD_KIT_FRAMEWORK InputMethodKit)
    list(APPEND PLATFORM_SPECIFIC_LIBS
        ${CARBON_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${INPUT_METHOD_KIT_FRAMEWORK}
    )
endif()

# Linux平台
if(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SPECIFIC_SOURCES
        linux/linux_ime.cpp
        linux/ibus_adapter.cpp
        linux/fcitx_adapter.cpp
    )
    list(APPEND PLATFORM_SPECIFIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/linux/linux_ime.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/linux/ibus_adapter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform/linux/fcitx_adapter.h
    )
    
    # 查找IBus和Fcitx依赖
    pkg_check_modules(IBUS ibus-1.0)
    pkg_check_modules(FCITX fcitx)
    
    if(IBUS_FOUND)
        list(APPEND PLATFORM_SPECIFIC_LIBS ${IBUS_LIBRARIES})
        target_compile_definitions(ow_cat_platform PRIVATE ENABLE_IBUS)
    endif()
    
    if(FCITX_FOUND)
        list(APPEND PLATFORM_SPECIFIC_LIBS ${FCITX_LIBRARIES})
        target_compile_definitions(ow_cat_platform PRIVATE ENABLE_FCITX)
    endif()
endif()

# 创建平台适配库
add_library(ow_cat_platform STATIC
    ${PLATFORM_COMMON_SOURCES}
    ${PLATFORM_SPECIFIC_SOURCES}
    ${PLATFORM_COMMON_HEADERS}
    ${PLATFORM_SPECIFIC_HEADERS}
)

# 设置包含目录
target_include_directories(ow_cat_platform
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖
target_link_libraries(ow_cat_platform
    PUBLIC
    ow_cat_core
    spdlog::spdlog
    ${PLATFORM_SPECIFIC_LIBS}
)

# 编译选项
if(MSVC)
    target_compile_options(ow_cat_platform PRIVATE /W4)
else()
    target_compile_options(ow_cat_platform PRIVATE -Wall -Wextra -Wpedantic)
endif()