#include "MinBoolFunc.h"

#include "imgui_internal.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#ifdef __ANDROID__
#define strncpy_s(dest, src, count) strncpy(dest, src, count)
#define _snprintf_s(buf, count, fmt, ...) snprintf(buf, count, fmt, __VA_ARGS__)
#endif

static const char* const formula_hint[] = {
	"",

	"function f(x)\n"
	"\treturn x\n"
	"end\n",

	"function f(x, y)\n"
	"\treturn x and y\n"
	"end\n",

	"function f(x, y, z)\n"
	"\treturn x and y and z\n"
	"end\n",

	"function f(x1, x2, x3, x4)\n"
	"\treturn x1 and x2 and x3 and x4\n"
	"end\n",

	"function f(x1, x2, x3, x4, x5)\n"
	"\treturn x1 and x2 and x3 and x4 and x5\n"
	"end\n",
};

static const char* const formula_default =
"local t = {\n"
"\t[\"000\"] = 1,\n"
"\t[\"001\"] = 1,\n"
"\t[\"010\"] = 1,\n"
"\t[\"011\"] = 0,\n"
"\t[\"100\"] = 1,\n"
"\t[\"101\"] = 1,\n"
"\t[\"110\"] = 0,\n"
"\t[\"111\"] = 0,\n"
"}\n"
"function f(x, y, z)\n"
"\tx = x and \"1\" or \"0\"\n"
"\ty = y and \"1\" or \"0\"\n"
"\tz = z and \"1\" or \"0\"\n"
"\treturn t[x .. y .. z] == 1\n"
"end\n";

/*
void* operator new(size_t size) {
	printf("new(%zu)\n", size);
	return malloc(size);
}

void* operator new[](size_t size) {
	printf("new[](%zu)\n", size);
	return malloc(size);
}
//*/

// 
// Don't assert on fail
// 
static ImFont* AddFontFromFileTTF(ImFontAtlas* atlas, const char* filename, float size_pixels, const ImFontConfig* font_cfg_template = nullptr, const ImWchar* glyph_ranges = nullptr)
{
	size_t data_size = 0;
	void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
	if (!data)
	{
		return NULL;
	}
	ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
	if (font_cfg.Name[0] == '\0')
	{
		// Store a short copy of filename into into the font name for convenience
		const char* p;
		for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
		ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
	}
	return atlas->AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

#ifdef __ANDROID__
static void ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button)
{
	static ImVec2 _delta = {};

	ImGuiContext& g = *ImGui::GetCurrentContext();
	ImGuiWindow* window = g.CurrentWindow;
	bool hovered = false;
	bool held = false;
	ImGuiID id = window->GetID("##scrolldraggingoverlay");
	ImGui::KeepAliveID(id);
	ImGuiButtonFlags button_flags = (mouse_button == 0) ? ImGuiButtonFlags_MouseButtonLeft : (mouse_button == 1) ? ImGuiButtonFlags_MouseButtonRight : ImGuiButtonFlags_MouseButtonMiddle;
	if (g.HoveredId == 0) // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
		ImGui::ButtonBehavior(window->Rect(), id, &hovered, &held, button_flags);
	if (held)
	{
		_delta = delta;
	}

	if (_delta.x != 0)
	{
		ImGui::SetScrollX(window, window->Scroll.x + _delta.x);
		_delta.x *= 0.80f;
	}
	if (_delta.y != 0)
	{
		ImGui::SetScrollY(window, window->Scroll.y + _delta.y);
		_delta.y *= 0.80f;
	}
}
#endif

