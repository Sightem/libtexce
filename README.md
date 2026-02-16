<h1 align="center">libtexce</h1>

<p align="center">
  <img src="assets/animation.png" alt="libtexce demo animation" />
</p>

<p align="center">
  <a href="./LICENSE"><img alt="License: AGPLv3" src="https://img.shields.io/badge/license-AGPLv3-blue.svg"></a>
</p>

A LaTeX rendering engine for the TI-84 Plus CE calculator. Renders mixed text and math expressions - fractions, integrals, matrices, Greek letters, and more, directly on the calculator's hardware.

libtexce is written in C, targets the ez80 via the [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/), and can also be built natively (via PortCE + SDL2) or for the browser (via Emscripten) for development and testing.

## Quick Start

The full rendering pipeline is five steps: load fonts, configure, format, create a renderer, draw.

```c
#include <fontlibc.h>
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include "tex/tex.h"

int main(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetTransparentColor(255);

    fontlib_font_t* font_main   = fontlib_GetFontByIndex("TeXFonts", 0);
    fontlib_font_t* font_script = fontlib_GetFontByIndex("TeXScrpt", 0);
    if (!font_main || !font_script) {
        gfx_PrintStringXY("Missing font packs!", 10, 10);
        gfx_SwapDraw();
        while (!os_GetCSC());
        gfx_End();
        return 1;
    }

    // set fonts (global state, call once)
    tex_draw_set_fonts(font_main, font_script);
    fontlib_SetTransparency(true);
    fontlib_SetForegroundColor(0);
    fontlib_SetBackgroundColor(255);

    // prepare a mutable input buffer.
    // tex_format() tokenizes in place, the buffer must be writable
    // and must remain allocated as long as the layout exists
    const char* source =
        "Quadratic Formula\n"
        "$$x = \\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}$$\n"
        "\n"
        "Taylor Series\n"
        "$$f(x) \\approx f(a) + f'(a)(x-a) + \\frac{f''(a)}{2}(x-a)^2$$";

    size_t len = strlen(source);
    char* buf = malloc(len + 1);
    memcpy(buf, source, len + 1);

    // format parses the document and computes layout metrics
    TeX_Config cfg = {
        .color_fg  = 0,       // black
        .color_bg  = 255,     // white
        .font_pack = "TeXFonts",
    };
    int margin = 10;
    TeX_Layout* layout = tex_format(buf, GFX_LCD_WIDTH - margin * 2, &cfg);

    // create a renderer (manages a transient memory pool for drawing)
    TeX_Renderer* renderer = tex_renderer_create();

    int scroll_y = 0;
    int total_h  = tex_get_total_height(layout);
    int max_scroll = total_h > GFX_LCD_HEIGHT ? total_h - GFX_LCD_HEIGHT : 0;

    // draw loop
    while (true) {
        kb_Scan();
        if (kb_Data[6] & kb_Clear) break;
        if (kb_Data[7] & kb_Up)   scroll_y -= 10;
        if (kb_Data[7] & kb_Down) scroll_y += 10;
        if (scroll_y < 0) scroll_y = 0;
        if (scroll_y > max_scroll) scroll_y = max_scroll;

        gfx_FillScreen(255);
        tex_draw(renderer, layout, margin, 0, scroll_y);
        gfx_SwapDraw();
    }

    // cleanup order matters: renderer, layout, then buffer
    tex_renderer_destroy(renderer);
    tex_free(layout);
    free(buf);
    gfx_End();
    return 0;
}
```

## Real-World Usage

