#pragma once

#include <cstdint>
#include <cstddef>

struct lv_obj_t { void* user_data{}; };
struct lv_event_t { lv_obj_t* target{}; void* user_data{}; };
struct lv_chart_series_t {};
struct lv_meter_scale_t {};
struct lv_meter_indicator_t {};
struct lv_font_t {};
struct lv_color_t { std::uint32_t full{}; };
using lv_chart_axis_t = int;
using lv_coord_t = int;

inline lv_font_t lv_font_montserrat_12{};
inline lv_font_t lv_font_montserrat_14{};
inline lv_font_t lv_font_montserrat_16{};
inline lv_font_t lv_font_montserrat_18{};
inline lv_font_t lv_font_montserrat_22{};
inline lv_font_t lv_font_montserrat_24{};
inline lv_font_t lv_font_montserrat_26{};
inline lv_font_t lv_font_montserrat_28{};
inline lv_font_t lv_font_montserrat_30{};
inline lv_font_t lv_font_montserrat_34{};
inline lv_font_t lv_font_montserrat_42{};
inline lv_font_t lv_font_montserrat_48{};

constexpr int LV_ALIGN_CENTER = 0;
constexpr int LV_ANIM_OFF = 0;
constexpr int LV_BORDER_SIDE_BOTTOM = 1;
constexpr int LV_BORDER_SIDE_TOP = 2;
constexpr int LV_CHART_AXIS_PRIMARY_Y = 0;
constexpr int LV_CHART_AXIS_SECONDARY_Y = 1;
constexpr int LV_CHART_POINT_NONE = -32768;
constexpr int LV_CHART_TYPE_LINE = 0;
constexpr int LV_CHART_UPDATE_MODE_SHIFT = 0;
constexpr int LV_EVENT_CLICKED = 1;
constexpr int LV_EVENT_PRESSED = 2;
constexpr int LV_EVENT_RELEASED = 3;
constexpr int LV_GRAD_DIR_VER = 0;
constexpr int LV_LABEL_LONG_DOT = 0;
constexpr int LV_LABEL_LONG_WRAP = 1;
constexpr int LV_OBJ_FLAG_CLICKABLE = 1 << 0;
constexpr int LV_OBJ_FLAG_HIDDEN = 1 << 1;
constexpr int LV_OBJ_FLAG_SCROLLABLE = 1 << 2;
constexpr std::uint8_t LV_OPA_TRANSP = 0;
constexpr std::uint8_t LV_OPA_0 = 0;
constexpr std::uint8_t LV_OPA_10 = 26;
constexpr std::uint8_t LV_OPA_20 = 51;
constexpr std::uint8_t LV_OPA_30 = 77;
constexpr std::uint8_t LV_OPA_40 = 102;
constexpr std::uint8_t LV_OPA_50 = 128;
constexpr std::uint8_t LV_OPA_60 = 153;
constexpr std::uint8_t LV_OPA_70 = 179;
constexpr std::uint8_t LV_OPA_80 = 204;
constexpr std::uint8_t LV_OPA_90 = 230;
constexpr std::uint8_t LV_OPA_100 = 255;
constexpr std::uint8_t LV_OPA_COVER = 255;
constexpr int LV_PART_INDICATOR = 1;
constexpr int LV_PART_ITEMS = 2;
constexpr int LV_PART_KNOB = 3;
constexpr int LV_PART_MAIN = 0;
constexpr int LV_RADIUS_CIRCLE = 32767;
constexpr int LV_SCROLLBAR_MODE_OFF = 0;
constexpr int LV_STATE_PRESSED = 1;
constexpr int LV_TEXT_ALIGN_CENTER = 0;
constexpr int LV_TEXT_ALIGN_RIGHT = 1;

inline lv_color_t lv_color_hex(std::uint32_t value) { return {value}; }
inline lv_obj_t* lv_scr_act() { static lv_obj_t object; return &object; }
inline lv_obj_t* new_object() { return new lv_obj_t{}; }
inline lv_obj_t* lv_obj_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_btn_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_label_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_chart_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_meter_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_arc_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_bar_create(lv_obj_t*) { return new_object(); }
inline lv_obj_t* lv_slider_create(lv_obj_t*) { return new_object(); }
inline lv_chart_series_t* lv_chart_add_series(lv_obj_t*, lv_color_t, lv_chart_axis_t) {
    return new lv_chart_series_t{};
}
inline lv_meter_scale_t* lv_meter_add_scale(lv_obj_t*) { return new lv_meter_scale_t{}; }
inline lv_meter_indicator_t* lv_meter_add_needle_line(lv_obj_t*, lv_meter_scale_t*, int,
                                                       lv_color_t, int) {
    return new lv_meter_indicator_t{};
}
inline lv_obj_t* lv_event_get_target(lv_event_t* event) { return event ? event->target : nullptr; }
inline void* lv_event_get_user_data(lv_event_t* event) { return event ? event->user_data : nullptr; }
inline void lv_obj_set_user_data(lv_obj_t* object, void* data) { if (object) object->user_data = data; }
inline void* lv_obj_get_user_data(lv_obj_t* object) { return object ? object->user_data : nullptr; }
inline lv_obj_t* lv_obj_get_child(lv_obj_t*, int) { return new_object(); }
inline bool lv_obj_has_state(lv_obj_t*, int) { return false; }
inline int lv_slider_get_value(lv_obj_t*) { return 500; }

