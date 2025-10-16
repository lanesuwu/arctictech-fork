#include "imgui.h"

namespace c {

    inline ImVec4 accent = ImColor(255, 182, 193, 255);

    namespace background {
        inline ImVec4 bg = ImColor(12, 10, 11, 240);
        inline ImVec2 size = ImVec2(950, 750);
        inline float rounding = 16.f;
    }

    namespace child {
        inline ImVec4 bg = ImColor(50, 45, 47, 75);
        inline ImVec4 border = ImColor(120, 100, 110, 65);
        inline ImVec4 border_text = ImColor(255, 255, 255, 255);

        inline float rounding = 8.f;
    }

    namespace tabs {
        inline ImVec4 bg_active = ImColor(255, 182, 193, 75);
        inline ImVec4 bg_hov = ImColor(255, 182, 193, 45);
        inline ImVec4 bg = ImColor(120, 100, 110, 25);

        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 45);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 25);

        inline float rounding = 4.f;
    }

    namespace checkbox {
        inline ImVec4 checkmark_active = ImColor(255, 255, 255, 255);
        inline ImVec4 checkmark_inactive = ImColor(200, 200, 200, 80);

        inline ImVec4 i_bg_hov = ImColor(255, 255, 255, 35);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 2.f;
    }

    namespace slider {
        inline ImVec4 circle = ImColor(255, 182, 193, 255);

        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 30.f;
    }

    namespace input_text {
        inline ImVec4 i_bg_selected = ImColor(255, 182, 193, 75);

        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 4.f;
    }

    namespace keybind {
        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 3.f;
    }

    namespace combo {
        inline ImVec4 i_bg_selected = ImColor(40, 30, 35, 230);

        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 4.f;
    }

    namespace selectable {
        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 4.f;
    }

    namespace scroll {
        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 30.f;
    }

    namespace picker {
        inline ImVec4 i_bg = ImColor(20, 15, 18, 255);

        inline float rounding = 4.f;
    }

    namespace button {
        inline ImVec4 i_bg_hov = ImColor(255, 182, 193, 75);
        inline ImVec4 i_bg = ImColor(120, 100, 110, 45);

        inline float rounding = 4.f;
    }

    namespace text {
        inline ImVec4 text_active = ImColor(255, 255, 255, 255);
        inline ImVec4 text_hov = ImColor(255, 255, 255, 180);
        inline ImVec4 text = ImColor(255, 255, 255, 130);
    }

}
