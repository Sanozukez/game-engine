// src/editor/editor_ui_panels.cpp

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

#include "editor_app.h"
#include "editor_layout.h"
#include "core/log.h"
#include "window/window.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <string>

void EditorApp::renderDockSpaceHost() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float topOffset = EditorLayout::kTopMenuHeight + (m_showToolbar ? EditorLayout::kToolbarHeight : 0.0f);
    const ImVec2 hostPos = ImVec2(viewport->Pos.x, viewport->Pos.y + topOffset);
    const ImVec2 hostSize = ImVec2(viewport->Size.x, viewport->Size.y - topOffset - EditorLayout::kStatusBarHeight);

    ImGui::SetNextWindowPos(hostPos);
    ImGui::SetNextWindowSize(hostSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (!m_dockLayoutInitialized || m_resetDockLayoutRequested) {
        rebuildDefaultDockLayout();
        m_dockLayoutInitialized = true;
        m_resetDockLayoutRequested = false;
    }

    ImGui::End();
}

void EditorApp::rebuildDefaultDockLayout() {
    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float topOffset = EditorLayout::kTopMenuHeight + (m_showToolbar ? EditorLayout::kToolbarHeight : 0.0f);
    const ImVec2 dockSize = ImVec2(viewport->Size.x, viewport->Size.y - topOffset - EditorLayout::kStatusBarHeight);
    ImGui::DockBuilderSetNodeSize(dockspaceId, dockSize);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockLeft = 0;
    ImGuiID dockRight = 0;
    ImGuiID dockBottom = 0;
    ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.22f, &dockLeft, &dockMain);
    ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.34f, &dockRight, &dockMain);
    ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.33f, &dockBottom, &dockMain);

    ImGuiID dockActors = 0;
    ImGuiID dockSceneEditor = 0;
    ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.44f, &dockActors, &dockSceneEditor);

    ImGuiID dockOutliner = 0;
    ImGuiID dockInspector = 0;
    ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Right, 0.45f, &dockOutliner, &dockInspector);

    ImGuiID dockBottomTop = 0;
    ImGuiID dockBottomBottom = 0;
    ImGui::DockBuilderSplitNode(dockBottom, ImGuiDir_Down, 0.42f, &dockBottomBottom, &dockBottomTop);

    ImGuiID dockInspectorBottom = 0;
    ImGuiID dockInspectorTop = 0;
    ImGui::DockBuilderSplitNode(dockInspector, ImGuiDir_Down, 0.42f, &dockInspectorBottom, &dockInspectorTop);

    ImGui::DockBuilderDockWindow("Actor Palette", dockActors);
    ImGui::DockBuilderDockWindow("Sky", dockActors);
    ImGui::DockBuilderDockWindow("Scene Editor", dockSceneEditor);
    ImGui::DockBuilderDockWindow("Viewport", dockMain);
    ImGui::DockBuilderDockWindow("Properties", dockInspectorTop);
    ImGui::DockBuilderDockWindow("Outliner", dockOutliner);
    ImGui::DockBuilderDockWindow("Asset Pipeline", dockInspectorBottom);
    ImGui::DockBuilderDockWindow("Content Browser", dockBottomTop);
    ImGui::DockBuilderDockWindow("Console", dockBottomBottom);

    ImGui::DockBuilderFinish(dockspaceId);
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
            if (ImGui::MenuItem("Toggle Outliner", nullptr, m_showHierarchy)) {
                m_showHierarchy = !m_showHierarchy;
            }
            if (ImGui::MenuItem("Toggle Actor Palette", nullptr, m_showActorPalette)) {
                m_showActorPalette = !m_showActorPalette;
            }
            if (ImGui::MenuItem("Toggle Sky", nullptr, m_showSkyPanel)) {
                m_showSkyPanel = !m_showSkyPanel;
            }
            if (ImGui::MenuItem("Toggle Properties", nullptr, m_showInspector)) {
                m_showInspector = !m_showInspector;
            }
            if (ImGui::MenuItem("Toggle Console", nullptr, m_showConsole)) {
                m_showConsole = !m_showConsole;
            }
            if (ImGui::MenuItem("Toggle Content Browser", nullptr, m_showContentBrowser)) {
                m_showContentBrowser = !m_showContentBrowser;
            }
            if (ImGui::MenuItem("Toggle Viewport", nullptr, m_showViewport)) {
                m_showViewport = !m_showViewport;
            }
            if (ImGui::MenuItem("Toggle Asset Pipeline", nullptr, m_showAssetPipeline)) {
                m_showAssetPipeline = !m_showAssetPipeline;
            }
            if (ImGui::MenuItem("Toggle Gizmo Debug", nullptr, m_showGizmoDebug)) {
                m_showGizmoDebug = !m_showGizmoDebug;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Docking Layout")) {
                m_resetDockLayoutRequested = true;
            }
            if (ImGui::MenuItem("Apply Professional Layout")) {
                resetLayout();
                Engine::Core::Log::Info("[Editor] Layout profissional aplicado (docking)");
            }
            if (ImGui::MenuItem("Reset Layout")) {
                resetLayout();
            }

            ImGui::Separator();
            if (ImGui::BeginMenu("Visual Theme")) {
                const bool isBlender = m_visualStyle == EditorVisualStyle::Blender;
                const bool isFL = m_visualStyle == EditorVisualStyle::FLStudio;

                if (ImGui::MenuItem("Blender Inspired", nullptr, isBlender)) {
                    m_visualStyle = EditorVisualStyle::Blender;
                    m_themeDirty = true;
                }

                if (ImGui::MenuItem("FL Studio Inspired", nullptr, isFL)) {
                    m_visualStyle = EditorVisualStyle::FLStudio;
                    m_themeDirty = true;
                }

                ImGui::EndMenu();
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
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        ImGui::Text("Theme: %s", m_visualStyle == EditorVisualStyle::FLStudio ? "FL Studio" : "Blender");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorApp::renderMainContent() {
    if (m_showToolbar) {
        renderToolbar();
    }

    renderDockSpaceHost();

    if (m_showHierarchy) {
        renderHierarchy();
    }

    if (m_showActorPalette) {
        renderActorPalette();
    }

    if (m_showSkyPanel) {
        renderSkyPanel();
    }

    if (m_showViewport) {
        renderViewport();
    }

    renderSceneSettings();

    if (m_showAssetPipeline) {
        renderAssetPipelinePanel();
    }

    if (m_showContentBrowser) {
        renderContentBrowser();
    }

    if (m_showInspector) {
        renderInspector();
    }

    if (m_showConsole) {
        renderConsole();
    }

}

void EditorApp::resetLayout() {
    m_showToolbar = true;
    m_showActorPalette = true;
    m_showSkyPanel = true;
    m_showViewport = true;
    m_showHierarchy = true;
    m_showInspector = true;
    m_showConsole = true;
    m_showContentBrowser = true;
    m_resetDockLayoutRequested = true;
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