using lv_event_cb_t = void (*)(lv_event_t*);
inline void lv_obj_add_event_cb(lv_obj_t*, lv_event_cb_t, int, void*) {}

#define LV_VOID_FN(name) template <typename... Args> inline void name(Args&&...) {}
LV_VOID_FN(lv_arc_set_bg_angles)
LV_VOID_FN(lv_arc_set_range)
LV_VOID_FN(lv_arc_set_rotation)
LV_VOID_FN(lv_arc_set_value)
LV_VOID_FN(lv_bar_set_range)
LV_VOID_FN(lv_bar_set_value)
LV_VOID_FN(lv_chart_refresh)
LV_VOID_FN(lv_chart_set_all_value)
LV_VOID_FN(lv_chart_set_div_line_count)
LV_VOID_FN(lv_chart_set_next_value)
LV_VOID_FN(lv_chart_set_point_count)
LV_VOID_FN(lv_chart_set_range)
LV_VOID_FN(lv_chart_set_type)
LV_VOID_FN(lv_chart_set_update_mode)
LV_VOID_FN(lv_label_set_long_mode)
LV_VOID_FN(lv_label_set_text)
LV_VOID_FN(lv_label_set_text_fmt)
LV_VOID_FN(lv_meter_set_indicator_value)
LV_VOID_FN(lv_meter_set_scale_major_ticks)
LV_VOID_FN(lv_meter_set_scale_range)
LV_VOID_FN(lv_meter_set_scale_ticks)
LV_VOID_FN(lv_obj_add_flag)
LV_VOID_FN(lv_obj_align)
LV_VOID_FN(lv_obj_center)
LV_VOID_FN(lv_obj_clear_flag)
LV_VOID_FN(lv_obj_move_foreground)
LV_VOID_FN(lv_obj_remove_style)
LV_VOID_FN(lv_obj_set_pos)
LV_VOID_FN(lv_obj_set_scrollbar_mode)
LV_VOID_FN(lv_obj_set_size)
LV_VOID_FN(lv_obj_set_style_arc_color)
LV_VOID_FN(lv_obj_set_style_arc_rounded)
LV_VOID_FN(lv_obj_set_style_arc_width)
LV_VOID_FN(lv_obj_set_style_bg_color)
LV_VOID_FN(lv_obj_set_style_bg_grad_color)
LV_VOID_FN(lv_obj_set_style_bg_grad_dir)
LV_VOID_FN(lv_obj_set_style_bg_opa)
LV_VOID_FN(lv_obj_set_style_border_color)
LV_VOID_FN(lv_obj_set_style_border_opa)
LV_VOID_FN(lv_obj_set_style_border_side)
LV_VOID_FN(lv_obj_set_style_border_width)
LV_VOID_FN(lv_obj_set_style_line_color)
LV_VOID_FN(lv_obj_set_style_line_opa)
LV_VOID_FN(lv_obj_set_style_line_rounded)
LV_VOID_FN(lv_obj_set_style_line_width)
LV_VOID_FN(lv_obj_set_style_pad_all)
LV_VOID_FN(lv_obj_set_style_pad_hor)
LV_VOID_FN(lv_obj_set_style_pad_ver)
LV_VOID_FN(lv_obj_set_style_radius)
LV_VOID_FN(lv_obj_set_style_shadow_color)
LV_VOID_FN(lv_obj_set_style_shadow_ofs_y)
LV_VOID_FN(lv_obj_set_style_shadow_opa)
LV_VOID_FN(lv_obj_set_style_shadow_width)
LV_VOID_FN(lv_obj_set_style_size)
LV_VOID_FN(lv_obj_set_style_text_align)
LV_VOID_FN(lv_obj_set_style_text_color)
LV_VOID_FN(lv_obj_set_style_text_font)
LV_VOID_FN(lv_obj_set_style_text_letter_space)
LV_VOID_FN(lv_obj_set_width)
LV_VOID_FN(lv_slider_set_range)
LV_VOID_FN(lv_slider_set_value)
#undef LV_VOID_FN