If you are building a notes app (the most common use case), start with
[`libtexce_notes_template`](https://github.com/Sightem/libtexce_notes_template). It gives you a ready to use
TI-84 Plus CE notes workflow with formatting and CI-built transfer artifacts

For a larger production example, see
[`matrix`](https://github.com/Sightem/matrix), a full linear algebra app that uses libtexce to render
step by step formatted math on device

## API Reference

The public API is declared in [`tex/tex.h`](src/tex/tex.h). All functions use C linkage.

### Core Lifecycle

| Function | Description |
|---|---|
| `TeX_Layout* tex_format(char* input, int width, TeX_Config* config)` | Parse a mixed text/math document and compute layout metrics. Returns `NULL` only on catastrophic failure (OOM during initialization). Check `tex_get_last_error()` for parse errors |
| `int tex_get_total_height(TeX_Layout* layout)` | Total rendered height in pixels. Use for scroll bounds. |
| `void tex_free(TeX_Layout* layout)` | Free all resources associated with a layout |

### Rendering

| Function | Description |
|---|---|
| `TeX_Renderer* tex_renderer_create(void)` | Create a renderer with the default 40 KB slab |
| `TeX_Renderer* tex_renderer_create_sized(size_t slab_size)` | Create a renderer with a custom slab size |
| `void tex_renderer_destroy(TeX_Renderer* r)` | Destroy the renderer and free its slab. |
| `void tex_draw(TeX_Renderer* r, TeX_Layout* layout, int x, int y, int scroll_y)` | Draw visible portion of the document to the current draw buffer |
| `void tex_draw_set_fonts(fontlib_font_t* main, fontlib_font_t* script)` | Set the font handles used for rendering. **Global state**, call once after loading fonts |

### Error Handling

| Function | Description |
|---|---|
| `TeX_Error tex_get_last_error(TeX_Layout* layout)` | Error code from last operation (`TEX_OK`, `TEX_ERR_OOM`, `TEX_ERR_FONT`, `TEX_ERR_PARSE`, `TEX_ERR_INPUT`, `TEX_ERR_DEPTH`). |
| `const char* tex_get_error_message(TeX_Layout* layout)` | Human readable error string (static, never `NULL`). |
| `int tex_get_error_value(TeX_Layout* layout)` | Detail value (byte offset, nesting depth, etc.) |

### Renderer Statistics

| Function | Description |
|---|---|
| `void tex_renderer_get_stats(TeX_Renderer* r, size_t* peak_used, size_t* capacity, size_t* alloc_count, size_t* reset_count)` | Query pool statistics. Pass `NULL` for stats you dont need. Useful for tuning `tex_renderer_create_sized()` |

## Configuration

```c
typedef struct {
    uint8_t      color_fg;        // Foreground color (palette index, 0-255)
    uint8_t      color_bg;        // Background color (palette index, 0-255)
    const char*  font_pack;       // Font pack name (default: "TeXFonts")
    TeX_ErrorLogFn error_callback; // Optional error/warning callback
    void*        error_userdata;   // Passed to callback
} TeX_Config;
```

Colors are 8 bit palette indices matching the graphx palette. The error callback receives a severity level (0 = info, 1 = warning, 2 = error), a message string, and in debug builds, the source file and line number where the error occurred.

## Ownership and Lifetime Rules

Understanding buffer ownership is critical for correct usage.

### The Input Buffer

`tex_format()` **tokenizes the input buffer in place**. After the call, the buffer's contents are modified (null terminators are inserted between tokens). You should consider the buffer opaque after passing it to `tex_format()`. do NOT attempt to read, modify, or reason about its contents

**the buffer must remain allocated and at the same address** for the entire lifetime of the `TeX_Layout`. This is because `tex_draw()` reparses the source text from the buffer on every frame (see [How Rendering Works](#how-rendering-works) below). The layout stores a pointer to the buffer and not a copy

**Cleanup order matters:**
```c
// correct: free in reverse order of creation
tex_renderer_destroy(renderer);
tex_free(layout);
free(buf);

// wrong: freeing buffer while layout still exists
free(buf);           // dangling pointer in layout->source
tex_free(layout);    // undefined behavior
```

### The Renderer

A `TeX_Renderer` owns a slab of memory used as a transient pool. Each call to `tex_draw()` may reset and reuse this pool. A single renderer can be shared across multiple layouts. it has no permanent binding to any particular layout.

### Summary

| Object | Owns | Must outlive |
|---|---|---|
| Input buffer (your `malloc`) | The raw text bytes | `TeX_Layout` |
| `TeX_Layout` | Checkpoint index, config copy, error state | Nothing (leaf) |
| `TeX_Renderer` | Transient slab pool | Nothing (leaf) |

## How Rendering Works

libtexce uses a **streaming two pass architecture** designed for the calculator's constrained memory.

### Pass 1: `tex_format()` Dry Run Layout

When you call `tex_format()`, the engine tokenizes and parses the entire document, measuring each lines height and accumulating the total document height. No nodes or render trees are retained, only the total height and a sparse checkpoint index are stored in the `TeX_Layout`

Checkpoints record `(y_position, source_pointer)` pairs at regular pixel intervals (~200px). These allow `tex_draw()` to jump into the middle of a long document without reparsing from the beginning

### Pass 2: `tex_draw()` Windowed Reparse

Each time `tex_draw()` is called, the renderer:

1. Checks its cache. If the scroll position falls within the previously hydrated window, the existing render tree is reused without reparsing
2. Otherwise, rehydrates. the renderer finds the nearest checkpoint before `scroll_y - padding`, reparses from that point forward, and builds a render tree covering `scroll_y +- 240px` (one screen of padding in each direction)
3. Draws the visible lines from the render tree to the current graphx draw buffer

This means the renderer only ever holds nodes for ~3 screens of content, regardless of total document length. the tradeoff is that scrolling to a completely new region triggers a reparse, but checkpoint indexing keeps this fast

### Why This Matters to You

- **Documents can be arbitrarily long** without proportional memory cost.
- **The input buffer must stay alive** because `tex_draw()` reads from it on every cache miss.
- **Renderer slab sizing** (`tex_renderer_create_sized()`) controls the upper bound on how much visible content can be rendered. The default 40 KB is too generous for most documents and you should consider using the sized init function. Use `tex_renderer_get_stats()` to measure actual usage.

## Font System and Assets

libtexce uses two custom font packs stored as appvars:

| AppVar | Description | Glyph Height |
|---|---|---|
| `TeXFonts.8xv` | Main text and math font | 16 px |
| `TeXScrpt.8xv` | Script/subscript/superscript font | 12 px |

Both must be transferred to the calculator before running any program that uses libtexce. prebuilt copies are in the [`assets/`](assets/) directory. these do not look great. Contributions are welcome

### What is in the Fonts

Each font pack provides the full ASCII range (0x20–0x7F) plus custom math symbols:

- **0x01–0x10**: Set theory symbols (∪, ∩, ∉, ∅, ∀, ∃, ⊆, ≡, ∼, ≅, ∝, ⊥, ∥, ∠, ∘, ⊕)
- **0x80–0x99**: Greek letters (α–ω, Γ, Δ, Θ, Λ, Ξ, Π, Σ, Φ, Ψ, Ω)
- **0x9A–0xA0**: Calculus (∂, ∞, ∇, ′, ℓ, ℏ, °)
- **0xA1–0xA3**: Big operators (∫, Σ, Π)
- **0xA4–0xBC**: Operators, arrows, logic, delimiters, structural glyphs

The header [`include/texfont.h`](include/texfont.h) defines named constants for all custom glyphs (e.g. `TEXFONT_alpha`, `TEXFONT_INTEGRAL_CHAR`), though you generally won' need these. the LaTeX parser maps `\alpha`, `\int`, etc. automatically

### Rebuilding Fonts

If you need to modify the fonts, the pipeline is:

1. Edit the source bitmap images in `tools/` (pixel grids with a red baseline guide)
2. Run `python tools/process_fonts.py export` to generate convfont compatible `.txt` descriptors
3. Run `python tools/process_fonts.py build` to produce the `.8xv` AppVars (requires `convfont` and `convbin` in `PATH`)

Or in one step: `python tools/process_fonts.py all`

Aseprite is recommended.

## Supported LaTeX

libtexce supports a substantial subset of LaTeX math mode. The full list is maintained in [LATEX_COMMANDS_SUPPORTED.md](LATEX_COMMANDS_SUPPORTED.md).

Highlights:

- **Fractions**: `\frac{a}{b}`, `\tfrac`, `\binom{n}{k}`
- **Roots**: `\sqrt{x}`, `\sqrt[3]{x}`
- **Scripts**: `x^2`, `x_n`, `x_i^2`
- **Greek**: `\alpha` through `\omega`, `\Gamma` through `\Omega`
- **Big operators**: `\int`, `\sum`, `\prod` with limits, plus `\iint`, `\iiint`, `\oint` variants
- **Functions**: `\sin`, `\cos`, `\lim`, `\log`, `\exp`, `\det`, `\gcd`, and many more
- **Accents**: `\hat`, `\bar`, `\vec`, `\dot`, `\tilde`, `\overline`, `\underline`
- **Decorations**: `\overbrace{...}^{label}`, `\underbrace{...}_{label}`
- **Delimiters**: `\left( ... \right)` with auto-sizing for `()`, `[]`, `\{\}`, `||`, `\langle\rangle`, `\lfloor\rfloor`, `\lceil\rceil`
- **Matrices**: `pmatrix`, `bmatrix`, `Bmatrix`, `vmatrix`, `matrix`, `array` (with column separators via `|`)
- **Spacing**: `\,`, `\:`, `\;`, `\!`, `\quad`, `\qquad`
- **Text mode**: `\text{...}` for roman text within math

Input uses standard LaTeX delimiters: `$...$` for inline math, `$$...$$` for display math (centered). everything outside `$` delimiters is rendered as plain text with automatic word wrapping

## Common Patterns

### Scrollable Document Viewer

The [Quick Start](#quick-start) example covers this pattern. Key points:
- Use `tex_get_total_height()` to compute scroll bounds
- Clamp `scroll_y` between `0` and `total_height - viewport_height`
- Pass `scroll_y` to `tex_draw()` the renderer handles windowed rendering automatically

### Multiple Layouts with a Shared Renderer

A single `TeX_Renderer` can draw different layouts on different frames. This is useful for chat style UIs:

```c
TeX_Renderer* renderer = tex_renderer_create();

// each message gets its own layout and buffer
char* buf1 = strdup("What is $E = mc^2$?");
TeX_Layout* msg1 = tex_format(buf1, width, &cfg);

char* buf2 = strdup("Einstein's mass-energy equivalence:\n$$E = mc^2$$");
TeX_Layout* msg2 = tex_format(buf2, width, &cfg);

// draw them at different positions using the same renderer
// note: scroll_y=0 since we position each message manually via the y parameter
tex_draw(renderer, msg1, x, y1, 0);
tex_draw(renderer, msg2, x, y2, 0);

// cleanup
tex_renderer_destroy(renderer);
tex_free(msg1); tex_free(msg2);
free(buf1); free(buf2);
```

> **Note:** When switching between layouts, the renderer invalidates its cache and reparses. For a scrolling view of a single layout, the cache avoids redundant work

### Error Handling

```c
TeX_Layout* layout = tex_format(buf, width, &cfg);

if (!layout) {
    // catastrophic failure (OOM during initialization)
    // cannot proceed
}

if (tex_get_last_error(layout) != TEX_OK) {
    // parse error, font error, etc.
    // the layout may still be partially renderable
    dbg_printf("TeX error: %s (code %d, detail %d)\n",
        tex_get_error_message(layout),
        tex_get_last_error(layout),
        tex_get_error_value(layout));
}
```

for richer diagnostics, use the error callback in `TeX_Config`:

```c
void my_error_handler(void* userdata, int level, const char* msg,
                      const char* file, int line) {
    (void)userdata;
    const char* prefix = level == 2 ? "ERROR" : level == 1 ? "WARN" : "INFO";
    dbg_printf("[%s] %s\n", prefix, msg);
}

TeX_Config cfg = {
    .color_fg = 0,
    .color_bg = 255,
    .font_pack = "TeXFonts",
    .error_callback = my_error_handler,
    .error_userdata = NULL,
};
```

### Tuning Renderer Memory

If you're rendering complex expressions (deeply nested fractions, large matrices) and suspect the renderer pool is too small, measure it:

```c
size_t peak, cap;
tex_renderer_get_stats(renderer, &peak, &cap, NULL, NULL);
dbg_printf("Pool: %u / %u bytes\n", (unsigned)peak, (unsigned)cap);
```

Though from real world testing, this is basically never a problem unless the input is maliciously nested.

## Building

libtexce uses CMake with presets. There are two independent build systems: the native/WASM host build (for development and testing), and the CE build (for the actual calculator)

### Prerequisites

| Target | Requirements |
|---|---|
| Native | Clang, CMake >= 3.20, Ninja, SDL2, SDL2_mixer |
| CE | [CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/) (provides `ez80-clang`, `fasmg`, `convbin`) |
| WASM | Emscripten SDK, CMake, Ninja |

### Native Build (Development + Tests)

```bash
cmake --preset native
cmake --build build/native
```

this builds the core engine, all host unit tests, and the PortCE SDL2 demo. variants:

| Preset | Description |
|---|---|
| `native` | Default debug build (system clang) |
| `native-clang20` | Explicit clang-20 |
| `native-asan-clang20` | AddressSanitizer + LeakSanitizer enabled |

Run tests:
```bash
cd build/native && ctest
# or
cmake --build build/native --target run_tests
```

Run the SDL2 demo:
```bash
./build/native/bin/demo_text_portce
```

### CE Build (Calculator)

the CE build lives in `demo/ce/` and uses the CEdev  toolchain:

```bash
cd demo/ce
cmake --preset ce
cmake --build ../../build/ce
```

This produces `.8xp` files in `build/ce/<target>/bin/`. Transfer the `.8xp` program along with `TeXFonts.8xv`, `TeXScrpt.8xv`, and `clibs.8xg` to the calculator.

### WASM Build (Browser)

```bash
source /path/to/emsdk/emsdk_env.sh
cmake --preset wasm
cmake --build build/wasm
```

Produces an HTML file you can serve locally... for whatever reason

## Testing

libtexce has two test tiers:

### Host Unit Tests

Located in [`tests/`](tests/), these test individual pipeline stages against the internal API:

| Test | What it covers |
|---|---|
| `test_token` | Tokenizer (text, math delimiters, escaping) |
| `test_parse` | Parser (fractions, scripts, overlays, matrices, delimiters) |
| `test_measure` | Measurement pass (node dimensions) |
| `test_layout` | Full dry-run layout (total height, line breaking) |
| `test_symbols` | Symbol table lookup |
| `test_pool` | Pool allocator (nodes, strings, lists, OOM) |

Run with:
```bash
cd build/native && ctest --output-on-failure
```

### On Device Autotests

The [`autotests/`](autotests/) directory contains a regression test suite that runs on CEmu via its autotester

Each test case renders a LaTeX expression on the calculator, then validates the LCD framebuffer against an expected CRC32 hash. Test cases are defined in [`autotests/casegen/cases.c`](autotests/casegen/cases.c):

```c
{ "quadratic", NULL, "$x = \\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}$", "025A9B2B", 10, 5 },
```

**Running autotests:**
```bash
cd autotests
make generate   # Generate test case directories from cases.c
make build      # Build all test .8xp programs
AUTOTESTER_ROM=/path/to/rom make test-all   # Run all tests in parallel
```

**Adding a new test case:**
1. Add an entry to the appropriate suite in `autotests/casegen/cases.c`
2. `make generate && make build`
3. Run with `--dry-run` or set the CRC to `"00000000"`, then use `./update_hashes.py` to capture the actual CRC

## Integrating into Your CE Project

To use libtexce in your own CE project:

### 1. Add the Source Files

Copy (or git submodule) the `src/tex/` directory and `include/texfont.h` into your project. The core engine is these files:

```
src/tex/tex_util.c      src/tex/tex_pool.c
src/tex/tex_symbols.c   src/tex/tex_metrics.c
src/tex/tex_fonts.c     src/tex/tex_token.c
src/tex/tex_parse.c     src/tex/tex_measure.c
src/tex/tex_layout.c    src/tex/tex_renderer.c
src/tex/tex_draw.c
```

### 2. Include Paths

Your build must be able to find:
- `src/` and `src/tex/` (internal headers)
- `include/` (public `texfont.h`)

### 3. Required Assets

Transfer these to the calculator alongside your `.8xp`:
- `TeXFonts.8xv` and `TeXScrpt.8xv` (from `assets/`)
- `clibs.8xg` (standard CE C libraries)

### 4. CMake Integration (Recommended)

if your CE project uses the provided `CEdevToolchain.cmake`, see [`demo/ce/CMakeLists.txt`](demo/ce/CMakeLists.txt) for a complete working example of `cedev_add_program()` with libtexce sources

## Demos

The repository includes two demo programs that build for both the CE and PortCE (native SDL2):

| Demo | Description |
|---|---|
| `demo_text` | Scrollable document renderer. Showcases fractions, integrals, Taylor series, matrices, and more in a paginated view. |
| `demo_thread` | Chat-style threaded conversation. Multiple independent `TeX_Layout` objects drawn with a shared renderer, demonstrating the multi-layout pattern. |

Build and run natively:
```bash
cmake --preset native && cmake --build build/native
./build/native/bin/demo_text_portce
```

## License

libtexce is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0-only).

See [`LICENSE`](LICENSE) for the full text.
See [`CONTRIBUTING.md`](CONTRIBUTING.md) for SPDX header guidance on new files.
