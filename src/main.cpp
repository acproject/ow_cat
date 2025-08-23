#include <iostream>
#include <memory>
#include "app/application.h"
#include "core/engine.h"
#include "platform/platform_manager.h"
#include "ui/ui_manager.h"

int main(int argc, char* argv[]) {
    try {
        // 初始化应用程序
        auto app = std::make_unique<owcat::Application>(argc, argv);
        
        // 运行应用程序
        return app->run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return -1;
    }
}