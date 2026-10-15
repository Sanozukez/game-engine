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
#include "editor_layout.h"
#include "render/opengl_types.h"
#include "window/window.h"
#include "core/log.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr const char* kEditorTitle = "Game Engine Editor v0.1";

    inline float SafeMax(float a, float b) {
        return (a > b) ? a : b;
    }

    inline float SafeMin(float a, float b) {
        return (a < b) ? a : b;
    }

    inline float SafeClamp(float value, float lo, float hi) {
        if (hi < lo) {
            std::swap(lo, hi);
        }
        return std::clamp(value, lo, hi);
    }

    inline ImVec2 ProjectToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, const ImVec2& viewportSize) {
        const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);
        if (std::abs(clip.w) < 1e-6f) {
            return ImVec2(0.0f, 0.0f);
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const float x = (ndc.x * 0.5f + 0.5f) * viewportSize.x;
        const float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y;
        return ImVec2(x, y);
    }

    inline ImVec2 ScaleVec2(const ImVec2& v, float scale) {
        return ImVec2(v.x * scale, v.y * scale);
    }

    inline ImVec2 AddScaled(const ImVec2& a, const ImVec2& b, float scale) {
        return ImVec2(a.x + (b.x - a.x) * scale, a.y + (b.y - a.y) * scale);
    }

    inline float DistanceToSegmentSquared(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
        const ImVec2 ab = ImVec2(b.x - a.x, b.y - a.y);
        const float denom = ab.x * ab.x + ab.y * ab.y;
        if (denom < 1e-6f) {
            const float dx = p.x - a.x;
            const float dy = p.y - a.y;
            return dx * dx + dy * dy;
        }

        const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom, 0.0f, 1.0f);
        const ImVec2 closest = ImVec2(a.x + ab.x * t, a.y + ab.y * t);
        const float dx = p.x - closest.x;
        const float dy = p.y - closest.y;
        return dx * dx + dy * dy;
    }

    inline std::string PickGizmoAxis(const ImVec2& mousePos, const std::vector<std::pair<std::string, std::pair<ImVec2, ImVec2>>>& axes) {
        float bestDistance = std::numeric_limits<float>::infinity();
        std::string bestAxis = "None";
        for (const auto& [axisName, points] : axes) {
            const float d = DistanceToSegmentSquared(mousePos, points.first, points.second);
            if (d < bestDistance && d < 100.0f) {
                bestDistance = d;
                bestAxis = axisName;
            }
        }
        return bestAxis;
    }

    inline void DrawAxisArrow(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness = 2.0f) {
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length < 0.0001f) {
            return;
        }

        const float nx = dx / length;
        const float ny = dy / length;
        const float px = -ny;
        const float py = nx;

        const float coneLength = 8.0f;
        const float coneRadius = 5.0f;
        const ImVec2 coneBase = ImVec2(end.x - nx * coneLength, end.y - ny * coneLength);
        const ImVec2 left = ImVec2(coneBase.x + px * coneRadius, coneBase.y + py * coneRadius);
        const ImVec2 right = ImVec2(coneBase.x - px * coneRadius, coneBase.y - py * coneRadius);

        drawList->AddLine(start, end, color, thickness);
        drawList->AddTriangleFilled(end, left, right, color);
    }

    inline void DrawBlenderGizmo(ImDrawList* drawList, const ImVec2& gizmoOrigin, const glm::vec3& cameraPosition, const glm::vec3& target, std::vector<std::pair<std::string, std::pair<ImVec2, ImVec2>>>& axisSegments) {
        constexpr float axisLength = 38.0f;

        const glm::vec3 cameraForward = glm::normalize(target - cameraPosition);
        const glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraForward));
        const glm::vec3 cameraUp = glm::normalize(glm::cross(cameraForward, cameraRight));

        const auto makeScreenDir = [](const glm::vec3& worldDir) {
            const float length = std::sqrt(worldDir.x * worldDir.x + worldDir.y * worldDir.y + worldDir.z * worldDir.z);
            if (length < 1e-5f) {
                return ImVec2(0.0f, 0.0f);
            }

            return ImVec2(worldDir.x / length, -worldDir.y / length);
        };

        const ImVec2 xDir = makeScreenDir(cameraRight);
        const ImVec2 yDir = makeScreenDir(cameraUp);
        const ImVec2 zDir = makeScreenDir(cameraForward);

        const ImVec2 xStart = ImVec2(gizmoOrigin.x - xDir.x * axisLength * 0.55f, gizmoOrigin.y - xDir.y * axisLength * 0.55f);
        const ImVec2 xEnd = ImVec2(gizmoOrigin.x + xDir.x * axisLength, gizmoOrigin.y + xDir.y * axisLength);
        const ImVec2 yStart = ImVec2(gizmoOrigin.x - yDir.x * axisLength * 0.55f, gizmoOrigin.y - yDir.y * axisLength * 0.55f);
        const ImVec2 yEnd = ImVec2(gizmoOrigin.x + yDir.x * axisLength, gizmoOrigin.y + yDir.y * axisLength);
        const ImVec2 zStart = ImVec2(gizmoOrigin.x - zDir.x * axisLength * 0.55f, gizmoOrigin.y - zDir.y * axisLength * 0.55f);
        const ImVec2 zEnd = ImVec2(gizmoOrigin.x + zDir.x * axisLength, gizmoOrigin.y + zDir.y * axisLength);

        axisSegments.clear();
        axisSegments.emplace_back("X", std::make_pair(xStart, xEnd));
        axisSegments.emplace_back("Y", std::make_pair(yStart, yEnd));
        axisSegments.emplace_back("Z", std::make_pair(zStart, zEnd));

        DrawAxisArrow(drawList, xStart, xEnd, IM_COL32(255, 90, 90, 255), 2.0f);
        DrawAxisArrow(drawList, yStart, yEnd, IM_COL32(120, 220, 120, 255), 2.0f);
        DrawAxisArrow(drawList, zStart, zEnd, IM_COL32(110, 120, 255, 255), 2.0f);

        drawList->AddText(ImVec2(xEnd.x + 8.0f, xEnd.y - 8.0f), IM_COL32(255, 255, 255, 255), "X");
        drawList->AddText(ImVec2(yEnd.x - 12.0f, yEnd.y - 12.0f), IM_COL32(255, 255, 255, 255), "Y");
        drawList->AddText(ImVec2(zEnd.x + 8.0f, zEnd.y + 8.0f), IM_COL32(255, 255, 255, 255), "Z");
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
    ApplyEditorTheme(style);
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
        m_selectedObject = "Ground";
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
        m_selectedObject = "Ground";
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
    m_selectedObject = "Ground";
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

