#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fstream>
#include <iostream>
#include <optional>
#include <png.h>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

static constexpr int kLcdWidth = 320;
static constexpr int kLcdHeight = 240;
static constexpr size_t kFrameSize = static_cast<size_t>(kLcdWidth) * kLcdHeight * 4;
static constexpr int kPadding = 10;
static constexpr int kLabelHeight = 20;
static constexpr int kCellWidth = kLcdWidth + kPadding;
static constexpr int kCellHeight = kLcdHeight + kLabelHeight + kPadding;

struct TestEntry
{
	std::string name;
	fs::path rgba_path;
	std::optional<fs::path> xp_path;
};

static std::optional<fs::path> find_first_entry(const fs::path& dir_path)
{
	if (!fs::exists(dir_path) || !fs::is_directory(dir_path))
	{
		return std::nullopt;
	}

	std::optional<fs::path> best;
	for (const auto& entry : fs::directory_iterator(dir_path))
	{
		const fs::path& p = entry.path();
		if (!best || p.filename().string() < best->filename().string())
		{
			best = p;
		}
	}

	return best;
}

static std::vector<TestEntry> scan_tests(const fs::path& generated_dir)
{
	std::vector<TestEntry> entries;

	for (const auto& group_entry : fs::directory_iterator(generated_dir))
	{
		if (!group_entry.is_directory())
		{
			continue;
		}
		for (const auto& case_entry : fs::directory_iterator(group_entry.path()))
		{
			if (!case_entry.is_directory())
			{
				continue;
			}

			const fs::path case_path = case_entry.path();
			fs::path base_name = group_entry.path().filename();
			base_name += "_";
			base_name += case_path.filename();
			const std::optional<fs::path> xp_path = find_first_entry(case_path / "bin");

			TestEntry entry;
			entry.name = base_name.string();
			entry.rgba_path = case_path / (base_name.string() + ".rgba");
			entry.xp_path = xp_path;
			entries.push_back(entry);
		}
	}

	std::sort(entries.begin(), entries.end(), [](const TestEntry& a, const TestEntry& b) { return a.name < b.name; });
	return entries;
}

