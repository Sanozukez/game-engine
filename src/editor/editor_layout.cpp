#include "editor_layout.h"

#include <imgui.h>

namespace EditorLayout {
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
        float& consoleH) {
        const float topAreaY = kTopMenuHeight + kToolbarHeight;
        const float usableWidth = viewport->Size.x;
        const float usableHeight = viewport->Size.y;

        topToolbarY = kTopMenuHeight;
        viewportX = kLeftPanelWidth;
        viewportY = topAreaY;
        viewportW = usableWidth - kLeftPanelWidth - kRightPanelWidth;
        viewportH = usableHeight - topAreaY - kBottomPanelHeight - kStatusBarHeight;

        hierarchyW = kLeftPanelWidth;
        hierarchyH = viewportH * 0.62f;

        inspectorX = usableWidth - kRightPanelWidth;
        inspectorY = topAreaY;
        inspectorW = kRightPanelWidth;
        inspectorH = viewportH;

        consoleX = kLeftPanelWidth;
        consoleY = usableHeight - kBottomPanelHeight - kStatusBarHeight;
        consoleW = usableWidth - kLeftPanelWidth - kRightPanelWidth;
        consoleH = kBottomPanelHeight;
    }
}
