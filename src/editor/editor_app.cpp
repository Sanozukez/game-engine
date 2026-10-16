// src/editor/editor_app.cpp
// Implementação do Editor Visual - Primeira arquitetura da interface

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#endif

#include "editor_app.h"
#include "editor_theme.h"
#include "render/opengl_types.h"
#include "window/window.h"
#include "core/log.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace {
    constexpr const char* kEditorTitle = "Game Engine Editor v0.1";
}

EditorApp::EditorApp() {
    Engine::Core::Log::Info("[Editor] Construtor chamado");
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    ensureProjectStructure();
    loadSceneDefinition();
    initWindow();
    initImGui();
    m_isInitialized = true;
}

EditorApp::~EditorApp() {
    shutdownImGui();
    Engine::Core::Log::Info("[Editor] Destrutor chamado");
}

void EditorApp::initWindow() {
    Engine::WindowConfig config;
    config.width = 1600;
    config.height = 900;
    config.title = kEditorTitle;
    config.resizable = true;
    config.fullscreen = false;
    config.maximized = true;

    m_window = std::make_unique<Engine::Window>(config);
    Engine::Core::Log::Info("[Editor] Janela criada");
}

void EditorApp::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    setupStyle();

    ImGui_ImplGlfw_InitForOpenGL(m_window->getGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    Engine::Core::Log::Info("[Editor] ImGui inicializado");
}

void EditorApp::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ApplyEditorTheme(style, m_visualStyle);
    Engine::Core::Log::Info("[Editor] Estilo visual do editor configurado");
}

void EditorApp::shutdownImGui() {
    if (!m_isInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Engine::Core::Log::Info("[Editor] ImGui finalizado");
}

void EditorApp::run() {
    Engine::Core::Log::Info("[Editor] Iniciando loop principal");

    while (!m_window->shouldClose()) {
        if (m_themeDirty) {
            setupStyle();
            m_themeDirty = false;
        }

        glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMenuBar();
        renderMainContent();
        renderStatusBar();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_window->swapBuffersAndPollEvents();
    }

    Engine::Core::Log::Info("[Editor] Loop principal encerrado");
}

void EditorApp::ensureProjectStructure() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    std::filesystem::create_directories(projectRoot / "scenes");
    std::filesystem::create_directories(projectRoot / "terrains");
    std::filesystem::create_directories(projectRoot / "assets");
    std::filesystem::create_directories(projectRoot / "config");

    const std::filesystem::path settingsFile = projectRoot / "config" / "project_settings.json";
    if (!std::filesystem::exists(settingsFile)) {
        std::ofstream out(settingsFile);
        if (out.is_open()) {
            out << "{\n"
                << "  \"project\": {\n"
                << "    \"name\": \"StarterGameProject\",\n"
                << "    \"default_scene\": \"scenes/starter_scene.scene\",\n"
                << "    \"terrains_folder\": \"terrains\",\n"
                << "    \"assets_folder\": \"assets\",\n"
                << "    \"scenes_folder\": \"scenes\",\n"
                << "    \"startup_terrain_size\": 50.0\n"
                << "  },\n"
                << "  \"scene\": {\n"
                << "    \"type\": \"starter\",\n"
                << "    \"ground_size\": 50.0,\n"
                << "    \"ground_height\": 0.0,\n"
                << "    \"player_spawn\": [0.0, 1.0, 0.0],\n"
                << "    \"npc_spawn\": [12.0, 1.0, 6.0]\n"
                << "  }\n"
                << "}\n";
            out.close();
        }
    }

    const std::filesystem::path starterSceneFile = projectRoot / "scenes" / "starter_scene.scene";
    if (!std::filesystem::exists(starterSceneFile)) {
        std::ofstream out(starterSceneFile);
        if (out.is_open()) {
            out << "scene_name: StarterScene\n"
                << "scene_type: starter\n"
                << "ground_size: 50\n"
                << "ground_center: 0,0,0\n"
                << "player_spawn: 0,1,0\n"
                << "npc_spawn: 12,1,6\n"
                << "terrain_source: project/terrains/default_terrain\n";
            out.close();
        }
    }

    m_projectReady = true;
    Engine::Core::Log::Info("[Editor] Estrutura mínima do projeto inicializada em: project/");
}

