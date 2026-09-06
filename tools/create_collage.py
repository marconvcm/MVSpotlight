#!/usr/bin/env python3
"""
tools/create_collage.py
Generates polished, high-impact feature collages for MVSpotlight social media posts
(LinkedIn, X/Twitter, GitHub releases).
"""

import math
import os
import subprocess
from PIL import Image, ImageDraw, ImageFilter, ImageFont

CANVAS_WIDTH = 2400
CANVAS_HEIGHT = 1350

FONT_BOLD = "/usr/share/fonts/julietaula-montserrat-fonts/Montserrat-Bold.otf"
FONT_SEMIBOLD = "/usr/share/fonts/julietaula-montserrat-fonts/Montserrat-SemiBold.otf"
FONT_REGULAR = "/usr/share/fonts/julietaula-montserrat-fonts/Montserrat-Regular.otf"

def draw_star(draw, cx, cy, r_outer, r_inner, fill_color):
    """Draws a clean 5-pointed vector star."""
    points = []
    for i in range(10):
        angle = i * math.pi / 5 - math.pi / 2
        r = r_outer if i % 2 == 0 else r_inner
        points.append((cx + r * math.cos(angle), cy + r * math.sin(angle)))
    draw.polygon(points, fill=fill_color)

def round_corners(im, radius):
    """Apply rounded corners with anti-aliasing to an RGBA image."""
    mask = Image.new("L", (im.width * 4, im.height * 4), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle([0, 0, im.width * 4 - 1, im.height * 4 - 1], radius=radius * 4, fill=255)
    mask = mask.resize(im.size, Image.Resampling.LANCZOS)
    result = im.copy()
    result.putalpha(mask)
    return result

def add_window_frame(im, radius=18, shadow_blur=28, shadow_offset=(0, 12),
                     shadow_alpha=140, border_color=(255, 255, 255, 38)):
    """Wraps an image in rounded corners, 1px subtle border, and realistic drop shadow."""
    rounded = round_corners(im, radius)
    
    pad = shadow_blur * 2 + abs(shadow_offset[1]) + 10
    shadow_w = rounded.width + pad * 2
    shadow_h = rounded.height + pad * 2
    
    s_mask = Image.new("L", (shadow_w, shadow_h), 0)
    s_draw = ImageDraw.Draw(s_mask)
    s_x0 = pad + shadow_offset[0]
    s_y0 = pad + shadow_offset[1]
    s_draw.rounded_rectangle(
        [s_x0, s_y0, s_x0 + rounded.width - 1, s_y0 + rounded.height - 1],
        radius=radius,
        fill=shadow_alpha
    )
    s_mask = s_mask.filter(ImageFilter.GaussianBlur(shadow_blur))
    
    frame = Image.new("RGBA", (shadow_w, shadow_h), (0, 0, 0, 0))
    shadow_layer = Image.new("RGBA", (shadow_w, shadow_h), (0, 0, 0, 255))
    shadow_layer.putalpha(s_mask)
    frame.paste(shadow_layer, (0, 0), shadow_layer)
    
    win_x = pad
    win_y = pad
    frame.paste(rounded, (win_x, win_y), rounded)
    
    b_layer = Image.new("RGBA", (shadow_w, shadow_h), (0, 0, 0, 0))
    b_draw = ImageDraw.Draw(b_layer)
    b_draw.rounded_rectangle(
        [win_x, win_y, win_x + rounded.width - 1, win_y + rounded.height - 1],
        radius=radius,
        outline=border_color,
        width=1
    )
    frame.paste(b_layer, (0, 0), b_layer)
    
    return frame, (win_x, win_y)

def create_background(w, h):
    """Generates a deep dark slate canvas with subtle ambient glows and micro-grid."""
    canvas = Image.new("RGBA", (w, h), (16, 20, 28, 255))
    draw = ImageDraw.Draw(canvas)
    
    for y in range(h):
        ratio = y / h
        r = int(14 + ratio * (22 - 14))
        g = int(18 + ratio * (26 - 18))
        b = int(28 + ratio * (36 - 28))
        draw.line([(0, y), (w, y)], fill=(r, g, b, 255))
    
    glow_canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_canvas)
    
    glow_draw.ellipse([int(w * 0.08), int(h * 0.1), int(w * 0.55), int(h * 0.85)], fill=(53, 132, 228, 30))
    glow_draw.ellipse([int(w * 0.50), int(h * 0.35), int(w * 0.95), int(h * 0.95)], fill=(154, 76, 220, 26))
    glow_draw.ellipse([int(w * 0.12), int(h * 0.55), int(w * 0.45), int(h * 1.05)], fill=(46, 194, 126, 18))
    
    glow_blurred = glow_canvas.filter(ImageFilter.GaussianBlur(140))
    canvas = Image.alpha_composite(canvas, glow_blurred)
    
    grid_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    grid_draw = ImageDraw.Draw(grid_layer)
    dot_spacing = 48
    for gx in range(40, w, dot_spacing):
        for gy in range(40, h, dot_spacing):
            grid_draw.point((gx, gy), fill=(255, 255, 255, 14))
    canvas = Image.alpha_composite(canvas, grid_layer)
    
    return canvas

