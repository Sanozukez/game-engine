// src/editor/editor_side_panels.cpp

#include "editor_app.h"
#include "editor_panel_layout_utils.h"
#include "core/log.h"

#include <imgui.h>

#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

namespace {

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

ImGuiWindowFlags BuildPanelFlags(ImGuiWindowFlags baseFlags) {
    return baseFlags;
}

bool ContainsInsensitive(const std::string& text, const std::string& term) {
    if (term.empty()) {
        return true;
    }

    std::string textLower = text;
    std::string termLower = term;

    std::transform(textLower.begin(), textLower.end(), textLower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    std::transform(termLower.begin(), termLower.end(), termLower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return textLower.find(termLower) != std::string::npos;
}

} // namespace

void EditorApp::renderHierarchy() {
    const ImGuiWindowFlags hierarchyFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);

    ImGui::Begin("Outliner", nullptr, hierarchyFlags);

    ImGui::Text("Chunk ativo: %s", getActiveChunkLabel().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(loaded: %d)", getLoadedChunkCount());

    ImGui::InputTextWithHint("##OutlinerFilter", "Filtrar objetos...", m_outlinerFilterBuffer, IM_ARRAYSIZE(m_outlinerFilterBuffer));
    ImGui::Separator();
    if (ImGui::TreeNode("World")) {
        const std::string filter = m_outlinerFilterBuffer;
        std::vector<std::string> objectIds;
        objectIds.reserve(m_objectPositions.size());
        for (const auto& entry : m_objectPositions) {
            objectIds.push_back(entry.first);
        }
        std::sort(objectIds.begin(), objectIds.end());

        for (const std::string& objectId : objectIds) {
            if (!isObjectVisibleInActiveChunk(objectId)) {
                continue;
            }
            if (!ContainsInsensitive(objectId, filter)) {
                continue;
            }
            if (ImGui::Selectable(objectId.c_str(), m_selectedObject == objectId)) {
                m_selectedObject = objectId;
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void EditorApp::renderActorPalette() {
    const ImGuiWindowFlags actorFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);
    ImGui::Begin("Actor Palette", nullptr, actorFlags);

    ImGui::TextWrapped("Painel para adicionar elementos no mundo 3D (equivalente ao Place Actors da Unreal).");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add Directional Light")) {
            Engine::Core::Log::Info("[Editor] Add Directional Light solicitado");
        }
        if (ImGui::Button("Add Point Light")) {
            Engine::Core::Log::Info("[Editor] Add Point Light solicitado");
        }
        if (ImGui::Button("Add Spot Light")) {
            Engine::Core::Log::Info("[Editor] Add Spot Light solicitado");
        }
    }

    if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add Cube")) {
            Engine::Core::Log::Info("[Editor] Add Cube solicitado");
        }
        if (ImGui::Button("Add Plane")) {
            Engine::Core::Log::Info("[Editor] Add Plane solicitado");
        }
        if (ImGui::Button("Add Sphere")) {
            Engine::Core::Log::Info("[Editor] Add Sphere solicitado");
        }
    }

    if (ImGui::CollapsingHeader("Volumes", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add Trigger Volume")) {
            Engine::Core::Log::Info("[Editor] Add Trigger Volume solicitado");
        }
        if (ImGui::Button("Add Nav Volume")) {
            Engine::Core::Log::Info("[Editor] Add Nav Volume solicitado");
        }
    }

    ImGui::End();
}

void EditorApp::renderSkyPanel() {
    const ImGuiWindowFlags skyFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);
    ImGui::Begin("Sky", nullptr, skyFlags);
    renderSceneSettingsSky();
    ImGui::End();
}

void EditorApp::renderInspector() {
    const ImGuiWindowFlags inspectorFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);

    ImGui::Begin("Properties", nullptr, inspectorFlags);

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
    const ImGuiWindowFlags consoleFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);

    ImGui::Begin("Console", nullptr, consoleFlags);

    for (const char* message : kConsoleMessages) {
        ImGui::TextUnformatted(message);
    }
    ImGui::End();
}