void MinBoolFunc::Init() {
	ImGui::StyleColorsLight();

	ImGuiIO& io = ImGui::GetIO();

#ifdef __ANDROID__
	ImGui::GetStyle().ScaleAllSizes(2);
	io.FontGlobalScale = 2;
#endif

	{
		ImVector<ImWchar> fnt_main_ranges;
		{
			ImWchar _ranges[] = {
				0x2000, 0x206F, // General Punctuation (for Overline)
				0x2070, 0x209F, // Superscripts and Subscripts
				0x2200, 0x22FF, // Mathematical Operators (for logical and, or)
				0x25A0, 0x25FF, // Geometric Shapes (for triangle arrows)
				0x3000, 0x036F, // Combining Diacritical Marks (for Combining Overline)
				0,
			};

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
			builder.AddRanges(_ranges);

			builder.BuildRanges(&fnt_main_ranges);
		}

#ifdef __ANDROID__
		fnt_main = AddFontFromFileTTF(io.Fonts, "/system/fonts/Roboto-Regular.ttf", 24, nullptr, fnt_main_ranges.Data);
#else
		fnt_main = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\segoeui.ttf", 24, nullptr, fnt_main_ranges.Data);
#endif

		if (!fnt_main) {
			fnt_main = AddFontFromFileTTF(io.Fonts, "segoeui.ttf", 24, nullptr, fnt_main_ranges.Data);
		}

		if (!fnt_main) {
			ImFontConfig config;
			config.SizePixels = 20;
			fnt_main = io.Fonts->AddFontDefault(&config);
		}

		fnt_mono = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\cour.ttf", 24);

		if (!fnt_mono) {
			fnt_mono = AddFontFromFileTTF(io.Fonts, "cour.ttf", 24);
		}

		if (!fnt_mono) {
			fnt_mono = fnt_main;
		}

		ImVector<ImWchar> fnt_math_ranges;
		{
			ImWchar _ranges[] = {
				0x2000, 0x206F, // General Punctuation (for Overline)
				0x2070, 0x209F, // Superscripts and Subscripts
				0x2200, 0x22FF, // Mathematical Operators (for logical and, or)
				0x25A0, 0x25FF, // Geometric Shapes (for triangle arrows)
				0x3000, 0x036F, // Combining Diacritical Marks (for Combining Overline)
				0,
			};

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			builder.AddRanges(_ranges);

			builder.BuildRanges(&fnt_math_ranges);
		}

		fnt_math = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\cambria.ttc", 24, nullptr, fnt_math_ranges.Data);

		if (!fnt_math) {
			fnt_math = AddFontFromFileTTF(io.Fonts, "cambria.ttc", 24, nullptr, fnt_math_ranges.Data);
		}

		if (!fnt_math) {
			fnt_math = fnt_main;
		}

		io.Fonts->Build();
	}

	io.IniFilename = nullptr;

	strncpy_s(formula, formula_default, sizeof(formula) - 1);

	L = luaL_newstate();
	luaL_openlibs(L);

	CompileScript();
}

void MinBoolFunc::Quit() {
	lua_close(L);
}

static int wrap(int a, int b) {
	return (a % b + b) % b;
}

static void StrAppend(char* buf, int count, const char* str) {
	bool found_null = false;

	int i;
	for (i = 0; i < count - 1; i++) {
		if (buf[i] == 0) {
			found_null = true;
		}
	}

	if (!found_null) {
		return;
	}

	int j;
	for (j = 0; i + j < count - 1; j++) {
		if (str[j] == 0) {
			break;
		}

		buf[i + j] = str[j];
	}

	buf[i + j] = 0;
}

