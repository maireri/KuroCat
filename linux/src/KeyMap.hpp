#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <linux/input-event-codes.h>

struct Pt {
    float x = 0.0f;
    float y = 0.0f;
};

enum class PhysicalKey {
    Escape, K1, K2, K3, K4, K5, K6, K7, K8, K9, K0, Minus, Equal, Backspace,
    Tab, Q, W, E, R, T, Y, U, I, O, P, LeftBracket, RightBracket, Backslash,
    CapsLock, A, S, D, F, G, H, J, K, L, Semicolon, Quote, Enter,
    LeftShift, Z, X, C, V, B, N, M, Comma, Period, Slash, RightShift, ArrowUp,
    LeftControl, LeftOption, LeftCommand, Space, RightCommand, RightOption, Fn, ArrowLeft, ArrowDown, ArrowRight,
    Grave, RightControl, ForwardDelete,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    None
};

inline std::string keyBadgeLabel(PhysicalKey k) {
    switch (k) {
        case PhysicalKey::Escape: return "ESC";
        case PhysicalKey::Grave: return "~";
        case PhysicalKey::K1: return "1"; case PhysicalKey::K2: return "2";
        case PhysicalKey::K3: return "3"; case PhysicalKey::K4: return "4";
        case PhysicalKey::K5: return "5"; case PhysicalKey::K6: return "6";
        case PhysicalKey::K7: return "7"; case PhysicalKey::K8: return "8";
        case PhysicalKey::K9: return "9"; case PhysicalKey::K0: return "0";
        case PhysicalKey::Minus: return "-"; case PhysicalKey::Equal: return "=";
        case PhysicalKey::Backspace: return "⌫";
        case PhysicalKey::ForwardDelete: return "DEL";
        case PhysicalKey::Tab: return "⇥";
        case PhysicalKey::Q: return "Q"; case PhysicalKey::W: return "W";
        case PhysicalKey::E: return "E"; case PhysicalKey::R: return "R";
        case PhysicalKey::T: return "T"; case PhysicalKey::Y: return "Y";
        case PhysicalKey::U: return "U"; case PhysicalKey::I: return "I";
        case PhysicalKey::O: return "O"; case PhysicalKey::P: return "P";
        case PhysicalKey::LeftBracket: return "["; case PhysicalKey::RightBracket: return "]";
        case PhysicalKey::Backslash: return "\\";
        case PhysicalKey::CapsLock: return "⇪";
        case PhysicalKey::A: return "A"; case PhysicalKey::S: return "S";
        case PhysicalKey::D: return "D"; case PhysicalKey::F: return "F";
        case PhysicalKey::G: return "G"; case PhysicalKey::H: return "H";
        case PhysicalKey::J: return "J"; case PhysicalKey::K: return "K";
        case PhysicalKey::L: return "L";
        case PhysicalKey::Semicolon: return ";"; case PhysicalKey::Quote: return "'";
        case PhysicalKey::Enter: return "↵";
        case PhysicalKey::LeftShift:
        case PhysicalKey::RightShift: return "⇧";
        case PhysicalKey::Z: return "Z"; case PhysicalKey::X: return "X";
        case PhysicalKey::C: return "C"; case PhysicalKey::V: return "V";
        case PhysicalKey::B: return "B"; case PhysicalKey::N: return "N";
        case PhysicalKey::M: return "M";
        case PhysicalKey::Comma: return ","; case PhysicalKey::Period: return ".";
        case PhysicalKey::Slash: return "/";
        case PhysicalKey::LeftControl:
        case PhysicalKey::RightControl: return "CTL";
        case PhysicalKey::LeftOption:
        case PhysicalKey::RightOption: return "ALT";
        case PhysicalKey::LeftCommand:
        case PhysicalKey::RightCommand: return "SUP";
        case PhysicalKey::Space: return "␣";
        case PhysicalKey::Fn: return "Fn";
        case PhysicalKey::ArrowUp: return "↑";
        case PhysicalKey::ArrowDown: return "↓";
        case PhysicalKey::ArrowLeft: return "←";
        case PhysicalKey::ArrowRight: return "→";
        case PhysicalKey::F1: return "F1"; case PhysicalKey::F2: return "F2";
        case PhysicalKey::F3: return "F3"; case PhysicalKey::F4: return "F4";
        case PhysicalKey::F5: return "F5"; case PhysicalKey::F6: return "F6";
        case PhysicalKey::F7: return "F7"; case PhysicalKey::F8: return "F8";
        case PhysicalKey::F9: return "F9"; case PhysicalKey::F10: return "F10";
        case PhysicalKey::F11: return "F11"; case PhysicalKey::F12: return "F12";
        default: return "";
    }
}

