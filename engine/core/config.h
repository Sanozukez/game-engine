// engine/core/config.h
#pragma once

namespace Engine {

// Define qual câmera será instanciada por padrão na Scene.
// Altere para 'false' para usar a OrbitCamera como padrão de desenvolvimento.
constexpr bool CAMERA_DEFAULT_IS_FREE = false; 

// **** NOVO: Configurações padrão da Janela ****
constexpr int DEFAULT_WINDOW_WIDTH = 1280;
constexpr int DEFAULT_WINDOW_HEIGHT = 720;
constexpr bool DEFAULT_WINDOW_FULLSCREEN = false; // Defina para true para iniciar em tela cheia
constexpr bool DEFAULT_WINDOW_RESIZABLE = false;  // Bloqueia redimensionamento manual

// Outras configurações globais do motor podem vir aqui no futuro.

} // namespace Engine