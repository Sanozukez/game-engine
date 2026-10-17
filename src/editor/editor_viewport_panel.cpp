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
#include <algorithm>
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
    using Editor::ViewportSceneRenderer::SkyRenderParams;
}

void EditorApp::renderViewport() {
    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Viewport", nullptr, viewportFlags);

    ImGui::BeginTabBar("ViewportTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);
    if (ImGui::BeginTabItem("Scene")) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        const int targetFramebufferWidth = static_cast<int>(size.x > 1.0f ? size.x : 1.0f);
        const int targetFramebufferHeight = static_cast<int>(size.y > 1.0f ? size.y : 1.0f);
        ensureSceneFramebuffer(targetFramebufferWidth, targetFramebufferHeight);

        const float canvasWidth = (size.x + 300.0f > 900.0f) ? (size.x + 300.0f) : 900.0f;
        const float canvasHeight = (size.y + 40.0f > 520.0f) ? (size.y + 40.0f) : 520.0f;
        const ImVec2 canvasSize = ImVec2(canvasWidth, canvasHeight);

        ImGui::BeginChild("SceneCanvas", ImVec2(size.x, size.y), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (m_sceneFramebufferWidth > 0 && m_sceneFramebufferHeight > 0) {
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
                            m_viewport.rotateActive = false;
                            m_viewport.scaleActive = false;
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                        if (!m_selectedObject.empty() && m_selectedObject != "Ground") {
                            auto rotIt = m_objectRotationsDeg.find(m_selectedObject);
                            if (rotIt != m_objectRotationsDeg.end()) {
                                m_viewport.transformMode = "Rotate";
                                m_viewport.rotateActive = true;
                                m_viewport.rotateStartEulerDeg = rotIt->second;
                                m_viewport.rotateStartMouse = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                                m_viewport.rotateAxisLock = "Y";
                                m_viewport.grabMovePending = false;
                                m_viewport.grabMoveActive = false;
                                m_viewport.scaleActive = false;
                            }
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                        if (!m_selectedObject.empty() && m_selectedObject != "Ground") {
                            auto scaleIt = m_objectScales.find(m_selectedObject);
                            if (scaleIt != m_objectScales.end()) {
                                m_viewport.transformMode = "Scale";
                                m_viewport.scaleActive = true;
                                m_viewport.scaleStartValue = scaleIt->second;
                                m_viewport.scaleStartMouse = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                                m_viewport.scaleAxisLock = "Uniform";
                                m_viewport.grabMovePending = false;
                                m_viewport.grabMoveActive = false;
                                m_viewport.rotateActive = false;
                            }
                        }
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        if (m_viewport.grabMoveActive) {
                            auto it = m_objectPositions.find(m_selectedObject);
                            if (it != m_objectPositions.end()) {
                                it->second = m_viewport.grabStartObjectPos;
                            }
                        }
                        if (m_viewport.rotateActive) {
                            auto it = m_objectRotationsDeg.find(m_selectedObject);
                            if (it != m_objectRotationsDeg.end()) {
                                it->second = m_viewport.rotateStartEulerDeg;
                            }
                        }
                        if (m_viewport.scaleActive) {
                            auto it = m_objectScales.find(m_selectedObject);
                            if (it != m_objectScales.end()) {
                                it->second = m_viewport.scaleStartValue;
                            }
                        }
                        m_viewport.transformMode = "None";
                        m_viewport.grabMovePending = false;
                        m_viewport.grabMoveActive = false;
                        m_viewport.grabAxisLock = "None";
                        m_viewport.rotateActive = false;
                        m_viewport.scaleActive = false;
                        m_viewport.rotateAxisLock = "Y";
                        m_viewport.scaleAxisLock = "Uniform";
                    }

                    const bool moveMode = (m_viewport.transformMode == "Move" && (m_viewport.grabMovePending || m_viewport.grabMoveActive));
                    const bool rotateMode = (m_viewport.transformMode == "Rotate" && m_viewport.rotateActive);
                    const bool scaleMode = (m_viewport.transformMode == "Scale" && m_viewport.scaleActive);
                    if (moveMode || rotateMode || scaleMode) {
                        if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                            if (moveMode) {
                                m_viewport.grabAxisLock = "X";
                            }
                            if (rotateMode) {
                                m_viewport.rotateAxisLock = "X";
                            }
                            if (scaleMode) {
                                m_viewport.scaleAxisLock = "X";
                            }
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                            if (moveMode) {
                                m_viewport.grabAxisLock = "Y";
                            }
                            if (rotateMode) {
                                m_viewport.rotateAxisLock = "Y";
                            }
                            if (scaleMode) {
                                m_viewport.scaleAxisLock = "Y";
                            }
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                            if (moveMode) {
                                m_viewport.grabAxisLock = "Z";
                            }
                            if (rotateMode) {
                                m_viewport.rotateAxisLock = "Z";
                            }
                            if (scaleMode) {
                                m_viewport.scaleAxisLock = "Z";
                            }
                        }
                        if (scaleMode && ImGui::IsKeyPressed(ImGuiKey_U)) {
                            m_viewport.scaleAxisLock = "Uniform";
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
                SkyRenderParams skyParams;
                skyParams.proceduralEnabled = m_sky.proceduralEnabled;
                skyParams.timeSeconds = static_cast<float>(ImGui::GetTime());
                skyParams.sunYawDeg = m_sky.sunYawDeg;
                skyParams.sunPitchDeg = m_sky.sunPitchDeg;
                skyParams.cloudSpeed = m_sky.cloudSpeed;
                skyParams.cloudDirectionDeg = m_sky.cloudDirectionDeg;
                skyParams.cloudCoverage = m_sky.cloudCoverage;
                skyParams.cloudSoftness = m_sky.cloudSoftness;
                skyParams.cloudScale = m_sky.cloudScale;
                skyParams.cloudOpacity = m_sky.cloudOpacity;
                skyParams.zenithColor = m_sky.zenithColor;
                skyParams.horizonColor = m_sky.horizonColor;
                skyParams.cloudColor = m_sky.cloudColor;
                RenderSimpleScene3D(
                    m_sceneFramebufferWidth,
                    m_sceneFramebufferHeight,
                    m_viewport.cameraYaw,
                    m_viewport.cameraPitch,
                    m_viewport.cameraDistance,
                    m_viewport.focus,
                    skyParams,
                    m_selectedObject,
                    m_objectPositions,
                    m_objectScales,
                    m_objectRotationsDeg);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

                const float textureAspect = static_cast<float>(m_sceneFramebufferWidth) /
                    static_cast<float>(m_sceneFramebufferHeight > 0 ? m_sceneFramebufferHeight : 1);
                const float availableAspect = previewWidth / (previewHeight > 0.0f ? previewHeight : 1.0f);

                ImVec2 displaySize = ImVec2(previewWidth, previewHeight);
                if (availableAspect > textureAspect) {
                    displaySize.y = previewHeight;
                    displaySize.x = displaySize.y * textureAspect;
                } else {
                    displaySize.x = previewWidth;
                    displaySize.y = displaySize.x / (textureAspect > 0.0f ? textureAspect : 1.0f);
                }

                const ImVec2 startCursor = ImGui::GetCursorPos();
                const float offsetX = (previewWidth - displaySize.x) * 0.5f;
                const float offsetY = (previewHeight - displaySize.y) * 0.5f;
                ImGui::SetCursorPos(ImVec2(startCursor.x + offsetX, startCursor.y + offsetY));

                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(m_sceneColorTexture)), displaySize, ImVec2(0, 1), ImVec2(1, 0));

                if (previewWidth >= 90.0f && previewHeight >= 60.0f) {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    const ImVec2 imageMin = ImGui::GetItemRectMin();
                    const ImVec2 imageMax = ImGui::GetItemRectMax();

                    const glm::vec3 cameraPosition = ComputeOrbitCameraPosition(
                        m_viewport.cameraYaw,
                        m_viewport.cameraPitch,
                        m_viewport.cameraDistance,
                        m_viewport.focus);
                    updateWorldPartitionFromCamera(cameraPosition);
                    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(m_sceneFramebufferWidth) / float(m_sceneFramebufferHeight > 0 ? m_sceneFramebufferHeight : 1), 0.1f, 100.0f);
                    const glm::mat4 view = BuildSceneView(m_viewport.cameraYaw, m_viewport.cameraPitch, m_viewport.cameraDistance, m_viewport.focus);
                    const ImVec2 imageSize = ImVec2(imageMax.x - imageMin.x, imageMax.y - imageMin.y);

                    const auto clearGrabState = [this](bool clearTransformMode) {
                        m_viewport.grabMoveActive = false;
                        m_viewport.grabMovePending = false;
                        m_viewport.grabAxisLock = "None";
                        m_viewport.rotateActive = false;
                        m_viewport.scaleActive = false;
                        m_viewport.rotateAxisLock = "Y";
                        m_viewport.scaleAxisLock = "Uniform";
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

                        if (m_viewport.transformMode == "Rotate" && m_viewport.rotateActive) {
                            auto rotIt = m_objectRotationsDeg.find(m_selectedObject);
                            if (rotIt != m_objectRotationsDeg.end()) {
                                const ImVec2 mousePos = ImGui::GetMousePos();
                                const float dx = mousePos.x - m_viewport.rotateStartMouse.x;
                                const float angleDelta = dx * 0.35f;

                                glm::vec3 euler = m_viewport.rotateStartEulerDeg;
                                if (m_viewport.rotateAxisLock == "X") {
                                    euler.x += angleDelta;
                                } else if (m_viewport.rotateAxisLock == "Z") {
                                    euler.z += angleDelta;
                                } else {
                                    euler.y += angleDelta;
                                }
                                rotIt->second = euler;

                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    clearGrabState(true);
                                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                    rotIt->second = m_viewport.rotateStartEulerDeg;
                                    clearGrabState(true);
                                }
                            }
                        }

                        if (m_viewport.transformMode == "Scale" && m_viewport.scaleActive) {
                            auto scaleIt = m_objectScales.find(m_selectedObject);
                            if (scaleIt != m_objectScales.end()) {
                                const ImVec2 mousePos = ImGui::GetMousePos();
                                const float dx = mousePos.x - m_viewport.scaleStartMouse.x;
                                const float factor = std::max(0.05f, 1.0f + dx * 0.01f);

                                glm::vec3 scaled = m_viewport.scaleStartValue;
                                if (m_viewport.scaleAxisLock == "X") {
                                    scaled.x = std::max(0.05f, m_viewport.scaleStartValue.x * factor);
                                } else if (m_viewport.scaleAxisLock == "Y") {
                                    scaled.y = std::max(0.05f, m_viewport.scaleStartValue.y * factor);
                                } else if (m_viewport.scaleAxisLock == "Z") {
                                    scaled.z = std::max(0.05f, m_viewport.scaleStartValue.z * factor);
                                } else {
                                    scaled = glm::max(glm::vec3(0.05f), m_viewport.scaleStartValue * factor);
                                }

                                scaleIt->second = scaled;

                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    clearGrabState(true);
                                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                    scaleIt->second = m_viewport.scaleStartValue;
                                    clearGrabState(true);
                                }
                            }
                        }
                    }

                    if (!m_selectedObject.empty() && imageSize.x > 1.0f && imageSize.y > 1.0f) {
                        glm::vec3 selectedCenter(0.0f, -0.05f, 0.0f);
                        glm::vec3 selectedHalfExtent(12.0f, 0.05f, 12.0f);
                        glm::vec3 selectedRotationDeg(0.0f);

                        const auto posIt = m_objectPositions.find(m_selectedObject);
                        const auto scaleIt = m_objectScales.find(m_selectedObject);
                        const auto rotIt = m_objectRotationsDeg.find(m_selectedObject);
                        if (posIt != m_objectPositions.end()) {
                            selectedCenter = posIt->second;
                        }
                        if (scaleIt != m_objectScales.end()) {
                            selectedHalfExtent = GetVisualHalfExtent(scaleIt->second);
                        }
                        if (rotIt != m_objectRotationsDeg.end()) {
                            selectedRotationDeg = rotIt->second;
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
                                selectedRotationDeg,
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

                    const bool isTransforming = m_viewport.grabMoveActive || m_viewport.rotateActive || m_viewport.scaleActive;
                    if (!isTransforming && !clickedGizmoAxis && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Middle) && hovered) {
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const bool insideImage =
                            mousePos.x >= imageMin.x && mousePos.x <= imageMax.x &&
                            mousePos.y >= imageMin.y && mousePos.y <= imageMax.y;

                        if (insideImage) {
                            glm::vec3 rayOrigin;
                            glm::vec3 rayDir;
                            if (BuildMouseRayInImage(mousePos, imageMin, imageMax, view, projection, rayOrigin, rayDir)) {
                                const std::string picked = PickObjectByRay(rayOrigin, rayDir, m_objectPositions, m_objectScales, m_objectRotationsDeg);
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
                        const std::string rotateAxisText = std::format("Rotate Axis: {}", m_viewport.rotateAxisLock);
                        const std::string scaleAxisText = std::format("Scale Axis: {}", m_viewport.scaleAxisLock);
                        DrawOutlinedText(drawList, statusAnchor, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), selectedText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 18.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), transformText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 36.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), axisText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 54.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), rotateAxisText.c_str());
                        DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 72.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), scaleAxisText.c_str());
                        if (!axisSegments.empty()) {
                            for (size_t i = 0; i < axisSegments.size(); ++i) {
                                const auto& [axisName, segment] = axisSegments[i];
                                const float sx = segment.first.x;
                                const float sy = segment.first.y;
                                const float ex = segment.second.x;
                                const float ey = segment.second.y;
                                const std::string screenText = std::format("{}: ({:.1f},{:.1f}) -> ({:.1f},{:.1f})", axisName, sx, sy, ex, ey);
                                DrawOutlinedText(drawList, ImVec2(statusAnchor.x, statusAnchor.y + 90.0f + float(i) * 16.0f), IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), screenText.c_str());
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
    if (ImGui::BeginTabItem("World Editor")) {
        renderWorldEditorTab();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();

    ImGui::End();
}

void EditorApp::renderWorldEditorTab() {
    const Engine::WorldPartition::WorldGridConfig& gridConfig = m_worldPartition.gridConfig();
    const int totalChunkColumns = std::max(1, gridConfig.macroColumns * gridConfig.chunksPerMacroSide);
    const int totalChunkRows = std::max(1, gridConfig.macroRows * gridConfig.chunksPerMacroSide);
    const int groupSize = 4;

    m_worldEditorSelectedChunkX = std::clamp(m_worldEditorSelectedChunkX, 0, totalChunkColumns - 1);
    m_worldEditorSelectedChunkZ = std::clamp(m_worldEditorSelectedChunkZ, 0, totalChunkRows - 1);

    ImGui::TextWrapped("Top view do mundo particionado. Clique numa celula para selecionar e importar mapa.");
    ImGui::Text("Grid real: %d x %d chunks | chunkSize: %.1fm", totalChunkColumns, totalChunkRows, gridConfig.chunkSizeMeters);
    ImGui::SameLine();
    if (ImGui::SmallButton("?")) {
        ImGui::OpenPopup("WorldEditor_TerrainRulesHelp");
    }

    if (ImGui::BeginPopup("WorldEditor_TerrainRulesHelp")) {
        const float minH = terrainMinHeightMeters();
        const float maxH = terrainMaxHeightMeters();
        const float zeroH = terrainZeroLevelMeters();
        const int bitDepth = terrainHeightBitDepth();
        const std::string normMode = terrainNormalizationMode();

        ImGui::TextUnformatted("Regras Globais de Heightmap (usar igual em todas as celulas)");
        ImGui::Separator();
        ImGui::BulletText("Min Height: %.1f m", minH);
        ImGui::BulletText("Max Height: %.1f m", maxH);
        ImGui::BulletText("Zero Level: %.1f m", zeroH);
        ImGui::BulletText("Bit Depth: %d-bit", bitDepth);
        ImGui::BulletText("Normalization: %s", normMode.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("Use a mesma faixa vertical para todos os mapas/chunks. Isso evita emenda quebrada entre celulas e evita mismatch entre editor, cliente e servidor.");
        ImGui::TextWrapped("No Blender/Gaea, configure export com essa faixa global. A diferenca visual entre regioes deve vir da escultura, nao de ranges diferentes por celula.");
        ImGui::Spacing();
        ImGui::TextUnformatted("Formula (sample 16-bit -> metros):");
        ImGui::TextWrapped("h = min + (sample / 65535) * (max - min)");
        ImGui::TextUnformatted("Formula (metros -> sample 16-bit):");
        ImGui::TextWrapped("sample = round((h - min) / (max - min) * 65535)");
        ImGui::Separator();
        ImGui::TextWrapped("Quando usar range por celula: apenas em casos muito especiais. Dificulta alinhamento de borda, LOD e validacao de gameplay.");
        ImGui::EndPopup();
    }
    ImGui::Separator();

    const bool selectedHasMap = doesChunkHaveCompiledMap(m_worldEditorSelectedChunkX, m_worldEditorSelectedChunkZ);
    ImGui::Text("Selecionado: chunk_%d_%d", m_worldEditorSelectedChunkX, m_worldEditorSelectedChunkZ);
    ImGui::SameLine();
    ImGui::TextColored(selectedHasMap ? ImVec4(0.45f, 0.85f, 0.52f, 1.0f) : ImVec4(0.90f, 0.74f, 0.35f, 1.0f),
        selectedHasMap ? "(mapa compilado)" : "(vazio)");

    if (ImGui::Button("Importar mapa para celula")) {
        importChunkMapForCell(m_worldEditorSelectedChunkX, m_worldEditorSelectedChunkZ);
    }
    ImGui::SameLine();
    if (ImGui::Button("Abrir cena da celula")) {
        setActiveChunkScene(m_worldEditorSelectedChunkX, m_worldEditorSelectedChunkZ);
    }

    ImGui::Separator();
    ImGui::TextWrapped("Legenda: celula escura = possui .mmap compilado. Celula pontilhada = sem mapa. Linha forte = limite de grupo 4x4.");

    const float cellSize = 20.0f;
    const float cellGap = 3.0f;
    const float gridWidth = totalChunkColumns * (cellSize + cellGap) + 4.0f;
    const float gridHeight = totalChunkRows * (cellSize + cellGap) + 4.0f;

    ImGui::BeginChild("WorldGridCanvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(origin, ImVec2(origin.x + gridWidth, origin.y + gridHeight), IM_COL32(20, 22, 24, 255), 6.0f);

    auto drawDottedBorder = [&](const ImVec2& minP, const ImVec2& maxP, ImU32 color) {
        constexpr float step = 6.0f;
        constexpr float seg = 3.0f;

        for (float x = minP.x; x < maxP.x; x += step) {
            drawList->AddLine(ImVec2(x, minP.y), ImVec2(std::min(x + seg, maxP.x), minP.y), color, 1.0f);
            drawList->AddLine(ImVec2(x, maxP.y), ImVec2(std::min(x + seg, maxP.x), maxP.y), color, 1.0f);
        }
        for (float y = minP.y; y < maxP.y; y += step) {
            drawList->AddLine(ImVec2(minP.x, y), ImVec2(minP.x, std::min(y + seg, maxP.y)), color, 1.0f);
            drawList->AddLine(ImVec2(maxP.x, y), ImVec2(maxP.x, std::min(y + seg, maxP.y)), color, 1.0f);
        }
    };

    for (int z = 0; z < totalChunkRows; ++z) {
        for (int x = 0; x < totalChunkColumns; ++x) {
            const float px = origin.x + 2.0f + x * (cellSize + cellGap);
            const float py = origin.y + 2.0f + z * (cellSize + cellGap);
            const ImVec2 cellMin(px, py);
            const ImVec2 cellMax(px + cellSize, py + cellSize);
            const bool hasMap = doesChunkHaveCompiledMap(x, z);
            const bool isSelected = (x == m_worldEditorSelectedChunkX && z == m_worldEditorSelectedChunkZ);

            ImU32 fillColor = hasMap ? IM_COL32(52, 64, 76, 255) : IM_COL32(36, 38, 40, 255);
            if (isSelected) {
                fillColor = hasMap ? IM_COL32(78, 106, 128, 255) : IM_COL32(98, 86, 58, 255);
            }

            drawList->AddRectFilled(cellMin, cellMax, fillColor, 2.0f);
            if (hasMap) {
                drawList->AddRect(cellMin, cellMax, IM_COL32(120, 150, 178, 255), 2.0f, ImDrawFlags_None, isSelected ? 2.0f : 1.0f);
            } else {
                drawDottedBorder(cellMin, cellMax, IM_COL32(118, 118, 118, 255));
                if (isSelected) {
                    drawList->AddRect(cellMin, cellMax, IM_COL32(220, 180, 92, 255), 2.0f, ImDrawFlags_None, 2.0f);
                }
            }

            ImGui::SetCursorScreenPos(cellMin);
            ImGui::InvisibleButton(std::format("##chunk_cell_{}_{}", x, z).c_str(), ImVec2(cellSize, cellSize));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                m_worldEditorSelectedChunkX = x;
                m_worldEditorSelectedChunkZ = z;
                m_worldEditorStatus = std::format("Celula selecionada: chunk_{}_{}", x, z);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("chunk_%d_%d\nStatus: %s", x, z, hasMap ? "mmap compilado" : "vazio");
            }
        }
    }

    for (int gx = 0; gx <= totalChunkColumns; gx += groupSize) {
        const float x = origin.x + 2.0f + gx * (cellSize + cellGap) - (cellGap * 0.5f);
        drawList->AddLine(
            ImVec2(x, origin.y + 2.0f),
            ImVec2(x, origin.y + 2.0f + totalChunkRows * (cellSize + cellGap) - cellGap),
            IM_COL32(186, 128, 64, 230),
            2.0f);
    }

    for (int gz = 0; gz <= totalChunkRows; gz += groupSize) {
        const float y = origin.y + 2.0f + gz * (cellSize + cellGap) - (cellGap * 0.5f);
        drawList->AddLine(
            ImVec2(origin.x + 2.0f, y),
            ImVec2(origin.x + 2.0f + totalChunkColumns * (cellSize + cellGap) - cellGap, y),
            IM_COL32(186, 128, 64, 230),
            2.0f);
    }

    ImGui::Dummy(ImVec2(gridWidth + 8.0f, gridHeight + 8.0f));
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextWrapped("Status: %s", m_worldEditorStatus.c_str());
}