static GLuint createSceneShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uProjection;
        uniform mat4 uView;
        uniform mat4 uModel;
        out vec3 vColor;
        void main() {
            gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vColor;
        uniform vec3 uColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor * uColor, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

static glm::mat4 buildSceneView(float yaw, float pitch, float distance, const glm::vec3& target) {
    const glm::vec3 cameraPosition = target + glm::vec3(
        std::cos(yaw) * std::cos(pitch) * distance,
        std::sin(pitch) * distance,
        std::sin(yaw) * std::cos(pitch) * distance
    );
    return glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

static bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& minBounds, const glm::vec3& maxBounds, float& tHit) {
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(rayDirection[axis]) < 1e-6f) {
            if (rayOrigin[axis] < minBounds[axis] || rayOrigin[axis] > maxBounds[axis]) {
                return false;
            }
        } else {
            const float inv = 1.0f / rayDirection[axis];
            float t1 = (minBounds[axis] - rayOrigin[axis]) * inv;
            float t2 = (maxBounds[axis] - rayOrigin[axis]) * inv;

            if (t1 > t2) {
                std::swap(t1, t2);
            }

            tMin = SafeMax(tMin, t1);
            tMax = SafeMin(tMax, t2);

            if (tMax < tMin) {
                return false;
            }
        }
    }

    tHit = tMin;
    return true;
}

static std::string pickObjectByRay(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const std::unordered_map<std::string, glm::vec3>& objectPositions,
    const std::unordered_map<std::string, glm::vec3>& objectScales) {
    std::string bestHit = "";
    float bestDistance = std::numeric_limits<float>::infinity();

    for (const auto& [name, position] : objectPositions) {
        const auto scaleIt = objectScales.find(name);
        const glm::vec3 half = scaleIt != objectScales.end() ? scaleIt->second * 0.5f : glm::vec3(1.0f);
        const glm::vec3 minBounds = position - half;
        const glm::vec3 maxBounds = position + half;
        float distance = 0.0f;

        if (rayIntersectsAABB(rayOrigin, rayDirection, minBounds, maxBounds, distance) && distance < bestDistance) {
            bestDistance = distance;
            bestHit = name;
        }
    }

    return bestHit;
}

