# 应用程序层模块
set(APP_SOURCES
    application.cpp
    config_manager.cpp
    service_manager.cpp
    event_dispatcher.cpp
)

set(APP_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/app/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/app/config_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/app/service_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/app/event_dispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/app/app_types.h
)

# 创建应用程序库
add_library(ow_cat_app STATIC
    ${APP_SOURCES}
    ${APP_HEADERS}
)

# 设置包含目录
target_include_directories(ow_cat_app
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖
target_link_libraries(ow_cat_app
    PUBLIC
    ow_cat_core
    ow_cat_platform
    ow_cat_ui
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    Boost::filesystem
    Boost::system
    Boost::thread
)

# 编译选项
if(MSVC)
    target_compile_options(ow_cat_app PRIVATE /W4)
else()
    target_compile_options(ow_cat_app PRIVATE -Wall -Wextra -Wpedantic)
endif()