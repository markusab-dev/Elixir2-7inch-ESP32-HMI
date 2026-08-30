#include "ui/ElixirUI.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "AppConfig.h"
#include "ui/UiTheme.h"

namespace elixir::ui {
namespace {

constexpr int kTopHeight = 46;
constexpr int kBottomHeight = 58;
constexpr int kContentHeight = 480 - kTopHeight - kBottomHeight;
constexpr int kContentY = kTopHeight;

constexpr uint8_t kVisualBrightnessPercent[] = {100, 45, 20, 8};
constexpr uint8_t kVisualDimOpacity[] = {
    LV_OPA_TRANSP, LV_OPA_50, LV_OPA_70, LV_OPA_80};
constexpr std::size_t kVisualBrightnessCount =
    sizeof(kVisualBrightnessPercent) / sizeof(kVisualBrightnessPercent[0]);

const char* sourceKindText(DataSourceKind kind) {
    switch (kind) {
        case DataSourceKind::Synthetic: return "SYNTHETIC";
        case DataSourceKind::Replay: return "RECORDED";
        case DataSourceKind::Live: return "LIVE N2K";
    }
    return "UNKNOWN";
}

const char* navText(Page page) {
    switch (page) {
        case Page::Overview: return "OVERVIEW";
        case Page::Sail: return "SAIL";
        case Page::Power: return "POWER";
        case Page::Engine: return "ENGINE";
        case Page::Tactical: return "TACTICAL";
        case Page::System: return "SYSTEM";
        case Page::Count: break;
    }
    return "";
}

void setHidden(lv_obj_t* object, bool hidden) {
    if (object == nullptr) return;
    if (hidden) lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
}

void setDot(lv_obj_t* dot, bool online) {
    if (dot == nullptr) return;
    const lv_color_t color = online ? theme::mint() : theme::muted();
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_shadow_color(dot, color, 0);
    lv_obj_set_style_shadow_opa(dot, online ? LV_OPA_60 : LV_OPA_TRANSP, 0);
}

int chartValue(float value, float multiplier = 1.0F) {
    return std::isfinite(value) ? static_cast<int>(std::lround(value * multiplier))
                                : LV_CHART_POINT_NONE;
}

lv_color_t altColor(float temp_c) {
    if (temp_c < 80.0F) return theme::mint();
    if (temp_c <= 100.0F) return theme::amber();
    return theme::coral();
}

lv_color_t engineColor(float temp_c) {
    if (temp_c < 85.0F) return theme::mint();
    if (temp_c <= 92.0F) return theme::amber();
    return theme::coral();
}

lv_color_t depthColor(float depth_keel_m) {
    if (depth_keel_m < 2.5F) return theme::coral();
    return theme::iceBlue();
}

}  // namespace

void ElixirUI::begin() {
    root_ = lv_scr_act();
    theme::styleScreen(root_);

    // Subtle background ambient glows for luxury glassmorphism depth
    lv_obj_t* ambient_top = lv_obj_create(root_);
    lv_obj_set_pos(ambient_top, 250, -180);
    lv_obj_set_size(ambient_top, 300, 300);
    lv_obj_set_style_radius(ambient_top, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ambient_top, theme::champagne(), 0);
    lv_obj_set_style_bg_opa(ambient_top, LV_OPA_10, 0);
    lv_obj_set_style_border_width(ambient_top, 0, 0);
    lv_obj_clear_flag(ambient_top, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* ambient_bot = lv_obj_create(root_);
    lv_obj_set_pos(ambient_bot, -100, 300);
    lv_obj_set_size(ambient_bot, 350, 350);
    lv_obj_set_style_radius(ambient_bot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ambient_bot, theme::iceBlue(), 0);
    lv_obj_set_style_bg_opa(ambient_bot, LV_OPA_10, 0);
    lv_obj_set_style_border_width(ambient_bot, 0, 0);
    lv_obj_clear_flag(ambient_bot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    buildChrome();
    buildOverviewPage();
    buildSailPage();
    buildPowerPage();
    buildEnginePage();
    buildTacticalPage();
    buildSystemPage();

    // Overlays for night filter, dimming and wake-from-sleep
    night_overlay_ = lv_obj_create(root_);
    lv_obj_set_pos(night_overlay_, 0, 0);
    lv_obj_set_size(night_overlay_, 800, 480);
    lv_obj_set_style_bg_color(night_overlay_, lv_color_hex(0x551100), 0);
    lv_obj_set_style_bg_opa(night_overlay_, LV_OPA_30, 0);
    lv_obj_set_style_border_width(night_overlay_, 0, 0);
    lv_obj_clear_flag(night_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(night_overlay_, LV_OBJ_FLAG_HIDDEN);

    dim_overlay_ = lv_obj_create(root_);
    lv_obj_set_pos(dim_overlay_, 0, 0);
    lv_obj_set_size(dim_overlay_, 800, 480);
    lv_obj_set_style_bg_color(dim_overlay_, theme::backgroundDeep(), 0);
    lv_obj_set_style_bg_opa(dim_overlay_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dim_overlay_, 0, 0);
    lv_obj_clear_flag(dim_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dim_overlay_, LV_OBJ_FLAG_HIDDEN);

    wake_overlay_ = lv_obj_create(root_);
    lv_obj_set_pos(wake_overlay_, 0, 0);
    lv_obj_set_size(wake_overlay_, 800, 480);
    lv_obj_set_style_bg_color(wake_overlay_, theme::backgroundDeep(), 0);
    lv_obj_set_style_bg_opa(wake_overlay_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wake_overlay_, 0, 0);
    lv_obj_clear_flag(wake_overlay_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(wake_overlay_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wake_overlay_, wakeEvent, LV_EVENT_PRESSED, this);
    lv_obj_add_flag(wake_overlay_, LV_OBJ_FLAG_HIDDEN);

    showPage(Page::Overview);
}

void ElixirUI::buildChrome() {
    top_bar_ = lv_obj_create(root_);
    lv_obj_set_pos(top_bar_, 0, 0);
    lv_obj_set_size(top_bar_, 800, kTopHeight);
    lv_obj_set_style_bg_color(top_bar_, theme::backgroundDeep(), 0);
    lv_obj_set_style_bg_opa(top_bar_, LV_OPA_60, 0);
    lv_obj_set_style_border_side(top_bar_, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(top_bar_, theme::border(), 0);
    lv_obj_set_style_border_width(top_bar_, 1, 0);
    lv_obj_set_style_pad_all(top_bar_, 0, 0);
    theme::makeNonScrollable(top_bar_);

    clock_label_ = lv_label_create(top_bar_);
    lv_label_set_text(clock_label_, "20:48");
    lv_obj_set_pos(clock_label_, 20, 11);
    lv_obj_set_style_text_color(clock_label_, theme::text(), 0);
    lv_obj_set_style_text_font(clock_label_, &lv_font_montserrat_18, 0);

    gnss_label_ = lv_label_create(top_bar_);
    lv_label_set_text(gnss_label_, "GNSS 3D");
    lv_obj_set_pos(gnss_label_, 90, 15);
    theme::styleCaption(gnss_label_);
    lv_obj_set_style_text_color(gnss_label_, theme::mint(), 0);

    lv_obj_t* brand = lv_label_create(top_bar_);
    lv_label_set_text(brand, "ELIXIR2");
    lv_obj_set_pos(brand, 350, 10);
    lv_obj_set_style_text_color(brand, theme::text(), 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_letter_space(brand, 3, 0);

    lv_obj_t* source_pill = lv_obj_create(top_bar_);
    lv_obj_set_pos(source_pill, 500, 9);
    lv_obj_set_size(source_pill, 140, 28);
    theme::stylePill(source_pill, theme::champagne(), false);
    lv_obj_set_style_pad_all(source_pill, 0, 0);

    source_label_ = lv_label_create(source_pill);
    lv_label_set_text(source_label_, "RECORDED 5.0x");
    lv_obj_center(source_label_);
    lv_obj_set_style_text_color(source_label_, theme::champagne(), 0);
    lv_obj_set_style_text_font(source_label_, &lv_font_montserrat_12, 0);

    auto makeStatusDot = [&](int x, const char* name, lv_obj_t** dot_out) {
        lv_obj_t* dot = lv_obj_create(top_bar_);
        lv_obj_set_pos(dot, x, 18);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, theme::muted(), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_shadow_width(dot, 6, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* label = lv_label_create(top_bar_);
        lv_label_set_text(label, name);
        lv_obj_set_pos(label, x + 12, 14);
        theme::styleCaption(label);
        *dot_out = dot;
    };
    makeStatusDot(660, "N2K", &can_dot_);
    makeStatusDot(710, "WIFI", &wifi_dot_);
    makeStatusDot(760, "SD", &sd_dot_);

    // 6-Tab Centered Floating Bottom Dock
    floating_dock_ = lv_obj_create(root_);
    lv_obj_set_pos(floating_dock_, 40, 420);
    lv_obj_set_size(floating_dock_, 720, 52);
    theme::styleFloatingDock(floating_dock_);

    constexpr int kDockBtnWidth = 112;
    for (std::size_t i = 0; i < static_cast<std::size_t>(Page::Count); ++i) {
        const Page page = static_cast<Page>(i);
        lv_obj_t* btn = lv_btn_create(floating_dock_);
        lv_obj_set_pos(btn, 6 + static_cast<int>(i) * (kDockBtnWidth + 6), 2);
        lv_obj_set_size(btn, kDockBtnWidth, 40);
        theme::styleDockButton(btn, page == Page::Overview);
        lv_obj_set_user_data(btn, this);
        lv_obj_add_event_cb(btn, navEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(page)));

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, navText(page));
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(label, page == Page::Overview ? theme::champagne() : theme::muted(), 0);
        nav_buttons_[i] = btn;
    }
}

lv_obj_t* ElixirUI::createPage() {
    lv_obj_t* page = lv_obj_create(root_);
    lv_obj_set_pos(page, 0, kContentY);
    lv_obj_set_size(page, 800, kContentHeight);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    theme::makeNonScrollable(page);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    return page;
}

lv_obj_t* ElixirUI::createGlassCard(lv_obj_t* parent, int x, int y, int w, int h,
                                    bool raised, int radius) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    theme::styleGlassCard(card, radius, raised);
    return card;
}

lv_obj_t* ElixirUI::createCaption(lv_obj_t* parent, const char* text, int x, int y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    theme::styleCaption(label);
    return label;
}

lv_obj_t* ElixirUI::createValue(lv_obj_t* parent, int x, int y,
                                const lv_font_t* font, lv_color_t color) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "--");
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_letter_space(label, -1, 0);
    return label;
}

lv_obj_t* ElixirUI::createUnit(lv_obj_t* parent, const char* text, int x, int y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, theme::muted(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    return label;
}

lv_obj_t* ElixirUI::createActionButton(lv_obj_t* parent, const char* text,
                                       int x, int y, int w, int h,
                                       uint32_t command) {
    lv_obj_t* button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, h);
    theme::styleButton(button, false);
    lv_obj_set_user_data(button, this);
    lv_obj_add_event_cb(button, commandEvent, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(command)));

    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, theme::text(), 0);
    return button;
}

lv_chart_series_t* ElixirUI::addSeries(lv_obj_t* chart, lv_color_t color,
                                       lv_chart_axis_t axis) {
    lv_chart_series_t* series = lv_chart_add_series(chart, color, axis);
    lv_chart_set_all_value(chart, series, LV_CHART_POINT_NONE);
    return series;
}

// -----------------------------------------------------------------------------
// Page 0: OVERVIEW (Master Dashboard / Båten i ett ögonkast)
// -----------------------------------------------------------------------------
void ElixirUI::buildOverviewPage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::Overview)] = page;

    // Left Card: Sailing & Nav
    lv_obj_t* card_left = createGlassCard(page, 14, 6, 244, 356, false, 20);
    createCaption(card_left, "SAILING & NAV", 2, 0);

    lv_obj_t* sog_box = lv_obj_create(card_left);
    lv_obj_set_pos(sog_box, 0, 24);
    lv_obj_set_size(sog_box, 220, 100);
    theme::styleInset(sog_box, 14);
    createCaption(sog_box, "SPEED OVER GROUND", 6, 6);
    ov_sog_ = createValue(sog_box, 6, 24, &lv_font_montserrat_34, theme::text());
    createUnit(sog_box, "kn", 120, 34);
    ov_stw_ = lv_label_create(sog_box);
    lv_label_set_text(ov_stw_, "STW 7.56 kn (x1.055)");
    lv_obj_set_pos(ov_stw_, 6, 70);
    lv_obj_set_style_text_color(ov_stw_, theme::iceBlue(), 0);
    lv_obj_set_style_text_font(ov_stw_, &lv_font_montserrat_12, 0);

    lv_obj_t* depth_box = lv_obj_create(card_left);
    lv_obj_set_pos(depth_box, 0, 130);
    lv_obj_set_size(depth_box, 220, 96);
    theme::styleInset(depth_box, 14);
    createCaption(depth_box, "DEPTH BELOW KEEL", 6, 6);
    ov_depth_ = createValue(depth_box, 6, 24, &lv_font_montserrat_28, theme::iceBlue());
    createUnit(depth_box, "m", 90, 32);
    ov_heading_ = lv_label_create(depth_box);
    lv_label_set_text(ov_heading_, "HDG 214° • COG 212°");
    lv_obj_set_pos(ov_heading_, 6, 64);
    lv_obj_set_style_text_color(ov_heading_, theme::champagne(), 0);
    lv_obj_set_style_text_font(ov_heading_, &lv_font_montserrat_12, 0);

    lv_obj_t* wind_box = lv_obj_create(card_left);
    lv_obj_set_pos(wind_box, 0, 232);
    lv_obj_set_size(wind_box, 220, 96);
    theme::styleInset(wind_box, 14);
    createCaption(wind_box, "WIND TELEMETRY (gWind)", 6, 6);
    ov_wind_badge_ = lv_label_create(wind_box);
    lv_label_set_text(ov_wind_badge_, "AWA 42° S • 14.8 kn\nTWA 48° • TWS 13.4 kn\nVMG +5.82 kn");
    lv_obj_set_pos(ov_wind_badge_, 6, 24);
    lv_obj_set_style_text_font(ov_wind_badge_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ov_wind_badge_, theme::iceBlue(), 0);

    // Center Card: Energy & Storage (Hero)
    lv_obj_t* card_center = createGlassCard(page, 268, 6, 264, 356, true, 20);
    createCaption(card_center, "HOUSE ENERGY STORAGE", 2, 0);

    ov_soc_arc_ = lv_arc_create(card_center);
    lv_obj_set_pos(ov_soc_arc_, 14, 26);
    lv_obj_set_size(ov_soc_arc_, 110, 110);
    lv_arc_set_rotation(ov_soc_arc_, 135);
    lv_arc_set_bg_angles(ov_soc_arc_, 0, 270);
    lv_arc_set_range(ov_soc_arc_, 0, 100);
    lv_arc_set_value(ov_soc_arc_, 92);
    lv_obj_remove_style(ov_soc_arc_, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(ov_soc_arc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(ov_soc_arc_, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ov_soc_arc_, theme::border(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ov_soc_arc_, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ov_soc_arc_, theme::champagne(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ov_soc_arc_, true, LV_PART_INDICATOR);

    ov_soc_val_ = lv_label_create(ov_soc_arc_);
    lv_label_set_text(ov_soc_val_, "92%");
    lv_obj_center(ov_soc_val_);
    lv_obj_set_style_text_font(ov_soc_val_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ov_soc_val_, theme::champagne(), 0);

    ov_soc_sub_ = lv_label_create(card_center);
    lv_label_set_text(ov_soc_sub_, "13.20 V\n+3.4 A\n18.4 h");
    lv_obj_set_pos(ov_soc_sub_, 140, 44);
    lv_obj_set_style_text_font(ov_soc_sub_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ov_soc_sub_, theme::text(), 0);

    lv_obj_t* sol_box = lv_obj_create(card_center);
    lv_obj_set_pos(sol_box, 0, 148);
    lv_obj_set_size(sol_box, 240, 86);
    theme::styleInset(sol_box, 14);
    createCaption(sol_box, "SMARTSOLAR MPPT", 6, 6);
    ov_solar_val_ = createValue(sol_box, 6, 24, &lv_font_montserrat_22, theme::amber());
    createUnit(sol_box, "W", 90, 28);
    lv_obj_t* sol_sub = lv_label_create(sol_box);
    lv_label_set_text(sol_sub, "286 Wh produced today");
    lv_obj_set_pos(sol_sub, 6, 56);
    lv_obj_set_style_text_font(sol_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sol_sub, theme::muted(), 0);

    lv_obj_t* alt_charge_box = lv_obj_create(card_center);
    lv_obj_set_pos(alt_charge_box, 0, 240);
    lv_obj_set_size(alt_charge_box, 240, 88);
    theme::styleInset(alt_charge_box, 14);
    createCaption(alt_charge_box, "GENERATOR RESIDUAL CHARGE", 6, 6);
    ov_alt_charge_val_ = createValue(alt_charge_box, 6, 24, &lv_font_montserrat_22, theme::mint());
    lv_obj_t* alt_sub = lv_label_create(alt_charge_box);
    lv_label_set_text(alt_sub, "Charging house bank");
    lv_obj_set_pos(alt_sub, 6, 56);
    lv_obj_set_style_text_font(alt_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(alt_sub, theme::muted(), 0);

    // Right Card: Machinery & Comfort
    lv_obj_t* card_right = createGlassCard(page, 542, 6, 244, 356, false, 20);
    createCaption(card_right, "MACHINERY & CLIMATE", 2, 0);

    lv_obj_t* eng_box = lv_obj_create(card_right);
    lv_obj_set_pos(eng_box, 0, 24);
    lv_obj_set_size(eng_box, 220, 96);
    theme::styleInset(eng_box, 14);
    createCaption(eng_box, "YANMAR 3YM30 BLOCK", 6, 6);
    ov_engine_val_ = createValue(eng_box, 6, 24, &lv_font_montserrat_24, theme::text());
    createUnit(eng_box, "°C", 90, 30);
    lv_obj_t* eng_sub = lv_label_create(eng_box);
    lv_label_set_text(eng_sub, "Thermostat 85°C • Limit 93°C");
    lv_obj_set_pos(eng_sub, 6, 64);
    lv_obj_set_style_text_font(eng_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(eng_sub, theme::muted(), 0);

    lv_obj_t* alt_box = lv_obj_create(card_right);
    lv_obj_set_pos(alt_box, 0, 126);
    lv_obj_set_size(alt_box, 220, 96);
    theme::styleInset(alt_box, 14);
    createCaption(alt_box, "ALTERNATOR TEMP", 6, 6);
    ov_alt_val_ = createValue(alt_box, 6, 24, &lv_font_montserrat_24, theme::mint());
    createUnit(alt_box, "°C", 90, 30);
    lv_obj_t* alt_sub2 = lv_label_create(alt_box);
    lv_label_set_text(alt_sub2, "Limits: 80°C warn / 100°C alert");
    lv_obj_set_pos(alt_sub2, 6, 64);
    lv_obj_set_style_text_font(alt_sub2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(alt_sub2, theme::muted(), 0);

    lv_obj_t* clim_box = lv_obj_create(card_right);
    lv_obj_set_pos(clim_box, 0, 228);
    lv_obj_set_size(clim_box, 220, 100);
    theme::styleInset(clim_box, 14);
    createCaption(clim_box, "ENVIRONMENT & TRIP LOG", 6, 6);
    ov_climate_val_ = lv_label_create(clim_box);
    lv_label_set_text(ov_climate_val_, "Water 18.4°C • EngRoom 46.6°C\nTrip 24.8 NM • Total 3840 NM");
    lv_obj_set_pos(ov_climate_val_, 6, 26);
    lv_obj_set_style_text_font(ov_climate_val_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ov_climate_val_, theme::iceBlue(), 0);
}

// -----------------------------------------------------------------------------
// Page 1: SAIL (Helmsman & Performance HUD)
// -----------------------------------------------------------------------------
void ElixirUI::buildSailPage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::Sail)] = page;

    // Left Card: Wind Rose & Vector HUD
    lv_obj_t* card_left = createGlassCard(page, 14, 6, 244, 356, false, 20);
    createCaption(card_left, "WIND VECTOR HUD (gWind)", 2, 0);

    lv_obj_t* awa_box = lv_obj_create(card_left);
    lv_obj_set_pos(awa_box, 0, 24);
    lv_obj_set_size(awa_box, 220, 148);
    theme::styleInset(awa_box, 14);
    createCaption(awa_box, "APPARENT WIND (AWA/AWS)", 6, 6);
    sail_awa_val_ = createValue(awa_box, 6, 24, &lv_font_montserrat_26, theme::iceBlue());
    createCaption(awa_box, "APPARENT SPEED", 6, 78);
    sail_aws_val_ = createValue(awa_box, 6, 96, &lv_font_montserrat_26, theme::text());
    createUnit(awa_box, "kn", 120, 106);

    lv_obj_t* twa_box = lv_obj_create(card_left);
    lv_obj_set_pos(twa_box, 0, 180);
    lv_obj_set_size(twa_box, 220, 148);
    theme::styleInset(twa_box, 14);
    createCaption(twa_box, "TRUE WIND (MCU COMPUTED)", 6, 6);
    sail_twa_val_ = createValue(twa_box, 6, 24, &lv_font_montserrat_22, theme::champagne());
    createCaption(twa_box, "TRUE SPEED & DIRECTION", 6, 74);
    sail_tws_val_ = createValue(twa_box, 6, 92, &lv_font_montserrat_16, theme::text());
    sail_vmg_val_ = lv_label_create(twa_box);
    lv_label_set_text(sail_vmg_val_, "VMG: +5.82 kn");
    lv_obj_set_pos(sail_vmg_val_, 6, 122);
    lv_obj_set_style_text_font(sail_vmg_val_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sail_vmg_val_, theme::mint(), 0);

    // Center Card: Primary Sailing HUD (Hero)
    lv_obj_t* card_center = createGlassCard(page, 268, 6, 264, 356, true, 20);
    createCaption(card_center, "SAILING HUD", 2, 0);

    createCaption(card_center, "SPEED OVER GROUND", 2, 26);
    sail_sog_ = createValue(card_center, 0, 44, &lv_font_montserrat_48, theme::text());
    createUnit(card_center, "kn", 188, 70);

    sail_stw_ = lv_label_create(card_center);
    lv_label_set_text(sail_stw_, "STW 7.56 kn (x1.055 calibrated)");
    lv_obj_set_pos(sail_stw_, 2, 108);
    lv_obj_set_style_text_color(sail_stw_, theme::iceBlue(), 0);
    lv_obj_set_style_text_font(sail_stw_, &lv_font_montserrat_12, 0);

    lv_obj_t* nav_box = lv_obj_create(card_center);
    lv_obj_set_pos(nav_box, 0, 134);
    lv_obj_set_size(nav_box, 240, 96);
    theme::styleInset(nav_box, 14);
    createCaption(nav_box, "HEADING (can0.1)", 6, 6);
    sail_heading_ = createValue(nav_box, 6, 24, &lv_font_montserrat_28, theme::champagne());
    sail_cog_ = lv_label_create(nav_box);
    lv_label_set_text(sail_cog_, "COG 212° (GPS)\nVar 6.2° E");
    lv_obj_set_pos(sail_cog_, 130, 24);
    lv_obj_set_style_text_font(sail_cog_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sail_cog_, theme::muted(), 0);

    lv_obj_t* depth_box = lv_obj_create(card_center);
    lv_obj_set_pos(depth_box, 0, 238);
    lv_obj_set_size(depth_box, 240, 90);
    theme::styleInset(depth_box, 14);
    createCaption(depth_box, "DEPTH BELOW KEEL (-1.4m)", 6, 6);
    sail_depth_ = createValue(depth_box, 6, 26, &lv_font_montserrat_28, theme::iceBlue());
    createUnit(depth_box, "m", 115, 34);

    // Right Card: Hull Attitude & Trim
    lv_obj_t* card_right = createGlassCard(page, 542, 6, 244, 356, false, 20);
    createCaption(card_right, "HULL ATTITUDE (DST can0.35)", 2, 0);

    lv_obj_t* roll_box = lv_obj_create(card_right);
    lv_obj_set_pos(roll_box, 0, 24);
    lv_obj_set_size(roll_box, 220, 148);
    theme::styleInset(roll_box, 14);
    createCaption(roll_box, "HEEL / ROLL ANGLE", 6, 6);
    sail_roll_val_ = createValue(roll_box, 6, 24, &lv_font_montserrat_30, theme::champagne());
    createCaption(roll_box, "PITCH / STAMPING", 6, 78);
    sail_pitch_val_ = createValue(roll_box, 6, 96, &lv_font_montserrat_24, theme::text());

    lv_obj_t* env_box = lv_obj_create(card_right);
    lv_obj_set_pos(env_box, 0, 180);
    lv_obj_set_size(env_box, 220, 148);
    theme::styleInset(env_box, 14);
    createCaption(env_box, "WATER TEMPERATURE", 6, 6);
    sail_water_val_ = createValue(env_box, 6, 24, &lv_font_montserrat_26, theme::text());
    createUnit(env_box, "°C", 110, 32);
    lv_obj_t* pol_lbl = lv_label_create(env_box);
    lv_label_set_text(pol_lbl, "Sun Fast 36 SRS-C\nTarget Polar: 98%\nOptimal Trim Range");
    lv_obj_set_pos(pol_lbl, 6, 80);
    lv_obj_set_style_text_font(pol_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(pol_lbl, theme::mint(), 0);
}

// -----------------------------------------------------------------------------
// Page 2: POWER (Victron Energy, Solar & Alternator Thermal)
// -----------------------------------------------------------------------------
void ElixirUI::buildPowerPage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::Power)] = page;

    // Battery Hero Card
    lv_obj_t* battery_card = createGlassCard(page, 14, 6, 376, 356, true, 20);
    createCaption(battery_card, "VICTRON ENERGY STORAGE (Venus MQTT)", 2, 0);

    power_soc_arc_ = lv_arc_create(battery_card);
    lv_obj_set_pos(power_soc_arc_, 14, 28);
    lv_obj_set_size(power_soc_arc_, 150, 150);
    lv_arc_set_rotation(power_soc_arc_, 135);
    lv_arc_set_bg_angles(power_soc_arc_, 0, 270);
    lv_arc_set_range(power_soc_arc_, 0, 100);
    lv_arc_set_value(power_soc_arc_, 92);
    lv_obj_remove_style(power_soc_arc_, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(power_soc_arc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(power_soc_arc_, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_color(power_soc_arc_, theme::border(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(power_soc_arc_, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(power_soc_arc_, theme::champagne(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(power_soc_arc_, true, LV_PART_INDICATOR);

    power_soc_val_ = lv_label_create(power_soc_arc_);
    lv_label_set_text(power_soc_val_, "92%");
    lv_obj_center(power_soc_val_);
    lv_obj_set_style_text_font(power_soc_val_, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(power_soc_val_, theme::champagne(), 0);

    createCaption(battery_card, "VOLTAGE", 185, 34);
    power_voltage_val_ = createValue(battery_card, 185, 50, &lv_font_montserrat_22, theme::text());

    createCaption(battery_card, "NET CURRENT", 185, 84);
    power_current_val_ = createValue(battery_card, 185, 100, &lv_font_montserrat_22, theme::mint());

    createCaption(battery_card, "NET POWER", 185, 134);
    power_power_val_ = createValue(battery_card, 185, 150, &lv_font_montserrat_22, theme::amber());

    lv_obj_t* autonomy_box = lv_obj_create(battery_card);
    lv_obj_set_pos(autonomy_box, 0, 196);
    lv_obj_set_size(autonomy_box, 352, 130);
    theme::styleInset(autonomy_box, 14);
    createCaption(autonomy_box, "ESTIMATED AUTONOMY", 10, 10);
    power_remaining_val_ = createValue(autonomy_box, 10, 32, &lv_font_montserrat_28, theme::champagne());
    createCaption(autonomy_box, "SMARTSHUNT STATUS", 10, 80);
    lv_obj_t* status_lbl = lv_label_create(autonomy_box);
    lv_label_set_text(status_lbl, "Venus MQTT Sync • 15m Staleness Window");
    lv_obj_set_pos(status_lbl, 10, 98);
    lv_obj_set_style_text_font(status_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(status_lbl, theme::muted(), 0);

    // Right Card: Solar & Alternator Thermal Management
    lv_obj_t* solar_card = createGlassCard(page, 402, 6, 384, 356, false, 20);
    createCaption(solar_card, "SMARTSOLAR MPPT & ALTERNATOR THERMAL", 2, 0);

    createCaption(solar_card, "SOLAR HARVEST", 12, 26);
    power_solar_val_ = createValue(solar_card, 12, 42, &lv_font_montserrat_22, theme::amber());
    createUnit(solar_card, "W", 90, 48);

    createCaption(solar_card, "TOTAL TODAY", 180, 26);
    power_solar_today_val_ = createValue(solar_card, 180, 42, &lv_font_montserrat_22, theme::champagne());

    lv_obj_t* alt_box = lv_obj_create(solar_card);
    lv_obj_set_pos(alt_box, 0, 86);
    lv_obj_set_size(alt_box, 360, 90);
    theme::styleInset(alt_box, 14);
    createCaption(alt_box, "ALTERNATOR TEMPERATURE & CHARGE", 6, 6);
    power_alt_temp_val_ = createValue(alt_box, 6, 24, &lv_font_montserrat_24, theme::mint());
    createUnit(alt_box, "°C", 90, 30);
    power_alt_status_val_ = lv_label_create(alt_box);
    lv_label_set_text(power_alt_status_val_, "CHARGING ~ 34.5 A • Headroom Nominal (<80°C)");
    lv_obj_set_pos(power_alt_status_val_, 6, 60);
    lv_obj_set_style_text_font(power_alt_status_val_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(power_alt_status_val_, theme::mint(), 0);

    power_chart_ = lv_chart_create(solar_card);
    lv_obj_set_pos(power_chart_, 0, 184);
    lv_obj_set_size(power_chart_, 360, 140);
    theme::styleInset(power_chart_, 14);
    lv_chart_set_type(power_chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(power_chart_, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(power_chart_, config::kHistoryPoints);
    lv_chart_set_range(power_chart_, LV_CHART_AXIS_PRIMARY_Y, 110, 150);
    lv_chart_set_range(power_chart_, LV_CHART_AXIS_SECONDARY_Y, 0, 450);
    lv_chart_set_div_line_count(power_chart_, 3, 0);
    lv_obj_set_style_line_width(power_chart_, 2, LV_PART_ITEMS);

    power_voltage_series_ = addSeries(power_chart_, theme::iceBlue(), LV_CHART_AXIS_PRIMARY_Y);
    power_solar_series_ = addSeries(power_chart_, theme::amber(), LV_CHART_AXIS_SECONDARY_Y);
}

// -----------------------------------------------------------------------------
// Page 3: ENGINE (Yanmar 3YM30 & Propulsion Deck)
// -----------------------------------------------------------------------------
void ElixirUI::buildEnginePage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::Engine)] = page;

    // Left Card: Yanmar Machinery
    lv_obj_t* card_left = createGlassCard(page, 14, 6, 376, 356, true, 20);
    createCaption(card_left, "YANMAR 3YM30 PROPULSION", 2, 0);

    lv_obj_t* block_box = lv_obj_create(card_left);
    lv_obj_set_pos(block_box, 0, 24);
    lv_obj_set_size(block_box, 352, 100);
    theme::styleInset(block_box, 14);
    createCaption(block_box, "ENGINE BLOCK (85°C Open / 93°C Alarm)", 6, 6);
    eng_temp_val_ = createValue(block_box, 6, 24, &lv_font_montserrat_30, theme::mint());
    createUnit(block_box, "°C", 110, 36);

    eng_temp_bar_ = lv_bar_create(block_box);
    lv_obj_set_pos(eng_temp_bar_, 6, 74);
    lv_obj_set_size(eng_temp_bar_, 320, 10);
    lv_bar_set_range(eng_temp_bar_, 0, 110);
    lv_obj_set_style_radius(eng_temp_bar_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(eng_temp_bar_, theme::border(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(eng_temp_bar_, theme::mint(), LV_PART_INDICATOR);

    lv_obj_t* alt_box = lv_obj_create(card_left);
    lv_obj_set_pos(alt_box, 0, 130);
    lv_obj_set_size(alt_box, 352, 100);
    theme::styleInset(alt_box, 14);
    createCaption(alt_box, "ALTERNATOR (80°C Warn / 100°C Red / 110°C Alarm)", 6, 6);
    eng_alt_val_ = createValue(alt_box, 6, 24, &lv_font_montserrat_28, theme::mint());
    createUnit(alt_box, "°C", 100, 34);

    eng_alt_bar_ = lv_bar_create(alt_box);
    lv_obj_set_pos(eng_alt_bar_, 6, 74);
    lv_obj_set_size(eng_alt_bar_, 320, 10);
    lv_bar_set_range(eng_alt_bar_, 0, 140);
    lv_obj_set_style_radius(eng_alt_bar_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(eng_alt_bar_, theme::border(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(eng_alt_bar_, theme::mint(), LV_PART_INDICATOR);

    lv_obj_t* room_box = lv_obj_create(card_left);
    lv_obj_set_pos(room_box, 0, 236);
    lv_obj_set_size(room_box, 352, 92);
    theme::styleInset(room_box, 14);
    createCaption(room_box, "ENGINEROOM AMBIENT", 6, 6);
    eng_room_val_ = createValue(room_box, 6, 24, &lv_font_montserrat_22, theme::text());
    createUnit(room_box, "°C", 90, 30);
    eng_state_badge_ = lv_label_create(room_box);
    lv_label_set_text(eng_state_badge_, "STATE: YANMAR RUNNING • GENERATOR ACTIVE");
    lv_obj_set_pos(eng_state_badge_, 6, 60);
    lv_obj_set_style_text_font(eng_state_badge_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(eng_state_badge_, theme::mint(), 0);

    // Right Card: Alternator Charge Output & Motoring Nav Safety Bar
    lv_obj_t* card_right = createGlassCard(page, 402, 6, 384, 356, false, 20);
    createCaption(card_right, "GENERATOR CHARGE OUTPUT & MOTORING NAV", 2, 0);

    lv_obj_t* chg_box = lv_obj_create(card_right);
    lv_obj_set_pos(chg_box, 0, 24);
    lv_obj_set_size(chg_box, 360, 96);
    theme::styleInset(chg_box, 14);
    createCaption(chg_box, "ALTERNATOR CHARGE OUTPUT", 6, 6);
    eng_charge_val_ = createValue(chg_box, 6, 24, &lv_font_montserrat_30, theme::mint());
    lv_obj_t* chg_sub = lv_label_create(chg_box);
    lv_label_set_text(chg_sub, "Residual estimate: battery.current - solar.current");
    lv_obj_set_pos(chg_sub, 6, 66);
    lv_obj_set_style_text_font(chg_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(chg_sub, theme::muted(), 0);

    lv_obj_t* nav_box = lv_obj_create(card_right);
    lv_obj_set_pos(nav_box, 0, 126);
    lv_obj_set_size(nav_box, 360, 80);
    theme::styleInset(nav_box, 14);
    createCaption(nav_box, "SOG", 10, 8);
    eng_sog_val_ = createValue(nav_box, 10, 28, &lv_font_montserrat_22, theme::text());
    createCaption(nav_box, "KEEL DEPTH", 130, 8);
    eng_depth_val_ = createValue(nav_box, 130, 28, &lv_font_montserrat_22, theme::iceBlue());
    createCaption(nav_box, "HEADING", 250, 8);
    eng_heading_val_ = createValue(nav_box, 250, 28, &lv_font_montserrat_22, theme::champagne());

    eng_chart_ = lv_chart_create(card_right);
    lv_obj_set_pos(eng_chart_, 0, 212);
    lv_obj_set_size(eng_chart_, 360, 116);
    theme::styleInset(eng_chart_, 14);
    lv_chart_set_type(eng_chart_, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(eng_chart_, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(eng_chart_, config::kHistoryPoints);
    lv_chart_set_range(eng_chart_, LV_CHART_AXIS_PRIMARY_Y, 0, 120);
    lv_chart_set_div_line_count(eng_chart_, 3, 0);
    lv_obj_set_style_line_width(eng_chart_, 2, LV_PART_ITEMS);

    eng_temp_series_ = addSeries(eng_chart_, theme::mint(), LV_CHART_AXIS_PRIMARY_Y);
    eng_alt_series_ = addSeries(eng_chart_, theme::amber(), LV_CHART_AXIS_PRIMARY_Y);
}

// -----------------------------------------------------------------------------
// Page 4: TACTICAL (AIS Targets & Anchor Watch)
// -----------------------------------------------------------------------------
void ElixirUI::buildTacticalPage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::Tactical)] = page;

    // Left Card: AIS Target Tracking
    lv_obj_t* ais_card = createGlassCard(page, 14, 6, 376, 356, true, 20);
    createCaption(ais_card, "AIS TARGET TRACKING (Plotter can0.43)", 2, 0);

    lv_obj_t* target_box = lv_obj_create(ais_card);
    lv_obj_set_pos(target_box, 0, 24);
    lv_obj_set_size(target_box, 352, 170);
    theme::styleInset(target_box, 14);
    createCaption(target_box, "CLOSEST AIS TARGET", 6, 6);
    ais_name_val_ = createValue(target_box, 6, 24, &lv_font_montserrat_24, theme::champagne());

    createCaption(target_box, "RANGE / BEARING", 6, 64);
    ais_range_val_ = createValue(target_box, 6, 80, &lv_font_montserrat_22, theme::text());

    createCaption(target_box, "TARGET SOG", 190, 64);
    ais_sog_val_ = createValue(target_box, 190, 80, &lv_font_montserrat_22, theme::iceBlue());

    createCaption(target_box, "CPA (CLOSEST PASSAGE)", 6, 116);
    ais_cpa_val_ = createValue(target_box, 6, 132, &lv_font_montserrat_22, theme::mint());

    createCaption(target_box, "TCPA (TIME TO CPA)", 190, 116);
    ais_tcpa_val_ = createValue(target_box, 190, 132, &lv_font_montserrat_22, theme::amber());

    lv_obj_t* count_box = lv_obj_create(ais_card);
    lv_obj_set_pos(count_box, 0, 204);
    lv_obj_set_size(count_box, 352, 120);
    theme::styleInset(count_box, 14);
    createCaption(count_box, "RECEIVER FLEET OVERVIEW", 6, 6);
    ais_count_val_ = createValue(count_box, 6, 26, &lv_font_montserrat_24, theme::iceBlue());
    lv_obj_t* fleet_sub = lv_label_create(count_box);
    lv_label_set_text(fleet_sub, "Class A & B AIS Decoded\nRange: 15+ NM • Active Fleet Radar");
    lv_obj_set_pos(fleet_sub, 6, 68);
    lv_obj_set_style_text_font(fleet_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(fleet_sub, theme::muted(), 0);

    // Right Card: Anchor Watch
    lv_obj_t* anchor_card = createGlassCard(page, 402, 6, 384, 356, false, 20);
    createCaption(anchor_card, "ANCHOR WATCH (signalk-anchoralarm)", 2, 0);

    lv_obj_t* anch_box = lv_obj_create(anchor_card);
    lv_obj_set_pos(anch_box, 0, 24);
    lv_obj_set_size(anch_box, 360, 170);
    theme::styleInset(anch_box, 14);
    createCaption(anch_box, "ANCHOR ALARM STATUS", 6, 6);
    anchor_status_val_ = createValue(anch_box, 6, 24, &lv_font_montserrat_24, theme::mint());

    createCaption(anch_box, "CURRENT DRIFT RADIUS", 6, 64);
    anchor_radius_val_ = createValue(anch_box, 6, 80, &lv_font_montserrat_22, theme::champagne());

    createCaption(anch_box, "MAX ALLOWED RADIUS", 190, 64);
    anchor_max_val_ = createValue(anch_box, 190, 80, &lv_font_montserrat_22, theme::text());

    createCaption(anch_box, "SWING DRIFT BEARING", 6, 116);
    anchor_bearing_val_ = createValue(anch_box, 6, 132, &lv_font_montserrat_22, theme::iceBlue());

    lv_obj_t* anch_info_box = lv_obj_create(anchor_card);
    lv_obj_set_pos(anch_info_box, 0, 204);
    lv_obj_set_size(anch_info_box, 360, 120);
    theme::styleInset(anch_info_box, 14);
    createCaption(anch_info_box, "ANCHOR WATCH TELEMETRY", 6, 6);
    lv_obj_t* anch_desc = lv_label_create(anch_info_box);
    lv_label_set_text(anch_desc, "GPS Geofence Guard Active\nMonitors True Wind shift & Swing Arc\nAudio/Visual Alert on Drag");
    lv_obj_set_pos(anch_desc, 6, 30);
    lv_obj_set_style_text_font(anch_desc, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(anch_desc, theme::muted(), 0);
}

// -----------------------------------------------------------------------------
// Page 5: SYSTEM (Trip Log, Calibration & Bus Diagnostics)
// -----------------------------------------------------------------------------
void ElixirUI::buildSystemPage() {
    lv_obj_t* page = createPage();
    pages_[static_cast<std::size_t>(Page::System)] = page;

    // Left Card: Trip Log & Transducer Calibration
    lv_obj_t* cal_card = createGlassCard(page, 14, 6, 400, 356, true, 20);
    createCaption(cal_card, "LOGBOOK & TRANSDUCER CALIBRATION", 2, 0);

    lv_obj_t* trip_box = lv_obj_create(cal_card);
    lv_obj_set_pos(trip_box, 0, 24);
    lv_obj_set_size(trip_box, 376, 96);
    theme::styleInset(trip_box, 14);
    createCaption(trip_box, "TRIP LOG (navigation.trip.log)", 6, 6);
    sys_trip_val_ = createValue(trip_box, 6, 24, &lv_font_montserrat_26, theme::champagne());
    createCaption(trip_box, "TOTAL ODOMETER", 190, 6);
    sys_total_val_ = createValue(trip_box, 190, 24, &lv_font_montserrat_22, theme::text());

    lv_obj_t* cal_box = lv_obj_create(cal_card);
    lv_obj_set_pos(cal_box, 0, 128);
    lv_obj_set_size(cal_box, 376, 96);
    theme::styleInset(cal_box, 14);
    createCaption(cal_box, "KEEL OFFSET (belowTransducer)", 6, 6);
    sys_keel_offset_val_ = createValue(cal_box, 6, 24, &lv_font_montserrat_22, theme::iceBlue());
    createCaption(cal_box, "STW CORRECTION FACTOR", 190, 6);
    sys_stw_factor_val_ = createValue(cal_box, 190, 24, &lv_font_montserrat_22, theme::mint());

    // Replay Controls in System Card
    mode_value_ = createValue(cal_card, 0, 232, &lv_font_montserrat_14, theme::champagne());
    replay_label_ = lv_label_create(cal_card);
    lv_label_set_text(replay_label_, "Signal K Replay Feed");
    lv_obj_set_pos(replay_label_, 0, 252);
    lv_obj_set_style_text_color(replay_label_, theme::muted(), 0);
    lv_obj_set_style_text_font(replay_label_, &lv_font_montserrat_12, 0);

    replay_slider_ = lv_slider_create(cal_card);
    lv_obj_set_pos(replay_slider_, 0, 274);
    lv_obj_set_size(replay_slider_, 376, 10);
    lv_slider_set_range(replay_slider_, 0, 1000);
    lv_obj_set_style_radius(replay_slider_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(replay_slider_, theme::border(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(replay_slider_, theme::champagne(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(replay_slider_, theme::champagne(), LV_PART_KNOB);
    lv_obj_set_style_radius(replay_slider_, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_user_data(replay_slider_, this);
    lv_obj_add_event_cb(replay_slider_, sliderEvent, LV_EVENT_RELEASED, nullptr);

    replay_progress_label_ = lv_label_create(cal_card);
    lv_label_set_text(replay_progress_label_, "00:00 / 00:00");
    lv_obj_set_pos(replay_progress_label_, 0, 292);
    theme::styleCaption(replay_progress_label_);

    auto* play = createActionButton(cal_card, "PAUSE", 0, 310, 116, 36, CommandToggleReplay);
    replay_play_button_label_ = lv_obj_get_child(play, 0);
    auto* speed = createActionButton(cal_card, "SPEED 5.0x", 126, 310, 116, 36, CommandCycleReplaySpeed);
    replay_speed_button_label_ = lv_obj_get_child(speed, 0);
    createActionButton(cal_card, "SOURCE", 252, 310, 116, 36, CommandToggleSource);

    // Right Card: N2K Bus Telemetry & Display Controls
    lv_obj_t* diag_card = createGlassCard(page, 424, 6, 362, 356, false, 20);
    createCaption(diag_card, "NMEA2000 & DISPLAY CONTROLS", 2, 0);

    createActionButton(diag_card, "NIGHT FILTER", 0, 24, 110, 40, CommandToggleNightMode);
    auto* dim = createActionButton(diag_card, "DIM 100%", 120, 24, 110, 40, CommandCycleVisualBrightness);
    brightness_button_label_ = lv_obj_get_child(dim, 0);
    createActionButton(diag_card, "SCREEN OFF", 240, 24, 110, 40, CommandScreenOff);

    auto makeDeviceRow = [&](const char* caption, int y, lv_obj_t** val_out, lv_color_t color) {
        createCaption(diag_card, caption, 0, y);
        *val_out = createValue(diag_card, 150, y - 2, &lv_font_montserrat_14, color);
        lv_obj_set_width(*val_out, 190);
        lv_obj_set_style_text_align(*val_out, LV_TEXT_ALIGN_RIGHT, 0);

        lv_obj_t* div = lv_obj_create(diag_card);
        lv_obj_set_pos(div, 0, y + 20);
        lv_obj_set_size(div, 338, 1);
        lv_obj_set_style_bg_color(div, theme::border(), 0);
        lv_obj_set_style_border_width(div, 0, 0);
        lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
    };

    makeDeviceRow("FLASH STORAGE", 78, &flash_value_, theme::cyan());
    makeDeviceRow("PSRAM AVAILABLE", 112, &psram_value_, theme::purple());
    makeDeviceRow("FREE HEAP", 146, &heap_value_, theme::mint());
    makeDeviceRow("UPTIME", 180, &uptime_value_, theme::text());
    makeDeviceRow("FIRMWARE", 214, &firmware_value_, theme::champagne());

    lv_obj_t* proto_box = lv_obj_create(diag_card);
    lv_obj_set_pos(proto_box, 0, 252);
    lv_obj_set_size(proto_box, 338, 76);
    theme::styleInset(proto_box, 12);
    lv_obj_t* proto_text = lv_label_create(proto_box);
    lv_label_set_text(proto_text, "Active Nodes: gWind(.0) DST(.35) Compass(.1)\nPlotter/AIS(.43) GPS(.2) Venus MQTT(.20)\nSignal K Server: 192.168.50.10");
    lv_obj_center(proto_text);
    lv_obj_set_style_text_color(proto_text, theme::muted(), 0);
    lv_obj_set_style_text_font(proto_text, &lv_font_montserrat_12, 0);
}

void ElixirUI::showPage(Page page) {
    current_page_ = page;
    for (std::size_t i = 0; i < pages_.size(); ++i) {
        const bool active = i == static_cast<std::size_t>(page);
        setHidden(pages_[i], !active);
        if (nav_buttons_[i] != nullptr) {
            theme::styleDockButton(nav_buttons_[i], active);
            lv_obj_t* label = lv_obj_get_child(nav_buttons_[i], 0);
            if (label != nullptr) {
                lv_obj_set_style_text_color(label, active ? theme::champagne() : theme::muted(), 0);
            }
        }
    }
    refreshOverlayOrder();
}

void ElixirUI::queueCommand(uint32_t command) {
    pending_commands_.fetch_or(command, std::memory_order_relaxed);
}

uint32_t ElixirUI::takeCommands() {
    return pending_commands_.exchange(0U, std::memory_order_acq_rel);
}

void ElixirUI::navEvent(lv_event_t* event) {
    auto* target = lv_event_get_target(event);
    auto* self = static_cast<ElixirUI*>(lv_obj_get_user_data(target));
    const auto page_value = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    if (self != nullptr && page_value < static_cast<uintptr_t>(Page::Count)) {
        self->showPage(static_cast<Page>(page_value));
    }
}

void ElixirUI::commandEvent(lv_event_t* event) {
    auto* target = lv_event_get_target(event);
    auto* self = static_cast<ElixirUI*>(lv_obj_get_user_data(target));
    if (self == nullptr) return;
    const auto command = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    if (command == CommandToggleNightMode) self->toggleNightOverlay();
    if (command == CommandCycleVisualBrightness) self->cycleVisualBrightness();
    self->queueCommand(command);
}

void ElixirUI::sliderEvent(lv_event_t* event) {
    auto* target = lv_event_get_target(event);
    auto* self = static_cast<ElixirUI*>(lv_obj_get_user_data(target));
    if (self == nullptr) return;
    self->pending_seek_.store(static_cast<float>(lv_slider_get_value(target)) / 1000.0F,
                              std::memory_order_relaxed);
    self->queueCommand(CommandSeekReplay);
}

void ElixirUI::wakeEvent(lv_event_t* event) {
    auto* target = lv_event_get_target(event);
    auto* self = static_cast<ElixirUI*>(lv_obj_get_user_data(target));
    if (self == nullptr) return;
    self->queueCommand(CommandScreenOn);
}

void ElixirUI::toggleNightOverlay() {
    night_mode_ = !night_mode_;
    setHidden(night_overlay_, !night_mode_);
    refreshOverlayOrder();
}

void ElixirUI::cycleVisualBrightness() {
    visual_brightness_index_ = static_cast<uint8_t>(
        (visual_brightness_index_ + 1U) % kVisualBrightnessCount);
    const uint8_t opacity = kVisualDimOpacity[visual_brightness_index_];
    lv_obj_set_style_bg_opa(dim_overlay_, opacity, 0);
    setHidden(dim_overlay_, opacity == LV_OPA_TRANSP);
    if (brightness_button_label_ != nullptr) {
        lv_label_set_text_fmt(brightness_button_label_, "DIM %u%%",
                              kVisualBrightnessPercent[visual_brightness_index_]);
    }
    refreshOverlayOrder();
}

void ElixirUI::setBacklightState(bool enabled) {
    if (wake_overlay_ == nullptr) return;
    setHidden(wake_overlay_, enabled);
    refreshOverlayOrder();
}

void ElixirUI::refreshOverlayOrder() {
    if (night_overlay_ != nullptr) lv_obj_move_foreground(night_overlay_);
    if (dim_overlay_ != nullptr) lv_obj_move_foreground(dim_overlay_);
    if (wake_overlay_ != nullptr) lv_obj_move_foreground(wake_overlay_);
}

void ElixirUI::appendChartSample(const HistorySample& sample) {
    if (power_chart_ != nullptr) {
        lv_chart_set_next_value(power_chart_, power_voltage_series_, chartValue(sample.voltage, 10.0F));
        lv_chart_set_next_value(power_chart_, power_solar_series_, chartValue(sample.solar));
    }
    if (eng_chart_ != nullptr) {
        lv_chart_set_next_value(eng_chart_, eng_temp_series_, chartValue(sample.engine_temp));
        lv_chart_set_next_value(eng_chart_, eng_alt_series_, chartValue(sample.alternator_temp));
    }
}

void ElixirUI::updateCharts(const HistoryBuffer& history) {
    if (history.empty() || history.sequence() == last_history_sequence_) return;
    if (last_history_sequence_ == 0U || history.sequence() < last_history_sequence_ ||
        history.sequence() - last_history_sequence_ > 1U) {
        if (power_chart_ != nullptr) {
            lv_chart_set_all_value(power_chart_, power_voltage_series_, LV_CHART_POINT_NONE);
            lv_chart_set_all_value(power_chart_, power_solar_series_, LV_CHART_POINT_NONE);
        }
        if (eng_chart_ != nullptr) {
            lv_chart_set_all_value(eng_chart_, eng_temp_series_, LV_CHART_POINT_NONE);
            lv_chart_set_all_value(eng_chart_, eng_alt_series_, LV_CHART_POINT_NONE);
        }
        for (std::size_t i = 0; i < history.size(); ++i) appendChartSample(history.oldest(i));
    } else {
        appendChartSample(history.newest());
    }
    last_history_sequence_ = history.sequence();
    if (power_chart_ != nullptr) lv_chart_refresh(power_chart_);
    if (eng_chart_ != nullptr) lv_chart_refresh(eng_chart_);
}

void ElixirUI::update(const BoatState& state, const HistoryBuffer& history,
                      const SystemSnapshot& system, const char* source_label) {
    if (root_ == nullptr) return;

    // Header updates
    const char* source = sourceKindText(state.source_kind);
    if (state.source_kind == DataSourceKind::Replay) {
        lv_label_set_text_fmt(source_label_, "%s %.1fx", source, state.replay_speed);
    } else {
        lv_label_set_text(source_label_, source);
    }
    lv_label_set_text_fmt(clock_label_, "%02u:%02u", state.clock_hour, state.clock_minute);
    lv_label_set_text(gnss_label_, state.gps_valid ? "GNSS 3D" : "DOCK OFFLINE");
    lv_obj_set_style_text_color(gnss_label_, state.gps_valid ? theme::mint() : theme::muted(), 0);

    setDot(can_dot_, state.can_online);
    setDot(wifi_dot_, state.wifi_online);
    setDot(sd_dot_, system.sd_mounted || state.sd_online);

    // -------------------------------------------------------------------------
    // Page 0: Overview Updates
    // -------------------------------------------------------------------------
    if (state.gps_valid && state.sog_kn > 0.05F) {
        lv_label_set_text_fmt(ov_sog_, "%.2f", state.sog_kn);
    } else {
        lv_label_set_text(ov_sog_, "--.-");
    }

    if (state.stw_raw_kn > 0.05F) {
        lv_label_set_text_fmt(ov_stw_, "STW %.2f kn (x1.055)", state.stw_kn);
    } else {
        lv_label_set_text(ov_stw_, "STW --.- kn");
    }

    if (state.depth_raw_m > 0.1F) {
        lv_label_set_text_fmt(ov_depth_, "%.1f", state.depth_keel_m);
        lv_obj_set_style_text_color(ov_depth_, depthColor(state.depth_keel_m), 0);
    } else {
        lv_label_set_text(ov_depth_, "--.-");
        lv_obj_set_style_text_color(ov_depth_, theme::muted(), 0);
    }

    if (std::isfinite(state.heading_deg) && state.gps_valid) {
        lv_label_set_text_fmt(ov_heading_, "HDG %03.0f° • COG %03.0f°", state.heading_deg, state.cog_deg);
    } else if (std::isfinite(state.heading_deg)) {
        lv_label_set_text_fmt(ov_heading_, "HDG %03.0f° • COG ---°", state.heading_deg);
    } else {
        lv_label_set_text(ov_heading_, "HDG ---° • COG ---°");
    }

    if (state.aws_kn > 0.1F) {
        lv_label_set_text_fmt(ov_wind_badge_, "AWA %.0f° %s • %.1f kn\nTWA %.0f° • TWS %.1f kn\nVMG %+.2f kn",
                              std::fabs(state.awa_deg), state.awa_deg < 0.0F ? "P" : "S", state.aws_kn,
                              std::fabs(state.twa_deg), state.tws_kn, state.vmg_kn);
    } else {
        lv_label_set_text(ov_wind_badge_, "AWA ---° • --.- kn\nTWA ---° • TWS --.- kn\nVMG --.- kn");
    }

    if (state.battery_soc_pct > 1.0F) {
        lv_arc_set_value(ov_soc_arc_, static_cast<int>(std::lround(state.battery_soc_pct)));
        lv_label_set_text_fmt(ov_soc_val_, "%.0f%%", state.battery_soc_pct);
    } else {
        lv_arc_set_value(ov_soc_arc_, 0);
        lv_label_set_text(ov_soc_val_, "--%");
    }

    if (state.battery_voltage_v > 5.0F) {
        if (state.battery_current_a >= 0.0F) {
            lv_label_set_text_fmt(ov_soc_sub_, "%.2f V\n%+.1f A\nCharging",
                                  state.battery_voltage_v, state.battery_current_a);
        } else {
            lv_label_set_text_fmt(ov_soc_sub_, "%.2f V\n%+.1f A\n%.1f h",
                                  state.battery_voltage_v, state.battery_current_a,
                                  state.estimated_hours_remaining);
        }
    } else {
        lv_label_set_text(ov_soc_sub_, "--.- V\n--.- A\n--.- h");
    }

    if (state.solar_power_w > 0.5F) {
        lv_label_set_text_fmt(ov_solar_val_, "%.0f", state.solar_power_w);
    } else {
        lv_label_set_text(ov_solar_val_, "0");
    }

    if (state.engine_running && state.generator_current_a > 0.5F) {
        lv_label_set_text_fmt(ov_alt_charge_val_, "~ %.1f A", state.generator_current_a);
        lv_obj_set_style_text_color(ov_alt_charge_val_, theme::mint(), 0);
    } else {
        lv_label_set_text(ov_alt_charge_val_, "STANDBY");
        lv_obj_set_style_text_color(ov_alt_charge_val_, theme::muted(), 0);
    }

    if (state.engine_temp_c > 15.0F) {
        lv_label_set_text_fmt(ov_engine_val_, "%.1f", state.engine_temp_c);
        lv_obj_set_style_text_color(ov_engine_val_, engineColor(state.engine_temp_c), 0);
    } else {
        lv_label_set_text(ov_engine_val_, "--.-");
        lv_obj_set_style_text_color(ov_engine_val_, theme::muted(), 0);
    }

    if (state.alternator_temp_c > 15.0F) {
        lv_label_set_text_fmt(ov_alt_val_, "%.1f", state.alternator_temp_c);
        lv_obj_set_style_text_color(ov_alt_val_, altColor(state.alternator_temp_c), 0);
    } else {
        lv_label_set_text(ov_alt_val_, "--.-");
        lv_obj_set_style_text_color(ov_alt_val_, theme::muted(), 0);
    }

    // Real environmental sensors on Elixir 2: Water (DST) and EngRoom (DS18B20)
    char water_str[16];
    char eng_room_str[16];
    if (state.water_temp_c > 0.5F) {
        snprintf(water_str, sizeof(water_str), "%.1f°C", state.water_temp_c);
    } else {
        snprintf(water_str, sizeof(water_str), "--.-°C");
    }
    if (state.engine_room_temp_c > 10.0F) {
        snprintf(eng_room_str, sizeof(eng_room_str), "%.1f°C", state.engine_room_temp_c);
    } else {
        snprintf(eng_room_str, sizeof(eng_room_str), "--.-°C");
    }
    lv_label_set_text_fmt(ov_climate_val_, "Water %s • EngRoom %s\nTrip %.1f NM • Total %.0f NM",
                          water_str, eng_room_str,
                          state.trip_log_nm, state.total_log_nm);

    // -------------------------------------------------------------------------
    // Page 1: Sail Updates
    // -------------------------------------------------------------------------
    if (state.gps_valid && state.sog_kn > 0.05F) {
        lv_label_set_text_fmt(sail_sog_, "%.2f", state.sog_kn);
    } else {
        lv_label_set_text(sail_sog_, "--.-");
    }

    if (state.stw_raw_kn > 0.05F) {
        lv_label_set_text_fmt(sail_stw_, "STW %.2f kn (x1.055 calibrated)", state.stw_kn);
    } else {
        lv_label_set_text(sail_stw_, "STW --.- kn");
    }

    if (state.depth_raw_m > 0.1F) {
        lv_label_set_text_fmt(sail_depth_, "%.1f", state.depth_keel_m);
        lv_obj_set_style_text_color(sail_depth_, depthColor(state.depth_keel_m), 0);
    } else {
        lv_label_set_text(sail_depth_, "--.-");
        lv_obj_set_style_text_color(sail_depth_, theme::muted(), 0);
    }

    if (std::isfinite(state.heading_deg)) {
        lv_label_set_text_fmt(sail_heading_, "%03.0f°", state.heading_deg);
    } else {
        lv_label_set_text(sail_heading_, "---°");
    }

    if (state.gps_valid && std::isfinite(state.cog_deg)) {
        lv_label_set_text_fmt(sail_cog_, "COG %03.0f° (GPS)\nVar %.1f° E", state.cog_deg, state.magnetic_variation_deg);
    } else {
        lv_label_set_text(sail_cog_, "COG ---° (GPS)\nVar 6.2° E");
    }

    if (state.aws_kn > 0.1F) {
        lv_label_set_text_fmt(sail_awa_val_, "%.0f° %s", std::fabs(state.awa_deg), state.awa_deg < 0.0F ? "PORT" : "STBD");
        lv_label_set_text_fmt(sail_aws_val_, "%.1f", state.aws_kn);
    } else {
        lv_label_set_text(sail_awa_val_, "---°");
        lv_label_set_text(sail_aws_val_, "--.-");
    }

    if (state.tws_kn > 0.1F) {
        lv_label_set_text_fmt(sail_twa_val_, "TWA %.0f° %s", std::fabs(state.twa_deg), state.twa_deg < 0.0F ? "P" : "S");
        lv_label_set_text_fmt(sail_tws_val_, "TWS %.1f kn • TWD %03.0f°", state.tws_kn, state.twd_deg);
        lv_label_set_text_fmt(sail_vmg_val_, "VMG: %+.2f kn", state.vmg_kn);
    } else {
        lv_label_set_text(sail_twa_val_, "TWA ---°");
        lv_label_set_text(sail_tws_val_, "TWS --.- kn • TWD ---°");
        lv_label_set_text(sail_vmg_val_, "VMG: --.- kn");
    }

    lv_label_set_text_fmt(sail_roll_val_, "%+.1f°", state.roll_deg);
    lv_label_set_text_fmt(sail_pitch_val_, "%+.1f°", state.pitch_deg);

    if (state.water_temp_c > 0.5F) {
        lv_label_set_text_fmt(sail_water_val_, "%.1f", state.water_temp_c);
    } else {
        lv_label_set_text(sail_water_val_, "--.-");
    }

    // -------------------------------------------------------------------------
    // Page 2: Power Updates
    // -------------------------------------------------------------------------
    if (state.battery_soc_pct > 1.0F) {
        lv_arc_set_value(power_soc_arc_, static_cast<int16_t>(std::lround(state.battery_soc_pct)));
        lv_label_set_text_fmt(power_soc_val_, "%.0f%%", state.battery_soc_pct);
    } else {
        lv_arc_set_value(power_soc_arc_, 0);
        lv_label_set_text(power_soc_val_, "--%");
    }

    if (state.battery_voltage_v > 5.0F) {
        lv_label_set_text_fmt(power_voltage_val_, "%.2f V", state.battery_voltage_v);
        lv_label_set_text_fmt(power_current_val_, "%+.1f A", state.battery_current_a);
        lv_label_set_text_fmt(power_power_val_, "%+.0f W", state.battery_power_w);
        if (state.battery_current_a >= 0.0F) {
            lv_label_set_text_fmt(power_remaining_val_, "CHARGING (+%.1f A)", state.battery_current_a);
        } else {
            lv_label_set_text_fmt(power_remaining_val_, "%.1f Hours", state.estimated_hours_remaining);
        }
    } else {
        lv_label_set_text(power_voltage_val_, "--.- V");
        lv_label_set_text(power_current_val_, "--.- A");
        lv_label_set_text(power_power_val_, "--.- W");
        lv_label_set_text(power_remaining_val_, "--.- Hours");
    }

    if (state.solar_power_w > 0.5F) {
        lv_label_set_text_fmt(power_solar_val_, "%.0f", state.solar_power_w);
    } else {
        lv_label_set_text(power_solar_val_, "0");
    }
    lv_label_set_text_fmt(power_solar_today_val_, "%.0f Wh produced today", state.solar_today_wh);

    if (state.alternator_temp_c > 15.0F) {
        lv_label_set_text_fmt(power_alt_temp_val_, "%.1f", state.alternator_temp_c);
        lv_obj_set_style_text_color(power_alt_temp_val_, altColor(state.alternator_temp_c), 0);
    } else {
        lv_label_set_text(power_alt_temp_val_, "--.-");
        lv_obj_set_style_text_color(power_alt_temp_val_, theme::muted(), 0);
    }

    if (state.engine_running && state.generator_current_a > 0.5F) {
        if (state.alternator_temp_c >= 100.0F) {
            lv_label_set_text_fmt(power_alt_status_val_, "ALERT: ALT OVERHEAT >100°C • Charging ~ %.1f A", state.generator_current_a);
            lv_obj_set_style_text_color(power_alt_status_val_, theme::coral(), 0);
        } else if (state.alternator_temp_c >= 80.0F) {
            lv_label_set_text_fmt(power_alt_status_val_, "WARN: ALT HIGH TEMP >80°C • Charging ~ %.1f A", state.generator_current_a);
            lv_obj_set_style_text_color(power_alt_status_val_, theme::amber(), 0);
        } else {
            lv_label_set_text_fmt(power_alt_status_val_, "CHARGING ~ %.1f A (%.0f W) • Headroom Nominal (<80°C)",
                                  state.generator_current_a, state.generator_current_a * state.battery_voltage_v);
            lv_obj_set_style_text_color(power_alt_status_val_, theme::mint(), 0);
        }
    } else {
        lv_label_set_text(power_alt_status_val_, "STANDBY • Alternator Inactive (Engine Stopped)");
        lv_obj_set_style_text_color(power_alt_status_val_, theme::muted(), 0);
    }

    // -------------------------------------------------------------------------
    // Page 3: Engine Updates
    // -------------------------------------------------------------------------
    if (state.engine_temp_c > 15.0F) {
        lv_label_set_text_fmt(eng_temp_val_, "%.1f", state.engine_temp_c);
        lv_obj_set_style_text_color(eng_temp_val_, engineColor(state.engine_temp_c), 0);
        lv_bar_set_value(eng_temp_bar_, static_cast<int32_t>(state.engine_temp_c), LV_ANIM_OFF);
    } else {
        lv_label_set_text(eng_temp_val_, "--.-");
        lv_obj_set_style_text_color(eng_temp_val_, theme::muted(), 0);
        lv_bar_set_value(eng_temp_bar_, 0, LV_ANIM_OFF);
    }

    if (state.alternator_temp_c > 15.0F) {
        lv_label_set_text_fmt(eng_alt_val_, "%.1f", state.alternator_temp_c);
        lv_obj_set_style_text_color(eng_alt_val_, altColor(state.alternator_temp_c), 0);
        lv_bar_set_value(eng_alt_bar_, static_cast<int32_t>(state.alternator_temp_c), LV_ANIM_OFF);
    } else {
        lv_label_set_text(eng_alt_val_, "--.-");
        lv_obj_set_style_text_color(eng_alt_val_, theme::muted(), 0);
        lv_bar_set_value(eng_alt_bar_, 0, LV_ANIM_OFF);
    }

    if (state.engine_room_temp_c > 10.0F) {
        lv_label_set_text_fmt(eng_room_val_, "%.1f", state.engine_room_temp_c);
    } else {
        lv_label_set_text(eng_room_val_, "--.-");
    }

    if (state.engine_running && state.generator_current_a > 0.5F) {
        lv_label_set_text_fmt(eng_charge_val_, "~ %.1f A (%.0f W)", state.generator_current_a,
                              state.generator_current_a * state.battery_voltage_v);
        lv_label_set_text(eng_state_badge_, "STATE: YANMAR RUNNING • GENERATOR ACTIVE");
        lv_obj_set_style_text_color(eng_state_badge_, theme::mint(), 0);
    } else {
        lv_label_set_text(eng_charge_val_, "0.0 A (Standby)");
        lv_label_set_text(eng_state_badge_, "STATE: ENGINE STOPPED • STANDBY");
        lv_obj_set_style_text_color(eng_state_badge_, theme::muted(), 0);
    }

    if (state.gps_valid && state.sog_kn > 0.05F) {
        lv_label_set_text_fmt(eng_sog_val_, "%.2f kn", state.sog_kn);
    } else {
        lv_label_set_text(eng_sog_val_, "--.- kn");
    }

    if (state.depth_raw_m > 0.1F) {
        lv_label_set_text_fmt(eng_depth_val_, "%.1f m", state.depth_keel_m);
        lv_obj_set_style_text_color(eng_depth_val_, depthColor(state.depth_keel_m), 0);
    } else {
        lv_label_set_text(eng_depth_val_, "--.- m");
        lv_obj_set_style_text_color(eng_depth_val_, theme::muted(), 0);
    }

    if (std::isfinite(state.heading_deg)) {
        lv_label_set_text_fmt(eng_heading_val_, "%03.0f°", state.heading_deg);
    } else {
        lv_label_set_text(eng_heading_val_, "---°");
    }

    // -------------------------------------------------------------------------
    // Page 4: Tactical Updates
    // -------------------------------------------------------------------------
    if (state.ais_targets_count > 0 && state.ais_target_name[0] != '\0') {
        lv_label_set_text(ais_name_val_, state.ais_target_name);
        lv_label_set_text_fmt(ais_range_val_, "%.2f NM @ %03.0f°", state.ais_range_nm, state.ais_bearing_deg);
        lv_label_set_text_fmt(ais_sog_val_, "%.1f kn", state.ais_sog_kn);
        lv_label_set_text_fmt(ais_cpa_val_, "%.2f NM", state.ais_cpa_nm);
        lv_label_set_text_fmt(ais_tcpa_val_, "%.1f min", state.ais_tcpa_min);
        lv_label_set_text_fmt(ais_count_val_, "%u Vessels", state.ais_targets_count);
    } else {
        lv_label_set_text(ais_name_val_, "-- (NO TARGET)");
        lv_label_set_text(ais_range_val_, "--.- NM");
        lv_label_set_text(ais_sog_val_, "--.- kn");
        lv_label_set_text(ais_cpa_val_, "--.- NM");
        lv_label_set_text(ais_tcpa_val_, "--.- min");
        lv_label_set_text(ais_count_val_, "0 Vessels");
    }

    if (state.anchor_active) {
        lv_label_set_text(anchor_status_val_, state.anchor_alarm ? "ALARM: DRAGGING!" : "GUARD ACTIVE • IN RADIUS");
        lv_obj_set_style_text_color(anchor_status_val_, state.anchor_alarm ? theme::coral() : theme::mint(), 0);
        lv_label_set_text_fmt(anchor_radius_val_, "%.1f m", state.anchor_radius_m);
        lv_label_set_text_fmt(anchor_max_val_, "%.1f m", state.anchor_max_radius_m);
        lv_label_set_text_fmt(anchor_bearing_val_, "%03.0f°", state.anchor_drift_bearing_deg);
    } else {
        lv_label_set_text(anchor_status_val_, "STANDBY (UNDER WAY)");
        lv_obj_set_style_text_color(anchor_status_val_, theme::iceBlue(), 0);
        lv_label_set_text(anchor_radius_val_, "--.- m");
        lv_label_set_text(anchor_max_val_, "--.- m");
        lv_label_set_text(anchor_bearing_val_, "---°");
    };

    // -------------------------------------------------------------------------
    // Page 5: System Updates
    // -------------------------------------------------------------------------
    lv_label_set_text_fmt(sys_trip_val_, "%.1f NM", state.trip_log_nm);
    lv_label_set_text_fmt(sys_total_val_, "%.1f NM", state.total_log_nm);
    lv_label_set_text_fmt(sys_keel_offset_val_, "%+.2f m", state.keel_offset_m);
    lv_label_set_text_fmt(sys_stw_factor_val_, "x%.3f (+5.5%%)", state.stw_calibration_factor);

    lv_label_set_text_fmt(mode_value_, "%s • %s",
                          state.source_kind == DataSourceKind::Replay ? "RECORDED LOG" : "GENERATED FEED",
                          state.source_valid ? "ACTIVE" : "OFFLINE");
    lv_label_set_text(replay_label_, source_label != nullptr ? source_label : "Unknown source");
    lv_label_set_text(replay_play_button_label_, state.replay_playing ? "PAUSE" : "PLAY");
    lv_label_set_text_fmt(replay_speed_button_label_, "SPEED %.1fx", state.replay_speed);
    if (!lv_obj_has_state(replay_slider_, LV_STATE_PRESSED)) {
        lv_slider_set_value(replay_slider_,
                            static_cast<int>(std::lround(state.replay_progress * 1000.0F)),
                            LV_ANIM_OFF);
    }
    const uint32_t elapsed_s = state.source_elapsed_ms / 1000U;
    const uint32_t duration_s = state.source_duration_ms / 1000U;
    lv_label_set_text_fmt(replay_progress_label_, "%02u:%02u / %02u:%02u",
                          elapsed_s / 60U, elapsed_s % 60U,
                          duration_s / 60U, duration_s % 60U);

    lv_label_set_text_fmt(flash_value_, "%.1f / %.1f MB",
                          system.sketch_bytes / 1048576.0F,
                          system.flash_bytes / 1048576.0F);
    if (system.psram_found) {
        lv_label_set_text_fmt(psram_value_, "%.1f MB",
                              system.free_psram_bytes / 1048576.0F);
    } else {
        lv_label_set_text(psram_value_, "NOT FOUND");
    }
    lv_label_set_text_fmt(heap_value_, "%.0f KB", system.free_heap_bytes / 1024.0F);
    lv_label_set_text_fmt(uptime_value_, "%ud %02u:%02u",
                          system.uptime_seconds / 86400U,
                          (system.uptime_seconds / 3600U) % 24U,
                          (system.uptime_seconds / 60U) % 60U);
    lv_label_set_text(firmware_value_, config::kVersion);

    updateCharts(history);
}

}  // namespace elixir::ui
