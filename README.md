# Game Engine (C++ Modular Engine)

Este projeto é uma engine modular em C++ com ImGui + OpenGL + GLFW + GLAD + GLM, estruturada com foco em Single Responsibility Principle (SRP) e modularização, ideal para jogos 2.5D estilo Rohan / MU Online.

📁 Estrutura de Diretórios
bash
Copiar
Editar

```
game-engine/
├── build/      # Arquivos gerados pela compilação (binários, intermediários)
├── assets/
│ ├── models/
│ │  └── my_test_model.obj (modelo simples de terreno (mundo) para teste inicial)
│ └── textures/
│    └── (imagens de textura .png ou .jpg e outras)
├── cmake/
│ └── dependencies.cmake # Setup de dependências via FetchContent
├── config/
│ └── engine_settings.json # Configurações diversas de jogo (in-game)
├── engine/
│ ├── asset/
│ │ ├── CmakeLists.txt
│ │ ├── gltf_loader_impl.cpp
│ │ ├── model.cpp
│ │ ├── model.h
│ │ ├── obj_loader.cpp
│ │ └── obj_loader.h
│ ├── camera/
│ │ ├── camera.cpp
│ │ └── camera.h
│ ├── core/     # Utilitários centrais (logs, config, helpers)
│ │ ├── CmakeLists.txt
│ │ ├── config.h (deprecated)
│ │ ├── config_manager.h
│ │ ├── config_manager.cpp
│ │ ├── log.cpp
│ │ ├── log.h
│ │ ├── path_utils.cpp
│ │ └── path_utils.h
│ ├── deps/ 
│ │ ├── CMakeLists.txt
│ │ ├── cgltf.h
│ │ └── stb_image.h
│ ├── ecs/      # Sistema de entidades e componentes (Entity Component System)
│ ├── geometry/ # Formas 3d basicas para teste e outras.
│ ├── input/    # Gerenciamento de input (teclado, mouse, gamepad)
│ │ ├── CMakeLists.txt
│ │ ├── input_manager.cpp
│ │ └── input_manager.h
│ ├── audio/    # Áudio e música (OpenAL, FMOD etc.)
│ ├── physics/  # Física, colisões, integração com motores como Box2D/Bullet
│ ├── scene/    # Sistema de cenas e carregamento de mundo
│ ├── shaders/ 
│ │ ├── basic.frag
│ │ └── basic.vert
│ ├── render/
│ │ ├── CMakeLists.txt
│ │ ├── opengl_renderer.cpp
│ │ ├── opengl_renderer.h
│ │ ├── renderer.cpp
│ │ ├── renderer.h
│ │ ├── shader.cpp
│ │ ├── shader.h
│ │ ├── texture.cpp
│ │ ├── texture.h
│ │ └── camera/
│ │     ├── CMakeLists.txt
│ │     ├── free_camera.cpp
│ │     ├── free_camera.h
│ │     ├── icamera.h   
│ │     ├── orbit_camera.cpp
│ │     └── orbit_camera.h
│ ├── ui/
│ │ ├── CMakeLists.txt
│ │ ├── imgui_layer.cpp
│ │ └── imgui_layer.h
│ └── window/
│   ├── CMakeLists.txt
│   ├── window.cpp
│   └── window.h
├── glad/           # Código fonte do GLAD (OpenGL loader)
├── src/
│ ├── app
│ │ ├── app.cpp
│ │ ├── app.h
│ │ ├── input.cpp
│ │ ├── input.h
│ │ ├── scene.cpp
│ │ ├── scene.h
│ │ ├── setup_glfw.cpp
│ │ └── setup_glfw.h
│ ├── CMakeLists.txt
│ └── main.cpp      # Ponto de entrada da aplicação
├── CMakeLists.txt  # CMake principal
├── imgui.ini       # Arquivo de configuração salvo pelo ImGui
└── README.md
```
🚀 Como compilar
1. Clone o projeto
bash
Copiar
Editar
git clone https://github.com/seu-usuario/game-engine.git
cd game-engine
2. Configure com CMake
css
Copiar
Editar
cmake -S . -B build
3. Compile
css
Copiar
Editar
cmake --build build
O executável será gerado em:
build/src/Debug/game-engine.exe

🧰 Stack Utilizada
C++17

GLFW – Criação e controle da janela

GLAD – Loader de funções OpenGL

ImGui – Interface gráfica

GLM – Math para gráficos 3D

CMake – Build system

FetchContent – Gerência de dependências

🧱 Design do Projeto
Modularização por SRP: A engine é dividida em window, render e ui.

Separação entre Engine e Executável:
engine/ é uma biblioteca (STATIC) e src/ define o ponto de entrada.

Flexível para expansão: Pode adicionar novos módulos ou testes criando outras pastas ao lado de src/.

✨ Próximos passos
 Criar sistema de logging

 Abstração completa de janela e renderizador

 Sistema de eventos e entrada (teclado/mouse)

 Loop principal desacoplado

 Separar lógica da GUI e lógica de renderização


cmake -B build
cmake --build build
cd build/src/Debug
./game-engine.exe

#define GLFW_INCLUDE_NONE