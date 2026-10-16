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
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace {
    constexpr const char* kEditorTitle = "Game Engine Editor v0.1";

    std::string TrimCopy(const std::string& value) {
        auto begin = value.begin();
        while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
            ++begin;
        }

        auto end = value.end();
        while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
            --end;
        }

        return std::string(begin, end);
    }

    bool StartsWith(const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    }
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
    std::filesystem::create_directories(projectRoot / "assets" / "sky");
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
                << "terrain_source: project/terrains/default_terrain\n"
                << "sky.procedural_enabled: 1\n"
                << "sky.sun_yaw_deg: 35.0\n"
                << "sky.sun_pitch_deg: 22.0\n"
                << "sky.cloud_speed: 0.0035\n"
                << "sky.cloud_direction_deg: 18.0\n"
                << "sky.cloud_coverage: 0.54\n"
                << "sky.cloud_softness: 0.08\n"
                << "sky.cloud_scale: 1.75\n"
                << "sky.cloud_opacity: 0.72\n"
                << "sky.zenith_color: 0.20,0.39,0.70\n"
                << "sky.horizon_color: 0.70,0.78,0.90\n"
                << "sky.cloud_color: 0.92,0.93,0.95\n";
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
    const std::filesystem::path sceneFilePath = projectRoot / "scenes" / "starter_scene.scene";

    if (std::filesystem::exists(terrainPath)) {
        m_sceneAssetPath = "project/terrains/default_terrain.mmap";
        m_sceneDefinitionPath = "project/scenes/starter_scene.scene";
        m_sceneLoaded = true;
        m_terrainReady = true;
        m_isEmptyScene = false;
        m_sceneName = "StarterScene";
        std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
        m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
        m_selectedObject = "Wall";
        loadSkySettingsFromSceneFile(sceneFilePath);
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
    m_sceneDefinitionPath = "project/scenes/starter_scene.scene";
    m_sceneLoaded = false;
    m_terrainReady = false;
    m_isEmptyScene = true;
    m_sceneName = "StarterScene";
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    m_selectedObject = "Wall";
    loadSkySettingsFromSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
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

void EditorApp::loadSkySettingsFromSceneFile(const std::filesystem::path& sceneFilePath) {
    std::ifstream in(sceneFilePath);
    if (!in.is_open()) {
        Engine::Core::Log::Warn(std::string("[Editor] Nao foi possivel abrir arquivo de cena para carregar sky: ") + sceneFilePath.string());
        return;
    }

    auto parseFloat = [](const std::string& value, float fallback) -> float {
        try {
            return std::stof(TrimCopy(value));
        } catch (...) {
            return fallback;
        }
    };

    auto parseBool = [](const std::string& value, bool fallback) -> bool {
        const std::string normalized = TrimCopy(value);
        if (normalized == "1" || normalized == "true" || normalized == "TRUE") {
            return true;
        }
        if (normalized == "0" || normalized == "false" || normalized == "FALSE") {
            return false;
        }
        return fallback;
    };

    auto parseColor3 = [&](const std::string& value, const glm::vec3& fallback) -> glm::vec3 {
        std::stringstream ss(value);
        std::string chunk;
        std::vector<float> channels;
        while (std::getline(ss, chunk, ',')) {
            channels.push_back(parseFloat(chunk, 0.0f));
        }
        if (channels.size() != 3) {
            return fallback;
        }
        return glm::vec3(channels[0], channels[1], channels[2]);
    };

    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const std::size_t separatorPos = trimmed.find(':');
        if (separatorPos == std::string::npos) {
            continue;
        }

        const std::string key = TrimCopy(trimmed.substr(0, separatorPos));
        const std::string value = TrimCopy(trimmed.substr(separatorPos + 1));

        if (key == "sky.procedural_enabled") {
            m_sky.proceduralEnabled = parseBool(value, m_sky.proceduralEnabled);
        } else if (key == "sky.sun_yaw_deg") {
            m_sky.sunYawDeg = parseFloat(value, m_sky.sunYawDeg);
        } else if (key == "sky.sun_pitch_deg") {
            m_sky.sunPitchDeg = parseFloat(value, m_sky.sunPitchDeg);
        } else if (key == "sky.cloud_speed") {
            m_sky.cloudSpeed = parseFloat(value, m_sky.cloudSpeed);
        } else if (key == "sky.cloud_direction_deg") {
            m_sky.cloudDirectionDeg = parseFloat(value, m_sky.cloudDirectionDeg);
        } else if (key == "sky.cloud_coverage") {
            m_sky.cloudCoverage = parseFloat(value, m_sky.cloudCoverage);
        } else if (key == "sky.cloud_softness") {
            m_sky.cloudSoftness = parseFloat(value, m_sky.cloudSoftness);
        } else if (key == "sky.cloud_scale") {
            m_sky.cloudScale = parseFloat(value, m_sky.cloudScale);
        } else if (key == "sky.cloud_opacity") {
            m_sky.cloudOpacity = parseFloat(value, m_sky.cloudOpacity);
        } else if (key == "sky.zenith_color") {
            m_sky.zenithColor = parseColor3(value, m_sky.zenithColor);
        } else if (key == "sky.horizon_color") {
            m_sky.horizonColor = parseColor3(value, m_sky.horizonColor);
        } else if (key == "sky.cloud_color") {
            m_sky.cloudColor = parseColor3(value, m_sky.cloudColor);
        }
    }

    Engine::Core::Log::Info(std::string("[Editor] Sky carregado da cena: ") + sceneFilePath.string());
}

