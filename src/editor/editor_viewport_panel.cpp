// src/editor/editor_viewport_panel.cpp

#include "editor_app.h"
#include "editor_gizmo_utils.h"
#include "editor_overlay_utils.h"
#include "editor_panel_layout_utils.h"
#include "editor_viewport_interaction_utils.h"
#include "editor_viewport_math.h"
#include "editor_viewport_scene_renderer.h"
#include "core/log.h"
#include "render/opengl_types.h"
#include "window/window.h"

#include <imgui.h>

#include <cmath>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace {
    using Editor::GizmoUtils::DrawBlenderGizmo;
    using Editor::GizmoUtils::DrawOutlinedText;
    using Editor::GizmoUtils::GetAxisColor;
    using Editor::GizmoUtils::GetWorldAxisFromName;
    using Editor::GizmoUtils::PickGizmoAxis;
    using Editor::GizmoUtils::SnapCameraToAxis;
    using Editor::OverlayUtils::DrawProjectedGroundOutline;
    using Editor::OverlayUtils::DrawProjectedVisibleBoxOutline;
    using Editor::OverlayUtils::GetVisualHalfExtent;
    using Editor::ViewportInteractionUtils::ApplyOrbitDrag;
    using Editor::ViewportInteractionUtils::ApplyZoomDelta;
    using Editor::ViewportInteractionUtils::BuildSceneView;
    using Editor::ViewportInteractionUtils::ComputeOrbitCameraPosition;
    using Editor::ViewportInteractionUtils::ComputePanDelta;
    using Editor::ViewportInteractionUtils::PickObjectByRay;
    using Editor::ViewportMath::BuildMouseRayInImage;
    using Editor::ViewportMath::IntersectRayWithPlane;
    using Editor::ViewportMath::ProjectToScreen;
    using Editor::ViewportSceneRenderer::RenderSimpleScene3D;
}