inline PhysicalKey visualKeyOf(PhysicalKey k) {
    switch (k) {
        case PhysicalKey::Grave: return PhysicalKey::Escape;
        case PhysicalKey::ForwardDelete: return PhysicalKey::Backspace;
        case PhysicalKey::RightControl: return PhysicalKey::RightOption;
        case PhysicalKey::F1: return PhysicalKey::K1;
        case PhysicalKey::F2: return PhysicalKey::K2;
        case PhysicalKey::F3: return PhysicalKey::K3;
        case PhysicalKey::F4: return PhysicalKey::K4;
        case PhysicalKey::F5: return PhysicalKey::K5;
        case PhysicalKey::F6: return PhysicalKey::K6;
        case PhysicalKey::F7: return PhysicalKey::K7;
        case PhysicalKey::F8: return PhysicalKey::K8;
        case PhysicalKey::F9: return PhysicalKey::K9;
        case PhysicalKey::F10: return PhysicalKey::K0;
        case PhysicalKey::F11: return PhysicalKey::Minus;
        case PhysicalKey::F12: return PhysicalKey::Equal;
        default: return k;
    }
}

/// Viewer-left paw = cat's RIGHT hand (mouse + right side of keyboard).
/// Viewer-right paw = cat's LEFT hand (WASD, Q-T, A-G, Z-B, Space, Esc, Shift).
inline bool prefersViewerLeftPaw(PhysicalKey k) {
    switch (visualKeyOf(k)) {
        case PhysicalKey::Escape: case PhysicalKey::K1: case PhysicalKey::K2:
        case PhysicalKey::K3: case PhysicalKey::K4: case PhysicalKey::K5: case PhysicalKey::K6:
        case PhysicalKey::Tab: case PhysicalKey::Q: case PhysicalKey::W:
        case PhysicalKey::E: case PhysicalKey::R: case PhysicalKey::T:
        case PhysicalKey::CapsLock: case PhysicalKey::A: case PhysicalKey::S:
        case PhysicalKey::D: case PhysicalKey::F: case PhysicalKey::G:
        case PhysicalKey::LeftShift: case PhysicalKey::Z: case PhysicalKey::X:
        case PhysicalKey::C: case PhysicalKey::V: case PhysicalKey::B:
        case PhysicalKey::LeftControl: case PhysicalKey::LeftOption: case PhysicalKey::LeftCommand:
        case PhysicalKey::Space:
            return false;
        default:
            return true;
    }
}

