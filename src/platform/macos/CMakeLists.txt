# macOS Platform Implementation

if(APPLE)
    # Find required macOS frameworks
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(CARBON_FRAMEWORK Carbon)
    find_library(INPUT_METHOD_KIT_FRAMEWORK InputMethodKit)
    find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
    find_library(SECURITY_FRAMEWORK Security)
    find_library(SERVICE_MANAGEMENT_FRAMEWORK ServiceManagement)
    
    if(NOT COCOA_FRAMEWORK)
        message(FATAL_ERROR "Cocoa framework not found")
    endif()
    
    if(NOT CARBON_FRAMEWORK)
        message(FATAL_ERROR "Carbon framework not found")
    endif()
    
    if(NOT INPUT_METHOD_KIT_FRAMEWORK)
        message(FATAL_ERROR "InputMethodKit framework not found")
    endif()
    
    if(NOT QUARTZ_CORE_FRAMEWORK)
        message(FATAL_ERROR "QuartzCore framework not found")
    endif()
    
    if(NOT SECURITY_FRAMEWORK)
        message(FATAL_ERROR "Security framework not found")
    endif()
    
    if(NOT SERVICE_MANAGEMENT_FRAMEWORK)
        message(FATAL_ERROR "ServiceManagement framework not found")
    endif()
    
    # Create macOS platform library
    add_library(owcat_platform_macos STATIC
        macos_ime_adapter.cpp
        macos_input_controller.cpp
        macos_candidate_window.cpp
        macos_system_integration.cpp
    )
    
    # Set target properties
    set_target_properties(owcat_platform_macos PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )
    
    # Include directories
    target_include_directories(owcat_platform_macos
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Link frameworks
    target_link_libraries(owcat_platform_macos
        PUBLIC
            ${COCOA_FRAMEWORK}
            ${CARBON_FRAMEWORK}
            ${INPUT_METHOD_KIT_FRAMEWORK}
            ${QUARTZ_CORE_FRAMEWORK}
            ${SECURITY_FRAMEWORK}
            ${SERVICE_MANAGEMENT_FRAMEWORK}
    )
    
    # Compiler flags for Objective-C++
    target_compile_options(owcat_platform_macos PRIVATE
        -fobjc-arc  # Enable Automatic Reference Counting
        -Wno-deprecated-declarations  # Suppress deprecation warnings for older APIs
    )
    
    # Define macOS-specific preprocessor macros
    target_compile_definitions(owcat_platform_macos
        PUBLIC
            OWCAT_PLATFORM_MACOS=1
        PRIVATE
            OWCAT_MACOS_IME_BUNDLE_ID="com.owcat.ime"
            OWCAT_MACOS_IME_NAME="OwCat IME"
    )
    
    # Set file extensions for Objective-C++
    set_source_files_properties(
        macos_ime_adapter.cpp
        macos_input_controller.cpp
        macos_candidate_window.cpp
        macos_system_integration.cpp
        PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
    
    # Install targets
    install(TARGETS owcat_platform_macos
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
    
    # Install headers
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/platform/macos/
        DESTINATION include/platform/macos
        FILES_MATCHING PATTERN "*.h"
    )
    
    message(STATUS "macOS platform support enabled")
    message(STATUS "  Cocoa Framework: ${COCOA_FRAMEWORK}")
    message(STATUS "  Carbon Framework: ${CARBON_FRAMEWORK}")
    message(STATUS "  InputMethodKit Framework: ${INPUT_METHOD_KIT_FRAMEWORK}")
    message(STATUS "  QuartzCore Framework: ${QUARTZ_CORE_FRAMEWORK}")
    message(STATUS "  Security Framework: ${SECURITY_FRAMEWORK}")
    message(STATUS "  ServiceManagement Framework: ${SERVICE_MANAGEMENT_FRAMEWORK}")
    
else()
    message(STATUS "macOS platform support disabled (not building on macOS)")
endif()