void EditorApp::renderViewport() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const Editor::PanelLayoutUtils::PanelRects panelRects = Editor::PanelLayoutUtils::ComputePanelRects(viewport);

    if (m_layoutLocked) {
        ImGui::SetNextWindowPos(ImVec2(panelRects.viewportX, panelRects.viewportY));
        ImGui::SetNextWindowSize(ImVec2(panelRects.viewportW, panelRects.viewportH));
    }

    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
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
                        if (!m_selectedObject.empty() && m_selectedObject != "Ground") {
                            m_viewport.transformMode = "Move";
                            m_viewport.grabMovePending = true;
                            m_viewport.grabMoveActive = false;
                            m_viewport.grabAxisLock = "None";
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                        m_viewport.transformMode = "Rotate";
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                        m_viewport.transformMode = "Scale";
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        if (m_viewport.grabMoveActive) {
                            auto it = m_objectPositions.find(m_selectedObject);
                            if (it != m_objectPositions.end()) {
                                it->second = m_viewport.grabStartObjectPos;
                            }
                        }
                        m_viewport.transformMode = "None";
                        m_viewport.grabMovePending = false;
                        m_viewport.grabMoveActive = false;
                        m_viewport.grabAxisLock = "None";
                    }

                    if (m_viewport.transformMode == "Move" && (m_viewport.grabMovePending || m_viewport.grabMoveActive)) {
                        if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                            m_viewport.grabAxisLock = "X";
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                            m_viewport.grabAxisLock = "Y";
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                            m_viewport.grabAxisLock = "Z";
                        }
                    }

                    if (ImGui::GetIO().MouseWheel != 0.0f) {
                        ApplyZoomDelta(m_viewport.cameraDistance, ImGui::GetIO().MouseWheel, 2.5f, 6.0f, 30.0f);
                    }

                    const glm::vec3 focusPoint = m_viewport.focus;

                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                        if (ImGui::GetIO().KeyShift) {
                            const glm::vec3 panDelta = ComputePanDelta(
                                m_viewport.cameraYaw,
                                m_viewport.cameraPitch,
                                m_viewport.cameraDistance,
                                focusPoint,
                                glm::vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y),
                                0.01f);
                            m_viewport.focus.x += panDelta.x;
                            m_viewport.focus.y += panDelta.y;
                            m_viewport.focus.z += panDelta.z;
                        } else if (ImGui::GetIO().KeyCtrl) {
                            ApplyZoomDelta(m_viewport.cameraDistance, ImGui::GetIO().MouseDelta.y, 0.02f, 6.0f, 30.0f);
                        } else {
                            ApplyOrbitDrag(
                                m_viewport.cameraYaw,
                                m_viewport.cameraPitch,
                                glm::vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y),
                                0.01f,
                                -1.2f,
                                1.2f);
                        }
                    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                        const glm::vec3 panDelta = ComputePanDelta(
                            m_viewport.cameraYaw,
                            m_viewport.cameraPitch,
                            m_viewport.cameraDistance,
                            focusPoint,
                            glm::vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y),
                            0.01f);
                        m_viewport.focus.x += panDelta.x;
                        m_viewport.focus.y += panDelta.y;
                        m_viewport.focus.z += panDelta.z;
                    }

                }

                glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer);
                glViewport(0, 0, m_sceneFramebufferWidth, m_sceneFramebufferHeight);
                RenderSimpleScene3D(
                    m_sceneFramebufferWidth,
                    m_sceneFramebufferHeight,
                    m_viewport.cameraYaw,
                    m_viewport.cameraPitch,
                    m_viewport.cameraDistance,
                    m_viewport.focus,
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

                    const glm::vec3 cameraPosition = ComputeOrbitCameraPosition(
                        m_viewport.cameraYaw,
                        m_viewport.cameraPitch,
                        m_viewport.cameraDistance,
                        m_viewport.focus);
                    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(m_sceneFramebufferWidth) / float(m_sceneFramebufferHeight > 0 ? m_sceneFramebufferHeight : 1), 0.1f, 100.0f);
                    const glm::mat4 view = BuildSceneView(m_viewport.cameraYaw, m_viewport.cameraPitch, m_viewport.cameraDistance, m_viewport.focus);
                    const ImVec2 imageSize = ImVec2(imageMax.x - imageMin.x, imageMax.y - imageMin.y);

                    const auto clearGrabState = [this](bool clearTransformMode) {
                        m_viewport.grabMoveActive = false;
                        m_viewport.grabMovePending = false;
                        m_viewport.grabAxisLock = "None";
                        if (clearTransformMode) {
                            m_viewport.transformMode = "None";
                        }
                    };

                    const auto tryBeginPendingGrab = [&](glm::vec3& selectedPos, const ImVec2& mousePos) {
                        if (!m_viewport.grabMovePending) {
                            return;
                        }

                        glm::vec3 rayOrigin;
                        glm::vec3 rayDir;
                        if (!BuildMouseRayInImage(mousePos, imageMin, imageMax, view, projection, rayOrigin, rayDir)) {
                            return;
                        }

                        const glm::vec3 cameraForward = glm::normalize(m_viewport.focus - cameraPosition);
                        glm::vec3 hitPoint;
                        if (!IntersectRayWithPlane(rayOrigin, rayDir, selectedPos, cameraForward, hitPoint)) {
                            return;
                        }

                        m_viewport.grabStartObjectPos = selectedPos;
                        m_viewport.grabPlaneOrigin = selectedPos;
                        m_viewport.grabPlaneNormal = cameraForward;
                        m_viewport.grabStartHitPoint = hitPoint;
                        m_viewport.grabMoveActive = true;
                        m_viewport.grabMovePending = false;
                    };

                    const auto updateActiveGrab = [&](glm::vec3& selectedPos, const ImVec2& mousePos) {
                        if (!(m_viewport.grabMoveActive && m_viewport.transformMode == "Move")) {
                            return;
                        }

                        glm::vec3 rayOrigin;
                        glm::vec3 rayDir;
                        glm::vec3 hitPoint;
                        if (BuildMouseRayInImage(mousePos, imageMin, imageMax, view, projection, rayOrigin, rayDir) &&
                            IntersectRayWithPlane(rayOrigin, rayDir, m_viewport.grabPlaneOrigin, m_viewport.grabPlaneNormal, hitPoint)) {
                            glm::vec3 delta = hitPoint - m_viewport.grabStartHitPoint;
                            if (m_viewport.grabAxisLock != "None") {
                                const glm::vec3 axis = GetWorldAxisFromName(m_viewport.grabAxisLock);
                                if (glm::length(axis) > 0.0f) {
                                    delta = axis * glm::dot(delta, axis);
                                }
                            }
                            selectedPos = m_viewport.grabStartObjectPos + delta;
                        }

                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            clearGrabState(true);
                        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            selectedPos = m_viewport.grabStartObjectPos;
                            clearGrabState(true);
                        }
                    };

                    if (!m_selectedObject.empty() && m_selectedObject != "Ground") {
                        auto selectedIt = m_objectPositions.find(m_selectedObject);
                        if (selectedIt != m_objectPositions.end()) {
                            const ImVec2 mousePos = ImGui::GetMousePos();
                            const bool insideImage =
                                mousePos.x >= imageMin.x && mousePos.x <= imageMax.x &&
                                mousePos.y >= imageMin.y && mousePos.y <= imageMax.y;

                            if (insideImage) {
                                tryBeginPendingGrab(selectedIt->second, mousePos);
                            }
                            updateActiveGrab(selectedIt->second, mousePos);
                        }
                    }

                    if (!m_selectedObject.empty() && imageSize.x > 1.0f && imageSize.y > 1.0f) {
                        glm::vec3 selectedCenter(0.0f, -0.05f, 0.0f);
                        glm::vec3 selectedHalfExtent(12.0f, 0.05f, 12.0f);

                        const auto posIt = m_objectPositions.find(m_selectedObject);
                        const auto scaleIt = m_objectScales.find(m_selectedObject);
                        if (posIt != m_objectPositions.end()) {
                            selectedCenter = posIt->second;
                        }
                        if (scaleIt != m_objectScales.end()) {
                            selectedHalfExtent = GetVisualHalfExtent(scaleIt->second);
                        }

                        if (m_selectedObject == "Ground") {
                            DrawProjectedGroundOutline(
                                drawList,
                                imageMin,
                                imageSize,
                                view,
                                projection,
                                selectedCenter,
                                selectedHalfExtent,
                                IM_COL32(255, 170, 40, 255),
                                2.8f);
                        } else {
                            DrawProjectedVisibleBoxOutline(
                                drawList,
                                imageMin,
                                imageSize,
                                view,
                                projection,
                                cameraPosition,
                                selectedCenter,
                                selectedHalfExtent,
                                IM_COL32(255, 170, 40, 255),
                                2.8f);
                        }
                    }

                    glm::vec3 gizmoWorldOrigin = m_viewport.focus;
                    if (m_selectedObject != "Ground") {
                        const auto it = m_objectPositions.find(m_selectedObject);
                        if (it != m_objectPositions.end()) {
                            gizmoWorldOrigin = it->second;
                        }
                    }

                    const ImVec2 gizmoOrigin = ImVec2(imageMax.x - 120.0f, imageMin.y + 80.0f);
                    std::vector<std::pair<std::string, std::pair<ImVec2, ImVec2>>> axisSegments;
                    DrawBlenderGizmo(drawList, gizmoOrigin, cameraPosition, m_viewport.focus, axisSegments);

                    if (m_viewport.grabMoveActive && m_viewport.transformMode == "Move" && m_viewport.grabAxisLock != "None") {
                        const auto selectedPosIt = m_objectPositions.find(m_selectedObject);
                        if (selectedPosIt != m_objectPositions.end()) {
                            const glm::vec3 axis = GetWorldAxisFromName(m_viewport.grabAxisLock);
                            const glm::vec3 centerWorld = selectedPosIt->second;

                            const glm::vec4 centerClip = projection * view * glm::vec4(centerWorld, 1.0f);
                            if (centerClip.w > 0.01f) {
                                const ImVec2 centerProjected = ProjectToScreen(centerWorld, view, projection, imageSize);
                                ImVec2 center = ImVec2(imageMin.x + centerProjected.x, imageMin.y + centerProjected.y);

                                ImVec2 axisDir(0.0f, -1.0f);
                                const glm::vec3 worldDirPoint = centerWorld + axis * 10.0f;
                                const glm::vec4 dirClip = projection * view * glm::vec4(worldDirPoint, 1.0f);
                                if (dirClip.w > 0.01f) {
                                    const ImVec2 dirProjected = ProjectToScreen(worldDirPoint, view, projection, imageSize);
                                    axisDir = ImVec2(dirProjected.x - centerProjected.x, dirProjected.y - centerProjected.y);
                                } else {
                                    const glm::vec3 axisView = glm::mat3(view) * axis;
                                    axisDir = ImVec2(axisView.x, -axisView.y);
                                }

                                const float axisLen = std::sqrt(axisDir.x * axisDir.x + axisDir.y * axisDir.y);
                                if (axisLen > 1e-5f) {
                                    axisDir.x /= axisLen;
                                    axisDir.y /= axisLen;
                                }

                                const float guideLength = std::sqrt((imageMax.x - imageMin.x) * (imageMax.x - imageMin.x) + (imageMax.y - imageMin.y) * (imageMax.y - imageMin.y));
                                const ImVec2 p0 = ImVec2(center.x - axisDir.x * guideLength, center.y - axisDir.y * guideLength);
                                const ImVec2 p1 = ImVec2(center.x + axisDir.x * guideLength, center.y + axisDir.y * guideLength);
                                const ImU32 color = GetAxisColor(m_viewport.grabAxisLock);
                                drawList->AddLine(p0, p1, IM_COL32(0, 0, 0, 120), 4.0f);
                                drawList->AddLine(p0, p1, color, 2.0f);
                            }
                        }
                    }

                    bool clickedGizmoAxis = false;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered) {
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const std::string pickedAxis = PickGizmoAxis(mousePos, axisSegments);
                        if (pickedAxis != "None") {
                            m_viewport.gizmoSelectedAxis = pickedAxis;
                            m_viewport.transformMode = "Move " + pickedAxis;
                            SnapCameraToAxis(m_viewport.cameraYaw, m_viewport.cameraPitch, pickedAxis);
                            clickedGizmoAxis = true;
                        }
                    }

                    if (!clickedGizmoAxis && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Middle) && hovered) {
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const bool insideImage =
                            mousePos.x >= imageMin.x && mousePos.x <= imageMax.x &&
                            mousePos.y >= imageMin.y && mousePos.y <= imageMax.y;

                        if (insideImage) {
                            glm::vec3 rayOrigin;
                            glm::vec3 rayDir;
                            if (BuildMouseRayInImage(mousePos, imageMin, imageMax, view, projection, rayOrigin, rayDir)) {
                                const std::string picked = PickObjectByRay(rayOrigin, rayDir, m_objectPositions, m_objectScales);
                                if (!picked.empty()) {
                                    m_selectedObject = picked;
                                }
                            }
                        }
                    }

                    if (m_showGizmoDebug) {
                        const ImVec2 statusAnchor = ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f);
                        const std::string selectedText = std::format("Selecionado: {}", m_selectedObject);
                        const std::string transformText = std::format("Transform: {}", m_viewport.transformMode);
                        const std::string axisText = std::format("Gizmo: {}", m_viewport.gizmoSelectedAxis);
                        DrawOutlinedText(drawList, statusAnchor, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), selectedText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 18.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), transformText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 36.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), axisText.c_str());
                        if (!axisSegments.empty()) {
                            for (size_t i = 0; i < axisSegments.size(); ++i) {
                                const auto& [axisName, segment] = axisSegments[i];
                                const float sx = segment.first.x;
                                const float sy = segment.first.y;
                                const float ex = segment.second.x;
                                const float ey = segment.second.y;
                                const std::string screenText = std::format("{}: ({:.1f},{:.1f}) -> ({:.1f},{:.1f})", axisName, sx, sy, ex, ey);
                                DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 54.0f + float(i) * 16.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), screenText.c_str());
                            }
                        }
                    }

                    const ImVec2 statusAnchor = ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f);
                    (void)statusAnchor;
                    (void)gizmoWorldOrigin;
                    (void)canvasSize;
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