void EditorApp::renderContentBrowser() {
    const ImGuiWindowFlags browserFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);
    ImGui::Begin("Content Browser", nullptr, browserFlags);

    if (ImGui::Button("Import")) {
        pickAndImportAssetFromFileBrowser();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Folder")) {
        ImGui::OpenPopup("CreateContentFolder");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save All")) {
        saveAllEditorChanges();
    }

    if (ImGui::BeginPopupModal("CreateContentFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Folder Name", m_newContentFolderBuffer, IM_ARRAYSIZE(m_newContentFolderBuffer));
        if (ImGui::Button("Create")) {
            std::string folderName = m_newContentFolderBuffer;
            for (char& c : folderName) {
                if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) {
                    c = '_';
                }
            }

            if (!folderName.empty()) {
                std::filesystem::path target = std::filesystem::current_path() / m_contentBrowserCurrentRoot / folderName;
                std::error_code ec;
                std::filesystem::create_directories(target, ec);
                if (ec) {
                    m_contentBrowserStatus = std::string("Erro ao criar pasta: ") + ec.message();
                } else {
                    m_contentBrowserStatus = std::string("Pasta criada: ") + target.filename().string();
                }
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::InputTextWithHint("##ContentFilter", "Filtrar arquivos...", m_contentFilterBuffer, IM_ARRAYSIZE(m_contentFilterBuffer));
    ImGui::Separator();

    struct ContentTab {
        const char* title;
        const char* root;
    };

    constexpr std::array<ContentTab, 12> tabs = {{
        {"All", "project/assets"},
        {"Audio", "project/assets/audio"},
        {"Maps", "project/assets/maps"},
        {"Materials", "project/assets/materials"},
        {"Textures", "project/assets/textures"},
        {"Particles", "project/assets/particles"},
        {"Scripts", "project/assets/scripts"},
        {"UI/HUD", "project/assets/ui/hud"},
        {"Prefabs", "project/assets/prefabs/archetypes"},
        {"Animations", "project/assets/animations/skeletal"},
        {"Shaders", "project/assets/shaders"},
        {"Data", "project/data/database"}
    }};

    static int activeQuickFilter = 0;
    if (activeQuickFilter < 0 || activeQuickFilter >= static_cast<int>(tabs.size())) {
        activeQuickFilter = 0;
    }

    if (ImGui::BeginTabBar("ContentQuickFilters", ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
            if (ImGui::BeginTabItem(tabs[i].title)) {
                activeQuickFilter = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    const std::filesystem::path assetsRoot = std::filesystem::current_path() / "project" / "assets";
    const std::filesystem::path dataRoot = std::filesystem::current_path() / "project" / "data";
    std::filesystem::create_directories(assetsRoot);
    std::filesystem::create_directories(dataRoot / "database");

    if (m_contentBrowserCurrentRoot.empty()) {
        m_contentBrowserCurrentRoot = "project/assets";
    }

    if (ImGui::BeginTable("ContentBrowserSplit", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 260.0f);
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("FolderTree", ImVec2(0.0f, 0.0f), true);
        ImGui::TextUnformatted("Folder Tree");
        ImGui::Separator();

        std::function<void(const std::filesystem::path&, int)> drawTree;
        drawTree = [&](const std::filesystem::path& dir, int depth) {
            std::vector<std::filesystem::path> subdirs;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_directory()) {
                    subdirs.push_back(entry.path());
                }
            }
            std::sort(subdirs.begin(), subdirs.end());

            for (const std::filesystem::path& subdir : subdirs) {
                std::error_code ec;
                const std::filesystem::path rel = std::filesystem::relative(subdir, std::filesystem::current_path(), ec);
                const std::string relPath = ec ? subdir.generic_string() : rel.generic_string();
                const bool selected = (relPath == m_contentBrowserCurrentRoot);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (selected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                const bool opened = ImGui::TreeNodeEx(relPath.c_str(), flags, "%s", subdir.filename().string().c_str());
                if (ImGui::IsItemClicked()) {
                    m_contentBrowserCurrentRoot = relPath;
                }

                if (opened) {
                    if (depth < 5) {
                        drawTree(subdir, depth + 1);
                    }
                    ImGui::TreePop();
                }
            }
        };

        if (ImGui::Selectable("project/assets", m_contentBrowserCurrentRoot == "project/assets")) {
            m_contentBrowserCurrentRoot = "project/assets";
        }
        drawTree(assetsRoot, 0);
        if (ImGui::Selectable("project/data/database", m_contentBrowserCurrentRoot == "project/data/database")) {
            m_contentBrowserCurrentRoot = "project/data/database";
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("AssetList", ImVec2(0.0f, 0.0f), true);
        static std::string selectedAsset;

        const std::filesystem::path quickFilterRoot = std::filesystem::current_path() / tabs[activeQuickFilter].root;
        const std::filesystem::path selectedFolder = std::filesystem::current_path() / m_contentBrowserCurrentRoot;
        std::filesystem::create_directories(quickFilterRoot);
        std::filesystem::create_directories(selectedFolder);

        ImGui::TextWrapped("Current Folder: %s", m_contentBrowserCurrentRoot.c_str());
        ImGui::TextWrapped("Quick Filter: %s", tabs[activeQuickFilter].title);
        ImGui::Separator();

        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(selectedFolder)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());

        const std::string filter = m_contentFilterBuffer;
        const std::string quickFilterRootText = quickFilterRoot.lexically_normal().generic_string();
        for (const std::filesystem::path& filePath : files) {
            const std::string fileName = filePath.filename().string();
            const std::string filePathText = filePath.lexically_normal().generic_string();

            if (!ContainsInsensitive(fileName, filter)) {
                continue;
            }
            if (tabs[activeQuickFilter].title != std::string("All") && filePathText.rfind(quickFilterRootText, 0) != 0) {
                continue;
            }

            const bool isSelected = (selectedAsset == filePathText);
            if (ImGui::Selectable(fileName.c_str(), isSelected)) {
                selectedAsset = filePathText;
            }
        }

        if (!selectedAsset.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("Selected: %s", selectedAsset.c_str());
            const std::filesystem::path selectedPath = selectedAsset;
            const std::string ext = selectedPath.extension().string();
            if (ext == ".glb" || ext == ".gltf" || ext == ".obj") {
                if (ImGui::Button("Use Selected as MMAP Source")) {
                    std::error_code ec;
                    const std::filesystem::path rel = std::filesystem::relative(selectedPath, std::filesystem::current_path(), ec);
                    const std::string displayPath = ec ? selectedPath.generic_string() : rel.generic_string();
                    std::strncpy(m_mmapSourceGlbPathBuffer, displayPath.c_str(), sizeof(m_mmapSourceGlbPathBuffer));
                    m_mmapSourceGlbPathBuffer[sizeof(m_mmapSourceGlbPathBuffer) - 1] = '\0';
                    m_assetPipelineStatus = std::string("Fonte MMAP selecionada: ") + displayPath;
                }
            }
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextWrapped("Status: %s", m_contentBrowserStatus.c_str());

    ImGui::End();
}

void EditorApp::renderSceneSettings() {
    const ImGuiWindowFlags sceneFlags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);

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

    auto toProjectRelativePath = [&](const std::filesystem::path& path) -> std::string {
        std::error_code ec;
        std::filesystem::path rel = std::filesystem::relative(path, std::filesystem::current_path(), ec);
        if (ec) {
            return path.lexically_normal().generic_string();
        }
        return rel.lexically_normal().generic_string();
    };

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
        const std::string profileRelativePath = toProjectRelativePath(profileFiles[i]);
        const bool isActiveProfile = (profileRelativePath == m_activeSkyProfilePath);
        if (isActiveProfile) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.96f, 0.54f, 1.0f));
        }
        const bool selected = (selectedProfileIndex == i);
        if (ImGui::Selectable(displayName.c_str(), selected)) {
            selectedProfileIndex = i;
            std::strncpy(m_skyProfileNameBuffer, displayName.c_str(), sizeof(m_skyProfileNameBuffer));
            m_skyProfileNameBuffer[sizeof(m_skyProfileNameBuffer) - 1] = '\0';
        }
        if (isActiveProfile) {
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextDisabled("(active)");
        }
    }

    if (selectedProfileIndex >= 0 && selectedProfileIndex < static_cast<int>(profileFiles.size())) {
        const std::filesystem::path selectedPath = profileFiles[selectedProfileIndex];
        ImGui::TextWrapped("Selected: %s", selectedPath.filename().string().c_str());
        const std::string selectedRelativePath = toProjectRelativePath(selectedPath);

        if (ImGui::Button("Apply Asset To Current Sky")) {
            loadSkySettingsFromSceneFile(selectedPath);
            m_activeSkyProfilePath = selectedRelativePath;
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply + Save To Scene")) {
            loadSkySettingsFromSceneFile(selectedPath);
            m_activeSkyProfilePath = selectedRelativePath;
            saveSkySettingsToSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
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
                if (m_activeSkyProfilePath == selectedRelativePath) {
                    m_activeSkyProfilePath.clear();
                }
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

void EditorApp::renderSceneAssetPipelineTools() {
    if (!ImGui::CollapsingHeader("Toolchain Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::TextWrapped("Compila e executa os compilers legados dentro do editor.");

    if (ImGui::Button("Build MMAP Compiler")) {
        buildMmapCompiler();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run MMAP Compiler")) {
        runMmapCompilerFromSource();
    }

    ImGui::InputText("MMAP Source GLB", m_mmapSourceGlbPathBuffer, IM_ARRAYSIZE(m_mmapSourceGlbPathBuffer));
    ImGui::TextDisabled("Exemplo: assets/models/test_scene_1.glb");
}

void EditorApp::renderAssetPipelinePanel() {
    ImGuiWindowFlags flags = BuildPanelFlags(ImGuiWindowFlags_NoCollapse);
    ImGui::Begin("Asset Pipeline", nullptr, flags);

    ImGui::TextWrapped("Fluxo recomendado: selecione um GLB, importe para project/assets/models, compile MMAP e copie para project/terrains.");
    ImGui::SeparatorText("Guided MMAP Flow");

    ImGui::InputText("Source GLB", m_mmapSourceGlbPathBuffer, IM_ARRAYSIZE(m_mmapSourceGlbPathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Browse GLB")) {
        pickMmapSourceGlbFromFileBrowser();
    }

    if (ImGui::Button("Import GLB + Compile MMAP + Copy to Project")) {
        runMmapImportAndCompilePipeline();
    }

    ImGui::SeparatorText("Dictionary Compiler");
    ImGui::TextWrapped("Gera data/assets_dictionary.bin a partir de data/asset_dictionary.json (catalogo binario consumido pela engine em runtime).");
    if (ImGui::Button("Build Dictionary Compiler")) {
        buildDictionaryCompiler();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run Dictionary Compiler")) {
        runDictionaryCompiler();
    }

    ImGui::SeparatorText("Advanced Toolchain");
    renderSceneAssetPipelineTools();

    ImGui::SeparatorText("Pipeline Status");
    ImGui::TextWrapped("%s", m_assetPipelineStatus.c_str());

    ImGui::End();
}