static void renderSimpleScene3D(
    int width,
    int height,
    float yaw,
    float pitch,
    float distance,
    const glm::vec3& target,
    const std::string& selectedObject,
    const std::unordered_map<std::string, glm::vec3>& objectPositions,
    const std::unordered_map<std::string, glm::vec3>& objectScales) {
    static GLuint program = 0;
    static GLuint gridVao = 0;
    static GLuint gridVbo = 0;
    static GLuint boxVao = 0;
    static GLuint boxVbo = 0;
    static GLuint outlineVao = 0;
    static GLuint outlineVbo = 0;
    static bool initialized = false;

    if (!initialized) {
        program = createSceneShader();

        struct Vertex {
            float x, y, z;
            float r, g, b;
        };

        std::vector<Vertex> gridVertices;
        for (int i = -10; i <= 10; ++i) {
            gridVertices.push_back({float(i), 0.0f, -10.0f, 0.50f, 0.55f, 0.62f});
            gridVertices.push_back({float(i), 0.0f, 10.0f, 0.50f, 0.55f, 0.62f});
            gridVertices.push_back({-10.0f, 0.0f, float(i), 0.50f, 0.55f, 0.62f});
            gridVertices.push_back({10.0f, 0.0f, float(i), 0.50f, 0.55f, 0.62f});
        }

        glGenVertexArrays(1, &gridVao);
        glGenBuffers(1, &gridVbo);
        glBindVertexArray(gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vertex), gridVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        std::vector<Vertex> boxVertices = {
            {-1.0f, -0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, { 1.0f, -0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, { 1.0f,  0.5f,  1.0f,  0.26f, 0.64f, 0.94f}, {-1.0f,  0.5f,  1.0f,  0.26f, 0.64f, 0.94f},
            {-1.0f, -0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, { 1.0f, -0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, { 1.0f,  0.5f, -1.0f,  0.20f, 0.80f, 0.74f}, {-1.0f,  0.5f, -1.0f,  0.20f, 0.80f, 0.74f},
            {-1.0f,  0.5f,  1.0f,  0.72f, 0.47f, 0.27f}, { 1.0f,  0.5f,  1.0f,  0.72f, 0.47f, 0.27f}, { 1.0f,  0.5f, -1.0f,  0.72f, 0.47f, 0.27f}, {-1.0f,  0.5f, -1.0f,  0.72f, 0.47f, 0.27f},
            {-1.0f, -0.5f,  1.0f,  0.31f, 0.76f, 0.41f}, { 1.0f, -0.5f,  1.0f,  0.31f, 0.76f, 0.41f}, { 1.0f, -0.5f, -1.0f,  0.31f, 0.76f, 0.41f}, {-1.0f, -0.5f, -1.0f,  0.31f, 0.76f, 0.41f},
            { 1.0f, -0.5f,  1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f,  0.5f,  1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f,  0.5f, -1.0f,  0.89f, 0.81f, 0.59f}, { 1.0f, -0.5f, -1.0f,  0.89f, 0.81f, 0.59f},
            {-1.0f, -0.5f,  1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f,  0.5f,  1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f,  0.5f, -1.0f,  0.85f, 0.66f, 0.74f}, {-1.0f, -0.5f, -1.0f,  0.85f, 0.66f, 0.74f},
            {-1.0f, -0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, { 1.0f, -0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, { 1.0f,  0.5f,  1.0f,  0.87f, 0.55f, 0.71f}, {-1.0f,  0.5f,  1.0f,  0.87f, 0.55f, 0.71f}
        };

        glGenVertexArrays(1, &boxVao);
        glGenBuffers(1, &boxVbo);
        glBindVertexArray(boxVao);
        glBindBuffer(GL_ARRAY_BUFFER, boxVbo);
        glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(Vertex), boxVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        const std::vector<glm::vec3> outlineVertices = {
            { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f },
            { 1.0f, -1.0f, -1.0f }, { 1.0f,  1.0f, -1.0f },
            { 1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
            { -1.0f,  1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f },
            { -1.0f, -1.0f,  1.0f }, { 1.0f, -1.0f,  1.0f },
            { 1.0f, -1.0f,  1.0f }, { 1.0f,  1.0f,  1.0f },
            { 1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f },
            { -1.0f,  1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f },
            { -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f,  1.0f },
            {  1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f,  1.0f },
            {  1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f,  1.0f },
            { -1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f,  1.0f }
        };

        glGenVertexArrays(1, &outlineVao);
        glGenBuffers(1, &outlineVbo);
        glBindVertexArray(outlineVao);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo);
        glBufferData(GL_ARRAY_BUFFER, outlineVertices.size() * sizeof(glm::vec3), outlineVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindVertexArray(0);

        initialized = true;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(width) / float(height > 0 ? height : 1), 0.1f, 100.0f);
    glm::mat4 view = buildSceneView(yaw, pitch, distance, target);
    glm::mat4 model = glm::mat4(1.0f);

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "uView"), 1, GL_FALSE, &view[0][0]);

    glBindVertexArray(gridVao);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_LINES, 0, 84);

    const glm::vec3 wallPos = objectPositions.count("Wall") ? objectPositions.at("Wall") : glm::vec3(0.0f, 1.5f, -2.0f);
    const glm::vec3 wallScale = objectScales.count("Wall") ? objectScales.at("Wall") : glm::vec3(3.0f, 1.5f, 0.6f);
    const glm::vec3 playerPos = objectPositions.count("Player Spawn") ? objectPositions.at("Player Spawn") : glm::vec3(-4.0f, 1.3f, 0.0f);
    const glm::vec3 playerScale = objectScales.count("Player Spawn") ? objectScales.at("Player Spawn") : glm::vec3(1.0f, 1.5f, 1.0f);
    const glm::vec3 npcPos = objectPositions.count("NPC Spawn") ? objectPositions.at("NPC Spawn") : glm::vec3(4.0f, 1.0f, 1.5f);
    const glm::vec3 npcScale = objectScales.count("NPC Spawn") ? objectScales.at("NPC Spawn") : glm::vec3(0.7f, 1.0f, 0.7f);
    glm::mat4 wallModel = glm::translate(glm::mat4(1.0f), wallPos) * glm::scale(glm::mat4(1.0f), wallScale);
    glm::mat4 playerModel = glm::translate(glm::mat4(1.0f), playerPos) * glm::scale(glm::mat4(1.0f), playerScale);
    glm::mat4 npcModel = glm::translate(glm::mat4(1.0f), npcPos) * glm::scale(glm::mat4(1.0f), npcScale);

    glBindVertexArray(boxVao);
    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &wallModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &playerModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &npcModel[0][0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    glm::mat4 selectedModel = glm::mat4(1.0f);
    if (selectedObject == "Wall") {
        selectedModel = wallModel;
    } else if (selectedObject == "Player Spawn") {
        selectedModel = playerModel;
    } else if (selectedObject == "NPC Spawn") {
        selectedModel = npcModel;
    } else if (selectedObject == "Ground") {
        selectedModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.05f, 0.0f)), glm::vec3(12.0f, 0.1f, 12.0f));
    }

    if (!selectedObject.empty()) {
        glBindVertexArray(outlineVao);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE, &selectedModel[0][0]);
        glUniform3f(glGetUniformLocation(program, "uColor"), 1.0f, 0.8f, 0.4f);
        glDrawArrays(GL_LINES, 0, 24);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
        glBindVertexArray(0);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void EditorApp::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                Engine::Core::Log::Info("[Editor] New Project");
            }
            if (ImGui::MenuItem("Open Project")) {
                Engine::Core::Log::Info("[Editor] Open Project");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save")) {
                Engine::Core::Log::Info("[Editor] Save");
            }
            if (ImGui::MenuItem("Save As")) {
                Engine::Core::Log::Info("[Editor] Save As");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(m_window->getGLFWWindow(), GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            ImGui::MenuItem("Duplicate");
            ImGui::MenuItem("Delete");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Toggle Toolbar", nullptr, m_showToolbar)) {
                m_showToolbar = !m_showToolbar;
            }
            if (ImGui::MenuItem("Toggle Hierarchy", nullptr, m_showHierarchy)) {
                m_showHierarchy = !m_showHierarchy;
            }
            if (ImGui::MenuItem("Toggle Inspector", nullptr, m_showInspector)) {
                m_showInspector = !m_showInspector;
            }
            if (ImGui::MenuItem("Toggle Console", nullptr, m_showConsole)) {
                m_showConsole = !m_showConsole;
            }
            if (ImGui::MenuItem("Toggle Viewport", nullptr, m_showViewport)) {
                m_showViewport = !m_showViewport;
            }
            if (ImGui::MenuItem("Toggle Gizmo Debug", nullptr, m_showGizmoDebug)) {
                m_showGizmoDebug = !m_showGizmoDebug;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(m_layoutLocked ? "Unlock Layout" : "Lock Layout")) {
                m_layoutLocked = !m_layoutLocked;
                Engine::Core::Log::Info(m_layoutLocked ? "[Editor] Layout travado" : "[Editor] Layout desbloqueado");
            }
            if (ImGui::MenuItem("Reset Layout")) {
                resetLayout();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("World")) {
            ImGui::MenuItem("Terrain");
            ImGui::MenuItem("Lighting");
            ImGui::MenuItem("Prefabs");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Asset Browser");
            ImGui::MenuItem("Profiler");
            ImGui::MenuItem("Console");
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 220.0f);
        ImGui::TextUnformatted("Game Engine Editor");

        ImGui::EndMainMenuBar();
    }
}

void EditorApp::renderStatusBar() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0.0f, viewport->Size.y - EditorLayout::kStatusBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, EditorLayout::kStatusBarHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 4.0f));
    if (ImGui::Begin("StatusBar", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::TextUnformatted("Ready");
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        ImGui::Text("Mode: Editor");
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        ImGui::Text("Scene: Default");
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorApp::renderMainContent() {
    if (m_showToolbar) {
        renderToolbar();
    }

    if (m_showViewport) {
        renderViewport();
    }

    if (m_showHierarchy) {
        renderHierarchy();
    }

    renderSceneSettings();

    if (m_showInspector) {
        renderInspector();
    }

    if (m_showConsole) {
        renderConsole();
    }
}

void EditorApp::resetLayout() {
    m_showToolbar = true;
    m_showViewport = true;
    m_showHierarchy = true;
    m_showInspector = true;
    m_showConsole = true;
    Engine::Core::Log::Info("[Editor] Layout resetado");
}

void EditorApp::launchGameRuntime() {
#if defined(_WIN32)
    std::filesystem::path exePath;
    std::filesystem::path workingDir;

    char editorPathBuffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, editorPathBuffer, MAX_PATH);

    if (length > 0) {
        const std::filesystem::path editorExe = std::filesystem::path(editorPathBuffer);
        const std::filesystem::path editorDir = editorExe.parent_path();

        const std::filesystem::path directExe = editorDir / "game-engine.exe";
        if (std::filesystem::exists(directExe)) {
            exePath = directExe;
            workingDir = editorDir;
        }
    }

    if (exePath.empty()) {
        std::filesystem::path root = std::filesystem::current_path();
        exePath = root / "build" / "src" / "client" / "Debug" / "game-engine.exe";
        workingDir = root / "build" / "src" / "client" / "Debug";
    }

    if (!std::filesystem::exists(exePath)) {
        Engine::Core::Log::Warn("[Editor] game-engine.exe nao encontrado. Tentando caminho alternativo.");

        std::filesystem::path fallbackRoot = std::filesystem::current_path();
        exePath = fallbackRoot / "build" / "src" / "client" / "Debug" / "game-engine.exe";
        workingDir = fallbackRoot / "build" / "src" / "client" / "Debug";

        if (!std::filesystem::exists(exePath)) {
            Engine::Core::Log::Error("[Editor] game-engine.exe nao encontrado em nenhum caminho conhecido");
            return;
        }
    }

    const std::string exe = exePath.string();
    const std::string working = workingDir.string();
    const HINSTANCE result = ShellExecuteA(nullptr, "open", exe.c_str(), nullptr, working.c_str(), SW_SHOWNORMAL);

    if ((intptr_t)result <= 32) {
        Engine::Core::Log::Error("[Editor] Falha ao iniciar game-engine.exe");
    } else {
        Engine::Core::Log::Info(std::string("[Editor] game-engine.exe iniciado em: ") + working);
    }
#else
    Engine::Core::Log::Warn("[Editor] Launch do runtime suportado apenas no Windows");
#endif
}

void EditorApp::renderToolbar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float toolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;

    EditorLayout::ComputePanelRects(
        viewport,
        toolbarY,
        viewportX,
        viewportY,
        viewportW,
        viewportH,
        hierarchyW,
        hierarchyH,
        inspectorX,
        inspectorY,
        inspectorW,
        inspectorH,
        consoleX,
        consoleY,
        consoleW,
        consoleH);

    ImGui::SetNextWindowPos(ImVec2(0.0f, EditorLayout::kTopMenuHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, EditorLayout::kToolbarHeight));
    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.33f, 0.34f, 0.38f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.83f, 0.45f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.93f, 0.57f, 0.22f, 1.00f));

    if (ImGui::Button("Play")) {
        launchGameRuntime();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        Engine::Core::Log::Info("[Editor] Botao Pause pressionado");
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        Engine::Core::Log::Info("[Editor] Botao Stop pressionado");
    }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(16.0f, 0.0f));
    ImGui::SameLine();
    if (ImGui::Button("Build")) {
        Engine::Core::Log::Info("[Editor] Build button pressed");
    }
    ImGui::SameLine();
    if (ImGui::Button("Bake")) {
        Engine::Core::Log::Info("[Editor] Bake button pressed");
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    ImGui::End();
}