/// Map Linux kernel evdev keycode (also X11 evdev keycode - 8) -> PhysicalKey.
inline PhysicalKey mapLinuxEvdevKey(uint16_t code) {
    switch (code) {
        case KEY_ESC: return PhysicalKey::Escape;
        case KEY_1: case KEY_KP1: return PhysicalKey::K1;
        case KEY_2: case KEY_KP2: return PhysicalKey::K2;
        case KEY_3: case KEY_KP3: return PhysicalKey::K3;
        case KEY_4: case KEY_KP4: return PhysicalKey::K4;
        case KEY_5: case KEY_KP5: return PhysicalKey::K5;
        case KEY_6: case KEY_KP6: return PhysicalKey::K6;
        case KEY_7: case KEY_KP7: return PhysicalKey::K7;
        case KEY_8: case KEY_KP8: return PhysicalKey::K8;
        case KEY_9: case KEY_KP9: return PhysicalKey::K9;
        case KEY_0: case KEY_KP0: return PhysicalKey::K0;
        case KEY_MINUS: case KEY_KPMINUS: return PhysicalKey::Minus;
        case KEY_EQUAL: case KEY_KPPLUS: return PhysicalKey::Equal;
        case KEY_BACKSPACE: return PhysicalKey::Backspace;
        case KEY_DELETE: return PhysicalKey::ForwardDelete;
        case KEY_TAB: return PhysicalKey::Tab;
        case KEY_Q: return PhysicalKey::Q;
        case KEY_W: return PhysicalKey::W;
        case KEY_E: return PhysicalKey::E;
        case KEY_R: return PhysicalKey::R;
        case KEY_T: return PhysicalKey::T;
        case KEY_Y: return PhysicalKey::Y;
        case KEY_U: return PhysicalKey::U;
        case KEY_I: return PhysicalKey::I;
        case KEY_O: return PhysicalKey::O;
        case KEY_P: return PhysicalKey::P;
        case KEY_LEFTBRACE: return PhysicalKey::LeftBracket;
        case KEY_RIGHTBRACE: return PhysicalKey::RightBracket;
        case KEY_BACKSLASH: return PhysicalKey::Backslash;
        case KEY_CAPSLOCK: return PhysicalKey::CapsLock;
        case KEY_A: return PhysicalKey::A;
        case KEY_S: return PhysicalKey::S;
        case KEY_D: return PhysicalKey::D;
        case KEY_F: return PhysicalKey::F;
        case KEY_G: return PhysicalKey::G;
        case KEY_H: return PhysicalKey::H;
        case KEY_J: return PhysicalKey::J;
        case KEY_K: return PhysicalKey::K;
        case KEY_L: return PhysicalKey::L;
        case KEY_SEMICOLON: return PhysicalKey::Semicolon;
        case KEY_APOSTROPHE: return PhysicalKey::Quote;
        case KEY_GRAVE: return PhysicalKey::Grave;
        case KEY_ENTER: case KEY_KPENTER: return PhysicalKey::Enter;
        case KEY_LEFTSHIFT: return PhysicalKey::LeftShift;
        case KEY_RIGHTSHIFT: return PhysicalKey::RightShift;
        case KEY_Z: return PhysicalKey::Z;
        case KEY_X: return PhysicalKey::X;
        case KEY_C: return PhysicalKey::C;
        case KEY_V: return PhysicalKey::V;
        case KEY_B: return PhysicalKey::B;
        case KEY_N: return PhysicalKey::N;
        case KEY_M: return PhysicalKey::M;
        case KEY_COMMA: return PhysicalKey::Comma;
        case KEY_DOT: case KEY_KPDOT: return PhysicalKey::Period;
        case KEY_SLASH: case KEY_KPSLASH: return PhysicalKey::Slash;
        case KEY_LEFTCTRL: return PhysicalKey::LeftControl;
        case KEY_RIGHTCTRL: return PhysicalKey::RightControl;
        case KEY_LEFTALT: return PhysicalKey::LeftOption;
        case KEY_RIGHTALT: return PhysicalKey::RightOption;
        case KEY_LEFTMETA: return PhysicalKey::LeftCommand;
        case KEY_RIGHTMETA: return PhysicalKey::RightCommand;
        case KEY_SPACE: return PhysicalKey::Space;
        case KEY_UP: return PhysicalKey::ArrowUp;
        case KEY_DOWN: return PhysicalKey::ArrowDown;
        case KEY_LEFT: return PhysicalKey::ArrowLeft;
        case KEY_RIGHT: return PhysicalKey::ArrowRight;
        case KEY_FN: return PhysicalKey::Fn;
        case KEY_F1: return PhysicalKey::F1;
        case KEY_F2: return PhysicalKey::F2;
        case KEY_F3: return PhysicalKey::F3;
        case KEY_F4: return PhysicalKey::F4;
        case KEY_F5: return PhysicalKey::F5;
        case KEY_F6: return PhysicalKey::F6;
        case KEY_F7: return PhysicalKey::F7;
        case KEY_F8: return PhysicalKey::F8;
        case KEY_F9: return PhysicalKey::F9;
        case KEY_F10: return PhysicalKey::F10;
        case KEY_F11: return PhysicalKey::F11;
        case KEY_F12: return PhysicalKey::F12;
        default: return PhysicalKey::None;
    }
}

struct VisualKeycap {
    PhysicalKey key;
    int row;
    float xUnits;
    float widthUnits;
    bool isAmberAccent;

    Pt normalizedCenter() const {
        return { xUnits + widthUnits * 0.5f, (row + 0.5f) / 5.0f };
    }
};

class KeyboardLayout {
public:
    static const std::vector<VisualKeycap>& keycaps() {
        static const std::vector<VisualKeycap> caps = build();
        return caps;
    }

