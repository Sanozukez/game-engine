#pragma once

struct ImGuiStyle;

enum class EditorVisualStyle {
	Blender,
	FLStudio
};

void ApplyEditorTheme(ImGuiStyle& style, EditorVisualStyle visualStyle);