def draw_pill(draw, xy, text, font, bg_color, text_color, border_color):
    """Draws a pill tag badge with solid background and crisp border."""
    x, y = xy
    bbox = font.getbbox(text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    
    padx = 16
    pady = 8
    w = tw + padx * 2
    h = th + pady * 2
    
    r = h // 2
    draw.rounded_rectangle([x, y, x + w, y + h], radius=r, fill=bg_color, outline=border_color, width=1)
    draw.text((x + padx, y + pady - bbox[1]), text, font=font, fill=text_color)
    return w, h

def draw_header_badge(draw, text, x, y, font, color=(53, 132, 228)):
    """Draws a section title badge with a small glowing bullet."""
    draw.ellipse([x, y + 4, x + 10, y + 14], fill=color)
    draw.text((x + 18, y), text, font=font, fill=color)

def generate_landscape_collage():
    print("Generating High-Resolution Landscape Collage (2400x1350)...")
    
    icon_png = "/tmp/mvspotlight_icon_128.png"
    subprocess.run([
        "magick", "-background", "none", "-size", "128x128",
        "assets/icons/mvspotlight.svg", icon_png
    ], check=True)
    icon_img = Image.open(icon_png).convert("RGBA")
    
    canvas = create_background(CANVAS_WIDTH, CANVAS_HEIGHT)
    draw = ImageDraw.Draw(canvas)
    
    font_brand = ImageFont.truetype(FONT_BOLD, 54)
    font_tagline = ImageFont.truetype(FONT_SEMIBOLD, 24)
    font_badge = ImageFont.truetype(FONT_BOLD, 15)
    font_sec = ImageFont.truetype(FONT_BOLD, 17)
    font_sec_desc = ImageFont.truetype(FONT_REGULAR, 15)
    font_footer = ImageFont.truetype(FONT_SEMIBOLD, 18)
    
    # -------------------------------------------------------------
    # HEADER SECTION
    # -------------------------------------------------------------
    header_y = 50
    icon_size = 84
    icon_resized = icon_img.resize((icon_size, icon_size), Image.Resampling.LANCZOS)
    canvas.paste(icon_resized, (72, header_y), icon_resized)
    
    title_x = 72 + icon_size + 24
    draw.text((title_x, header_y + 4), "MVSpotlight", font=font_brand, fill=(255, 255, 255, 255))
    draw.text(
        (title_x, header_y + 58),
        "macOS Spotlight & Raycast Experience for Linux • GNOME 50 & Wayland Native",
        font=font_tagline,
        fill=(160, 175, 200, 255)
    )
    
    pills = [
        ("v1.0.0 RELEASE", (20, 55, 35, 220), (58, 230, 140), (46, 194, 126, 160)),
        ("PURE WAYLAND", (20, 42, 70, 220), (130, 195, 255), (53, 132, 228, 160)),
        ("C++20 / Qt 6", (24, 45, 60, 220), (120, 215, 255), (50, 150, 210, 160)),
        ("LIBADWAITA", (45, 25, 65, 220), (220, 175, 255), (154, 76, 220, 160)),
        ("LUA 5.4 PLUGINS", (60, 35, 15, 220), (255, 180, 100), (230, 120, 20, 160)),
        ("AI ASSISTANT", (65, 20, 35, 220), (255, 150, 170), (224, 60, 90, 160)),
    ]
    
    pill_y = header_y + 22
    curr_x = CANVAS_WIDTH - 72
    for text, bg, fg, border in reversed(pills):
        bbox = font_badge.getbbox(text)
        pw = (bbox[2] - bbox[0]) + 32
        curr_x -= pw
        draw_pill(draw, (curr_x, pill_y), text, font_badge, bg, fg, border)
        curr_x -= 12
    
    div_y = header_y + 104
    draw.line([(72, div_y), (CANVAS_WIDTH - 72, div_y)], fill=(255, 255, 255, 20), width=1)
    
    # -------------------------------------------------------------
    # MAIN CONTENT GRID
    # -------------------------------------------------------------
    col_left_x = 72
    col_right_x = 1260
    body_y = div_y + 26
    
    # Left Feature: Native AI Assistant
    draw_header_badge(draw, "AI COMMAND PALETTE  (Prefix '> prompt')", col_left_x, body_y, font_sec, (120, 185, 255))
    draw.text(
        (col_left_x + 405, body_y + 1),
        "— Ollama, Gemini, Claude, OpenAI with markdown & code syntax",
        font=font_sec_desc,
        fill=(140, 155, 175)
    )
    
    ai_img = Image.open("assets/screenshots/ai_chat.png").convert("RGBA")
    ai_bbox = ai_img.getbbox()
    ai_cropped = ai_img.crop((ai_bbox[0] + 12, ai_bbox[1] + 12, ai_bbox[2] - 12, ai_bbox[3] - 12))
    left_target_w = 1130
    ai_ratio = left_target_w / ai_cropped.width
    ai_target_h = int(ai_cropped.height * ai_ratio)
    ai_resized = ai_cropped.resize((left_target_w, ai_target_h), Image.Resampling.LANCZOS)
    
    ai_framed, (ai_ox, ai_oy) = add_window_frame(ai_resized, radius=16, shadow_blur=32, shadow_offset=(0, 14), shadow_alpha=160)
    canvas.paste(ai_framed, (col_left_x - ai_ox, body_y + 30 - ai_oy), ai_framed)
    
    # Right Top Feature: Instant Calculation & Currency Exchange
    draw_header_badge(draw, "FAST EXTENSIONS & LUA PLUGINS", col_right_x, body_y, font_sec, (46, 220, 130))
    draw.text(
        (col_right_x + 325, body_y + 1),
        "— Sub-50ms math, currency, unit conversions & web search",
        font=font_sec_desc,
        fill=(140, 155, 175)
    )
    
    curr_img = Image.open("assets/screenshots/search_currency.png").convert("RGBA")
    curr_bbox = curr_img.getbbox()
    curr_cropped = curr_img.crop((curr_bbox[0] + 16, curr_bbox[1] + 16, curr_bbox[2] - 16, curr_bbox[3] - 16))
    right_target_w = 1068
    curr_ratio = right_target_w / curr_cropped.width
    curr_target_h = int(curr_cropped.height * curr_ratio)
    curr_resized = curr_cropped.resize((right_target_w, curr_target_h), Image.Resampling.LANCZOS)
    
    curr_framed, (c_ox, c_oy) = add_window_frame(curr_resized, radius=16, shadow_blur=26, shadow_offset=(0, 10), shadow_alpha=140)
    canvas.paste(curr_framed, (col_right_x - c_ox, body_y + 30 - c_oy), curr_framed)
    
    # Right Bottom Feature: Modern Libadwaita Preferences
    pref_badge_y = body_y + 30 + curr_target_h + 38
    draw_header_badge(draw, "GNOME LIBADWAITA SETTINGS", col_right_x, pref_badge_y, font_sec, (215, 165, 255))
    draw.text(
        (col_right_x + 300, pref_badge_y + 1),
        "— Dynamic accent colors, dark/light themes, and AI provider configs",
        font=font_sec_desc,
        fill=(140, 155, 175)
    )
    
    pref_img = Image.open("assets/screenshots/preferences_appearance.png").convert("RGBA")
    avail_h = (CANVAS_HEIGHT - 54 - 18) - (pref_badge_y + 30) - 20
    pref_ratio_h = avail_h / pref_img.height
    pref_ratio_w = right_target_w / pref_img.width
    pref_ratio = min(pref_ratio_w, pref_ratio_h)
    pref_target_w = int(pref_img.width * pref_ratio)
    pref_target_h = int(pref_img.height * pref_ratio)
    pref_resized = pref_img.resize((pref_target_w, pref_target_h), Image.Resampling.LANCZOS)
    
    pref_framed, (p_ox, p_oy) = add_window_frame(pref_resized, radius=16, shadow_blur=30, shadow_offset=(0, 12), shadow_alpha=150)
    pref_x_offset = (right_target_w - pref_target_w) // 2
    canvas.paste(pref_framed, (col_right_x + pref_x_offset - p_ox, pref_badge_y + 30 - p_oy), pref_framed)
    
    # -------------------------------------------------------------
    # FOOTER BAR
    # -------------------------------------------------------------
    footer_y = CANVAS_HEIGHT - 52
    draw.line([(72, footer_y - 18), (CANVAS_WIDTH - 72, footer_y - 18)], fill=(255, 255, 255, 20), width=1)
    
    draw_star(draw, 84, footer_y + 9, 10, 4.5, (255, 205, 50, 255))
    draw.text((104, footer_y), "GitHub: github.com/marconvcm/MVSpotlight", font=font_footer, fill=(255, 255, 255, 230))
    
    stack_text = "Free & Open Source (MIT) • RPM & DEB Available • Ready for Fedora 41+ & Ubuntu 24.04+"
    bbox_s = font_footer.getbbox(stack_text)
    draw.text((CANVAS_WIDTH - 72 - (bbox_s[2] - bbox_s[0]), footer_y), stack_text, font=font_footer, fill=(160, 175, 200, 255))
    
    out_png = "assets/screenshots/linkedin_collage.png"
    canvas.save(out_png, "PNG", quality=100, optimize=True)
    print(f"✓ High-res landscape saved: {out_png} ({CANVAS_WIDTH}x{CANVAS_HEIGHT})")
    
    out_1080p = "assets/screenshots/linkedin_collage_1080p.png"
    canvas_1080p = canvas.resize((1920, 1080), Image.Resampling.LANCZOS)
    canvas_1080p.save(out_1080p, "PNG", quality=95, optimize=True)
    print(f"✓ 1080p landscape saved: {out_1080p} (1920x1080)")

def generate_square_collage():
    print("Generating High-Resolution Square Collage (1200x1200)...")
    SQ_SIZE = 1200
    canvas = create_background(SQ_SIZE, SQ_SIZE)
    draw = ImageDraw.Draw(canvas)
    
    font_brand = ImageFont.truetype(FONT_BOLD, 38)
    font_tagline = ImageFont.truetype(FONT_SEMIBOLD, 17)
    font_sec = ImageFont.truetype(FONT_BOLD, 15)
    font_footer = ImageFont.truetype(FONT_SEMIBOLD, 15)
    
    icon_png = "/tmp/mvspotlight_icon_128.png"
    icon_img = Image.open(icon_png).convert("RGBA")
    
    # Header
    hy = 36
    isize = 58
    ires = icon_img.resize((isize, isize), Image.Resampling.LANCZOS)
    canvas.paste(ires, (48, hy), ires)
    
    tx = 48 + isize + 16
    draw.text((tx, hy + 2), "MVSpotlight", font=font_brand, fill=(255, 255, 255, 255))
    draw.text(
        (tx, hy + 38),
        "macOS Spotlight & AI Palette for Linux • Wayland Native",
        font=font_tagline,
        fill=(160, 175, 200, 255)
    )
    
    div_y = hy + 72
    draw.line([(48, div_y), (SQ_SIZE - 48, div_y)], fill=(255, 255, 255, 20), width=1)
    
    # Top Feature: Currency / Search pill
    # In square mode, a compact launcher card looks great at top
    draw_header_badge(draw, "INSTANT EXTENSIONS & PLUGINS", 48, div_y + 14, font_sec, (46, 220, 130))
    curr_img = Image.open("assets/screenshots/search_currency.png").convert("RGBA")
    curr_bbox = curr_img.getbbox()
    curr_crop = curr_img.crop((curr_bbox[0] + 16, curr_bbox[1] + 16, curr_bbox[2] - 16, curr_bbox[3] - 16))
    tw = SQ_SIZE - 96
    c_ratio = tw / curr_crop.width
    curr_res = curr_crop.resize((tw, int(curr_crop.height * c_ratio)), Image.Resampling.LANCZOS)
    c_frame, (cx, cy_off) = add_window_frame(curr_res, radius=14, shadow_blur=18, shadow_offset=(0, 6), shadow_alpha=130)
    canvas.paste(c_frame, (48 - cx, div_y + 36 - cy_off), c_frame)
    
    # Middle Feature: AI Assistant Card (Scaled to fit remaining vertical space perfectly)
    ai_top = div_y + 36 + curr_res.height + 24
    draw_header_badge(draw, "AI COMMAND PALETTE (Ollama • Gemini • Claude • OpenAI)", 48, ai_top, font_sec, (255, 150, 170))
    
    fy = SQ_SIZE - 40
    avail_ai_h = (fy - 14) - (ai_top + 24) - 10
    
    ai_img = Image.open("assets/screenshots/ai_chat.png").convert("RGBA")
    ai_bbox = ai_img.getbbox()
    ai_crop = ai_img.crop((ai_bbox[0] + 12, ai_bbox[1] + 12, ai_bbox[2] - 12, ai_bbox[3] - 12))
    
    ai_scale_w = tw / ai_crop.width
    ai_scale_h = avail_ai_h / ai_crop.height
    ai_scale = min(ai_scale_w, ai_scale_h)
    
    ai_w = int(ai_crop.width * ai_scale)
    ai_h = int(ai_crop.height * ai_scale)
    ai_res = ai_crop.resize((ai_w, ai_h), Image.Resampling.LANCZOS)
    
    ai_frame, (ax, ay_off) = add_window_frame(ai_res, radius=14, shadow_blur=22, shadow_offset=(0, 8), shadow_alpha=140)
    ai_x = 48 + (tw - ai_w) // 2
    canvas.paste(ai_frame, (ai_x - ax, ai_top + 24 - ay_off), ai_frame)
    
    # Bottom Footer
    draw.line([(48, fy - 10), (SQ_SIZE - 48, fy - 10)], fill=(255, 255, 255, 20), width=1)
    draw_star(draw, 56, fy + 7, 7, 3, (255, 205, 50, 255))
    draw.text((70, fy), "github.com/marconvcm/MVSpotlight", font=font_footer, fill=(255, 255, 255, 230))
    
    stack_text = "Free & Open Source (MIT) • C++20 • Qt 6 • Libadwaita"
    bbox_s = font_footer.getbbox(stack_text)
    draw.text((SQ_SIZE - 48 - (bbox_s[2] - bbox_s[0]), fy), stack_text, font=font_footer, fill=(160, 175, 200, 255))
    
    out_sq = "assets/screenshots/linkedin_square.png"
    canvas.save(out_sq, "PNG", quality=95, optimize=True)
    print(f"✓ Square collage saved: {out_sq} ({SQ_SIZE}x{SQ_SIZE})")

def main():
    generate_landscape_collage()
    generate_square_collage()

if __name__ == "__main__":
    main()
