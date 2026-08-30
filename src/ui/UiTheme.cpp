#include "ui/UiTheme.h"

namespace elixir::ui::theme {

void makeNonScrollable(lv_obj_t* object) {
    if (object == nullptr) return;
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(object, LV_SCROLLBAR_MODE_OFF);
}

void styleScreen(lv_obj_t* object) {
    if (object == nullptr) return;
    lv_obj_set_style_bg_color(object, background(), 0);
    lv_obj_set_style_bg_grad_color(object, backgroundDeep(), 0);
    lv_obj_set_style_bg_grad_dir(object, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    makeNonScrollable(object);
}

void styleGlassCard(lv_obj_t* object, int radius, bool raised) {
    if (object == nullptr) return;
    lv_obj_set_style_bg_color(object, raised ? surfaceRaised() : surface(), 0);
    lv_obj_set_style_bg_grad_color(object, raised ? surface() : surfaceSoft(), 0);
    lv_obj_set_style_bg_grad_dir(object, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_90, 0);
    lv_obj_set_style_border_color(object, raised ? borderLight() : border(), 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_opa(object, raised ? LV_OPA_80 : LV_OPA_60, 0);
    lv_obj_set_style_radius(object, radius, 0);
    lv_obj_set_style_shadow_color(object, backgroundDeep(), 0);
    lv_obj_set_style_shadow_width(object, raised ? 16 : 8, 0);
    lv_obj_set_style_shadow_opa(object, raised ? LV_OPA_60 : LV_OPA_40, 0);
    lv_obj_set_style_shadow_ofs_y(object, raised ? 4 : 2, 0);
    lv_obj_set_style_pad_all(object, 12, 0);
    makeNonScrollable(object);
}

void stylePanel(lv_obj_t* object, bool raised, int radius) {
    styleGlassCard(object, radius, raised);
}

void styleInset(lv_obj_t* object, int radius) {
    if (object == nullptr) return;
    lv_obj_set_style_bg_color(object, backgroundDeep(), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_60, 0);
    lv_obj_set_style_border_color(object, border(), 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_opa(object, LV_OPA_60, 0);
    lv_obj_set_style_radius(object, radius, 0);
    lv_obj_set_style_pad_all(object, 8, 0);
    makeNonScrollable(object);
}

void styleCaption(lv_obj_t* label) {
    if (label == nullptr) return;
    lv_obj_set_style_text_color(label, muted(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
}

void stylePill(lv_obj_t* object, lv_color_t accent, bool filled) {
    if (object == nullptr) return;
    lv_obj_set_style_bg_color(object, filled ? accent : surfaceSoft(), 0);
    lv_obj_set_style_bg_opa(object, filled ? LV_OPA_30 : LV_OPA_80, 0);
    lv_obj_set_style_border_color(object, accent, 0);
    lv_obj_set_style_border_opa(object, LV_OPA_60, 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_hor(object, 10, 0);
    lv_obj_set_style_pad_ver(object, 4, 0);
    makeNonScrollable(object);
}

void styleButton(lv_obj_t* button, bool active) {
    if (button == nullptr) return;
    lv_obj_set_style_bg_color(button, active ? champagne() : surfaceRaised(), 0);
    lv_obj_set_style_bg_opa(button, active ? LV_OPA_20 : LV_OPA_80, 0);
    lv_obj_set_style_border_color(button, active ? champagne() : border(), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_opa(button, active ? LV_OPA_80 : LV_OPA_50, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, active ? 10 : 0, 0);
    lv_obj_set_style_shadow_color(button, champagne(), 0);
    lv_obj_set_style_shadow_opa(button, active ? LV_OPA_20 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(button, champagne(), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_30, LV_STATE_PRESSED);
    makeNonScrollable(button);
}

void styleFloatingDock(lv_obj_t* dock) {
    if (dock == nullptr) return;
    lv_obj_set_style_bg_color(dock, surface(), 0);
    lv_obj_set_style_bg_grad_color(dock, surfaceSoft(), 0);
    lv_obj_set_style_bg_grad_dir(dock, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(dock, LV_OPA_90, 0);
    lv_obj_set_style_border_color(dock, borderLight(), 0);
    lv_obj_set_style_border_width(dock, 1, 0);
    lv_obj_set_style_border_opa(dock, LV_OPA_80, 0);
    lv_obj_set_style_radius(dock, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_color(dock, backgroundDeep(), 0);
    lv_obj_set_style_shadow_width(dock, 18, 0);
    lv_obj_set_style_shadow_opa(dock, LV_OPA_70, 0);
    lv_obj_set_style_shadow_ofs_y(dock, 4, 0);
    lv_obj_set_style_pad_all(dock, 4, 0);
    makeNonScrollable(dock);
}

void styleDockButton(lv_obj_t* button, bool active) {
    if (button == nullptr) return;
    lv_obj_set_style_bg_color(button, active ? champagne() : surfaceRaised(), 0);
    lv_obj_set_style_bg_opa(button, active ? LV_OPA_20 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(button, active ? champagne() : border(), 0);
    lv_obj_set_style_border_width(button, active ? 1 : 0, 0);
    lv_obj_set_style_border_opa(button, active ? LV_OPA_90 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(button, active ? 12 : 0, 0);
    lv_obj_set_style_shadow_color(button, champagne(), 0);
    lv_obj_set_style_shadow_opa(button, active ? LV_OPA_30 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(button, champagne(), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_30, LV_STATE_PRESSED);
    makeNonScrollable(button);
}

}  // namespace elixir::ui::theme