    static Pt screenQuadPoint(float su, float sv) {
        const Pt topLeft     = { 109.0f, 238.0f };
        const Pt topRight    = { 281.0f, 247.0f };
        const Pt bottomLeft  = { 103.0f, 282.0f };
        const Pt bottomRight = { 286.0f, 292.0f };

        float topX = topLeft.x + (topRight.x - topLeft.x) * su;
        float topY = topLeft.y + (topRight.y - topLeft.y) * su;
        float botX = bottomLeft.x + (bottomRight.x - bottomLeft.x) * su;
        float botY = bottomLeft.y + (bottomRight.y - bottomLeft.y) * su;
        return { topX + (botX - topX) * sv, topY + (botY - topY) * sv };
    }

    /// 180° rotated to face the cat: Spacebar closest to cat (top), amber Enter left, amber Esc bottom-right!
    static Pt canvasPoint(float u, float v) {
        return screenQuadPoint(1.0f - u, 1.0f - v);
    }

    static Pt canvasPointForKey(PhysicalKey key) {
        PhysicalKey vk = visualKeyOf(key);
        const auto& map = lookup();
        auto it = map.find(vk);
        if (it != map.end()) {
            Pt c = it->second.normalizedCenter();
            return canvasPoint(c.x, c.y);
        }
        return canvasPoint(0.5f, 0.5f);
    }

    struct V2KeyCellInfo {
        int row;
        int col;
        Pt center;
    };

    /// Bilinear interpolation on Xiao Douni Video 2's angled hand-drawn keyboard surface (`u` left->right, `v` top->bottom in 832x720 V2Space).
    static Pt v2KbPoint(float u, float v) {
        constexpr Pt tl = { 242.0f, 336.0f };
        constexpr Pt tr = { 782.0f, 446.0f };
        constexpr Pt br = { 584.0f, 586.0f };
        constexpr Pt bl = {  28.0f, 440.0f };
        float topX = tl.x + (tr.x - tl.x) * u;
        float topY = tl.y + (tr.y - tl.y) * u;
        float botX = bl.x + (br.x - bl.x) * u;
        float botY = bl.y + (br.y - bl.y) * u;
        return { topX + (botX - topX) * v, topY + (botY - topY) * v };
    }

    /// Single source of truth mapping any PhysicalKey -> exact Video 2 keyboard cell (row, col) and V2Space center point.
    static V2KeyCellInfo v2KeyCellInfo(PhysicalKey key) {
        constexpr float wellU0 = 0.31f, wellU1 = 0.95f;
        constexpr float wellV0 = 0.11f, wellV1 = 0.87f;
        PhysicalKey vk = visualKeyOf(key);
        if (vk == PhysicalKey::Space) {
            float su = wellU0 + (wellU1 - wellU0) * (4.0f / 8.0f);
            float sv = wellV0 + (wellV1 - wellV0) * (3.5f / 4.0f);
            return { 3, 2, v2KbPoint(su, sv) };
        }
        const auto& map = lookup();
        auto it = map.find(vk);
        if (it == map.end()) {
            float su = wellU0 + (wellU1 - wellU0) * (3.5f / 8.0f);
            float sv = wellV0 + (wellV1 - wellV0) * (1.5f / 4.0f);
            return { 1, 3, v2KbPoint(su, sv) };
        }
        const VisualKeycap& cap = it->second;
        float normU = std::clamp(1.0f - (cap.xUnits + cap.widthUnits * 0.5f), 0.0f, 1.0f);
        int col = std::clamp((int)(normU * 7.99f), 0, 7);
        int row = std::clamp(cap.row >= 3 ? 3 : cap.row, 0, 3);
        if (row == 3 && col >= 2 && col <= 5) {
            row = 2;
        }
        float su = wellU0 + (wellU1 - wellU0) * (((float)col + 0.5f) / 8.0f);
        float sv = wellV0 + (wellV1 - wellV0) * (((float)row + 0.5f) / 4.0f);
        return { row, col, v2KbPoint(su, sv) };
    }

