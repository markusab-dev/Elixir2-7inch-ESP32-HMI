#!/usr/bin/env python3
"""Render static design previews of the Minimalist Modern Yachting 800×480 LVGL interface for Elixir 2."""
from __future__ import annotations

import math
import os
from pathlib import Path
from typing import Sequence

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "preview"
OUT.mkdir(exist_ok=True)
S = 2
W, H = 800, 480

COLORS = {
    "bg": "#0A0E14",
    "deep": "#05080C",
    "surface": "#10161F",
    "raised": "#161F2B",
    "soft": "#121A24",
    "border": "#202B3B",
    "borderLight": "#2E3D52",
    "text": "#FFFFFF",
    "muted": "#8E9AA8",
    "champagne": "#FFD166",
    "iceBlue": "#5AC8FA",
    "cyan": "#40C4FF",
    "mint": "#30D158",
    "amber": "#FF9F0A",
    "coral": "#FF453A",
    "purple": "#BF5AF2",
}

def find_font(bold: bool = False) -> str | None:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf" if bold else "C:\\Windows\\Fonts\\arial.ttf",
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    return None

FONT_REGULAR = find_font(False)
FONT_BOLD = find_font(True)

def rgb(value: str) -> tuple[int, int, int]:
    value = value.lstrip("#")
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4))

def lerp_color(a: str, b: str, t: float) -> tuple[int, int, int]:
    ar, ag, ab = rgb(a)
    br, bg, bb = rgb(b)
    return (
        int(ar + (br - ar) * t),
        int(ag + (bg - ag) * t),
        int(ab + (bb - ab) * t),
    )

