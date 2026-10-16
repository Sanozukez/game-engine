// src/editor/editor_side_panels.cpp

#include "editor_app.h"
#include "editor_panel_layout_utils.h"

#include <imgui.h>

#include <array>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

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
    renderSceneSettingsSky();
}

void EditorApp::renderSceneSettingsSky() {
    if (!ImGui::CollapsingHeader("Sky / Clouds", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Checkbox("Procedural Sky", &m_sky.proceduralEnabled);

    if (ImGui::Button("Preset: Noon")) {
        m_sky.sunYawDeg = 35.0f;
        m_sky.sunPitchDeg = 42.0f;
        m_sky.cloudSpeed = 0.0032f;
        m_sky.cloudDirectionDeg = 18.0f;
        m_sky.cloudCoverage = 0.50f;
        m_sky.cloudSoftness = 0.08f;
        m_sky.cloudScale = 1.70f;
        m_sky.cloudOpacity = 0.65f;
        m_sky.zenithColor = glm::vec3(0.20f, 0.39f, 0.70f);
        m_sky.horizonColor = glm::vec3(0.72f, 0.80f, 0.91f);
        m_sky.cloudColor = glm::vec3(0.92f, 0.93f, 0.95f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Preset: Sunset")) {
        m_sky.sunYawDeg = -22.0f;
        m_sky.sunPitchDeg = 9.0f;
        m_sky.cloudSpeed = 0.0026f;
        m_sky.cloudDirectionDeg = -12.0f;
        m_sky.cloudCoverage = 0.62f;
        m_sky.cloudSoftness = 0.06f;
        m_sky.cloudScale = 1.92f;
        m_sky.cloudOpacity = 0.78f;
        m_sky.zenithColor = glm::vec3(0.30f, 0.26f, 0.46f);
        m_sky.horizonColor = glm::vec3(0.96f, 0.56f, 0.33f);
        m_sky.cloudColor = glm::vec3(0.96f, 0.83f, 0.70f);
    }
    if (ImGui::Button("Preset: Night")) {
        m_sky.sunYawDeg = -88.0f;
        m_sky.sunPitchDeg = -13.0f;
        m_sky.cloudSpeed = 0.0018f;
        m_sky.cloudDirectionDeg = 32.0f;
        m_sky.cloudCoverage = 0.44f;
        m_sky.cloudSoftness = 0.05f;
        m_sky.cloudScale = 2.10f;
        m_sky.cloudOpacity = 0.46f;
        m_sky.zenithColor = glm::vec3(0.03f, 0.06f, 0.13f);
        m_sky.horizonColor = glm::vec3(0.08f, 0.10f, 0.17f);
        m_sky.cloudColor = glm::vec3(0.42f, 0.46f, 0.58f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Sky")) {
        m_sky = SkySettings{};
    }

    if (ImGui::Button("Save Sky To Scene")) {
        saveSkySettingsToSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Sky From Scene")) {
        loadSkySettingsFromSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
    }

    ImGui::SliderFloat("Sun Yaw", &m_sky.sunYawDeg, -180.0f, 180.0f, "%.1f deg");
    ImGui::SliderFloat("Sun Pitch", &m_sky.sunPitchDeg, -20.0f, 85.0f, "%.1f deg");
    ImGui::SliderFloat("Cloud Speed", &m_sky.cloudSpeed, 0.0001f, 0.02f, "%.4f");
    ImGui::SliderFloat("Cloud Direction", &m_sky.cloudDirectionDeg, -180.0f, 180.0f, "%.1f deg");
    ImGui::SliderFloat("Cloud Coverage", &m_sky.cloudCoverage, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Cloud Softness", &m_sky.cloudSoftness, 0.01f, 0.25f, "%.2f");
    ImGui::SliderFloat("Cloud Scale", &m_sky.cloudScale, 0.3f, 4.0f, "%.2f");
    ImGui::SliderFloat("Cloud Opacity", &m_sky.cloudOpacity, 0.0f, 1.0f, "%.2f");

    float zenithColor[3] = {m_sky.zenithColor.r, m_sky.zenithColor.g, m_sky.zenithColor.b};
    float horizonColor[3] = {m_sky.horizonColor.r, m_sky.horizonColor.g, m_sky.horizonColor.b};
    float cloudColor[3] = {m_sky.cloudColor.r, m_sky.cloudColor.g, m_sky.cloudColor.b};

    if (ImGui::ColorEdit3("Zenith Color", zenithColor)) {
        m_sky.zenithColor = glm::vec3(zenithColor[0], zenithColor[1], zenithColor[2]);
    }
    if (ImGui::ColorEdit3("Horizon Color", horizonColor)) {
        m_sky.horizonColor = glm::vec3(horizonColor[0], horizonColor[1], horizonColor[2]);
    }
    if (ImGui::ColorEdit3("Cloud Color", cloudColor)) {
        m_sky.cloudColor = glm::vec3(cloudColor[0], cloudColor[1], cloudColor[2]);
    }

    renderSkyAssetBrowser();
}

void EditorApp::renderSkyAssetBrowser() {
    if (!ImGui::CollapsingHeader("Sky Asset Browser", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::TextWrapped("Perfis salvos em project/assets/sky. Eles sao independentes da cena e podem ser reutilizados.");

    const std::filesystem::path skyAssetDir = std::filesystem::current_path() / m_projectRootPath / "assets" / "sky";
    std::filesystem::create_directories(skyAssetDir);

    std::vector<std::filesystem::path> profileFiles;
    for (const auto& entry : std::filesystem::directory_iterator(skyAssetDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".skyprofile") {
            profileFiles.push_back(entry.path());
        }
    }

    std::sort(profileFiles.begin(), profileFiles.end());

    ImGui::InputText("Profile Name", m_skyProfileNameBuffer, IM_ARRAYSIZE(m_skyProfileNameBuffer));
    if (ImGui::Button("Save Current As Asset")) {
        std::string baseName = m_skyProfileNameBuffer;
        if (baseName.empty()) {
            baseName = "sky_profile";
        }
        for (char& c : baseName) {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) {
                c = '_';
            }
        }
        saveSkySettingsToSceneFile(skyAssetDir / (baseName + ".skyprofile"));
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh List")) {
        // Intencionalmente vazio: a lista ja e recalculada a cada frame.
    }

    static int selectedProfileIndex = -1;
    ImGui::SeparatorText("Available Sky Assets");
    if (profileFiles.empty()) {
        ImGui::TextDisabled("Nenhum .skyprofile encontrado em project/assets/sky");
        selectedProfileIndex = -1;
        return;
    }

    if (selectedProfileIndex >= static_cast<int>(profileFiles.size())) {
        selectedProfileIndex = static_cast<int>(profileFiles.size()) - 1;
    }

    for (int i = 0; i < static_cast<int>(profileFiles.size()); ++i) {
        const std::string displayName = profileFiles[i].stem().string();
        const bool selected = (selectedProfileIndex == i);
        if (ImGui::Selectable(displayName.c_str(), selected)) {
            selectedProfileIndex = i;
            std::strncpy(m_skyProfileNameBuffer, displayName.c_str(), sizeof(m_skyProfileNameBuffer));
            m_skyProfileNameBuffer[sizeof(m_skyProfileNameBuffer) - 1] = '\0';
        }
    }

    if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(profileFiles.size())) {
        const std::filesystem::path selectedPath = profileFiles[selectedProfileIndex];
        ImGui::TextWrapped("Selected: %s", selectedPath.filename().string().c_str());

        if (ImGui::Button("Apply Asset To Current Sky")) {
            loadSkySettingsFromSceneFile(selectedPath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Over Asset")) {
            saveSkySettingsToSceneFile(selectedPath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Asset")) {
            std::error_code ec;
            std::filesystem::remove(selectedPath, ec);
            if (ec) {
                ImGui::OpenPopup("DeleteSkyAssetError");
            } else {
                selectedProfileIndex = -1;
            }
        }

        if (ImGui::BeginPopupModal("DeleteSkyAssetError", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Nao foi possivel deletar o asset de sky.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void EditorApp::renderSceneSettingsSummary() const {
    ImGui::Text("World: %s", m_sceneName.c_str());
    ImGui::Text("Scene status: %s", m_sceneLoaded ? "Loaded" : "Not loaded");
    ImGui::Text("Selected: %s", m_selectedObject.c_str());
}
