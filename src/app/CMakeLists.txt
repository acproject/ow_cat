# 应用程序层模块
# 暂时创建空的接口库，等待实现
add_library(ow_cat_app INTERFACE)

# 设置包含目录
target_include_directories(ow_cat_app
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# 链接依赖
target_link_libraries(ow_cat_app
    INTERFACE
    ow_cat_core
    ow_cat_platform
    ow_cat_ui
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)