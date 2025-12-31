// src/editor/editor_main.cpp
// Editor visual para a engine - Tela inicial básica

#define GLFW_INCLUDE_NONE
#include "client/render/opengl_types.h"
#include <GLFW/glfw3.h>

#include "editor_app.h"

#include <iostream>

int main() {
    std::cout << "[EDITOR] Iniciando editor visual..." << std::endl;
    
    EditorApp editor;
    
    std::cout << "[EDITOR] Executando loop principal..." << std::endl;
    editor.run();
    
    std::cout << "[EDITOR] Editor encerrado." << std::endl;
    return 0;
}