void EditorApp::saveSkySettingsToSceneFile(const std::filesystem::path& sceneFilePath) const {
    std::vector<std::string> preservedLines;

    {
        std::ifstream in(sceneFilePath);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                const std::string trimmed = TrimCopy(line);
                if (StartsWith(trimmed, "sky.")) {
                    continue;
                }
                preservedLines.push_back(line);
            }
        }
    }

    const std::filesystem::path parentPath = sceneFilePath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    std::ofstream out(sceneFilePath, std::ios::trunc);
    if (!out.is_open()) {
        Engine::Core::Log::Error(std::string("[Editor] Falha ao salvar sky no arquivo de cena: ") + sceneFilePath.string());
        return;
    }

    for (const std::string& line : preservedLines) {
        out << line << "\n";
    }

    out << "sky.procedural_enabled: " << (m_sky.proceduralEnabled ? 1 : 0) << "\n";
    out << "sky.sun_yaw_deg: " << m_sky.sunYawDeg << "\n";
    out << "sky.sun_pitch_deg: " << m_sky.sunPitchDeg << "\n";
    out << "sky.cloud_speed: " << m_sky.cloudSpeed << "\n";
    out << "sky.cloud_direction_deg: " << m_sky.cloudDirectionDeg << "\n";
    out << "sky.cloud_coverage: " << m_sky.cloudCoverage << "\n";
    out << "sky.cloud_softness: " << m_sky.cloudSoftness << "\n";
    out << "sky.cloud_scale: " << m_sky.cloudScale << "\n";
    out << "sky.cloud_opacity: " << m_sky.cloudOpacity << "\n";
    out << "sky.zenith_color: " << m_sky.zenithColor.r << "," << m_sky.zenithColor.g << "," << m_sky.zenithColor.b << "\n";
    out << "sky.horizon_color: " << m_sky.horizonColor.r << "," << m_sky.horizonColor.g << "," << m_sky.horizonColor.b << "\n";
    out << "sky.cloud_color: " << m_sky.cloudColor.r << "," << m_sky.cloudColor.g << "," << m_sky.cloudColor.b << "\n";

    Engine::Core::Log::Info(std::string("[Editor] Sky salvo no arquivo de cena: ") + sceneFilePath.string());
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


