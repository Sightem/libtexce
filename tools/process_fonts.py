import argparse
import os
import shutil
import subprocess
from PIL import Image

# =============================================================================
# 1. CONFIGURATION & SYMBOL MAP
# =============================================================================

# Grid Config (Universal for all our BMPs)
CELL_SIZE = 20
COLS = 16
BMP_RED_LINE_Y = 15  # The row index of the red baseline in the BMP cell

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)

# Low-Index Math Symbols (0x01 - 0x10)
# These replace the old cursor definitions
LOW_SYMBOLS = [
    # Position 0x01-0x10 (indices 1-16)
    0x222A,  # ∪ Union
    0x2229,  # ∩ Intersection
    0x2209,  # ∉ Not element of (notin)
    0x2205,  # ∅ Empty set
    0x2200,  # ∀ For all
    0x2203,  # ∃ Exists
    0x2286,  # ⊆ Subset or equal
    0x2261,  # ≡ Equivalent (identical to)
    0x223C,  # ∼ Similar (tilde operator)
    0x2245,  # ≅ Congruent (approximately equal)
    0x221D,  # ∝ Proportional to
    0x22A5,  # ⊥ Perpendicular (up tack)
    0x2225,  # ∥ Parallel to
    0x2220,  # ∠ Angle
    0x2218,  # ∘ Ring operator (composition)
    0x2295,  # ⊕ Circled plus (oplus)
]

# The Master Symbol List (0x80 - 0xBC)
SYMBOLS = [
    # Greek Lower
    0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8,
    0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF, 0x03C0,
    0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9,
    # Greek Upper
    0x0393, 0x0394, 0x0398, 0x039B, 0x039E, 0x03A0, 0x03A3, 0x03A6, 0x03A8, 0x03A9,
    # Calculus
    0x2202, 0x221E, 0x2207, 0x2032, 0x2113, 0x210F, 0x00B0,
    # Big Ops
    0x222B, 0x2211, 0x220F,
    # Operators
    0x00B1, 0x2213, 0x22C5, 0x00D7, 0x00F7, 0x2248, 0x2260, 0x2264, 0x2265,
    # Arrows
    0x2192, 0x2190,
    # Logic/Sets
    0x2208, 0x2282, 0x2234, 0x27E8, 0x27E9,
    # Structural
    0x221A
]

# Font Profiles
PROFILES = {
    "main": {
        "bmp": os.path.join(TOOLS_DIR, "tex_guide_16px.bmp"),
        "out_txt": os.path.join(TOOLS_DIR, "tex_main.txt"),
        "bin": os.path.join(TOOLS_DIR, "tex_main.bin"),
        "pack_name": "TeXFonts",
        "display_name": "TeX Main",
        "description": "Main 16px Math Font",
        "convfont_args": ["-w", "normal", "-s", "sans-serif"],
        "height": 16,
        "baseline": 13, # Pixel 13 is the baseline
        "padding": 1    # Add 1px space to right
    },
    "script": {
        "bmp": os.path.join(TOOLS_DIR, "tex_script_12px_guided.bmp"),
        "out_txt": os.path.join(TOOLS_DIR, "tex_script.txt"),
        "bin": os.path.join(TOOLS_DIR, "tex_script.bin"),
        "pack_name": "TeXScrpt",
        "display_name": "TeX Script",
        "description": "Script 12px Math Font",
        "convfont_args": ["-w", "normal"],
        "height": 12,
        "baseline": 10, # Pixel 10 is the baseline
        "padding": 1    # Add 1px space to right
    }
}

# =============================================================================
# 2. CORE LOGIC: EXTRACT GLYPH & METRICS
# =============================================================================

