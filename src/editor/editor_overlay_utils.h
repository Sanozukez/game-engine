#pragma once

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "editor_viewport_math.h"

namespace Editor::OverlayUtils {

inline glm::vec3 GetVisualHalfExtent(const glm::vec3& visualScale) {
    // The mock box mesh spans X/Z in [-1, 1] and Y in [-0.5, 0.5].
    return glm::vec3(visualScale.x, visualScale.y * 0.5f, visualScale.z);
}

inline void DrawProjectedVisibleBoxOutline(
    ImDrawList* drawList,
    const ImVec2& imageMin,
    const ImVec2& imageSize,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& center,
    const glm::vec3& halfExtent,
    const glm::vec3& rotationDeg,
    ImU32 color,
    float thickness) {
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rot = glm::rotate(rot, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rot = glm::rotate(rot, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::mat3 rot3(rot);

    const glm::vec3 localCorners[8] = {
        glm::vec3(-halfExtent.x, -halfExtent.y, -halfExtent.z),
        glm::vec3(halfExtent.x, -halfExtent.y, -halfExtent.z),
        glm::vec3(halfExtent.x, halfExtent.y, -halfExtent.z),
        glm::vec3(-halfExtent.x, halfExtent.y, -halfExtent.z),
        glm::vec3(-halfExtent.x, -halfExtent.y, halfExtent.z),
        glm::vec3(halfExtent.x, -halfExtent.y, halfExtent.z),
        glm::vec3(halfExtent.x, halfExtent.y, halfExtent.z),
        glm::vec3(-halfExtent.x, halfExtent.y, halfExtent.z)
    };
    glm::vec3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i] = center + rot3 * localCorners[i];
    }

    const int faces[6][4] = {
        {0, 1, 2, 3}, // back  (z-)
        {4, 5, 6, 7}, // front (z+)
        {0, 3, 7, 4}, // left  (x-)
        {1, 2, 6, 5}, // right (x+)
        {3, 2, 6, 7}, // top   (y+)
        {0, 1, 5, 4}  // bottom(y-)
    };

    const glm::vec3 faceNormals[6] = {
        rot3 * glm::vec3(0.0f, 0.0f, -1.0f),
        rot3 * glm::vec3(0.0f, 0.0f, 1.0f),
        rot3 * glm::vec3(-1.0f, 0.0f, 0.0f),
        rot3 * glm::vec3(1.0f, 0.0f, 0.0f),
        rot3 * glm::vec3(0.0f, 1.0f, 0.0f),
        rot3 * glm::vec3(0.0f, -1.0f, 0.0f)
    };

    bool faceVisible[6] = {};
    for (int f = 0; f < 6; ++f) {
        glm::vec3 faceCenter(0.0f);
        for (int c = 0; c < 4; ++c) {
            faceCenter += corners[faces[f][c]];
        }
        faceCenter *= 0.25f;

        const glm::vec3 toCamera = cameraPosition - faceCenter;
        faceVisible[f] = glm::dot(faceNormals[f], toCamera) > 0.0f;
    }

    struct EdgeInfo {
        int a;
        int b;
        int face0;
        int face1;
    };

    const EdgeInfo edges[12] = {
        {0, 1, 0, 5}, {1, 2, 0, 3}, {2, 3, 0, 4}, {3, 0, 0, 2},
        {4, 5, 1, 5}, {5, 6, 1, 3}, {6, 7, 1, 4}, {7, 4, 1, 2},
        {0, 4, 2, 5}, {1, 5, 3, 5}, {2, 6, 3, 4}, {3, 7, 2, 4}
    };

    const glm::mat4 vp = projection * view;
    constexpr float kMinClipW = 0.01f;
    for (int i = 0; i < 12; ++i) {
        const EdgeInfo& edge = edges[i];
        const bool silhouetteEdge = faceVisible[edge.face0] != faceVisible[edge.face1];
        if (!silhouetteEdge) {
            continue;
        }

        const glm::vec3 p0 = corners[edge.a];
        const glm::vec3 p1 = corners[edge.b];
        glm::vec4 c0 = vp * glm::vec4(p0, 1.0f);
        glm::vec4 c1 = vp * glm::vec4(p1, 1.0f);
        const bool v0 = c0.w > kMinClipW;
        const bool v1 = c1.w > kMinClipW;
        if (!v0 && !v1) {
            continue;
        }

        glm::vec3 a = p0;
        glm::vec3 b = p1;
        if (v0 != v1) {
            const float denom = c1.w - c0.w;
            if (std::abs(denom) < 1e-6f) {
                continue;
            }
            const float t = ViewportMath::SafeClamp((kMinClipW - c0.w) / denom, 0.0f, 1.0f);
            const glm::vec3 clipped = glm::mix(p0, p1, t);
            if (!v0) {
                a = clipped;
            } else {
                b = clipped;
            }
        }

        const ImVec2 pa = ViewportMath::ProjectToScreen(a, view, projection, imageSize);
        const ImVec2 pb = ViewportMath::ProjectToScreen(b, view, projection, imageSize);
        drawList->AddLine(
            ImVec2(imageMin.x + pa.x, imageMin.y + pa.y),
            ImVec2(imageMin.x + pb.x, imageMin.y + pb.y),
            color,
            thickness);
    }
}

inline void DrawProjectedGroundOutline(
    ImDrawList* drawList,
    const ImVec2& imageMin,
    const ImVec2& imageSize,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& center,
    const glm::vec3& halfExtent,
    ImU32 color,
    float thickness) {
    const float topY = center.y + halfExtent.y;
    const glm::vec3 corners[4] = {
        glm::vec3(center.x - halfExtent.x, topY, center.z - halfExtent.z),
        glm::vec3(center.x + halfExtent.x, topY, center.z - halfExtent.z),
        glm::vec3(center.x + halfExtent.x, topY, center.z + halfExtent.z),
        glm::vec3(center.x - halfExtent.x, topY, center.z + halfExtent.z)
    };

    const glm::mat4 vp = projection * view;
    constexpr float kMinClipW = 0.01f;
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        const glm::vec3 p0 = corners[i];
        const glm::vec3 p1 = corners[j];

        glm::vec4 c0 = vp * glm::vec4(p0, 1.0f);
        glm::vec4 c1 = vp * glm::vec4(p1, 1.0f);

        const bool v0 = c0.w > kMinClipW;
        const bool v1 = c1.w > kMinClipW;
        if (!v0 && !v1) {
            continue;
        }

        glm::vec3 a = p0;
        glm::vec3 b = p1;
        if (v0 != v1) {
            const float w0 = c0.w;
            const float w1 = c1.w;
            const float denom = (w1 - w0);
            if (std::abs(denom) < 1e-6f) {
                continue;
            }

            const float t = ViewportMath::SafeClamp((kMinClipW - w0) / denom, 0.0f, 1.0f);
            const glm::vec3 clipped = glm::mix(p0, p1, t);
            if (!v0) {
                a = clipped;
            } else {
                b = clipped;
            }
        }

        const ImVec2 pa = ViewportMath::ProjectToScreen(a, view, projection, imageSize);
        const ImVec2 pb = ViewportMath::ProjectToScreen(b, view, projection, imageSize);
        drawList->AddLine(
            ImVec2(imageMin.x + pa.x, imageMin.y + pa.y),
            ImVec2(imageMin.x + pb.x, imageMin.y + pb.y),
            color,
            thickness);
    }
}

} // namespace Editor::OverlayUtils