static bool write_png(const fs::path& path, const unsigned char* data, const int width, const int height, const int color_type)
{
	const std::string path_str = path.string();
	FILE* const fp = fopen(path_str.c_str(), "wb");
	if (!fp)
	{
		std::perror("fopen png");
		return false;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
	{
		fclose(fp);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, nullptr);
		fclose(fp);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	const int rowbytes = (color_type == PNG_COLOR_TYPE_RGBA) ? width * 4 : width * 3;
	std::vector<png_bytep> rows(static_cast<size_t>(height));
	for (int y = 0; y < height; ++y)
	{
		rows[y] = const_cast<png_bytep>(data + static_cast<size_t>(y) * rowbytes);
	}

	png_write_image(png_ptr, rows.data());
	png_write_end(png_ptr, nullptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return true;
}

static bool rgba_from_file(const fs::path& path, std::vector<unsigned char>& rgba)
{
	const std::string path_str = path.string();
	std::ifstream in(path, std::ios::binary | std::ios::ate);
	if (!in)
	{
		std::perror("fopen rgba");
		return false;
	}

	const auto size = in.tellg();
	if (size != static_cast<std::streamsize>(kFrameSize))
	{
		std::cerr << "Invalid RGBA file size: " << path_str << " (" << size << " bytes)\n";
		return false;
	}
	in.seekg(0, std::ios::beg);

	rgba.resize(kFrameSize);
	if (!in.read(reinterpret_cast<char*>(rgba.data()), rgba.size()))
	{
		std::cerr << "Short read on " << path_str << "\n";
		return false;
	}

	for (size_t i = 0; i < kFrameSize; i += 4)
	{
		std::swap(rgba[i + 0], rgba[i + 2]);
	}

	return true;
}

static void blit_mosaic(std::vector<unsigned char>& mosaic, const int mosaic_w, const int x, const int y,
                        const std::vector<unsigned char>& rgba)
{
	for (int row = 0; row < kLcdHeight; ++row)
	{
		for (int col = 0; col < kLcdWidth; ++col)
		{
			const size_t src = (static_cast<size_t>(row) * kLcdWidth + col) * 4;
			const size_t dst = (static_cast<size_t>(y + row) * mosaic_w + (x + col)) * 3;
			mosaic[dst + 0] = rgba[src + 0];
			mosaic[dst + 1] = rgba[src + 1];
			mosaic[dst + 2] = rgba[src + 2];
		}
	}
}

static bool load_font(FT_Library* ft, FT_Face* face)
{
	FcPattern* const pattern = FcPatternCreate();
	if (!pattern)
	{
		std::cerr << "Error: Failed to create fontconfig pattern\n";
		return false;
	}

	FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>("DejaVu Sans"));
	FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN);
	FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_NORMAL);

	FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	FcResult result;
	FcPattern* const match = FcFontMatch(nullptr, pattern, &result);
	FcPatternDestroy(pattern);
	if (!match)
	{
		std::cerr << "Error: Could not find DejaVu Sans via fontconfig\n";
		return false;
	}

	FcChar8* file = nullptr;
	int index = 0;
	if (FcPatternGetString(match, FC_FILE, 0, &file) != FcResultMatch)
	{
		std::cerr << "Error: Fontconfig match missing font file\n";
		FcPatternDestroy(match);
		return false;
	}
	if (FcPatternGetInteger(match, FC_INDEX, 0, &index) != FcResultMatch)
	{
		index = 0;
	}
	const std::string font_path(reinterpret_cast<char*>(file));
	FcPatternDestroy(match);

	if (FT_Init_FreeType(ft) != 0)
	{
		std::cerr << "Error: Failed to initialize FreeType\n";
		return false;
	}
	if (FT_New_Face(*ft, font_path.c_str(), index, face) != 0)
	{
		std::cerr << "Error: Failed to load font file: " << font_path << "\n";
		FT_Done_FreeType(*ft);
		*ft = nullptr;
		return false;
	}
	if (FT_Set_Pixel_Sizes(*face, 0, 12) != 0)
	{
		std::cerr << "Error: Failed to set font size\n";
		FT_Done_Face(*face);
		FT_Done_FreeType(*ft);
		*face = nullptr;
		*ft = nullptr;
		return false;
	}

	return true;
}

static void draw_text(std::vector<unsigned char>& mosaic, const int mosaic_w, const int mosaic_h, const int x, const int y_top,
                      const std::string_view text, const FT_Face face)
{
	const int ascender = face->size->metrics.ascender >> 6;
	int pen_x = x;
	const int pen_y = y_top + ascender;

	for (const char ch : text)
	{
		if (FT_Load_Char(face, ch, FT_LOAD_RENDER) != 0)
		{
			continue;
		}

		const FT_GlyphSlot g = face->glyph;
		const FT_Bitmap* const bm = &g->bitmap;

		for (int row = 0; row < static_cast<int>(bm->rows); ++row)
		{
			for (int col = 0; col < static_cast<int>(bm->width); ++col)
			{
				const int px = pen_x + g->bitmap_left + col;
				const int py = pen_y - g->bitmap_top + row;
				const unsigned char alpha = bm->buffer[row * bm->pitch + col];

				if (alpha == 0)
				{
					continue;
				}
				if (px < 0 || py < 0 || px >= mosaic_w || py >= mosaic_h)
				{
					continue;
				}

				const size_t idx = (static_cast<size_t>(py) * mosaic_w + px) * 3;
				const unsigned char inv = static_cast<unsigned char>(255 - alpha);
				mosaic[idx + 0] = static_cast<unsigned char>((mosaic[idx + 0] * inv) / 255);
				mosaic[idx + 1] = static_cast<unsigned char>((mosaic[idx + 1] * inv) / 255);
				mosaic[idx + 2] = static_cast<unsigned char>((mosaic[idx + 2] * inv) / 255);
			}
		}

		pen_x += g->advance.x >> 6;
	}
}

static void usage(const char* prog) { std::cerr << "Usage: " << prog << " [--generated DIR] [--output DIR]\n"; }

