#include "SegmentGui.h"
#include <string>
#include <format>

void SegmentGui::update() {
    if (!visible_)
        return;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

    const float oldScale = ImGui::GetFont()->Scale;
    if (font_scale_ != 1.0f) {
        ImGui::GetFont()->Scale *= font_scale_;
        ImGui::PushFont(ImGui::GetFont());
    }

    constexpr auto WinFlags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoBringToFrontOnFocus | 
                           ImGuiWindowFlags_NoNav;

    ImGui::Begin("Segments", nullptr, WinFlags); {
        ImGui::Text(current_level_name_.c_str());

        if (ImGui::BeginTable("##Segments", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Sector", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Delta", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();
            for (size_t i = 0; i < state_.size(); ++i) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("#%d", i + 1);
                ImGui::TableSetColumnIndex(1);
                const auto time = state_.segment(i);
                ImGui::Text("%.3fs", time);
                ImGui::TableSetColumnIndex(2);
                auto &time_to_compare = state_.segment_target(i);
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat(std::format("##seg{}", i).c_str(), &time_to_compare,
                                 0.1f, 0.0f, 1e6f,
                                 (time_to_compare >= 0.f) ? "%.3fs" : "----");
                /*if (time_to_compare < 0.f)
                    ImGui::Text("----");
                else
                    ImGui::Text("%.3fs", time_to_compare);*/
                ImGui::TableSetColumnIndex(3);
                if (time_to_compare < 0.f)
                    ImGui::Text("----");
                else
                    ImGui::Text("%+.3fs", time - time_to_compare);

                if (cursor_visible_ && i == state_.get_current_segment()) {
                    if (time_to_compare < 0 || std::abs(time - time_to_compare) < 1e-7)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, even_color);
                    else if (time < time_to_compare)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, lead_color);
                    else
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, lag_color);
                }
            }

            ImGui::EndTable();
        }

        if (settings_visible_ && ImGui::TreeNode("History Settings")) {
            if (ImGui::Button("Clear History"))
                state_.clear_history();
            ImGui::Checkbox("Update History", &state_.is_saving_);
            ImGui::TreePop();
        }

        ImGui::End();
    }

    if (font_scale_ != 1.0f) {
        ImGui::GetFont()->Scale = oldScale;
        ImGui::PopFont();
    }
    ImGui::PopStyleColor();
}