void EditorApp::loadSceneDefinition() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path terrainPath = projectRoot / "terrains" / "default_terrain.mmap";
    const std::filesystem::path fallbackPath = std::filesystem::current_path() / "assets" / "models" / "test_scene_1.mmap";

    if (std::filesystem::exists(terrainPath)) {
        m_sceneAssetPath = "project/terrains/default_terrain.mmap";
        m_sceneLoaded = true;
        m_terrainReady = true;
        m_isEmptyScene = false;
        m_sceneName = "StarterScene";
        std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
        m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
        m_selectedObject = "Wall";
        Engine::Core::Log::Info(std::string("[Editor] Cena carregada a partir do projeto: ") + terrainPath.string());
        return;
    }

    if (std::filesystem::exists(fallbackPath)) {
        m_sceneAssetPath = "assets/models/test_scene_1.mmap";
        m_sceneLoaded = true;
        m_terrainReady = true;
        m_isEmptyScene = false;
        m_sceneName = "TestScene_1";
        std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
        m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
        m_selectedObject = "Wall";
        Engine::Core::Log::Info(std::string("[Editor] Cena carregada do fallback de assets: ") + fallbackPath.string());
        return;
    }

    loadStarterScene();
}

void EditorApp::loadStarterScene() {
    m_sceneAssetPath = "project/terrains/default_terrain.mmap";
    m_sceneLoaded = false;
    m_terrainReady = false;
    m_isEmptyScene = true;
    m_sceneName = "StarterScene";
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    m_selectedObject = "Wall";
    Engine::Core::Log::Warn("[Editor] Nenhuma cena pronta foi encontrada. Iniciando cena mínima padrão do projeto.");
}

void EditorApp::importTerrainIntoProject() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path terrainFolder = projectRoot / "terrains";
    std::filesystem::create_directories(terrainFolder);

    const std::filesystem::path importedTerrain = terrainFolder / "default_terrain.mmap";
    if (!std::filesystem::exists(importedTerrain)) {
        std::ofstream out(importedTerrain);
        if (out.is_open()) {
            out << "type: terrain\n"
                << "name: default_terrain\n"
                << "size: 50\n"
                << "origin: 0,0,0\n"
                << "status: imported_to_project\n";
            out.close();
        }
    }

    m_sceneAssetPath = "project/terrains/default_terrain.mmap";
    m_sceneLoaded = true;
    m_terrainReady = true;
    m_isEmptyScene = false;
    m_sceneName = "StarterScene";
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    Engine::Core::Log::Info("[Editor] Terreno importado para a estrutura do projeto.");
}

void EditorApp::pickTerrainFromFileBrowser() {
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Terrain and Scene Files (*.mmap;*.gltf;*.glb;*.obj)\0*.mmap;*.gltf;*.glb;*.obj\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        Engine::Core::Log::Warn("[Editor] Seleção de terreno cancelada pelo usuário.");
        return;
    }

    const std::filesystem::path selectedFile = fileBuffer;
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path targetFolder = projectRoot / "terrains";
    std::filesystem::create_directories(targetFolder);

    const std::filesystem::path targetFile = targetFolder / selectedFile.filename();
    std::filesystem::copy_file(selectedFile, targetFile, std::filesystem::copy_options::overwrite_existing);

    m_sceneAssetPath = std::string("project/terrains/") + selectedFile.filename().string();
    m_sceneLoaded = true;
    m_terrainReady = true;
    m_isEmptyScene = false;
    m_sceneName = selectedFile.stem().string();
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    Engine::Core::Log::Info(std::string("[Editor] Terreno importado do arquivo: ") + selectedFile.string());
#else
    Engine::Core::Log::Warn("[Editor] File browser de terreno suportado apenas no Windows.");
#endif
}

bool EditorApp::createRenderTexture(unsigned int& textureId, unsigned int& framebufferId, unsigned int& depthId, int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &depthId);
    glBindRenderbuffer(GL_RENDERBUFFER, depthId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glGenFramebuffers(1, &framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthId);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void EditorApp::ensureSceneFramebuffer() {
    ImGuiIO& io = ImGui::GetIO();
    const int targetWidth = static_cast<int>(io.DisplaySize.x * 0.75f);
    const int targetHeight = static_cast<int>(io.DisplaySize.y * 0.60f);

    if (m_sceneFramebuffer == 0 || m_sceneFramebufferWidth != targetWidth || m_sceneFramebufferHeight != targetHeight) {
        if (m_sceneFramebuffer != 0) {
            glDeleteFramebuffers(1, &m_sceneFramebuffer);
            m_sceneFramebuffer = 0;
        }
        if (m_sceneColorTexture != 0) {
            glDeleteTextures(1, &m_sceneColorTexture);
            m_sceneColorTexture = 0;
        }
        if (m_sceneDepthRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &m_sceneDepthRenderbuffer);
            m_sceneDepthRenderbuffer = 0;
        }

        const bool ok = createRenderTexture(m_sceneColorTexture, m_sceneFramebuffer, m_sceneDepthRenderbuffer, targetWidth, targetHeight);
        m_sceneFramebufferWidth = ok ? targetWidth : 0;
        m_sceneFramebufferHeight = ok ? targetHeight : 0;
    }
}


