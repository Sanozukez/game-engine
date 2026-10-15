#pragma once

struct ImGuiViewport;

namespace EditorLayout {
    constexpr float kTopMenuHeight = 24.0f;
    constexpr float kToolbarHeight = 48.0f;
    constexpr float kLeftPanelWidth = 270.0f;
    constexpr float kRightPanelWidth = 310.0f;
    constexpr float kBottomPanelHeight = 180.0f;
    constexpr float kStatusBarHeight = 26.0f;

    void ComputePanelRects(
        const ImGuiViewport* viewport,
        float& topToolbarY,
        float& viewportX,
        float& viewportY,
        float& viewportW,
        float& viewportH,
        float& hierarchyW,
        float& hierarchyH,
        float& inspectorX,
        float& inspectorY,
        float& inspectorW,
        float& inspectorH,
        float& consoleX,
        float& consoleY,
        float& consoleW,
        float& consoleH);
}
