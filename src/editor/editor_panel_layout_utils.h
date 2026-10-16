#pragma once

#include <imgui.h>

#include "editor_layout.h"

namespace Editor::PanelLayoutUtils {

struct PanelRects {
    float topToolbarY = 0.0f;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportW = 0.0f;
    float viewportH = 0.0f;
    float hierarchyW = 0.0f;
    float hierarchyH = 0.0f;
    float inspectorX = 0.0f;
    float inspectorY = 0.0f;
    float inspectorW = 0.0f;
    float inspectorH = 0.0f;
    float consoleX = 0.0f;
    float consoleY = 0.0f;
    float consoleW = 0.0f;
    float consoleH = 0.0f;
};

struct SceneEditorRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 260.0f;
    float h = 240.0f;
};

inline PanelRects ComputePanelRects(const ImGuiViewport* viewport) {
    PanelRects rects;
    EditorLayout::ComputePanelRects(
        viewport,
        rects.topToolbarY,
        rects.viewportX,
        rects.viewportY,
        rects.viewportW,
        rects.viewportH,
        rects.hierarchyW,
        rects.hierarchyH,
        rects.inspectorX,
        rects.inspectorY,
        rects.inspectorW,
        rects.inspectorH,
        rects.consoleX,
        rects.consoleY,
        rects.consoleW,
        rects.consoleH);
    return rects;
}

inline SceneEditorRect ComputeSceneEditorRect(const ImGuiViewport* viewport) {
    SceneEditorRect rect;
    rect.y = viewport->Size.y - 300.0f;
    return rect;
}

} // namespace Editor::PanelLayoutUtils
