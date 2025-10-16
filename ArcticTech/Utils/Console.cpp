#include "Console.h"

#include "../SDK/Interfaces.h"


CGameConsole* Console = new CGameConsole;

// Pastel pink accent: #FFB6C1
static const Color ACCENT_COLOR = Color(255, 182, 193);

inline int HexToDec(char h) {
    if (h >= '0' && h <= '9') return h - '0';
    if (h >= 'A' && h <= 'F') return 10 + h - 'A';
    if (h >= 'a' && h <= 'f') return 10 + h - 'a';
    return 0;
}

Color ParseColorFromHex(const std::string& clr) {
    // expects 6 chars: RRGGBB
    if (clr.size() < 6) return Color(255, 255, 255);
    return Color(
        HexToDec(clr[0]) * 16 + HexToDec(clr[1]),
        HexToDec(clr[2]) * 16 + HexToDec(clr[3]),
        HexToDec(clr[4]) * 16 + HexToDec(clr[5])
    );
}


void CGameConsole::ArcticTag() {
    CVar->ConsoleColorPrintf(ACCENT_COLOR, "arctic | ");
}

void CGameConsole::Print(const std::string& msg, Color color_def) {
    Color current_clr = color_def;
    std::string str;
    std::string clr_to_parse;
    int clr_digits = -1;
    for (auto c : msg) {
        if (c != '\a' && clr_digits == -1) {
            str += c;
            continue;
        }

        if (c == '\a') {
            if (!str.empty())
                ColorPrint(str, current_clr);
            str.clear();
            clr_digits = 0;
            continue;
        }

        clr_to_parse += c;
        clr_digits++;

        if (clr_digits == 6) {
            clr_digits = -1;
            if (clr_to_parse == "ACCENT")
                current_clr = ACCENT_COLOR;
            else if (clr_to_parse == "_MAIN_")
                current_clr = Color(232); // keep main as before (grayscale)
            else
                current_clr = ParseColorFromHex(clr_to_parse);
            clr_to_parse.clear();
        }
    }
    if (!str.empty())
        ColorPrint(str, current_clr);
}

void CGameConsole::ColorPrint(const std::string& msg, const Color& color) {
    CVar->ConsoleColorPrintf(color, msg.c_str());
}

void CGameConsole::Log(const std::string& msg) {
    CVar->ConsoleColorPrintf(ACCENT_COLOR, "arctic | ");
    CVar->ConsoleColorPrintf(Color(232), msg.c_str());
    CVar->ConsolePrintf("\n");
}

void CGameConsole::Error(const std::string& error) {
    ArcticTag();

    CVar->ConsoleColorPrintf(Color(255, 50, 50), error.c_str());
    CVar->ConsolePrintf("\n");
}