void MinBoolFunc::ImGuiStep() {
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowPos({});
		ImGui::SetNextWindowSize(io.DisplaySize);
	}

	if (ImGui::Begin(u8"Минимизация булевых функций", nullptr,
					 ImGuiWindowFlags_NoDecoration
					 | ImGuiWindowFlags_NoSavedSettings)) {

		ImGuiTableFlags table_flags = (ImGuiTableFlags_Borders
									   | ImGuiTableFlags_RowBg);

		// 
		// Выбор кол-ва переменных
		// 
		{
			ImGui::Text(u8"Кол-во переменных: ");

			char preview[] = {(char)('0' + variable_count), 0};

			ImGui::SameLine();
			if (ImGui::BeginCombo("##variable_count", preview,
								  ImGuiComboFlags_WidthFitPreview)) {
				for (int i = 2; i <= MAX_VARIABLE_COUNT; i++) {
					char label[] = {(char)('0' + i), 0};

					if (ImGui::Selectable(label)) {
						variable_count = i;

						if (formula[0] == 0) {
							strncpy_s(formula, formula_hint[variable_count], sizeof(formula) - 1);
						}

						CompileScript();
					}

					if (i == variable_count) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}
		}

		// 
		// Ввод формулы
		// 
		ImGui::Text(u8"Формула (Lua)");
		ImGui::PushFont(fnt_mono);
		if (ImGui::InputTextEx("##lua_source", formula_hint[variable_count],
							   formula, sizeof(formula),
							   {-1, ImGui::GetTextLineHeight() * 16},
							   ImGuiInputTextFlags_Multiline
							   | ImGuiInputTextFlags_AllowTabInput)) {
			CompileScript();
		}
		ImGui::PopFont();

		if (script_error) {
			ImGui::PushTextWrapPos(0);
			ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)COLOR_RED);

			ImGui::TextUnformatted(lua_err_msg);

			ImGui::PopStyleColor();
			ImGui::PopTextWrapPos();

			goto l_window_end;
		}

		// 
		// Таблица истинности
		// 
		ImGui::Text(u8"Таблица истинности");
		if (ImGui::BeginTable("truth_table", variable_count + 1, table_flags)) {

			// header
			{
				for (int i = 0; i < variable_count; i++) {
					char label[] = {'x', (char)0xE2, (char)0x82, (char)(0x80 + (i + 1)), 0}; // U+2080

					ImGui::TableSetupColumn(label);
				}

				ImGui::TableSetupColumn("f");

				ImGui::TableHeadersRow();
			}

			for (int i = 0; i < (1 << variable_count); i++) {
				ImGui::TableNextRow();

				for (int j = variable_count; j--;) {
					bool value = (i & (1 << j)) != 0;

					ImGui::TableNextColumn();
					if (value) {
						ImGui::Text("1");
					} else {
						ImGui::TextDisabled("0");
					}
				}

				ImGui::TableNextColumn();
				if (truth_table[i]) {
					ImGui::Text("1");
				} else {
					ImGui::TextDisabled("0");
				}
			}



			ImGui::EndTable();
		}

		// 
		// Карта Карно
		// 
		{
			static std::vector<const char*> row_header;
			static std::vector<const char*> col_header;
			static std::vector<const char*> row_header2;
			static std::vector<const char*> col_header2;
			static std::vector<bool> cells;
			const char* cell_header = "?";
			static std::vector<const char*> var_names_lua;
			static std::vector<const char*> var_names_unicode;

			switch (variable_count) {
				case 2: {
					row_header = {"0", "1"}; // x
					col_header = {"0", "1"}; // y
					cells = {
						truth_table[0b00], truth_table[0b10],
						truth_table[0b01], truth_table[0b11],
					};
					cell_header = "y\\x";
					row_header2 = {"0..", "1.."};
					col_header2 = {"0..", "1.."};
					break;
				}
				case 3: {
					row_header = {"00", "01", "11", "10"}; // xy
					col_header = {"0", "1"}; // z
					cells = {
						truth_table[0b000], truth_table[0b010], truth_table[0b110], truth_table[0b100],
						truth_table[0b001], truth_table[0b011], truth_table[0b111], truth_table[0b101],
					};
					cell_header = "z\\xy";
					row_header2 = {"00.", "01.", "11.", "10."};
					col_header2 = {"0..", "1.."};
					break;
				}
				case 4: {
					row_header = {"00", "01", "11", "10"}; // x1 x2
					col_header = {"00", "01", "11", "10"}; // x3 x4
					cells = {
						truth_table[0b0000], truth_table[0b0100], truth_table[0b1100], truth_table[0b1000],
						truth_table[0b0001], truth_table[0b0101], truth_table[0b1101], truth_table[0b1001],
						truth_table[0b0011], truth_table[0b0111], truth_table[0b1111], truth_table[0b1011],
						truth_table[0b0010], truth_table[0b0110], truth_table[0b1110], truth_table[0b1010],
					};
					cell_header = u8"x₃x₄\\x₁x₂";
					row_header2 = {"00.", "01.", "11.", "10."};
					col_header2 = {"00.", "01.", "11.", "10."};
					break;
				}
				case 5: {
					row_header = {"000", "001", "011", "010", "110", "111", "101", "100"}; // x1 x2 x3
					col_header = {"00", "01", "11", "10"}; // x4 x5
					cells = {
						truth_table[0b00000], truth_table[0b00100], truth_table[0b01100], truth_table[0b01000], truth_table[0b11000], truth_table[0b11100], truth_table[0b10100], truth_table[0b10000],
						truth_table[0b00001], truth_table[0b00101], truth_table[0b01101], truth_table[0b01001], truth_table[0b11001], truth_table[0b11101], truth_table[0b10101], truth_table[0b10001],
						truth_table[0b00011], truth_table[0b00111], truth_table[0b01111], truth_table[0b01011], truth_table[0b11011], truth_table[0b11111], truth_table[0b10111], truth_table[0b10011],
						truth_table[0b00010], truth_table[0b00110], truth_table[0b01110], truth_table[0b01010], truth_table[0b11010], truth_table[0b11110], truth_table[0b10110], truth_table[0b10010],
					};
					cell_header = u8"x₄x₅\\x₁x₂x₃";
					row_header2 = {"000", "001", "011", "010", "110", "111", "101", "100"};
					col_header2 = {"00.", "01.", "11.", "10."};
					break;
				}
			}

			switch (variable_count) {
				case 2:
				case 3: {
					var_names_lua = {"x", "y", "z", "w"};
					var_names_unicode = {"x", "y", "z", "w"};
					break;
				}

				default:
				case 4:
				case 5: {
					var_names_lua = {"x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9"};
					var_names_unicode = {u8"x₁", u8"x₂", u8"x₃", u8"x₄", u8"x₅", u8"x₆", u8"x₇", u8"x₈", u8"x₉"};
					break;
				}
			}

			static std::vector<std::vector<ImRect>> cell_rects;
			static std::vector<std::vector<ImRect>> cell_rects_abs;
			static std::vector<std::vector<ImGuiID>> cell_ids_abs;

			cell_rects.resize(col_header.size());
			cell_rects_abs.resize(col_header.size());
			cell_ids_abs.resize(col_header.size());

			for (auto& v : cell_rects) v.resize(row_header.size());
			for (auto& v : cell_rects_abs) v.resize(row_header.size());
			for (auto& v : cell_ids_abs) v.resize(row_header.size());

			ImGui::Text(u8"Карта Карно");

			ImGui::PushFont(fnt_math);
			{
				if (ImGui::Button(u8"▲")) {
					karnaugh_yoff++;
					karnaugh_yoff = wrap(karnaugh_yoff, col_header.size());
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"▼")) {
					karnaugh_yoff--;
					karnaugh_yoff = wrap(karnaugh_yoff, col_header.size());
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"◀")) {
					karnaugh_xoff++;
					karnaugh_xoff = wrap(karnaugh_xoff, row_header.size());
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"▶")) {
					karnaugh_xoff--;
					karnaugh_xoff = wrap(karnaugh_xoff, row_header.size());
				}
			}
			ImGui::PopFont();

			if (ImGui::BeginTable("Karnaugh_map", row_header.size() + 1, table_flags)) {

				// header
				{
					ImGui::TableSetupColumn(cell_header);

					for (int _i = 0; _i < row_header.size(); _i++) {
						int i = wrap(_i + karnaugh_xoff, row_header.size());

						const char* label = row_header[i];
						ImGui::TableSetupColumn(label);
					}

					ImGui::TableHeadersRow();
				}

				for (int _i = 0; _i < col_header.size(); _i++) {
					int i = wrap(_i + karnaugh_yoff, col_header.size());

					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(col_header[i]);

					for (int _j = 0; _j < row_header.size(); _j++) {
						int j = wrap(_j + karnaugh_xoff, row_header.size());

						ImGui::TableNextColumn();
						int w = row_header.size();
						if (cells[j + i * w]) {
							ImGui::PushID((i + 1) * 10000 + j);
							ImGui::Selectable("1");
							ImGui::PopID();

							cell_rects[i][j] = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
							cell_rects_abs[_i][_j] = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
							cell_ids_abs[_i][_j] = ImGui::GetItemID();
						} else {
							cell_rects[i][j] = {};
							cell_rects_abs[_i][_j] = {};
							cell_ids_abs[_i][_j] = 0;
						}
					}
				}

				ImGui::EndTable();
			}

			ImU32 area_colors[] = {
				// IM_COL32(0xFF, 0x00, 0x00, 0x40),
				// IM_COL32(0xFF, 0x92, 0x00, 0x40),
				// IM_COL32(0x42, 0xD5, 0x2B, 0x40),
				// IM_COL32(0x24, 0xE0, 0xD4, 0x40),
				// IM_COL32(0x00, 0x1B, 0xFF, 0x40),
				// IM_COL32(0xFF, 0x00, 0xFE, 0x40),

				IM_COL32(0xFF, 0x00, 0x00, 0x40),
				IM_COL32(0x00, 0x00, 0xFF, 0x40),
			};

			// areas = {
			// 	{3, 0, 2, 2},
			// 	{0, 0, 2, 1},
			// };

			for (int i = 0; i < areas.size(); i++) {
				const Area& area = areas[i];

				ImDrawList* list = ImGui::GetWindowDrawList();

				for (int _y = area.y; _y < area.y + area.h; _y++) {
					int y = wrap(_y, col_header.size());

					for (int _x = area.x; _x < area.x + area.w; _x++) {
						int x = wrap(_x, row_header.size());

						ImRect rect = cell_rects[y][x];

						ImU32 color = area_colors[i % IM_ARRAYSIZE(area_colors)];
						list->AddRectFilled(rect.Min, rect.Max, color);
					}
				}
			}

			if (dragging) {
				if (ImGui::GetActiveID() == drag_id) {

					for (int i = 0; i < col_header.size(); i++) {
						for (int j = 0; j < row_header.size(); j++) {
							if (cell_ids_abs[i][j] != 0 && cell_rects_abs[i][j].Contains(ImGui::GetMousePos())) {
								drag_x2 = j;
								drag_y2 = i;
								goto l_break1;
							}
						}
					}

				l_break1:

					ImDrawList* list = ImGui::GetWindowDrawList();

					ImRect rect1 = cell_rects_abs[drag_y1][drag_x1];
					ImRect rect2 = cell_rects_abs[drag_y2][drag_x2];

					ImRect rect;
					rect.Min = ImMin(rect1.Min, rect2.Min);
					rect.Max = ImMax(rect1.Max, rect2.Max);

					list->AddRect(rect.Min, rect.Max, COLOR_RED, 0, 0, 4);
				} else {
					if (areas.size() < 99) {
						Area area = {};
						area.x = ImMin(drag_x1, drag_x2) + karnaugh_xoff;
						area.y = ImMin(drag_y1, drag_y2) + karnaugh_yoff;

						area.x = wrap(area.x, row_header.size());
						area.y = wrap(area.y, col_header.size());

						area.w = ImAbs(drag_x2 - drag_x1) + 1;
						area.h = ImAbs(drag_y2 - drag_y1) + 1;

						// check if area already exists
						for (int i = 0; i < areas.size(); i++) {
							if (area.x == areas[i].x && area.y == areas[i].y
								&& area.w == areas[i].w && area.h == areas[i].h) {
								goto l_area_add_out;
							}
						}

						int S = area.w * area.h;
						if (!ImIsPowerOfTwo(S)) {
							goto l_area_add_out;
						}

						// must contain only 1's
						for (int _y = area.y; _y < area.y + area.h; _y++) {
							int y = wrap(_y, col_header.size());
							for (int _x = area.x; _x < area.x + area.w; _x++) {
								int x = wrap(_x, row_header.size());
								int w = row_header.size();
								if (!cells[x + y * w]) {
									goto l_area_add_out;
								}
							}
						}

						areas.push_back(area);
					}

				l_area_add_out:

					dragging = false;
					drag_id = 0;
				}
			} else {
				for (int i = 0; i < col_header.size(); i++) {
					for (int j = 0; j < row_header.size(); j++) {
						if (cell_ids_abs[i][j] != 0 && cell_ids_abs[i][j] == ImGui::GetActiveID()) {
							dragging = true;
							drag_id = cell_ids_abs[i][j];
							drag_x1 = j;
							drag_y1 = i;
							drag_x2 = drag_x1;
							drag_y2 = drag_y1;
							goto l_break2;
						}
					}
				}

			l_break2:;

			}

			ImGui::Text(u8"Области:");
			for (int i = 0; i < areas.size(); i++) {
				const Area& area = areas[i];

				char subscript[16];
				if ((i + 1) >= 10) {
					int j = 0;
					subscript[j++] = 0xE2; // U+2080
					subscript[j++] = 0x82;
					subscript[j++] = 0x80 + (i + 1) / 10;
					subscript[j++] = 0xE2;
					subscript[j++] = 0x82;
					subscript[j++] = 0x80 + (i + 1) % 10;
					subscript[j++] = 0;
				} else {
					int j = 0;
					subscript[j++] = 0xE2; // U+2080
					subscript[j++] = 0x82;
					subscript[j++] = 0x80 + (i + 1) % 10;
					subscript[j++] = 0;
				}

				ImGui::PushID((i + 1) * 100);
				bool b = ImGui::Button(" - ");
				ImGui::PopID();

				if (b) {
					areas.erase(areas.begin() + i);
					i--;
					continue;
				}

				ImGui::SameLine();

				ImVec4 color = ImGui::ColorConvertU32ToFloat4(area_colors[i % IM_ARRAYSIZE(area_colors)]);
				color.w = 1;
				ImGui::TextColored(color,
								   "S%s: (%d,%d), (%dx%d)",
								   subscript, area.x, area.y, area.w, area.h);
			}

			ImGui::Text(u8"МДНФ:");
			{
				result_lua.clear();
				result_unicode.clear();

				for (int i = 0; i < areas.size(); i++) {
					const Area& area = areas[i];

					char v1 = row_header2[area.x][0]; // x
					char v2 = row_header2[area.x][1]; // y
					char v3 = row_header2[area.x][2];
					char v4 = col_header2[area.y][0]; // z
					char v5 = col_header2[area.y][1];

					bool v1_changes = false;
					bool v2_changes = false;
					bool v3_changes = false;
					bool v4_changes = false;
					bool v5_changes = false;

					for (int _y = area.y; _y < area.y + area.h; _y++) {
						int y = wrap(_y, col_header2.size());
						for (int _x = area.x; _x < area.x + area.w; _x++) {
							int x = wrap(_x, row_header2.size());
							if (row_header2[x][0] != v1) v1_changes = true;
							if (row_header2[x][1] != v2) v2_changes = true;
							if (row_header2[x][2] != v3) v3_changes = true;
							if (col_header2[y][0] != v4) v4_changes = true;
							if (col_header2[y][1] != v5) v5_changes = true;
						}
					}

					if (i > 0) {
						result_lua += " or ";
						result_unicode += u8" ∨ ";
					}

					bool x_changes[5] = {};
					char x[5] = {};

					switch (variable_count) {
						case 2: {
							x_changes[0] = v1_changes;
							x_changes[1] = v4_changes;
							x[0] = v1;
							x[1] = v4;
							break;
						}
						case 3: {
							x_changes[0] = v1_changes;
							x_changes[1] = v2_changes;
							x_changes[2] = v4_changes;
							x[0] = v1;
							x[1] = v2;
							x[2] = v4;
							break;
						}
						case 4: {
							x_changes[0] = v1_changes;
							x_changes[1] = v2_changes;
							x_changes[2] = v4_changes;
							x_changes[3] = v5_changes;
							x[0] = v1;
							x[1] = v2;
							x[2] = v4;
							x[3] = v5;
							break;
						}
						case 5: {
							x_changes[0] = v1_changes;
							x_changes[1] = v2_changes;
							x_changes[2] = v3_changes;
							x_changes[3] = v4_changes;
							x_changes[4] = v5_changes;
							x[0] = v1;
							x[1] = v2;
							x[2] = v3;
							x[3] = v4;
							x[4] = v5;
							break;
						}
					}

					result_lua += "(";
					result_unicode += "(";

					bool needdelim = false;

					for (int i = 0; i < variable_count; i++) {
						if (!x_changes[i]) {
							if (needdelim) {
								result_lua += " and ";
								result_unicode += u8" ∧ ";

								needdelim = false;
							}

							if (x[i] == '0') {
								{
									// char buf[] = {'n', 'o', 't', '(', 'x', '0' + (i + 1) % 10, ')', 0};
									// result_lua += buf;

									result_lua += "not(";
									result_lua += var_names_lua[i];
									result_lua += ")";
								}

								{
									// char buf[] = {0xC2, 0xAC, 'x', 0xE2, 0x82, 0x80 + (i + 1) % 10, 0}; // U+00AC U+2080
									// result_unicode += buf;

									result_unicode += u8"¬";
									result_unicode += var_names_unicode[i];
								}

								needdelim = true;
							} else if (x[i] == '1') {
								{
									// char buf[] = {'x', '0' + (i + 1) % 10, 0};
									// result_lua += buf;

									result_lua += var_names_lua[i];
								}

								{
									// char buf[] = {'x', 0xE2, 0x82, 0x80 + (i + 1) % 10, 0}; // U+2080
									// result_unicode += buf;

									result_unicode += var_names_unicode[i];
								}

								needdelim = true;
							}
						}
					}

					result_lua += ")";
					result_unicode += ")";
				}
			}

			ImGui::PushTextWrapPos(0);
			ImGui::PushFont(fnt_mono);
			ImGui::TextUnformatted(result_lua.c_str());
			ImGui::PopFont();
			ImGui::PopTextWrapPos();

			if (ImGui::BeginPopupContextItem("result_lua_context")) {
				if (ImGui::Button(u8"Копировать")) {
					ImGui::SetClipboardText(result_lua.c_str());
				}

				ImGui::EndPopup();
			}

			ImGui::PushTextWrapPos(0);
			ImGui::PushFont(fnt_math);
			ImGui::TextUnformatted(result_unicode.c_str());
			ImGui::PopFont();
			ImGui::PopTextWrapPos();

			if (ImGui::BeginPopupContextItem("result_unicode_context")) {
				if (ImGui::Button(u8"Копировать")) {
					ImGui::SetClipboardText(result_unicode.c_str());
				}

				ImGui::EndPopup();
			}

			// printf("result_lua capacity: %zu\n", result_lua.capacity());
			// printf("result_unicode capacity: %zu\n", result_unicode.capacity());

		}


	l_window_end:;