int main(int argc, char** argv)
{
	fs::path generated_dir = "generated";
	fs::path output_dir = "artifact";

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--generated") == 0)
		{
			if (i + 1 >= argc)
			{
				usage(argv[0]);
				return 1;
			}
			generated_dir = argv[++i];
		}
		else if (strcmp(argv[i], "--output") == 0)
		{
			if (i + 1 >= argc)
			{
				usage(argv[0]);
				return 1;
			}
			output_dir = argv[++i];
		}
		else
		{
			usage(argv[0]);
			return 1;
		}
	}

	if (!fs::exists(generated_dir) || !fs::is_directory(generated_dir))
	{
		std::cerr << "Error: Generated directory not found: " << generated_dir << "\n";
		return 1;
	}

	const std::vector<TestEntry> entries = scan_tests(generated_dir);
	if (entries.empty())
	{
		std::cerr << "No .rgba files found. Did you run autotests with AUTOTESTER_FLAGS=-s?\n";
		return 1;
	}

	for (const TestEntry& entry : entries)
	{
		if (entry.name.size() > 40)
		{
			std::cerr << "Error: Test name exceeds 40 characters: " << entry.name << "\n";
			return 1;
		}
	}

	std::cout << "Found " << entries.size() << " test outputs\n";
	fs::create_directories(output_dir);

	if (!FcInit())
	{
		std::cerr << "Error: Failed to initialize fontconfig\n";
		return 1;
	}
	const bool fc_inited = true;

	FT_Library ft = nullptr;
	FT_Face face = nullptr;
	if (!load_font(&ft, &face))
	{
		FcFini();
		return 1;
	}

	const int cols = std::clamp(static_cast<int>(
	    std::ceil(std::sqrt(static_cast<double>(entries.size()) * (static_cast<double>(kLcdWidth) / kLcdHeight)))), 1, 8);
	const int rows = static_cast<int>((entries.size() + cols - 1) / cols);
	const int mosaic_w = cols * kCellWidth + kPadding;
	const int mosaic_h = rows * kCellHeight + kPadding;

	std::vector<unsigned char> mosaic(static_cast<size_t>(mosaic_w) * mosaic_h * 3, 255);
	std::vector<unsigned char> rgba;

	size_t mosaic_index = 0;
	for (const TestEntry& entry : entries)
	{
		const fs::path test_dir = output_dir / entry.name;
		fs::create_directories(test_dir);

		const fs::path png_path = test_dir / (entry.name + ".png");
		if (!rgba_from_file(entry.rgba_path, rgba))
		{
			std::cerr << "  Error converting " << entry.rgba_path << "\n";
			continue;
		}
		if (!write_png(png_path, rgba.data(), kLcdWidth, kLcdHeight, PNG_COLOR_TYPE_RGBA))
		{
			std::cerr << "  Error writing " << png_path << "\n";
			continue;
		}
		std::cout << "  Converted: " << png_path << "\n";

		if (entry.xp_path)
		{
			const fs::path xp_out = test_dir / entry.xp_path->filename();
			std::error_code ec;
			fs::copy_file(*entry.xp_path, xp_out, fs::copy_options::overwrite_existing, ec);
		}

		const int row = static_cast<int>(mosaic_index / static_cast<size_t>(cols));
		const int col = static_cast<int>(mosaic_index % static_cast<size_t>(cols));
		const int x = kPadding + col * kCellWidth;
		const int y = kPadding + row * kCellHeight;

		blit_mosaic(mosaic, mosaic_w, x, y, rgba);

		draw_text(mosaic, mosaic_w, mosaic_h, x, y + kLcdHeight + 2, entry.name, face);
		++mosaic_index;
	}

	const fs::path mosaic_path = output_dir / "mosaic.png";
	if (!write_png(mosaic_path, mosaic.data(), mosaic_w, mosaic_h, PNG_COLOR_TYPE_RGB))
	{
		std::cerr << "Error: Failed to write mosaic\n";
		return 1;
	}

	std::cout << "Generated mosaic: " << mosaic_path << " (" << cols << "x" << rows << " grid)\n";
	std::cout << "\nArtifact ready: " << output_dir << "/\n";

	if (face)
	{
		FT_Done_Face(face);
	}
	if (ft)
	{
		FT_Done_FreeType(ft);
	}
	if (fc_inited)
	{
		FcFini();
	}
	return 0;
}
