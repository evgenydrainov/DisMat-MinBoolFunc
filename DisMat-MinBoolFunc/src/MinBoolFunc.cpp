#include "MinBoolFunc.h"

#include "imgui_internal.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

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

void MinBoolFunc::Init() {
	ImGuiIO& io = ImGui::GetIO();

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

		fnt_main = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\segoeui.ttf", 24, nullptr, fnt_main_ranges.Data);
		if (!fnt_main) {
			ImFontConfig config;
			config.SizePixels = 20;
			fnt_main = io.Fonts->AddFontDefault(&config);
		}

		fnt_mono = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\cour.ttf", 24);
		if (!fnt_mono) {
			fnt_mono = fnt_main;
		}

		ImVector<ImWchar> fnt_math_ranges;
		{
			ImWchar _ranges[] = {
				0x2000, 0x206F, // General Punctuation (for Overline)
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
			fnt_math = fnt_main;
		}

		io.Fonts->Build();
	}

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

void MinBoolFunc::ImGuiStep() {
	if (ImGui::Begin(u8"Минимизация булевых функций")) {

		ImGuiTableFlags table_flags = (ImGuiTableFlags_Borders
									   | ImGuiTableFlags_RowBg);

		// 
		// Выбор кол-ва переменных
		// 
		{
			ImGui::Text(u8"Кол-во переменных: ");

			char preview[] = {'0' + variable_count, 0};

			ImGui::SameLine();
			if (ImGui::BeginCombo("##variable_count", preview,
								  ImGuiComboFlags_WidthFitPreview)) {
				for (int i = 2; i <= MAX_VARIABLE_COUNT; i++) {
					char label[] = {'0' + i, 0};

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
					char label[] = {'x', 0xE2, 0x82, 0x80 + (i + 1), 0}; // U+2080

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
		if (variable_count != 3) {
			goto l_window_end;
		}

		{
			const char* row_header[] = {"00", "01", "11", "10"};
			const char* col_header[] = {"0", "1"};

			bool items[][4] = {
				{truth_table[0b000], truth_table[0b010], truth_table[0b110], truth_table[0b100]},
				{truth_table[0b001], truth_table[0b011], truth_table[0b111], truth_table[0b101]},
			};

			struct ItemData {
				ImGuiID id;
				ImRect rect;
				int i;
				int j;
			};

			static ItemData item_data[IM_ARRAYSIZE(items)][IM_ARRAYSIZE(items[0])];
			memset(item_data, 0, sizeof(item_data));

			ImGui::Text(u8"Карта Карно");

			ImGui::PushFont(fnt_math);
			{
				if (ImGui::Button(u8"▲")) {
					karnaugh_yoff++;
					karnaugh_yoff = wrap(karnaugh_yoff, IM_ARRAYSIZE(col_header));
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"▼")) {
					karnaugh_yoff--;
					karnaugh_yoff = wrap(karnaugh_yoff, IM_ARRAYSIZE(col_header));
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"◀")) {
					karnaugh_xoff++;
					karnaugh_xoff = wrap(karnaugh_xoff, IM_ARRAYSIZE(row_header));
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"▶")) {
					karnaugh_xoff--;
					karnaugh_xoff = wrap(karnaugh_xoff, IM_ARRAYSIZE(row_header));
				}
			}
			ImGui::PopFont();

			if (ImGui::BeginTable("Karnaugh_map", 5, table_flags)) {

				// header
				{
					ImGui::TableSetupColumn("z\\xy");

					for (int _i = 0; _i < IM_ARRAYSIZE(row_header); _i++) {
						int i = wrap(_i + karnaugh_xoff, IM_ARRAYSIZE(row_header));

						const char* label = row_header[i];
						ImGui::TableSetupColumn(label);
					}

					ImGui::TableHeadersRow();
				}

				for (int _i = 0; _i < IM_ARRAYSIZE(col_header); _i++) {
					int i = wrap(_i + karnaugh_yoff, IM_ARRAYSIZE(col_header));

					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text(col_header[i]);

					for (int _j = 0; _j < IM_ARRAYSIZE(row_header); _j++) {
						int j = wrap(_j + karnaugh_xoff, IM_ARRAYSIZE(row_header));

						ImGui::TableNextColumn();
						if (items[i][j]) {
							ImGui::PushID((i + 1) * 10000 + j);
							ImGui::Selectable("1");
							ImGui::PopID();

							item_data[i][j].id = ImGui::GetItemID();
							item_data[i][j].rect.Min = ImGui::GetItemRectMin();
							item_data[i][j].rect.Max = ImGui::GetItemRectMax();
							item_data[i][j].i = i;
							item_data[i][j].j = j;
						}
					}
				}

				ImGui::EndTable();
			}

			ImU32 area_colors[] = {
				IM_COL32(0xFF, 0x00, 0x00, 0x80),
				IM_COL32(0xFF, 0x92, 0x00, 0x80),
				IM_COL32(0x42, 0xD5, 0x2B, 0x80),
				IM_COL32(0x24, 0xE0, 0xD4, 0x80),
				IM_COL32(0x00, 0x1B, 0xFF, 0x80),
				IM_COL32(0xFF, 0x00, 0xFE, 0x80),
			};

			for (int i = 0; i < areas.size(); i++) {
				const Area& area = areas[i];

				ImDrawList* list = ImGui::GetWindowDrawList();

				for (int _y = area.y; _y < area.y + area.h; _y++) {
					int y = wrap(_y, IM_ARRAYSIZE(col_header));

					for (int _x = area.x; _x < area.x + area.w; _x++) {
						int x = wrap(_x, IM_ARRAYSIZE(row_header));

						ImRect rect = item_data[y][x].rect;

						ImU32 color = area_colors[i % IM_ARRAYSIZE(area_colors)];
						list->AddRect(rect.Min, rect.Max, color, 0, 0, 2);
					}
				}
			}

			if (dragging) {
				if (ImGui::GetActiveID() == drag_id) {

					for (int i = 0; i < IM_ARRAYSIZE(item_data); i++) {
						for (int j = 0; j < IM_ARRAYSIZE(item_data[i]); j++) {
							if (item_data[i][j].id != 0 && item_data[i][j].rect.Contains(ImGui::GetMousePos())) {
								drag_x2 = j;
								drag_y2 = i;
								goto l_break1;
							}
						}
					}

				l_break1:

					ImDrawList* list = ImGui::GetWindowDrawList();

					ImRect rect1 = item_data[drag_y][drag_x].rect;
					ImRect rect2 = item_data[drag_y2][drag_x2].rect;

					ImRect rect;
					rect.Min.x = fminf(rect1.Min.x, rect2.Min.x);
					rect.Min.y = fminf(rect1.Min.y, rect2.Min.y);

					rect.Max.x = fmaxf(rect1.Max.x, rect2.Max.x);
					rect.Max.y = fmaxf(rect1.Max.y, rect2.Max.y);

					list->AddRect(rect.Min, rect.Max, COLOR_RED, 0, 0, 4);
				} else {
					if (areas.size() < 99) {
						Area area = {};
						area.x = drag_x;
						area.y = drag_y;
						area.w = drag_x2 - drag_x + 1;
						area.h = drag_y2 - drag_y + 1;

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
						for (int y = area.y1;
							 y != wrap(area.y2 + 1, IM_ARRAYSIZE(col_header));
							 y = wrap(y + 1, IM_ARRAYSIZE(col_header))) {

							for (int x = area.x1;
								 x != wrap(area.x2 + 1, IM_ARRAYSIZE(row_header));
								 x = wrap(x + 1, IM_ARRAYSIZE(row_header))) {

								if (!items[y][x]) {
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
				for (int i = 0; i < IM_ARRAYSIZE(item_data); i++) {
					for (int j = 0; j < IM_ARRAYSIZE(item_data[i]); j++) {
						if (item_data[i][j].id != 0 && item_data[i][j].id == ImGui::GetActiveID()) {
							dragging = true;
							drag_id = item_data[i][j].id;
							drag_x = j;
							drag_y = i;
							drag_x2 = drag_x;
							drag_y2 = drag_y;
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

				ImU32 color = area_colors[i % IM_ARRAYSIZE(area_colors)];
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "S%s: (%d, %d) -> (%d, %d)", subscript, area.x1, area.y1, area.x2, area.y2);
			}

			ImGui::Text(u8"МДНФ:");
			{
				result_lua.clear();
				result_unicode.clear();

				for (int i = 0; i < areas.size(); i++) {
					const Area& area = areas[i];

					char cx = row_header[area.x1][0];
					char cy = row_header[area.x1][1];
					char cz = col_header[area.y1][0];

					bool x_changes = false;
					bool y_changes = false;
					bool z_changes = false;

					for (int y = area.y1; y <= area.y2; y++) {
						for (int x = area.x1; x <= area.x2; x++) {
							if (row_header[x][0] != cx) x_changes = true;
							if (row_header[x][1] != cy) y_changes = true;
							if (col_header[y][0] != cz) z_changes = true;
						}
					}

					if (i > 0) {
						result_lua += " or ";
						result_unicode += u8" ∨ ";
					}

					result_lua += "(";
					result_unicode += "(";

					if (!x_changes) {
						if (cx == '0') {
							result_lua += "not(x)";
							result_unicode += u8"¬x";
						} else if (cx == '1') {
							result_lua += "x";
							result_unicode += "x";
						}

						if (!y_changes || !z_changes) {
							result_lua += " and ";
							result_unicode += u8" ∧ ";
						}
					}

					if (!y_changes) {
						if (cy == '0') {
							result_lua += "not(y)";
							result_unicode += u8"¬y";
						} else if (cy == '1') {
							result_lua += "y";
							result_unicode += "y";
						}

						if (!z_changes) {
							result_lua += " and ";
							result_unicode += u8" ∧ ";
						}
					}

					if (!z_changes) {
						if (cz == '0') {
							result_lua += "not(z)";
							result_unicode += u8"¬z";
						} else if (cz == '1') {
							result_lua += "z";
							result_unicode += "z";
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

		}


	l_window_end:;

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