class Canvas:
    def __init__(self) -> None:
        self.image = Image.new("RGB", (W * S, H * S), rgb(COLORS["bg"]))
        self.draw = ImageDraw.Draw(self.image, "RGBA")
        self._fonts: dict[tuple[int, bool], ImageFont.FreeTypeFont] = {}
        self.background()

    @staticmethod
    def q(value: float) -> int:
        return round(value * S)

    def font(self, size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
        key = (size, bold)
        if key not in self._fonts:
            font_path = FONT_BOLD if bold else FONT_REGULAR
            if font_path and os.path.exists(font_path):
                self._fonts[key] = ImageFont.truetype(font_path, self.q(size))
            else:
                self._fonts[key] = ImageFont.load_default()
        return self._fonts[key]

    def background(self) -> None:
        for y in range(H * S):
            t = y / (H * S - 1)
            color = lerp_color(COLORS["deep"], COLORS["bg"], t)
            self.draw.line((0, y, W * S, y), fill=color + (255,))
        self.glow(300, -80, 250, COLORS["champagne"], 30)
        self.glow(-50, 320, 260, COLORS["iceBlue"], 25)

    def glow(self, x: int, y: int, radius: int, color: str, alpha: int) -> None:
        size = radius * 2
        layer = Image.new("RGBA", (size * S, size * S), (0, 0, 0, 0))
        d = ImageDraw.Draw(layer)
        c = rgb(color)
        d.ellipse((0, 0, size * S, size * S), fill=c + (alpha,))
        layer = layer.filter(ImageFilter.GaussianBlur(self.q(radius // 2)))
        self.image.paste(layer, (self.q(x - radius), self.q(y - radius)), layer)

    def glass_card(self, x: int, y: int, w: int, h: int, radius: int = 20, raised: bool = False) -> None:
        box = (self.q(x), self.q(y), self.q(x + w), self.q(y + h))
        bg_col = rgb(COLORS["raised"] if raised else COLORS["surface"])
        bg_alpha = 190 if raised else 165
        self.draw.rounded_rectangle(box, radius=self.q(radius), fill=bg_col + (bg_alpha,))
        b_col = rgb(COLORS["borderLight"] if raised else COLORS["border"])
        self.draw.rounded_rectangle(box, radius=self.q(radius), outline=b_col + (200,), width=self.q(1))

    def inset_box(self, x: int, y: int, w: int, h: int, radius: int = 14) -> None:
        box = (self.q(x), self.q(y), self.q(x + w), self.q(y + h))
        self.draw.rounded_rectangle(box, radius=self.q(radius), fill=rgb(COLORS["deep"]) + (140,))
        self.draw.rounded_rectangle(box, radius=self.q(radius), outline=rgb(COLORS["border"]) + (140,), width=self.q(1))

    def text(self, text: str, x: int, y: int, size: int = 14, color: str = "text", bold: bool = False, align: str = "left") -> None:
        f = self.font(size, bold)
        lines = text.split("\n")
        line_height = self.q(size * 1.25)
        for i, line in enumerate(lines):
            cur_y = self.q(y) + i * line_height
            if align == "center":
                w = f.getlength(line)
                cur_x = self.q(x) - w / 2
            elif align == "right":
                w = f.getlength(line)
                cur_x = self.q(x) - w
            else:
                cur_x = self.q(x)
            self.draw.text((cur_x, cur_y), line, font=f, fill=rgb(COLORS[color]) + (255,))

    def caption(self, text: str, x: int, y: int) -> None:
        self.text(text.upper(), x, y, size=11, color="muted", bold=True)

    def arc_gauge(self, cx: int, cy: int, radius: int, value: float, max_value: float, color: str, thickness: int = 9) -> None:
        r = self.q(radius)
        t = self.q(thickness)
        box = (self.q(cx) - r, self.q(cy) - r, self.q(cx) + r, self.q(cy) + r)
        self.draw.arc(box, start=135, end=405, fill=rgb(COLORS["border"]) + (180,), width=t)
        angle = 135 + 270 * (min(max_value, max(0.0, value)) / max_value)
        self.draw.arc(box, start=135, end=angle, fill=rgb(COLORS[color]) + (255,), width=t)

    def top_bar(self, source: str = "RECORDED 5.0x") -> None:
        self.draw.rectangle((0, 0, W * S, self.q(46)), fill=rgb(COLORS["deep"]) + (180,))
        self.draw.line((0, self.q(46), W * S, self.q(46)), fill=rgb(COLORS["border"]) + (200,), width=self.q(1))
        self.text("20:48", 20, 12, size=18, color="text", bold=True)
        self.text("GNSS 3D", 90, 16, size=11, color="mint", bold=True)
        self.text("ELIXIR2", 400, 12, size=20, color="text", bold=True, align="center")

        pill_box = (self.q(490), self.q(8), self.q(630), self.q(36))
        self.draw.rounded_rectangle(pill_box, radius=self.q(14), fill=rgb(COLORS["deep"]) + (220,), outline=rgb(COLORS["champagne"]) + (200,), width=self.q(1))
        self.text(source, 560, 14, size=11, color="champagne", bold=True, align="center")

        for x, label, online in [(660, "N2K", True), (710, "WIFI", True), (760, "SD", True)]:
            dot_color = "mint" if online else "muted"
            self.draw.ellipse((self.q(x), self.q(18), self.q(x + 8), self.q(26)), fill=rgb(COLORS[dot_color]) + (255,))
            self.caption(label, x + 12, 16)

    def floating_dock(self, active_tab: str = "OVERVIEW") -> None:
        dock_box = (self.q(40), self.q(420), self.q(760), self.q(472))
        self.draw.rounded_rectangle(dock_box, radius=self.q(25), fill=rgb(COLORS["deep"]) + (220,), outline=rgb(COLORS["borderLight"]) + (220,), width=self.q(1))
        tabs = ["OVERVIEW", "SAIL", "POWER", "ENGINE", "TACTICAL", "SYSTEM"]
        w = 112
        for i, tab in enumerate(tabs):
            x = 46 + i * 118
            active = tab == active_tab
            btn_box = (self.q(x), self.q(424), self.q(x + w), self.q(466))
            if active:
                self.draw.rounded_rectangle(btn_box, radius=self.q(20), fill=rgb(COLORS["surface"]) + (255,), outline=rgb(COLORS["champagne"]) + (220,), width=self.q(1))
            self.text(tab, x + w / 2, 436, size=12, color="champagne" if active else "muted", bold=True, align="center")

    def save(self, name: str) -> Path:
        out_img = self.image.resize((W, H), Image.Resampling.LANCZOS)
        path = OUT / f"{name}.png"
        out_img.save(path, quality=95)
        return path


def render_overview() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("OVERVIEW")

    # Left: Sailing & Nav
    c.glass_card(14, 52, 244, 356)
    c.caption("SAILING & NAV", 26, 62)
    c.inset_box(24, 80, 224, 100)
    c.caption("SPEED OVER GROUND", 34, 88)
    c.text("7.85", 34, 104, size=34, color="text", bold=True)
    c.text("kn", 130, 116, size=14, color="muted")
    c.text("STW 7.56 kn (x1.055)", 34, 148, size=12, color="iceBlue")

    c.inset_box(24, 186, 224, 96)
    c.caption("DEPTH BELOW KEEL", 34, 194)
    c.text("16.2", 34, 210, size=28, color="iceBlue", bold=True)
    c.text("m", 110, 220, size=14, color="muted")
    c.text("HDG 214° • COG 212°", 34, 250, size=12, color="champagne")

    c.inset_box(24, 288, 224, 100)
    c.caption("WIND TELEMETRY (gWind)", 34, 296)
    c.text("AWA 42° S • 14.8 kn\nTWA 48° • TWS 13.4 kn\nVMG +5.82 kn", 34, 314, size=12, color="iceBlue")

    # Center: Energy Storage
    c.glass_card(268, 52, 264, 356, raised=True)
    c.caption("HOUSE ENERGY STORAGE", 280, 62)
    c.arc_gauge(335, 130, 48, 92, 100, "champagne", thickness=10)
    c.text("92%", 335, 120, size=24, color="champagne", bold=True, align="center")
    c.text("13.20 V\n+3.4 A\n18.4 h", 400, 102, size=14, color="text", bold=True)

    c.inset_box(278, 196, 244, 90)
    c.caption("SMARTSOLAR MPPT", 288, 204)
    c.text("420", 288, 220, size=24, color="amber", bold=True)
    c.text("W", 350, 226, size=14, color="muted")
    c.text("286 Wh produced today", 288, 256, size=12, color="muted")

    c.inset_box(278, 292, 244, 96)
    c.caption("GENERATOR RESIDUAL CHARGE", 288, 300)
    c.text("≈ 34.5 A", 288, 316, size=22, color="mint", bold=True)
    c.text("Charging house bank (480 W)", 288, 350, size=12, color="muted")

    # Right: Machinery & Climate
    c.glass_card(542, 52, 244, 356)
    c.caption("MACHINERY & CLIMATE", 554, 62)
    c.inset_box(552, 80, 224, 96)
    c.caption("YANMAR 3YM30 BLOCK", 562, 88)
    c.text("78.2", 562, 104, size=24, color="text", bold=True)
    c.text("°C", 630, 110, size=14, color="muted")
    c.text("Thermostat 85°C • Limit 93°C", 562, 142, size=12, color="muted")

    c.inset_box(552, 182, 224, 96)
    c.caption("ALTERNATOR TEMP", 562, 190)
    c.text("65.4", 562, 206, size=24, color="mint", bold=True)
    c.text("°C", 630, 212, size=14, color="muted")
    c.text("Limits: 80°C warn / 100°C alert", 562, 244, size=12, color="muted")

    c.inset_box(552, 284, 224, 104)
    c.caption("CLIMATE & TRIP LOG", 562, 292)
    c.text("Cabin 20.5°C • Fridge 4.2°C\nWater 17.4°C • Trip 24.8 NM", 562, 312, size=12, color="iceBlue")

    c.save("01_overview")


def render_sail() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("SAIL")

    # Left: Wind Vector HUD
    c.glass_card(14, 52, 244, 356)
    c.caption("WIND VECTOR HUD (gWind)", 26, 62)
    c.inset_box(24, 80, 224, 146)
    c.caption("APPARENT WIND (AWA/AWS)", 34, 88)
    c.text("42° STBD", 34, 106, size=26, color="iceBlue", bold=True)
    c.caption("APPARENT SPEED", 34, 150)
    c.text("14.8", 34, 168, size=26, color="text", bold=True)
    c.text("kn", 110, 176, size=14, color="muted")

    c.inset_box(24, 234, 224, 154)
    c.caption("TRUE WIND (MCU COMPUTED)", 34, 242)
    c.text("TWA 48° S", 34, 258, size=22, color="champagne", bold=True)
    c.caption("TRUE SPEED & DIRECTION", 34, 294)
    c.text("TWS 13.4 kn • TWD 256°", 34, 310, size=14, color="text", bold=True)
    c.text("VMG: +5.82 kn", 34, 342, size=14, color="mint", bold=True)

    # Center: Primary Sailing HUD
    c.glass_card(268, 52, 264, 356, raised=True)
    c.caption("SAILING HUD", 280, 62)
    c.caption("SPEED OVER GROUND", 280, 84)
    c.text("7.85", 280, 100, size=48, color="text", bold=True)
    c.text("kn", 470, 126, size=16, color="muted")
    c.text("STW 7.56 kn (x1.055 calibrated)", 280, 164, size=12, color="iceBlue")

    c.inset_box(278, 188, 244, 96)
    c.caption("HEADING (can0.1)", 288, 196)
    c.text("214°", 288, 212, size=28, color="champagne", bold=True)
    c.text("COG 212° (GPS)\nVar 6.2° E", 410, 214, size=12, color="muted")

    c.inset_box(278, 290, 244, 98)
    c.caption("DEPTH BELOW KEEL (-1.4m)", 288, 298)
    c.text("16.2", 288, 318, size=28, color="iceBlue", bold=True)
    c.text("m", 370, 326, size=14, color="muted")

    # Right: Hull Attitude
    c.glass_card(542, 52, 244, 356)
    c.caption("HULL ATTITUDE (DST can0.35)", 554, 62)
    c.inset_box(552, 80, 224, 146)
    c.caption("HEEL / ROLL ANGLE", 562, 88)
    c.text("+12.4°", 562, 106, size=30, color="champagne", bold=True)
    c.caption("PITCH / STAMPING", 562, 150)
    c.text("+1.2°", 562, 168, size=24, color="text", bold=True)

    c.inset_box(552, 234, 224, 154)
    c.caption("WATER TEMPERATURE", 562, 242)
    c.text("17.4", 562, 258, size=26, color="text", bold=True)
    c.text("°C", 640, 266, size=14, color="muted")
    c.text("Sun Fast 36 SRS-C\nTarget Polar: 98%\nOptimal Trim Range", 562, 310, size=12, color="mint")

    c.save("02_sail")


def render_power() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("POWER")

    # Left: Battery Hero Card
    c.glass_card(14, 52, 376, 356, raised=True)
    c.caption("VICTRON ENERGY STORAGE (Venus MQTT)", 26, 62)
    c.arc_gauge(96, 150, 64, 92, 100, "champagne", thickness=12)
    c.text("92%", 96, 138, size=30, color="champagne", bold=True, align="center")

    c.caption("VOLTAGE", 200, 90)
    c.text("13.20 V", 200, 106, size=22, color="text", bold=True)
    c.caption("NET CURRENT", 200, 138)
    c.text("+3.4 A", 200, 154, size=22, color="mint", bold=True)
    c.caption("NET POWER", 200, 186)
    c.text("+45 W", 200, 202, size=22, color="amber", bold=True)

    c.inset_box(24, 250, 356, 138)
    c.caption("ESTIMATED AUTONOMY", 36, 260)
    c.text("18.4 Hours", 36, 282, size=28, color="champagne", bold=True)
    c.caption("SMARTSHUNT STATUS", 36, 330)
    c.text("Venus MQTT Sync • 15m Staleness Window", 36, 348, size=12, color="muted")

    # Right: Solar & Alternator Thermal
    c.glass_card(402, 52, 384, 356)
    c.caption("SMARTSOLAR MPPT & ALTERNATOR THERMAL", 414, 62)
    c.caption("SOLAR HARVEST", 420, 84)
    c.text("420 W", 420, 100, size=22, color="amber", bold=True)
    c.caption("TOTAL TODAY", 600, 84)
    c.text("286 Wh", 600, 100, size=22, color="champagne", bold=True)

    c.inset_box(412, 142, 364, 90)
    c.caption("ALTERNATOR TEMPERATURE & CHARGE", 422, 150)
    c.text("65.4", 422, 168, size=24, color="mint", bold=True)
    c.text("°C", 500, 174, size=14, color="muted")
    c.text("CHARGING ≈ 34.5 A • Headroom Nominal (<80°C)", 422, 204, size=12, color="mint")

    # Voltage Chart
    c.inset_box(412, 240, 364, 148)
    c.caption("VOLTAGE & SOLAR TREND (120s)", 422, 248)
    c.draw.line((c.q(430), c.q(330), c.q(750), c.q(330)), fill=rgb(COLORS["iceBlue"]) + (200,), width=c.q(2))
    c.draw.line((c.q(430), c.q(350), c.q(750), c.q(350)), fill=rgb(COLORS["amber"]) + (200,), width=c.q(2))
    c.text("13.2V Solid • Solar Peak 420W", 422, 360, size=11, color="muted")

    c.save("03_power")


def render_engine() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("ENGINE")

    # Left: Yanmar Machinery
    c.glass_card(14, 52, 376, 356, raised=True)
    c.caption("YANMAR 3YM30 PROPULSION", 26, 62)
    c.inset_box(24, 80, 356, 100)
    c.caption("ENGINE BLOCK (85°C Open / 93°C Alarm)", 34, 88)
    c.text("78.2", 34, 106, size=30, color="mint", bold=True)
    c.text("°C", 120, 118, size=14, color="muted")
    # Bar
    c.draw.rounded_rectangle((c.q(34), c.q(154), c.q(350), c.q(164)), radius=c.q(5), fill=rgb(COLORS["border"]) + (200,))
    c.draw.rounded_rectangle((c.q(34), c.q(154), c.q(250), c.q(164)), radius=c.q(5), fill=rgb(COLORS["mint"]) + (255,))

    c.inset_box(24, 186, 356, 100)
    c.caption("ALTERNATOR (80°C Warn / 100°C Alert)", 34, 194)
    c.text("65.4", 34, 212, size=28, color="mint", bold=True)
    c.text("°C", 120, 222, size=14, color="muted")
    c.draw.rounded_rectangle((c.q(34), c.q(258), c.q(350), c.q(268)), radius=c.q(5), fill=rgb(COLORS["border"]) + (200,))
    c.draw.rounded_rectangle((c.q(34), c.q(258), c.q(200), c.q(268)), radius=c.q(5), fill=rgb(COLORS["mint"]) + (255,))

    c.inset_box(24, 292, 356, 96)
    c.caption("ENGINEROOM AMBIENT", 34, 300)
    c.text("32.1 °C", 34, 318, size=22, color="text", bold=True)
    c.text("STATE: YANMAR RUNNING • GENERATOR ACTIVE", 34, 352, size=12, color="mint", bold=True)

    # Right: Alternator Charge Output & Motoring Nav Safety Bar
    c.glass_card(402, 52, 384, 356)
    c.caption("GENERATOR CHARGE OUTPUT & MOTORING NAV", 414, 62)
    c.inset_box(412, 80, 364, 96)
    c.caption("ALTERNATOR CHARGE OUTPUT", 422, 88)
    c.text("≈ 34.5 A (480 W)", 422, 106, size=28, color="mint", bold=True)
    c.text("Residual estimate: battery.current - solar.current", 422, 146, size=11, color="muted")

    c.inset_box(412, 184, 364, 80)
    c.caption("MOTORING NAVIGATION SAFETY", 422, 192)
    c.text("SOG: 6.85 kn", 422, 212, size=18, color="text", bold=True)
    c.text("DEPTH: 16.2 m", 540, 212, size=18, color="iceBlue", bold=True)
    c.text("HDG: 214°", 670, 212, size=18, color="champagne", bold=True)

    # Thermal trend
    c.inset_box(412, 270, 364, 118)
    c.caption("THERMAL TREND (ENGINE & ALTERNATOR)", 422, 278)
    c.draw.line((c.q(430), c.q(340), c.q(750), c.q(340)), fill=rgb(COLORS["mint"]) + (200,), width=c.q(2))
    c.draw.line((c.q(430), c.q(355), c.q(750), c.q(355)), fill=rgb(COLORS["amber"]) + (200,), width=c.q(2))
    c.text("Engine 78.2°C • Alternator 65.4°C", 422, 366, size=11, color="muted")

    c.save("04_engine")


def render_tactical() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("TACTICAL")

    # Left: AIS Target Tracking
    c.glass_card(14, 52, 376, 356, raised=True)
    c.caption("AIS TARGET TRACKING (Plotter can0.43)", 26, 62)
    c.inset_box(24, 80, 356, 170)
    c.caption("CLOSEST AIS TARGET", 34, 88)
    c.text("FINNFELLOW", 34, 106, size=24, color="champagne", bold=True)

    c.caption("RANGE / BEARING", 34, 144)
    c.text("3.85 NM @ 142°", 34, 160, size=20, color="text", bold=True)
    c.caption("TARGET SOG", 220, 144)
    c.text("16.8 kn", 220, 160, size=20, color="iceBlue", bold=True)

    c.caption("CPA (CLOSEST PASSAGE)", 34, 196)
    c.text("1.42 NM", 34, 212, size=22, color="mint", bold=True)
    c.caption("TCPA (TIME TO CPA)", 220, 196)
    c.text("18.5 min", 220, 212, size=22, color="amber", bold=True)

    c.inset_box(24, 258, 356, 120)
    c.caption("RECEIVER FLEET OVERVIEW", 34, 266)
    c.text("14 Vessels Detected", 34, 286, size=22, color="iceBlue", bold=True)
    c.text("Class A & B AIS Decoded\nRange: 15+ NM • Active Fleet Radar", 34, 324, size=12, color="muted")

    # Right: Anchor Watch
    c.glass_card(402, 52, 384, 356)
    c.caption("ANCHOR WATCH (signalk-anchoralarm)", 414, 62)
    c.inset_box(412, 80, 364, 170)
    c.caption("ANCHOR ALARM STATUS", 422, 88)
    c.text("STANDBY (UNDER WAY)", 422, 106, size=22, color="iceBlue", bold=True)

    c.caption("CURRENT DRIFT RADIUS", 422, 144)
    c.text("12.4 m", 422, 160, size=22, color="champagne", bold=True)
    c.caption("MAX ALLOWED RADIUS", 600, 144)
    c.text("35.0 m", 600, 160, size=22, color="text", bold=True)

    c.caption("SWING DRIFT BEARING", 422, 196)
    c.text("215° SW", 422, 212, size=20, color="iceBlue", bold=True)

    c.inset_box(412, 258, 364, 120)
    c.caption("ANCHOR WATCH TELEMETRY", 422, 266)
    c.text("GPS Geofence Guard Active\nMonitors True Wind shift & Swing Arc\nAudio/Visual Alert on Drag", 422, 288, size=12, color="muted")

    c.save("05_tactical")


def render_system() -> None:
    c = Canvas()
    c.top_bar("RECORDED 5.0x")
    c.floating_dock("SYSTEM")

    # Left: Trip Log & Calibration
    c.glass_card(14, 52, 400, 356, raised=True)
    c.caption("LOGBOOK & TRANSDUCER CALIBRATION", 26, 62)
    c.inset_box(24, 80, 380, 96)
    c.caption("TRIP LOG (navigation.trip.log)", 34, 88)
    c.text("24.8 NM", 34, 106, size=26, color="champagne", bold=True)
    c.caption("TOTAL ODOMETER", 220, 88)
    c.text("3864.8 NM", 220, 106, size=22, color="text", bold=True)

    c.inset_box(24, 184, 380, 96)
    c.caption("KEEL OFFSET (belowTransducer)", 34, 192)
    c.text("-1.40 m", 34, 210, size=22, color="iceBlue", bold=True)
    c.caption("STW CORRECTION FACTOR", 220, 192)
    c.text("x1.055 (+5.5%)", 220, 210, size=22, color="mint", bold=True)

    # Replay Controls
    c.text("RECORDED LOG • ACTIVE", 24, 288, size=14, color="champagne", bold=True)
    c.text("Signal K Replay Feed", 24, 306, size=12, color="muted")
    # Slider
    c.draw.rounded_rectangle((c.q(24), c.q(326), c.q(394), c.q(334)), radius=c.q(4), fill=rgb(COLORS["border"]) + (200,))
    c.draw.rounded_rectangle((c.q(24), c.q(326), c.q(240), c.q(334)), radius=c.q(4), fill=rgb(COLORS["champagne"]) + (255,))
    c.text("04:32 / 12:00", 24, 342, size=11, color="muted", bold=True)

    # Right: Diagnostics
    c.glass_card(424, 52, 362, 356)
    c.caption("NMEA2000 & DISPLAY CONTROLS", 436, 62)

    # Action buttons
    for x, lbl in [(436, "NIGHT FILTER"), (554, "DIM 100%"), (670, "SCREEN OFF")]:
        c.draw.rounded_rectangle((c.q(x), c.q(80), c.q(x + 104), c.q(118)), radius=c.q(10), fill=rgb(COLORS["deep"]) + (200,), outline=rgb(COLORS["borderLight"]) + (200,), width=c.q(1))
        c.text(lbl, x + 52, 94, size=11, color="text", bold=True, align="center")

    rows = [
        ("FLASH STORAGE", "1.2 / 16.0 MB", "cyan"),
        ("PSRAM AVAILABLE", "7.8 MB", "purple"),
        ("FREE HEAP", "294 KB", "mint"),
        ("UPTIME", "0d 03:42", "text"),
        ("FIRMWARE", "0.2.0-elixir2", "champagne"),
    ]
    for i, (k, v, col) in enumerate(rows):
        y = 136 + i * 32
        c.caption(k, 436, y)
        c.text(v, 760, y - 2, size=13, color=col, bold=True, align="right")
        c.draw.line((c.q(436), c.q(y + 20), c.q(760), c.q(y + 20)), fill=rgb(COLORS["border"]) + (140,), width=c.q(1))

    c.inset_box(436, 304, 338, 76)
    c.text("Active Nodes: gWind(.0) DST(.35) Compass(.1)\nPlotter/AIS(.43) GPS(.2) Venus MQTT(.20)\nSignal K Server: 192.168.50.10", 446, 316, size=11, color="muted")

    c.save("06_system")


def render_overview_composite() -> None:
    files = ["01_overview.png", "02_sail.png", "03_power.png", "04_engine.png", "05_tactical.png", "06_system.png"]
    images = [Image.open(OUT / f) for f in files]
    cols, rows = 3, 2
    thumb_w, thumb_h = 380, 228
    gap = 20
    comp_w = cols * thumb_w + (cols + 1) * gap
    comp_h = rows * thumb_h + (rows + 1) * gap + 40

    comp = Image.new("RGB", (comp_w, comp_h), rgb(COLORS["deep"]))
    d = ImageDraw.Draw(comp)
    font_path = FONT_BOLD or FONT_REGULAR
    title_font = ImageFont.truetype(font_path, 24) if font_path else ImageFont.load_default()
    d.text((gap, 12), "ELIXIR2 HMI • 6 OPERATIONAL MODES (JEANNEAU SUN FAST 36)", font=title_font, fill=rgb(COLORS["champagne"]))

    for i, img in enumerate(images):
        r, c_idx = i // cols, i % cols
        x = gap + c_idx * (thumb_w + gap)
        y = 48 + r * (thumb_h + gap)
        thumb = img.resize((thumb_w, thumb_h), Image.Resampling.LANCZOS)
        comp.paste(thumb, (x, y))
        d.rectangle((x, y, x + thumb_w, y + thumb_h), outline=rgb(COLORS["borderLight"]), width=1)

    comp.save(OUT / "overview.png", quality=95)


def main() -> None:
    render_overview()
    render_sail()
    render_power()
    render_engine()
    render_tactical()
    render_system()
    render_overview_composite()
    print("Rendered all 7 preview images in preview/")


if __name__ == "__main__":
    main()