    static Pt v2KeyCellCenter(PhysicalKey key) {
        return v2KeyCellInfo(key).center;
    }

private:
    static std::vector<VisualKeycap> build() {
        struct Entry { PhysicalKey k; float w; bool amber; };
        const std::vector<std::vector<Entry>> rows = {
            {
                {PhysicalKey::Escape, 1.0f, true}, {PhysicalKey::K1, 1.0f, false}, {PhysicalKey::K2, 1.0f, false},
                {PhysicalKey::K3, 1.0f, false}, {PhysicalKey::K4, 1.0f, false}, {PhysicalKey::K5, 1.0f, false},
                {PhysicalKey::K6, 1.0f, false}, {PhysicalKey::K7, 1.0f, false}, {PhysicalKey::K8, 1.0f, false},
                {PhysicalKey::K9, 1.0f, false}, {PhysicalKey::K0, 1.0f, false}, {PhysicalKey::Minus, 1.0f, false},
                {PhysicalKey::Equal, 1.0f, false}, {PhysicalKey::Backspace, 2.0f, false}
            },
            {
                {PhysicalKey::Tab, 1.5f, false}, {PhysicalKey::Q, 1.0f, false}, {PhysicalKey::W, 1.0f, false},
                {PhysicalKey::E, 1.0f, false}, {PhysicalKey::R, 1.0f, false}, {PhysicalKey::T, 1.0f, false},
                {PhysicalKey::Y, 1.0f, false}, {PhysicalKey::U, 1.0f, false}, {PhysicalKey::I, 1.0f, false},
                {PhysicalKey::O, 1.0f, false}, {PhysicalKey::P, 1.0f, false}, {PhysicalKey::LeftBracket, 1.0f, false},
                {PhysicalKey::RightBracket, 1.0f, false}, {PhysicalKey::Backslash, 1.5f, false}
            },
            {
                {PhysicalKey::CapsLock, 1.75f, false}, {PhysicalKey::A, 1.0f, false}, {PhysicalKey::S, 1.0f, false},
                {PhysicalKey::D, 1.0f, false}, {PhysicalKey::F, 1.0f, false}, {PhysicalKey::G, 1.0f, false},
                {PhysicalKey::H, 1.0f, false}, {PhysicalKey::J, 1.0f, false}, {PhysicalKey::K, 1.0f, false},
                {PhysicalKey::L, 1.0f, false}, {PhysicalKey::Semicolon, 1.0f, false}, {PhysicalKey::Quote, 1.0f, false},
                {PhysicalKey::Enter, 2.25f, true}
            },
            {
                {PhysicalKey::LeftShift, 2.25f, false}, {PhysicalKey::Z, 1.0f, false}, {PhysicalKey::X, 1.0f, false},
                {PhysicalKey::C, 1.0f, false}, {PhysicalKey::V, 1.0f, false}, {PhysicalKey::B, 1.0f, false},
                {PhysicalKey::N, 1.0f, false}, {PhysicalKey::M, 1.0f, false}, {PhysicalKey::Comma, 1.0f, false},
                {PhysicalKey::Period, 1.0f, false}, {PhysicalKey::Slash, 1.0f, false},
                {PhysicalKey::RightShift, 1.75f, false}, {PhysicalKey::ArrowUp, 1.0f, false}
            },
            {
                {PhysicalKey::LeftControl, 1.25f, false}, {PhysicalKey::LeftOption, 1.25f, false},
                {PhysicalKey::LeftCommand, 1.25f, false}, {PhysicalKey::Space, 6.25f, false},
                {PhysicalKey::RightCommand, 1.0f, false}, {PhysicalKey::RightOption, 1.0f, false},
                {PhysicalKey::ArrowLeft, 1.0f, false}, {PhysicalKey::ArrowDown, 1.0f, false},
                {PhysicalKey::ArrowRight, 1.0f, false}
            }
        };

        std::vector<VisualKeycap> out;
        constexpr float totalUnits = 15.0f;
        for (int r = 0; r < (int)rows.size(); ++r) {
            float cursor = 0.0f;
            for (const auto& e : rows[r]) {
                out.push_back({ e.k, r, cursor / totalUnits, e.w / totalUnits, e.amber });
                cursor += e.w;
            }
        }
        return out;
    }

    static const std::unordered_map<PhysicalKey, VisualKeycap>& lookup() {
        static const auto map = [] {
            std::unordered_map<PhysicalKey, VisualKeycap> m;
            for (const auto& c : keycaps()) m[c.key] = c;
            return m;
        }();
        return map;
    }
};
