#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>

#include "editor_viewport_math.h"

namespace Editor::GizmoUtils {

using GizmoAxisSegments = std::vector<std::pair<std::string, std::pair<ImVec2, ImVec2>>>;

inline float DistanceToSegmentSquared(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
    return ViewportMath::DistanceToSegmentSquared(p, a, b);
}

inline std::string PickGizmoAxis(const ImVec2& mousePos, const GizmoAxisSegments& axes) {
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

inline void DrawOutlinedText(ImDrawList* drawList, const ImVec2& pos, ImU32 fillColor, ImU32 outlineColor, const char* text) {
    static const ImVec2 offsets[] = {
        ImVec2(-1.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(0.0f, -1.0f), ImVec2(0.0f, 1.0f)
    };

    for (const ImVec2& offset : offsets) {
        drawList->AddText(ImVec2(pos.x + offset.x, pos.y + offset.y), outlineColor, text);
    }
    drawList->AddText(pos, fillColor, text);
}

inline ImU32 SetColorAlpha(ImU32 color, int alpha) {
    const ImU32 r = (color >> IM_COL32_R_SHIFT) & 0xFF;
    const ImU32 g = (color >> IM_COL32_G_SHIFT) & 0xFF;
    const ImU32 b = (color >> IM_COL32_B_SHIFT) & 0xFF;
    return IM_COL32(r, g, b, static_cast<ImU8>(alpha));
}

inline void SnapCameraToAxis(float& cameraYaw, float& cameraPitch, const std::string& axisName) {
    if (axisName == "X") {
        cameraYaw = 0.0f;
        cameraPitch = 0.0f;
    } else if (axisName == "Y") {
        cameraYaw = 0.0f;
        cameraPitch = glm::half_pi<float>();
    } else if (axisName == "Z") {
        cameraYaw = glm::half_pi<float>();
        cameraPitch = 0.0f;
    }
}

inline glm::vec3 GetWorldAxisFromName(const std::string& axisName) {
    if (axisName == "X") {
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }
    if (axisName == "Y") {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (axisName == "Z") {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return glm::vec3(0.0f);
}

inline ImU32 GetAxisColor(const std::string& axisName) {
    if (axisName == "X") {
        return IM_COL32(244, 72, 72, 255);
    }
    if (axisName == "Y") {
        return IM_COL32(122, 220, 122, 255);
    }
    if (axisName == "Z") {
        return IM_COL32(111, 126, 255, 255);
    }
    return IM_COL32(255, 255, 255, 255);
}

inline void DrawBlenderGizmo(ImDrawList* drawList, const ImVec2& gizmoOrigin, const glm::vec3& cameraPosition, const glm::vec3& target, GizmoAxisSegments& axisSegments) {
    constexpr float axisLength = 42.0f;
    constexpr float handleRadius = 9.0f;

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

    struct AxisStyle {
        const char* name;
        ImVec2 dir;
        glm::vec3 worldDir;
        ImU32 positiveColor;
        ImU32 negativeColor;
        char label;
    };

    const AxisStyle axes[] = {
        {"X", xDir, cameraRight, IM_COL32(244, 72, 72, 255), IM_COL32(255, 168, 168, 255), 'X'},
        {"Y", yDir, cameraUp, IM_COL32(122, 220, 122, 255), IM_COL32(194, 245, 194, 255), 'Y'},
        {"Z", zDir, cameraForward, IM_COL32(111, 126, 255, 255), IM_COL32(191, 197, 255, 255), 'Z'}
    };

    axisSegments.clear();
    for (const AxisStyle& axis : axes) {
        const ImVec2 positiveEnd = ImVec2(gizmoOrigin.x + axis.dir.x * axisLength, gizmoOrigin.y + axis.dir.y * axisLength);
        const ImVec2 negativeEnd = ImVec2(gizmoOrigin.x - axis.dir.x * axisLength, gizmoOrigin.y - axis.dir.y * axisLength);
        const float facing = glm::dot(glm::normalize(axis.worldDir), cameraForward);
        const float depthBlend = ViewportMath::SafeClamp((facing + 1.0f) * 0.5f, 0.0f, 1.0f);

        const int positiveAlpha = 120 + static_cast<int>(135.0f * depthBlend);
        const int negativeAlpha = 120 + static_cast<int>(135.0f * (1.0f - depthBlend));
        const int lineAlpha = positiveAlpha;

        axisSegments.emplace_back(axis.name, std::make_pair(gizmoOrigin, positiveEnd));

        drawList->AddLine(gizmoOrigin, positiveEnd, SetColorAlpha(axis.positiveColor, lineAlpha), 2.5f);
        drawList->AddCircleFilled(positiveEnd, handleRadius, SetColorAlpha(axis.positiveColor, positiveAlpha), 18);
        drawList->AddCircleFilled(negativeEnd, handleRadius - 1.0f, SetColorAlpha(axis.negativeColor, negativeAlpha), 18);

        const std::string labelText(1, axis.label);
        const ImVec2 labelSize = ImGui::CalcTextSize(labelText.c_str());
        const ImVec2 labelPos = ImVec2(
            positiveEnd.x - labelSize.x * 0.5f,
            positiveEnd.y - labelSize.y * 0.5f - 1.0f);
        drawList->AddText(ImVec2(labelPos.x + 1.0f, labelPos.y + 1.0f), IM_COL32(255, 255, 255, 90), labelText.c_str());
        drawList->AddText(labelPos, IM_COL32(0, 0, 0, 255), labelText.c_str());
    }
}

} // namespace Editor::GizmoUtils
