# 核心引擎模块
set(CORE_SOURCES
    engine.cpp
    pinyin_converter.cpp
    dictionary_manager.cpp
    prediction_engine.cpp
    llama_predictor.cpp
)

set(CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/pinyin_converter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/dictionary_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/prediction_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/llama_predictor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core/types.h
)

# 创建核心库
add_library(ow_cat_core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# 设置包含目录
target_include_directories(ow_cat_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖
target_link_libraries(ow_cat_core
    PUBLIC
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
    Boost::filesystem
    Boost::system
)

# 如果启用llama.cpp功能
if(TARGET llama)
    target_link_libraries(ow_cat_core PUBLIC llama)
    target_compile_definitions(ow_cat_core PUBLIC ENABLE_LLAMA_CPP)
endif()

# 编译选项
if(MSVC)
    target_compile_options(ow_cat_core PRIVATE /W4)
else()
    target_compile_options(ow_cat_core PRIVATE -Wall -Wextra -Wpedantic)
endif()