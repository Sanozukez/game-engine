// src/main.cpp
#define GLFW_INCLUDE_NONE
#include "../client/render/opengl_types.h"
#include <GLFW/glfw3.h>

#include "client/app/app.h"

#include <iostream>

int main() {
    std::cout << "[MAIN] Creating App..." << std::endl;
    
    App app;
    
    std::cout << "[MAIN] Running App..." << std::endl;
    app.run();
    
    std::cout << "[MAIN] App finished." << std::endl;
    return 0;
}