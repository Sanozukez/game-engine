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

#include <filesystem>
#include <string>

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