void EditorApp::renderViewport() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float topToolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;

    EditorLayout::ComputePanelRects(
        viewport,
        topToolbarY,
        viewportX,
        viewportY,
        viewportW,
        viewportH,
        hierarchyW,
        hierarchyH,
        inspectorX,
        inspectorY,
        inspectorW,
        inspectorH,
        consoleX,
        consoleY,
        consoleW,
        consoleH);

    ImGui::SetNextWindowPos(ImVec2(viewportX, viewportY));
    ImGui::SetNextWindowSize(ImVec2(viewportW, viewportH));

    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    if (m_layoutLocked) {
        viewportFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("Viewport", nullptr, viewportFlags);

    ImGui::BeginTabBar("ViewportTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);
    if (ImGui::BeginTabItem("Scene")) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        const float canvasWidth = (size.x + 300.0f > 900.0f) ? (size.x + 300.0f) : 900.0f;
        const float canvasHeight = (size.y + 40.0f > 520.0f) ? (size.y + 40.0f) : 520.0f;
        const ImVec2 canvasSize = ImVec2(canvasWidth, canvasHeight);

        ImGui::BeginChild("SceneCanvas", ImVec2(size.x, size.y), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (m_sceneFramebufferWidth > 0 && m_sceneFramebufferHeight > 0) {
            ensureSceneFramebuffer();
            const float previewWidth = (size.x > 1.0f) ? size.x : 1.0f;
            const float previewHeight = (size.y > 1.0f) ? size.y : 1.0f;
            const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

            if (previewWidth < 90.0f || previewHeight < 60.0f) {
                ImGui::Text("Viewport too small. Resize the panel to continue.");
            } else {
                if (hovered) {
                    if (ImGui::IsKeyPressed(ImGuiKey_G)) {
                        m_transformMode = "Move";
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                        m_transformMode = "Rotate";
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                        m_transformMode = "Scale";
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        m_transformMode = "None";
                    }

                    if (ImGui::GetIO().MouseWheel != 0.0f) {
                        m_cameraDistance = SafeClamp(m_cameraDistance - ImGui::GetIO().MouseWheel * 2.5f, 6.0f, 30.0f);
                    }

                    const glm::vec3 viewPos = glm::vec3(
                        std::cos(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance,
                        std::sin(m_cameraPitch) * m_cameraDistance,
                        std::sin(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance
                    );
                    const glm::vec3 cameraPosition = glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) + viewPos;
                    const glm::vec3 forward = glm::normalize(glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) - cameraPosition);
                    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                    const glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
                    const glm::vec3 up = glm::normalize(glm::cross(forward, right));

                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                        if (ImGui::GetIO().KeyShift) {
                            const glm::vec3 cameraPosition = glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) + glm::vec3(
                                std::cos(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance,
                                std::sin(m_cameraPitch) * m_cameraDistance,
                                std::sin(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance);
                            const glm::vec3 viewDir = glm::normalize(glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) - cameraPosition);
                            const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                            const glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, viewDir));
                            const glm::vec3 cameraUp = glm::normalize(glm::cross(viewDir, cameraRight));
                            const glm::vec3 panDelta = cameraRight * ImGui::GetIO().MouseDelta.x * 0.01f + cameraUp * ImGui::GetIO().MouseDelta.y * 0.01f;
                            m_sceneFocusX += panDelta.x;
                            m_sceneFocusY += panDelta.y;
                            m_sceneFocusZ += panDelta.z;
                        } else if (ImGui::GetIO().KeyCtrl) {
                            m_cameraDistance = SafeClamp(m_cameraDistance - ImGui::GetIO().MouseDelta.y * 0.02f, 6.0f, 30.0f);
                        } else {
                            m_cameraYaw += ImGui::GetIO().MouseDelta.x * 0.01f;
                            m_cameraPitch = SafeClamp(m_cameraPitch + ImGui::GetIO().MouseDelta.y * 0.01f, -1.2f, 1.2f);
                        }
                    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                        const glm::vec3 cameraPosition = glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) + glm::vec3(
                            std::cos(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance,
                            std::sin(m_cameraPitch) * m_cameraDistance,
                            std::sin(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance);
                        const glm::vec3 viewDir = glm::normalize(glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ) - cameraPosition);
                        const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                        const glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, viewDir));
                        const glm::vec3 cameraUp = glm::normalize(glm::cross(viewDir, cameraRight));
                        const glm::vec3 panDelta = cameraRight * ImGui::GetIO().MouseDelta.x * 0.01f + cameraUp * ImGui::GetIO().MouseDelta.y * 0.01f;
                        m_sceneFocusX += panDelta.x;
                        m_sceneFocusY += panDelta.y;
                        m_sceneFocusZ += panDelta.z;
                    }

                    const bool isManipulatingObject = m_transformMode != "None" && !m_selectedObject.empty() && m_selectedObject != "Ground";
                    if (isManipulatingObject && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        const glm::vec3 groundRight = glm::normalize(glm::vec3(std::cos(m_cameraYaw), 0.0f, std::sin(m_cameraYaw)));
                        const glm::vec3 groundForward = glm::normalize(glm::vec3(-std::sin(m_cameraYaw), 0.0f, std::cos(m_cameraYaw)));
                        const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                        glm::vec3 delta(0.0f);
                        if (m_transformMode == "Move" || m_transformMode == "Move X") {
                            delta += groundRight * ImGui::GetIO().MouseDelta.x * 0.01f;
                        }
                        if (m_transformMode == "Move" || m_transformMode == "Move Y") {
                            delta += worldUp * (-ImGui::GetIO().MouseDelta.y) * 0.01f;
                        }
                        if (m_transformMode == "Move" || m_transformMode == "Move Z") {
                            delta += groundForward * (-ImGui::GetIO().MouseDelta.y) * 0.01f;
                        }
                        if (m_transformMode == "Rotate") {
                            delta += groundRight * ImGui::GetIO().MouseDelta.x * 0.005f;
                        }
                        if (m_transformMode == "Scale") {
                            delta += groundRight * ImGui::GetIO().MouseDelta.x * 0.0025f;
                        }
                        if (m_transformMode != "None") {
                            auto it = m_objectPositions.find(m_selectedObject);
                            if (it != m_objectPositions.end()) {
                                it->second += delta;
                            }
                        }
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const ImVec2 windowPos = ImGui::GetWindowPos();
                        const glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(m_sceneFramebufferWidth) / float(m_sceneFramebufferHeight > 0 ? m_sceneFramebufferHeight : 1), 0.1f, 100.0f);
                        const glm::mat4 view = buildSceneView(m_cameraYaw, m_cameraPitch, m_cameraDistance, glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ));
                        const float normX = ((mousePos.x - windowPos.x) / ImGui::GetWindowWidth()) * 2.0f - 1.0f;
                        const float normY = 1.0f - ((mousePos.y - windowPos.y) / ImGui::GetWindowHeight()) * 2.0f;
                        const glm::vec4 nearPoint = glm::vec4(normX, normY, -1.0f, 1.0f);
                        const glm::vec4 farPoint = glm::vec4(normX, normY, 1.0f, 1.0f);
                        const glm::mat4 invVP = glm::inverse(projection * view);
                        const glm::vec4 nearWorld = invVP * nearPoint;
                        const glm::vec4 farWorld = invVP * farPoint;
                        const glm::vec3 nearPos = glm::vec3(nearWorld / nearWorld.w);
                        const glm::vec3 farPos = glm::vec3(farWorld / farWorld.w);
                        const glm::vec3 rayDir = glm::normalize(farPos - nearPos);
                        const std::string picked = pickObjectByRay(nearPos, rayDir, m_objectPositions, m_objectScales);
                        if (!picked.empty()) {
                            m_selectedObject = picked;
                        }
                    }
                }

                glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);
                glViewport(0, 0, m_sceneFramebufferWidth, m_sceneFramebufferHeight);
                renderSimpleScene3D(
                    m_sceneFramebufferWidth,
                    m_sceneFramebufferHeight,
                    m_cameraYaw,
                    m_cameraPitch,
                    m_cameraDistance,
                    glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ),
                    m_selectedObject,
                    m_objectPositions,
                    m_objectScales);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(m_sceneColorTexture)), ImVec2(previewWidth, previewHeight), ImVec2(0, 1), ImVec2(1, 0));

                if (previewWidth >= 90.0f && previewHeight >= 60.0f) {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    const ImVec2 imageMin = ImGui::GetItemRectMin();
                    const ImVec2 imageMax = ImGui::GetItemRectMax();

                    const glm::vec3 cameraPosition = glm::vec3(
                        m_sceneFocusX + std::cos(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance,
                        m_sceneFocusY + std::sin(m_cameraPitch) * m_cameraDistance,
                        m_sceneFocusZ + std::sin(m_cameraYaw) * std::cos(m_cameraPitch) * m_cameraDistance);
                    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(m_sceneFramebufferWidth) / float(m_sceneFramebufferHeight > 0 ? m_sceneFramebufferHeight : 1), 0.1f, 100.0f);
                    const glm::mat4 view = buildSceneView(m_cameraYaw, m_cameraPitch, m_cameraDistance, glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ));
                    glm::vec3 gizmoWorldOrigin = glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ);
                    if (m_selectedObject != "Ground") {
                        const auto it = m_objectPositions.find(m_selectedObject);
                        if (it != m_objectPositions.end()) {
                            gizmoWorldOrigin = it->second;
                        }
                    }

                    const ImVec2 gizmoOrigin = ImVec2(imageMax.x - 120.0f, imageMin.y + 80.0f);
                    std::vector<std::pair<std::string, std::pair<ImVec2, ImVec2>>> axisSegments;
                    DrawBlenderGizmo(drawList, gizmoOrigin, cameraPosition, glm::vec3(m_sceneFocusX, m_sceneFocusY, m_sceneFocusZ), axisSegments);

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered) {
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const std::string pickedAxis = PickGizmoAxis(mousePos, axisSegments);
                        if (pickedAxis != "None") {
                            m_gizmoSelectedAxis = pickedAxis;
                            m_transformMode = "Move " + pickedAxis;
                        }
                    }

                    if (m_showGizmoDebug) {
                        const ImVec2 statusAnchor = ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f);
                        const std::string selectedText = std::format("Selecionado: {}", m_selectedObject);
                        const std::string transformText = std::format("Transform: {}", m_transformMode);
                        const std::string axisText = std::format("Gizmo: {}", m_gizmoSelectedAxis);
                        drawList->AddText(statusAnchor, IM_COL32(240, 240, 240, 255), selectedText.c_str());
                        drawList->AddText(ImVec2(statusAnchor.x, statusAnchor.y + 18.0f), IM_COL32(200, 200, 200, 255), transformText.c_str());
                        drawList->AddText(ImVec2(statusAnchor.x, statusAnchor.y + 36.0f), IM_COL32(160, 160, 160, 255), axisText.c_str());
                        if (!axisSegments.empty()) {
                            for (size_t i = 0; i < axisSegments.size(); ++i) {
                                const auto& [axisName, segment] = axisSegments[i];
                                const float sx = segment.first.x;
                                const float sy = segment.first.y;
                                const float ex = segment.second.x;
                                const float ey = segment.second.y;
                                const std::string screenText = std::format("{}: ({:.1f},{:.1f}) -> ({:.1f},{:.1f})", axisName, sx, sy, ex, ey);
                                drawList->AddText(ImVec2(statusAnchor.x, statusAnchor.y + 54.0f + float(i) * 16.0f), IM_COL32(120, 120, 120, 255), screenText.c_str());
                            }
                        }
                    }

                    const ImVec2 statusAnchor = ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f);
                    (void)statusAnchor;
                }
            }
        } else {
            ensureSceneFramebuffer();
            ImGui::Text("Preparing 3D viewport...");
        }

        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Camera")) {
        ImGui::Text("Camera preview and debug overlay");
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Runtime")) {
        ImGui::Text("Game runtime session");
        ImGui::Separator();
        ImGui::Text("Command: game-engine.exe");
        ImGui::Text("Status: Ready");
        ImGui::Text("World: Default Scene");
        ImGui::Separator();
        if (ImGui::Button("Launch game-engine.exe")) {
            launchGameRuntime();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop runtime")) {
            Engine::Core::Log::Info("[Editor] Runtime stop requested");
        }
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();

    ImGui::End();
}