def extract_glyph(img, cell_x, cell_y, profile):
    font_h = profile["height"]
    base_off = profile["baseline"]
    padding = profile["padding"]
    
    # Calculate Y alignment
    # We align the Font's logical baseline to the BMP's visual Red Line (Row 15)
    start_y = cell_y + (BMP_RED_LINE_Y - base_off)
    
    pixels = set()
    min_x = CELL_SIZE
    max_x = -1
    
    # Scan the defined font height area
    for row in range(font_h):
        bmp_y = start_y + row
        for x in range(CELL_SIZE):
            bmp_x = cell_x + x
            
            # Safety check
            if bmp_x >= img.width or bmp_y >= img.height: continue
            
            px = img.getpixel((bmp_x, bmp_y))
            
            # STRICT WHITE CHECK
            # We ignore Cyan (guides), Red (baseline), Gray (grid), Black (bg)
            is_white = False
            if isinstance(px, int): is_white = (px == 1) # Palette index 1
            elif isinstance(px, tuple): is_white = (px == (255,255,255))
            
            if is_white:
                pixels.add((x, row))
                if x < min_x: min_x = x
                if x > max_x: max_x = x
                
    # Handle Empty Glyph (Space)
    if not pixels:
        final_w = 4 # Default space width
        return final_w, [" " * final_w] * font_h
        
    # Calculate Width + Padding
    content_w = (max_x - min_x) + 1
    final_w = content_w + padding
    
    # Build String Rows
    output_rows = []
    for row in range(font_h):
        row_str = ""
        # Draw pixels
        for x in range(content_w):
            if (min_x + x, row) in pixels:
                row_str += "#"
            else:
                row_str += " "
        # Add padding spaces
        row_str += " " * padding
        output_rows.append(row_str)
        
    return final_w, output_rows

def get_math_axis_height(img, cell_x, cell_y, profile):
    """
    Scans a specific glyph (usually '+') to find its visual vertical center.
    Returns the distance (in pixels) from the baseline to that center.
    """
    font_h = profile["height"]
    base_off = profile["baseline"]
    
    # Calculate Y alignment logic identical to extract_glyph
    start_y = cell_y + (BMP_RED_LINE_Y - base_off)
    
    min_y = font_h  # Initialize outside bounds
    max_y = -1
    found_pixels = False

    # Scan the target glyph area
    for row in range(font_h):
        bmp_y = start_y + row
        for x in range(CELL_SIZE):
            bmp_x = cell_x + x
            if bmp_x >= img.width or bmp_y >= img.height: continue
            
            px = img.getpixel((bmp_x, bmp_y))
            
            # Use same strict white detection
            is_white = False
            if isinstance(px, int): is_white = (px == 1)
            elif isinstance(px, tuple): is_white = (px == (255,255,255))
            
            if is_white:
                if row < min_y: min_y = row
                if row > max_y: max_y = row
                found_pixels = True

    if not found_pixels:
        print("    Warning: Math axis reference glyph is empty. Using heuristic.")
        return int(font_h / 2)

    # Calculate visual center row relative to the top of the glyph box
    center_y = (min_y + max_y) / 2.0
    
    # Math Axis = Distance from Baseline to Center
    # Since 'row' increases downwards, and baseline is at 'base_off' (e.g., 13):
    # If center is at row 6, axis = 13 - 6 = 7.
    axis_height = base_off - center_y
    
    return int(round(axis_height))

# =============================================================================
# 3. OPERATION: EXPORT TO TXT (For convfont)
# =============================================================================