#ifdef __ANDROID__
		{
			ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
			ScrollWhenDraggingOnVoid(ImVec2(0.0f, -mouse_delta.y), ImGuiMouseButton_Left);
		}
#endif

	}
	ImGui::End();


}

void MinBoolFunc::CompileScript() {
	dragging = false;
	drag_id = 0;
	areas.clear();

	script_error = true;

	if (luaL_loadstring(L, formula) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		strncpy_s(lua_err_msg, err, sizeof(lua_err_msg) - 1);
		lua_settop(L, 0);
		return;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		strncpy_s(lua_err_msg, err, sizeof(lua_err_msg) - 1);
		lua_settop(L, 0);
		return;
	}

	script_error = false;

	BuildTruthTable();
}

void MinBoolFunc::BuildTruthTable() {
	if (script_error) {
		return;
	}

	memset(truth_table, 0, sizeof(truth_table));

	lua_getglobal(L, "f");
	if (!lua_isfunction(L, -1)) {
		const char* tname = lua_typename(L, lua_type(L, -1));
		_snprintf_s(lua_err_msg, sizeof(lua_err_msg) - 1, u8"Переменная \"f\" не является функцией (%s).", tname);
		lua_settop(L, 0);
		script_error = true;
		return;
	}

	for (int i = 0; i < (1 << variable_count); i++) {
		lua_pushvalue(L, -1); // duplicate function

		// push args
		for (int j = variable_count; j--;) {
			bool value = (i & (1 << j)) != 0;
			lua_pushboolean(L, value);
		}

		if (lua_pcall(L, variable_count, 1, 0) != LUA_OK) {
			const char* err = lua_tostring(L, -1);
			strncpy_s(lua_err_msg, err, sizeof(lua_err_msg) - 1);
			lua_settop(L, 0);
			script_error = true;
			return;
		}

		bool result = lua_toboolean(L, -1);
		truth_table[i] = result;

		lua_pop(L, 1); // pop result
	}

	lua_settop(L, 0);
}
