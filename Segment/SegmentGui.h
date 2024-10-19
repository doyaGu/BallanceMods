#pragma once
#include <format>
#include <imgui.h>

#include "PerLevelSegmentState.h"

class SegmentGui {
public:
    SegmentGui(PerLevelSegmentState &state, const int level)
        : state_(state), current_level_name_(std::format("Level {}", level)) {}

    SegmentGui(PerLevelSegmentState &state, const std::string_view level_name)
        : state_(state), current_level_name_(level_name) {}

    void update();

    void set_visible(bool visible) { visible_ = visible; }
    void set_cursor_visible(bool visible) { cursor_visible_ = visible; }
    void set_settings_visible_(bool visible) { settings_visible_ = visible; }
    void set_font_scale(float scale) { font_scale_ = scale; }

    PerLevelSegmentState &state_;
    const std::string current_level_name_;
    bool visible_ = true;
    bool cursor_visible_ = true;
    bool settings_visible_ = false;
    float font_scale_ = 0.7f;

    const static inline std::string STYLES_SAVE_PATH = "../ModLoader/Configs/SegmentStyles.ini";
    const static inline ImU32 lead_color = ImGui::GetColorU32(ImVec4(0.2f, 0.8f, 0.2f, 0.75f));
    const static inline ImU32 even_color = ImGui::GetColorU32(ImVec4(1.f, 0.66f, 0.f, 0.75f));
    const static inline ImU32 lag_color = ImGui::GetColorU32(ImVec4(0.85f, 0.08f, 0.25f, 0.75f));
    const static inline ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 0.3f));
};