void EditorApp::renderHierarchy() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float topToolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;

    EditorLayout::ComputePanelRects(
        viewport,
        topToolbarY,
        viewportX,
        viewportY,
        viewportW,
        viewportH,
        hierarchyW,
        hierarchyH,
        inspectorX,
        inspectorY,
        inspectorW,
        inspectorH,
        consoleX,
        consoleY,
        consoleW,
        consoleH);

    ImGui::SetNextWindowPos(ImVec2(0.0f, EditorLayout::kTopMenuHeight + EditorLayout::kToolbarHeight));
    ImGui::SetNextWindowSize(ImVec2(hierarchyW, hierarchyH));

    ImGuiWindowFlags hierarchyFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (m_layoutLocked) {
        hierarchyFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("Scene Hierarchy", nullptr, hierarchyFlags);

    ImGui::Text("World Outliner");
    ImGui::Separator();
    if (ImGui::TreeNode("RootScene")) {
        if (ImGui::Selectable("Ground")) {
            m_selectedObject = "Ground";
        }
        if (ImGui::Selectable("Wall")) {
            m_selectedObject = "Wall";
        }
        if (ImGui::Selectable("Player Spawn")) {
            m_selectedObject = "Player";
        }
        if (ImGui::Selectable("NPC Spawn")) {
            m_selectedObject = "NPC_001";
        }
        if (ImGui::Selectable("Castle")) {
            m_selectedObject = "Castle";
        }
        if (ImGui::Selectable("Tree_001")) {
            m_selectedObject = "Tree_001";
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void EditorApp::renderInspector() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float topToolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;

    EditorLayout::ComputePanelRects(
        viewport,
        topToolbarY,
        viewportX,
        viewportY,
        viewportW,
        viewportH,
        hierarchyW,
        hierarchyH,
        inspectorX,
        inspectorY,
        inspectorW,
        inspectorH,
        consoleX,
        consoleY,
        consoleW,
        consoleH);

    ImGui::SetNextWindowPos(ImVec2(inspectorX, inspectorY));
    ImGui::SetNextWindowSize(ImVec2(inspectorW, inspectorH));

    ImGuiWindowFlags inspectorFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (m_layoutLocked) {
        inspectorFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("Inspector", nullptr, inspectorFlags);

    std::string typeName = "Object";
    std::string defaultPosition = "(0, 0, 0)";
    std::string defaultRotation = "(0, 0, 0)";

    if (m_selectedObject == "Player") {
        typeName = "Player Spawn";
        defaultPosition = "(0, 0, 0)";
    } else if (m_selectedObject == "Ground") {
        typeName = "Terrain";
        defaultPosition = "(0, 0, 0)";
    } else if (m_selectedObject == "Wall") {
        typeName = "Structure";
        defaultPosition = "(0, 2, 0)";
    } else if (m_selectedObject == "Castle") {
        typeName = "Structure";
        defaultPosition = "(40, 0, 30)";
    } else if (m_selectedObject == "NPC_001") {
        typeName = "NPC Spawn";
        defaultPosition = "(12, 0, 25)";
    } else if (m_selectedObject == "Tree_001") {
        typeName = "Prop";
        defaultPosition = "(18, 0, 12)";
    }

    ImGui::Text("Selection");
    ImGui::Separator();
    ImGui::Text("Name: %s", m_selectedObject.c_str());
    ImGui::Text("Type: %s", typeName.c_str());
    ImGui::Text("Position: %s", defaultPosition.c_str());
    ImGui::Text("Rotation: %s", defaultRotation.c_str());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Transform")) {
        float position[3] = { 0.0f, 0.0f, 0.0f };
        float rotation[3] = { 0.0f, 0.0f, 0.0f };
        float scale[3] = { 1.0f, 1.0f, 1.0f };
        ImGui::InputFloat3("Position", position);
        ImGui::InputFloat3("Rotation", rotation);
        ImGui::InputFloat3("Scale", scale);
    }
    if (ImGui::CollapsingHeader("Renderer")) {
        ImGui::Checkbox("Visible", &m_showDemoWindow);
        ImGui::SliderFloat("Scale", &m_scale, 0.0f, 2.0f);
    }
    if (ImGui::CollapsingHeader("Physics")) {
        ImGui::Checkbox("Use Gravity", &m_showDemoWindow);
        ImGui::Checkbox("Kinematic", &m_showDemoWindow);
    }
    if (ImGui::CollapsingHeader("Runtime")) {
        ImGui::RadioButton("Windowed", &m_runtimeMode, 0);
        ImGui::RadioButton("Borderless", &m_runtimeMode, 1);
        if (ImGui::Button("Launch Runtime")) {
            launchGameRuntime();
        }
    }
    ImGui::End();
}

void EditorApp::renderConsole() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float topToolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;

    EditorLayout::ComputePanelRects(
        viewport,
        topToolbarY,
        viewportX,
        viewportY,
        viewportW,
        viewportH,
        hierarchyW,
        hierarchyH,
        inspectorX,
        inspectorY,
        inspectorW,
        inspectorH,
        consoleX,
        consoleY,
        consoleW,
        consoleH);

    ImGui::SetNextWindowPos(ImVec2(consoleX, consoleY));
    ImGui::SetNextWindowSize(ImVec2(consoleW, consoleH));

    ImGuiWindowFlags consoleFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (m_layoutLocked) {
        consoleFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("Console", nullptr, consoleFlags);
    ImGui::TextUnformatted("[Info] Editor inicializado");
    ImGui::TextUnformatted("[Info] Engine client ready");
    ImGui::TextUnformatted("[Info] Runtime launch path ready: build/src/client/Debug/game-engine.exe");
    ImGui::TextUnformatted("[Warning] Asset missing: ground_01.png");
    ImGui::TextUnformatted("[Log] Scene loaded successfully");
    ImGui::End();
}

void EditorApp::renderSceneSettings() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0.0f, viewport->Size.y - 300.0f));
    ImGui::SetNextWindowSize(ImVec2(260.0f, 240.0f));

    ImGuiWindowFlags sceneFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (m_layoutLocked) {
        sceneFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("Scene Editor", nullptr, sceneFlags);

    ImGui::InputText("Scene Name", m_sceneNameBuffer, IM_ARRAYSIZE(m_sceneNameBuffer));
    m_sceneName = m_sceneNameBuffer;
    ImGui::Separator();
    ImGui::Text("Terrain source:");
    ImGui::TextWrapped("%s", m_sceneAssetPath.c_str());
    ImGui::Separator();
    ImGui::Text("Scene type: %s", m_isEmptyScene ? "Starter scene" : "Map scene");
    ImGui::Text("Project root: %s", m_projectRootPath.c_str());
    if (ImGui::Button("Load Starter Scene")) {
        loadStarterScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Terrain Scene")) {
        loadSceneDefinition();
    }
    if (ImGui::Button("Import Terrain to Project")) {
        importTerrainIntoProject();
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse Terrain")) {
        pickTerrainFromFileBrowser();
    }
    ImGui::Separator();
    ImGui::Checkbox("Enable physics", &m_scenePhysicsEnabled);
    ImGui::Checkbox("Enable navmesh", &m_sceneNavmeshEnabled);
    ImGui::Checkbox("Enable audio", &m_sceneAudioEnabled);
    ImGui::SliderInt("Player count", &m_playerCount, 1, 200);
    ImGui::Separator();
    ImGui::Text("World: %s", m_sceneName.c_str());
    ImGui::Text("Scene status: %s", m_sceneLoaded ? "Loaded" : "Not loaded");
    ImGui::Text("Selected: %s", m_selectedObject.c_str());
    ImGui::End();
}