def perform_export():
    print("--- STARTING EXPORT ---")
    
    for key, p in PROFILES.items():
        print(f"Processing Profile: {key.upper()}...")
        img = Image.open(p["bmp"])
            
        font_h = p["height"]
        
        # --- CALCULATE DYNAMIC MATH AXIS ---
        # We use the '+' symbol (ASCII 43) as the reference for the math axis.
        plus_idx = 43
        p_cell_x = (plus_idx % COLS) * CELL_SIZE
        p_cell_y = (plus_idx // COLS) * CELL_SIZE
        
        math_axis = get_math_axis_height(img, p_cell_x, p_cell_y, p)
        print(f"  [Calculated] Math Axis (based on '+'): {math_axis}px from baseline")
        # -----------------------------------

        with open(p["out_txt"], "w", encoding="utf-8") as f:
            # Write Header
            f.write("convfont\n")
            f.write(f": Generated from {p['bmp']}\n")
            f.write(f"Height: {font_h}\n")
            f.write("Double width: false\n")
            f.write("Code page: Win-1252\n")
            f.write(f"Baseline: {p['baseline']}\n")
            f.write(f"Cap height: {int(font_h * 0.7)}\n")
            f.write(f"x-height: {math_axis}\n")
            f.write("Space above: 0\n")
            f.write("Space below: 0\n")
            f.write("Font Data:\n")
            
            def write_entry(idx, uni_cp, cell_x, cell_y):
                w, data = extract_glyph(img, cell_x, cell_y, p)
                f.write(f"Code point: {idx}\n")
                if idx == 32: f.write("Name: Space\n")
                elif uni_cp: f.write(f"Unicode: U+{uni_cp:08X}\n")
                f.write(f"Width: {w}\nData:\n")
                for row in data: f.write(row + "\n")
                f.write("\n")

            # 0. Low-index math symbols (0x01-0x10)
            for i, uni_cp in enumerate(LOW_SYMBOLS):
                idx = 1 + i  # Start at position 1
                write_entry(idx, uni_cp, (idx % COLS) * CELL_SIZE, (idx // COLS) * CELL_SIZE)

            # 0.5. Fill Gap (0x11 - 0x1F) with empty glyphs to satisfy convfont
            # These are currently unused in the font definition
            for idx in range(17, 32):
                f.write(f"Code point: {idx}\n")
                f.write("Width: 4\n") # Standard empty width
                f.write("Data:\n")
                # Write blank rows matching the current profile height
                blank_row = "    " # 4 spaces
                for _ in range(font_h):
                    f.write(blank_row + "\n")
                f.write("\n")

            # 1. ASCII 32-127
            for cp in range(32, 128):
                write_entry(cp, cp, (cp % COLS) * CELL_SIZE, (cp // COLS) * CELL_SIZE)

            # 2. Custom Symbols 128+
            for i, uni_cp in enumerate(SYMBOLS):
                idx = 128 + i
                write_entry(idx, uni_cp, (idx % COLS) * CELL_SIZE, (idx // COLS) * CELL_SIZE)
        
        print(f"  -> Wrote {p['out_txt']}")
    print("--- EXPORT COMPLETE ---")

# =============================================================================
# 4. OPERATION: BUILD 8XV
# =============================================================================

def require_tool(name):
    if shutil.which(name) is None:
        raise SystemExit(f"Error: Required tool '{name}' not found in PATH.")

def run_cmd(args):
    print(f"Running: {' '.join(args)}")
    subprocess.run(args, check=True)

def perform_build(outdir, keep_bin):
    require_tool("convfont")
    require_tool("convbin")

    if not os.path.isabs(outdir):
        outdir = os.path.join(ROOT_DIR, outdir)

    os.makedirs(outdir, exist_ok=True)

    for key, p in PROFILES.items():
        print(f"Building Pack: {key.upper()}...")
        convfont_cmd = [
            "convfont",
            "-o", "fontpack",
            "-N", p["display_name"],
            "-P", "Win-1252",
            "-D", p["description"],
            "-t", p["out_txt"],
            "-a", "0",
        ] + p["convfont_args"] + [p["bin"]]
        run_cmd(convfont_cmd)

        out_path = os.path.join(outdir, f"{p['pack_name']}.8xv")
        convbin_cmd = [
            "convbin",
            "-r",
            "-k", "8xv",
            "-n", p["pack_name"],
            "-i", p["bin"],
            "-o", out_path,
        ]
        run_cmd(convbin_cmd)

        if not keep_bin:
            os.remove(p["bin"])

        print(f"  -> Wrote {out_path}")

    print("--- BUILD COMPLETE ---")

# =============================================================================
# 5. CLI INTERFACE
# =============================================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TeX Font Build Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)
    
    # Command: Export
    parser_exp = subparsers.add_parser("export", help="Convert BMPs to .txt for convfont")
    
    # Command: Build
    parser_build = subparsers.add_parser("build", help="Convert .txt to .8xv using convfont/convbin")
    parser_build.add_argument("--outdir", default="assets", help="Output directory for .8xv files")
    parser_build.add_argument("--keep-bin", action="store_true", help="Keep intermediate .bin files")

    # Command: Export + Build
    parser_all = subparsers.add_parser("all", help="Export then build")
    parser_all.add_argument("--outdir", default="assets", help="Output directory for .8xv files")
    parser_all.add_argument("--keep-bin", action="store_true", help="Keep intermediate .bin files")

    args = parser.parse_args()
    
    if args.command == "export":
        perform_export()

    elif args.command == "build":
        perform_build(args.outdir, args.keep_bin)

    elif args.command == "all":
        perform_export()
        perform_build(args.outdir, args.keep_bin)
