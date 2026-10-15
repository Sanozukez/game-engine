// src/editor/editor_app.h
// Aplicação do Editor Visual

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Engine {
    class Window;
}

class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    void run();

private:
    void initWindow();
    void initImGui();
    void setupStyle();
    void shutdownImGui();
    void launchGameRuntime();

    void renderMenuBar();
    void renderStatusBar();
    void renderMainContent();
    void renderViewport();
    void renderHierarchy();
    void renderInspector();
    void renderConsole();
    void renderToolbar();
    void renderSceneSettings();
    void resetLayout();
    bool createRenderTexture(unsigned int& textureId, unsigned int& framebufferId, unsigned int& depthId, int width, int height);
    void ensureSceneFramebuffer();
    void ensureProjectStructure();
    void loadSceneDefinition();
    void loadStarterScene();
    void importTerrainIntoProject();
    void pickTerrainFromFileBrowser();

    // Estado visual do editor
    std::unique_ptr<Engine::Window> m_window;

    bool m_showDemoWindow = false;
    bool m_showHierarchy = true;
    bool m_showInspector = true;
    bool m_showConsole = true;
    bool m_showToolbar = true;
    bool m_showViewport = true;
    bool m_showGizmoDebug = true;
    bool m_layoutLocked = true;
    unsigned int m_sceneFramebuffer = 0;
    unsigned int m_sceneColorTexture = 0;
    unsigned int m_sceneDepthRenderbuffer = 0;
    int m_sceneFramebufferWidth = 0;
    int m_sceneFramebufferHeight = 0;
    float m_scale = 1.0f;
    float m_cameraYaw = 0.85f;
    float m_cameraPitch = 0.42f;
    float m_cameraDistance = 14.0f;
    float m_sceneFocusX = 0.0f;
    float m_sceneFocusY = 1.0f;
    float m_sceneFocusZ = 0.0f;
    std::string m_transformMode = "None";
    std::string m_gizmoSelectedAxis = "None";
    std::unordered_map<std::string, glm::vec3> m_objectPositions = {
        {"Wall", glm::vec3(0.0f, 1.5f, -2.0f)},
        {"Player Spawn", glm::vec3(-4.0f, 1.3f, 0.0f)},
        {"NPC Spawn", glm::vec3(4.0f, 1.0f, 1.5f)},
        {"Ground", glm::vec3(0.0f, -0.05f, 0.0f)}
    };
    std::unordered_map<std::string, glm::vec3> m_objectScales = {
        {"Wall", glm::vec3(3.0f, 1.5f, 0.6f)},
        {"Player Spawn", glm::vec3(1.0f, 1.5f, 1.0f)},
        {"NPC Spawn", glm::vec3(0.7f, 1.0f, 0.7f)},
        {"Ground", glm::vec3(12.0f, 0.1f, 12.0f)}
    };
    bool m_isInitialized = false;
    std::string m_selectedObject = "Ground";
    std::string m_sceneName = "StarterScene";
    std::string m_sceneAssetPath = "project/terrains";
    std::string m_projectRootPath = "project";
    bool m_sceneLoaded = false;
    bool m_terrainReady = false;
    bool m_isEmptyScene = true;
    bool m_projectReady = false;
    char m_sceneNameBuffer[128] = "StarterScene";
    int m_runtimeMode = 0;
    int m_playerCount = 1;
    bool m_scenePhysicsEnabled = true;
    bool m_sceneNavmeshEnabled = true;
    bool m_sceneAudioEnabled = true;
};
