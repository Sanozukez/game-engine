// src/editor/editor_app.h
// Aplicação do Editor Visual

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include "editor_theme.h"

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
    void renderInspectorSelectionSummary();
    void renderInspectorTransformSection();
    void renderInspectorRendererSection();
    void renderInspectorPhysicsSection();
    void renderInspectorRuntimeSection();
    void renderConsole();
    void renderToolbar();
    void renderSceneSettings();
    void renderSceneSettingsLoadActions();
    void renderSceneSettingsOptions();
    void renderSceneSettingsSky();
    void renderSkyAssetBrowser();
    void renderSceneSettingsSummary() const;
    void resetLayout();
    bool createRenderTexture(unsigned int& textureId, unsigned int& framebufferId, unsigned int& depthId, int width, int height);
    void ensureSceneFramebuffer();
    void ensureProjectStructure();
    void loadSceneDefinition();
    void loadStarterScene();
    void importTerrainIntoProject();
    void pickTerrainFromFileBrowser();
    void loadSkySettingsFromSceneFile(const std::filesystem::path& sceneFilePath);
    void saveSkySettingsToSceneFile(const std::filesystem::path& sceneFilePath) const;

    struct ViewportState {
        float cameraYaw = 0.85f;
        float cameraPitch = 0.42f;
        float cameraDistance = 14.0f;
        glm::vec3 focus = glm::vec3(0.0f, 1.0f, 0.0f);
        std::string transformMode = "None";
        std::string gizmoSelectedAxis = "None";
        bool grabMovePending = false;
        bool grabMoveActive = false;
        std::string grabAxisLock = "None";
        glm::vec3 grabStartObjectPos = glm::vec3(0.0f);
        glm::vec3 grabPlaneOrigin = glm::vec3(0.0f);
        glm::vec3 grabPlaneNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 grabStartHitPoint = glm::vec3(0.0f);
    };

    struct SkySettings {
        bool proceduralEnabled = true;
        float sunYawDeg = 35.0f;
        float sunPitchDeg = 22.0f;
        float cloudSpeed = 0.0035f;
        float cloudDirectionDeg = 18.0f;
        float cloudCoverage = 0.54f;
        float cloudSoftness = 0.08f;
        float cloudScale = 1.75f;
        float cloudOpacity = 0.72f;
        glm::vec3 zenithColor = glm::vec3(0.20f, 0.39f, 0.70f);
        glm::vec3 horizonColor = glm::vec3(0.70f, 0.78f, 0.90f);
        glm::vec3 cloudColor = glm::vec3(0.92f, 0.93f, 0.95f);
    };

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
    ViewportState m_viewport;
    SkySettings m_sky;
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
    std::string m_sceneDefinitionPath = "project/scenes/starter_scene.scene";
    std::string m_projectRootPath = "project";
    char m_skyProfileNameBuffer[64] = "rohan_soft_day";
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
    EditorVisualStyle m_visualStyle = EditorVisualStyle::Blender;
    bool m_themeDirty = false;
};
