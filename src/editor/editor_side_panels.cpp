// src/editor/editor_side_panels.cpp

#include "editor_app.h"
#include "editor_panel_layout_utils.h"

#include <imgui.h>

#include <array>
#include <string>

namespace {

struct HierarchyItem {
    const char* label;
    const char* objectId;
    bool isSelectable;
};

constexpr std::array<HierarchyItem, 6> kHierarchyItems = {{
    {"Ground (reference)", "Ground", false},
    {"Wall", "Wall", true},
    {"Player Spawn", "Player Spawn", true},
    {"NPC Spawn", "NPC Spawn", true},
    {"Castle", "Castle", true},
    {"Tree_001", "Tree_001", true}
}};

constexpr std::array<const char*, 5> kConsoleMessages = {{
    "[Info] Editor inicializado",
    "[Info] Engine client ready",
    "[Info] Runtime launch path ready: build/src/client/Debug/game-engine.exe",
    "[Warning] Asset missing: ground_01.png",
    "[Log] Scene loaded successfully"
}};

struct InspectorSelectionMetadata {
    const char* typeName;
    const char* defaultPosition;
    const char* defaultRotation;
};

InspectorSelectionMetadata GetInspectorSelectionMetadata(const std::string& selectedObject) {
    if (selectedObject == "Player" || selectedObject == "Player Spawn") {
        return {"Player Spawn", "(0, 0, 0)", "(0, 0, 0)"};
    }
    if (selectedObject == "Ground") {
        return {"Terrain", "(0, 0, 0)", "(0, 0, 0)"};
    }
    if (selectedObject == "Wall") {
        return {"Structure", "(0, 2, 0)", "(0, 0, 0)"};
    }
    if (selectedObject == "Castle") {
        return {"Structure", "(40, 0, 30)", "(0, 0, 0)"};
    }
    if (selectedObject == "NPC_001" || selectedObject == "NPC Spawn") {
        return {"NPC Spawn", "(12, 0, 25)", "(0, 0, 0)"};
    }
    if (selectedObject == "Tree_001") {
        return {"Prop", "(18, 0, 12)", "(0, 0, 0)"};
    }
    return {"Object", "(0, 0, 0)", "(0, 0, 0)"};
}

void ApplyLockedWindowRect(bool layoutLocked, const ImVec2& pos, const ImVec2& size) {
    if (!layoutLocked) {
        return;
    }
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
}

ImGuiWindowFlags BuildPanelFlags(ImGuiWindowFlags baseFlags, bool layoutLocked) {
    if (layoutLocked) {
        return baseFlags | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }
    return baseFlags;
}

} // namespace

