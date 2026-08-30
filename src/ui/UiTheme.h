#pragma once
#include <lvgl.h>

namespace elixir::ui::theme {

// Minimalist Modern Yachting Color Tokens
inline lv_color_t background() { return lv_color_hex(0x0A0E14); }
inline lv_color_t backgroundDeep() { return lv_color_hex(0x05080C); }
inline lv_color_t surface() { return lv_color_hex(0x10161F); }
inline lv_color_t surfaceRaised() { return lv_color_hex(0x161F2B); }
inline lv_color_t surfaceSoft() { return lv_color_hex(0x121A24); }
inline lv_color_t border() { return lv_color_hex(0x202B3B); }
inline lv_color_t borderLight() { return lv_color_hex(0x2E3D52); }
inline lv_color_t text() { return lv_color_hex(0xFFFFFF); }
inline lv_color_t muted() { return lv_color_hex(0x8E9AA8); }

inline lv_color_t champagne() { return lv_color_hex(0xFFD166); }
inline lv_color_t iceBlue() { return lv_color_hex(0x5AC8FA); }
inline lv_color_t cyan() { return lv_color_hex(0x40C4FF); }
inline lv_color_t mint() { return lv_color_hex(0x30D158); }
inline lv_color_t lime() { return lv_color_hex(0x30D158); }
inline lv_color_t amber() { return lv_color_hex(0xFF9F0A); }
inline lv_color_t coral() { return lv_color_hex(0xFF453A); }
inline lv_color_t red() { return lv_color_hex(0xFF453A); }
inline lv_color_t purple() { return lv_color_hex(0xBF5AF2); }

// Layout & Style helpers
void makeNonScrollable(lv_obj_t* object);
void styleScreen(lv_obj_t* object);
void stylePanel(lv_obj_t* object, bool raised = false, int radius = 20);
void styleGlassCard(lv_obj_t* object, int radius = 20, bool raised = false);
void styleInset(lv_obj_t* object, int radius = 12);
void styleCaption(lv_obj_t* label);
void stylePill(lv_obj_t* object, lv_color_t accent, bool filled = false);
void styleButton(lv_obj_t* button, bool active = false);
void styleFloatingDock(lv_obj_t* dock);
void styleDockButton(lv_obj_t* button, bool active);

}  // namespace elixir::ui::theme
