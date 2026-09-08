#include "editor_theme.h"

#include <imgui.h>

void ApplyEditorTheme(ImGuiStyle& style) {
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.23f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.29f, 0.33f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.82f, 0.44f, 0.18f, 0.78f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.52f, 0.20f, 0.88f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.96f, 0.60f, 0.24f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.27f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.36f, 0.40f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.75f, 0.40f, 0.18f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.85f, 0.45f, 0.18f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.62f, 0.65f, 0.70f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.79f, 0.42f, 0.18f, 0.70f);
    colors[ImGuiCol_TabActive] = ImVec4(0.83f, 0.45f, 0.20f, 0.90f);

    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ChildBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ScrollbarSize = 10.0f;
}