void EditorApp::renderHierarchy() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const Editor::PanelLayoutUtils::PanelRects panelRects = Editor::PanelLayoutUtils::ComputePanelRects(viewport);

    ApplyLockedWindowRect(
        m_layoutLocked,
        ImVec2(0.0f, panelRects.topToolbarY),
        ImVec2(panelRects.hierarchyW, panelRects.hierarchyH));

    const ImGuiWindowFlags hierarchyFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse, m_layoutLocked);

    ImGui::Begin("Scene Hierarchy", nullptr, hierarchyFlags);

    ImGui::Text("World Outliner");
    ImGui::Separator();
    if (ImGui::TreeNode("RootScene")) {
        for (const HierarchyItem& item : kHierarchyItems) {
            if (!item.isSelectable) {
                ImGui::TextDisabled("%s", item.label);
                continue;
            }
            if (ImGui::Selectable(item.label)) {
                m_selectedObject = item.objectId;
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void EditorApp::renderInspector() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const Editor::PanelLayoutUtils::PanelRects panelRects = Editor::PanelLayoutUtils::ComputePanelRects(viewport);

    ApplyLockedWindowRect(
        m_layoutLocked,
        ImVec2(panelRects.inspectorX, panelRects.inspectorY),
        ImVec2(panelRects.inspectorW, panelRects.inspectorH));

    const ImGuiWindowFlags inspectorFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse, m_layoutLocked);

    ImGui::Begin("Inspector", nullptr, inspectorFlags);
    renderInspectorSelectionSummary();
    renderInspectorTransformSection();
    renderInspectorRendererSection();
    renderInspectorPhysicsSection();
    renderInspectorRuntimeSection();
    ImGui::End();
}

void EditorApp::renderInspectorSelectionSummary() {
    const InspectorSelectionMetadata selectionMetadata = GetInspectorSelectionMetadata(m_selectedObject);

    ImGui::Text("Selection");
    ImGui::Separator();
    ImGui::Text("Name: %s", m_selectedObject.c_str());
    ImGui::Text("Type: %s", selectionMetadata.typeName);
    ImGui::Text("Position: %s", selectionMetadata.defaultPosition);
    ImGui::Text("Rotation: %s", selectionMetadata.defaultRotation);
    ImGui::Separator();
}

void EditorApp::renderInspectorTransformSection() {
    if (ImGui::CollapsingHeader("Transform")) {
        float position[3] = { 0.0f, 0.0f, 0.0f };
        float rotation[3] = { 0.0f, 0.0f, 0.0f };
        float scale[3] = { 1.0f, 1.0f, 1.0f };
        ImGui::InputFloat3("Position", position);
        ImGui::InputFloat3("Rotation", rotation);
        ImGui::InputFloat3("Scale", scale);
    }
}

void EditorApp::renderInspectorRendererSection() {
    if (ImGui::CollapsingHeader("Renderer")) {
        ImGui::Checkbox("Visible", &m_showDemoWindow);
        ImGui::SliderFloat("Scale", &m_scale, 0.0f, 2.0f);
    }
}

void EditorApp::renderInspectorPhysicsSection() {
    if (ImGui::CollapsingHeader("Physics")) {
        ImGui::Checkbox("Use Gravity", &m_showDemoWindow);
        ImGui::Checkbox("Kinematic", &m_showDemoWindow);
    }
}

void EditorApp::renderInspectorRuntimeSection() {
    if (ImGui::CollapsingHeader("Runtime")) {
        ImGui::RadioButton("Windowed", &m_runtimeMode, 0);
        ImGui::RadioButton("Borderless", &m_runtimeMode, 1);
        if (ImGui::Button("Launch Runtime")) {
            launchGameRuntime();
        }
    }
}

void EditorApp::renderConsole() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const Editor::PanelLayoutUtils::PanelRects panelRects = Editor::PanelLayoutUtils::ComputePanelRects(viewport);

    ApplyLockedWindowRect(
        m_layoutLocked,
        ImVec2(panelRects.consoleX, panelRects.consoleY),
        ImVec2(panelRects.consoleW, panelRects.consoleH));

    const ImGuiWindowFlags consoleFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse, m_layoutLocked);

    ImGui::Begin("Console", nullptr, consoleFlags);
    for (const char* message : kConsoleMessages) {
        ImGui::TextUnformatted(message);
    }
    ImGui::End();
}

void EditorApp::renderSceneSettings() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const Editor::PanelLayoutUtils::SceneEditorRect rect = Editor::PanelLayoutUtils::ComputeSceneEditorRect(viewport);
    ApplyLockedWindowRect(m_layoutLocked, ImVec2(rect.x, rect.y), ImVec2(rect.w, rect.h));

    const ImGuiWindowFlags sceneFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse, m_layoutLocked);

    ImGui::Begin("Scene Editor", nullptr, sceneFlags);

    ImGui::InputText("Scene Name", m_sceneNameBuffer, IM_ARRAYSIZE(m_sceneNameBuffer));
    m_sceneName = m_sceneNameBuffer;
    ImGui::Separator();
    ImGui::Text("Terrain source:");
    ImGui::TextWrapped("%s", m_sceneAssetPath.c_str());
    ImGui::Separator();
    ImGui::Text("Scene type: %s", m_isEmptyScene ? "Starter scene" : "Map scene");
    ImGui::Text("Project root: %s", m_projectRootPath.c_str());
    renderSceneSettingsLoadActions();
    ImGui::Separator();
    renderSceneSettingsOptions();
    ImGui::Separator();
    renderSceneSettingsSummary();
    ImGui::End();
}

void EditorApp::renderSceneSettingsLoadActions() {
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
}

void EditorApp::renderSceneSettingsOptions() {
    ImGui::Checkbox("Enable physics", &m_scenePhysicsEnabled);
    ImGui::Checkbox("Enable navmesh", &m_sceneNavmeshEnabled);
    ImGui::Checkbox("Enable audio", &m_sceneAudioEnabled);
    ImGui::SliderInt("Player count", &m_playerCount, 1, 200);
}

void EditorApp::renderSceneSettingsSummary() const {
    ImGui::Text("World: %s", m_sceneName.c_str());
    ImGui::Text("Scene status: %s", m_sceneLoaded ? "Loaded" : "Not loaded");
    ImGui::Text("Selected: %s", m_selectedObject.c_str());
}
