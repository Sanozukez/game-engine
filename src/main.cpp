// src/main.cpp
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

// Apenas inclua o app.h, que já deve cuidar de Scene e FreeCamera.
// Se main.cpp precisar acessar FreeCamera diretamente, então inclua free_camera.h.
// No seu caso, main.cpp apenas cria um App e chama run(), então não precisa de Scene/FreeCamera diretamente aqui.
#include "app/app.h" 

#include <iostream>

int main() {
    App app;
    app.run();
    return 0;
}