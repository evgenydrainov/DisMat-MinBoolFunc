#include "MinBoolFunc.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

static const char* const formula_hint[] = {
	"",

R"(function f(x)
	return not x
end
)",

R"(function f(x, y)
	return sheffer(x, y)
end
)",

R"(function f(x, y, z)
	return impl(xor(x, y), z)
end
)",

R"(function f(x1, x2, x3, x4)
	return peirce(x1, x2) or xor(x3, x4)
end
)",

R"(function f(x1, x2, x3, x4, x5)
	return sheffer(x1, x2) and xor(impl(x3, x4), x5)
end
)",

};

static const char* const formula_examples[] = {
u8R"(local s = "11101100"
function f(...)
	return fromstr(s, ...)
end
)",

u8R"(local t = {1,1,1,0,1,1,0,0}
function f(x, y, z)
	return fromtable(t, x, y, z)
end
)",

u8R"(local t = {
	["000"] = 1,
	["001"] = 1,
	["010"] = 1,
	["011"] = 0,
	["100"] = 1,
	["101"] = 1,
	["110"] = 0,
	["111"] = 0,
}
function f(x, y, z)
	x = x and "1" or "0"
	y = y and "1" or "0"
	z = z and "1" or "0"
	return t[x .. y .. z] == 1
end
)",

u8R"(local s = "10100111101000001001100100010001"
function f(...)
	return fromstr(s, ...)
end
)",

u8R"(function f(x, y)
	return sheffer(x, y)

	-- Или
	-- return not(x and y)
end
)",

u8R"(function f(x, y)
	return peirce(x, y)

	-- Или
	-- return not(x or y)
end
)",

u8R"(function f(x, y)
	return impl(x, y)

	-- Или
	-- return not(x) or y
end
)",

u8R"(function f(x, y)
	return xor(x, y)

	-- Или
	-- return x ~= y
end
)",

u8R"(local s = "00101100001111000010111110000100"
function f(...)
	return fromstr(s, ...)
end
)",

};

static const char* const lua_std_code =
R"(function fromstr(s, ...)
	local arg = {...}
	local index = 0
	for i = 1, #arg do
		index = index * 2
		index = index + (arg[i] and 1 or 0)
	end
	return string.sub(s, index+1, index+1)=="1"
end

function fromtable(t, ...)
	local arg = {...}
	local index = 0
	for i = 1, #arg do
		index = index * 2
		index = index + (arg[i] and 1 or 0)
	end
	return t[index+1]==1
end

function sheffer(x, y)
	return not(x and y)
end

function peirce(x, y)
	return not(x or y)
end

function impl(x, y)
	return not(x) or y
end

function xor(x, y)
	return x ~= y
end
)";

static const ImColor area_colors[] = {
	// {0xFF, 0x00, 0x00, 0x40},
	// {0xFF, 0x92, 0x00, 0x40},
	// {0x42, 0xD5, 0x2B, 0x40},
	// {0x24, 0xE0, 0xD4, 0x40},
	// {0x00, 0x1B, 0xFF, 0x40},
	// {0xFF, 0x00, 0xFE, 0x40},

	{0xFF, 0x00, 0x00, 0x40},
	{0x00, 0x00, 0xFF, 0x40},
};

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

	ImGuiStyle& style = ImGui::GetStyle();
	// style.SelectableTextAlign = {0.5, 0};

#ifdef __ANDROID__
	style.ScaleAllSizes(2);
	io.FontGlobalScale = 2;
#else
	style.ScaleAllSizes(dpi_scale);
#endif

	{
		ImVector<ImWchar> fnt_main_ranges;
		{
			ImWchar _ranges[] = {
				0x2070, 0x209F, // Superscripts and Subscripts
				0,
			};

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
			builder.AddRanges(_ranges);

			builder.BuildRanges(&fnt_main_ranges);
		}

		ImVector<ImWchar> fnt_mono_ranges;
		{
			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());

			builder.BuildRanges(&fnt_mono_ranges);
		}

		ImVector<ImWchar> fnt_math_ranges;
		{
			ImWchar _ranges[] = {
				0x2070, 0x209F, // Superscripts and Subscripts
				0x2200, 0x22FF, // Mathematical Operators (for logical and, or)
				0x25A0, 0x25FF, // Geometric Shapes (for triangle arrows)
				0,
			};

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			builder.AddRanges(_ranges);

			builder.BuildRanges(&fnt_math_ranges);
		}

		const int FONT_SIZE = 18 * dpi_scale;

#ifdef __ANDROID__

		// 
		// Fonts are loaded from main.cpp.
		// 


#else
		fnt_main = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\segoeui.ttf", FONT_SIZE, nullptr, fnt_main_ranges.Data);

		if (!fnt_main) {
			fnt_main = AddFontFromFileTTF(io.Fonts, "segoeui.ttf", FONT_SIZE, nullptr, fnt_main_ranges.Data);
		}

		fnt_mono = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\cour.ttf", FONT_SIZE, nullptr, fnt_mono_ranges.Data);

		if (!fnt_mono) {
			fnt_mono = AddFontFromFileTTF(io.Fonts, "cour.ttf", FONT_SIZE, nullptr, fnt_mono_ranges.Data);
		}

		fnt_math = AddFontFromFileTTF(io.Fonts, "C:\\Windows\\Fonts\\cambria.ttc", FONT_SIZE, nullptr, fnt_math_ranges.Data);

		if (!fnt_math) {
			fnt_math = AddFontFromFileTTF(io.Fonts, "cambria.ttc", FONT_SIZE, nullptr, fnt_math_ranges.Data);
		}
#endif

		if (!fnt_main) {
			fnt_main = io.Fonts->AddFontDefault();
		}

		if (!fnt_mono) {
			fnt_mono = fnt_main;
		}

		if (!fnt_math) {
			fnt_math = fnt_main;
		}

		io.Fonts->Build();
	}

	io.IniFilename = nullptr;

	ImStrncpy(formula, formula_examples[0], sizeof(formula));

	L = luaL_newstate();
	luaL_openlibs(L);

	SetVariableCount(3);
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

#ifndef SHOW_IMGUI_DEMO
		ImGui::SetNextWindowPos({});
		ImGui::SetNextWindowSize(io.DisplaySize);
#endif
	}

	if (ImGui::Begin(u8"Минимизация булевых функций", nullptr,
					 ImGuiWindowFlags_NoDecoration
					 | ImGuiWindowFlags_NoSavedSettings)) {

		ImGuiTableFlags table_flags = (ImGuiTableFlags_Borders
									   | ImGuiTableFlags_RowBg
									   /* | ImGuiTableFlags_SizingFixedSame
									   | ImGuiTableFlags_NoHostExtendX */
									   );

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
						if (formula[0] == 0) {
							ImStrncpy(formula, formula_hint[i], sizeof(formula));
						}

						SetVariableCount(i);
					}

					if (i == variable_count) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			ImGui::SameLine();
			if (ImGui::Button(u8"Примеры")) {
				ImGui::OpenPopup("example_context");
			}

			if (ImGui::BeginPopupContextItem("example_context")) {
				if (ImGui::Button(u8"Вектор функции с тремя переменными")) {
					ImStrncpy(formula, formula_examples[0], sizeof(formula));
					SetVariableCount(3);
				}

				if (ImGui::Button(u8"Вектор функции с пятью переменными 1")) {
					ImStrncpy(formula, formula_examples[3], sizeof(formula));
					SetVariableCount(5);
				}

				if (ImGui::Button(u8"Вектор функции с пятью переменными 2")) {
					ImStrncpy(formula, formula_examples[8], sizeof(formula));
					SetVariableCount(5);
				}

				if (ImGui::Button(u8"Штрих Шеффера")) {
					ImStrncpy(formula, formula_examples[4], sizeof(formula));
					SetVariableCount(2);
				}

				if (ImGui::Button(u8"Стрелка Пирса")) {
					ImStrncpy(formula, formula_examples[5], sizeof(formula));
					SetVariableCount(2);
				}

				if (ImGui::Button(u8"Импликация")) {
					ImStrncpy(formula, formula_examples[6], sizeof(formula));
					SetVariableCount(2);
				}

				if (ImGui::Button(u8"Сложение по модулю 2")) {
					ImStrncpy(formula, formula_examples[7], sizeof(formula));
					SetVariableCount(2);
				}

				ImGui::EndPopup();
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
					ImGui::TableSetupColumn(var_names_unicode[i]);
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
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));

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
					karnaugh_xoff = wrap(karnaugh_xoff, (variable_count == 5) ? 4 : row_header.size());
				}

				ImGui::SameLine();
				if (ImGui::Button(u8"▶")) {
					karnaugh_xoff--;
					karnaugh_xoff = wrap(karnaugh_xoff, (variable_count == 5) ? 4 : row_header.size());
				}
			}
			ImGui::PopFont();

			ImVec2 line_p1 = {};
			ImVec2 line_p2 = {};

			int column_count = row_header.size() + 1;
			float table_width = 0; // 90 * column_count;
			if (ImGui::BeginTable("Karnaugh_map", column_count, table_flags, {table_width, 0})) {

				float row_height = ImGui::GetTextLineHeight() * 1.5f;

				// header
				if (variable_count == 5) {
					{
						ImGui::TableNextRow(0, row_height);

						ImGui::TableNextColumn();
						ImGui::Text(u8"x₃");

						ImGui::TableNextColumn();
						ImGui::Text("0");

						ImGui::TableNextColumn();

						ImGui::TableNextColumn();

						ImGui::TableNextColumn();

						ImGui::TableNextColumn();

						line_p1.x = ImGui::GetCursorScreenPos().x - ImGui::GetStyle().CellPadding.x;
						line_p1.y = ImGui::GetCursorScreenPos().y - ImGui::GetStyle().CellPadding.y;

						ImGui::Text("1");
					}

					{
						ImGui::TableNextRow(0, row_height);

						ImGui::TableNextColumn();
						ImGui::Text(u8"x₁x₂\\x₄x₅");

						{
							ImVec2 cursor = ImGui::GetCursorScreenPos();
							// ImGui::GetWindowDrawList()->AddRect(cursor, {cursor.x + 10, cursor.y + 10}, COLOR_BLUE);

							ImVec2 content;
							content.x = ImGui::GetContentRegionAvail().x;
							content.y = row_height - 2.0f * ImGui::GetStyle().CellPadding.y;

							// ImGui::GetWindowDrawList()->AddRect(cursor, {cursor.x + content.x, cursor.y + content.y}, COLOR_BLUE);

						}

						for (int i = 0; i < 2; i++) {
							const char* labels[] = {"00", "01", "11", "10"};

							for (int i = 0; i < 4; i++) {
								const char* label = labels[wrap(i + karnaugh_xoff, 4)];

								ImGui::TableNextColumn();
								ImGui::TextUnformatted(label);
							}
						}
					}
				} else {
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

					ImGui::TableNextRow(0, row_height);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(col_header[i]);

					for (int _j = 0; _j < row_header.size(); _j++) {
						int j;
						if (variable_count == 5) {
							if (_j >= 4) {
								j = 4 + wrap(_j - 4 + karnaugh_xoff, 4);
							} else {
								j = wrap(_j + karnaugh_xoff, 4);
							}
						} else {
							j = wrap(_j + karnaugh_xoff, row_header.size());
						}

						ImGui::TableNextColumn();

						if (_i == 3 && _j == 4) {
							line_p2.x = ImGui::GetCursorScreenPos().x - ImGui::GetStyle().CellPadding.x;
							line_p2.y = row_height + ImGui::GetCursorScreenPos().y + ImGui::GetStyle().CellPadding.y;
						}

						int w = row_header.size();
						if (cells[j + i * w]) {
							ImGui::PushID((i + 1) * 10000 + j);
							ImGui::Selectable("1", false, 0, {0, row_height});
							ImGui::PopID();

							cell_rects[i][j] = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
							cell_rects_abs[_i][_j] = {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
							cell_ids[i][j] = ImGui::GetItemID();
							cell_ids_abs[_i][_j] = ImGui::GetItemID();
						} else {
							cell_rects[i][j] = {};
							cell_rects_abs[_i][_j] = {};
							cell_ids[i][j] = 0;
							cell_ids_abs[_i][_j] = 0;
						}
					}
				}

				ImGui::EndTable();
			}

			if (variable_count == 5) {
				ImDrawList* list = ImGui::GetWindowDrawList();
				list->AddLine(line_p1, line_p2, COLOR_RED, 2);
			}

			// areas = {
			// 	{3, 0, 2, 2},
			// 	{0, 0, 2, 1},
			// };

			if (show_correct_answer) {
				DrawAreas(areas2);
			} else {
				DrawAreas(areas);
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

						// must be in left or right area
						if (drag_x1 < 4) {
							if (drag_x2 >= 4) {
								ImGui::OpenPopup("###area_must_be_in_one_area");
								goto l_area_add_out;
							}
						}
						if (drag_x1 >= 4) {
							if (drag_x2 < 4) {
								ImGui::OpenPopup("###area_must_be_in_one_area");
								goto l_area_add_out;
							}
						}

						area.x = ImMin(drag_x1, drag_x2) + karnaugh_xoff;
						area.y = ImMin(drag_y1, drag_y2) + karnaugh_yoff;

						if (variable_count == 5) {
							if (drag_x1 >= 4) {
								area.x = 4 + wrap(area.x - 4, 4);
							} else {
								area.x = wrap(area.x, 4);
							}
						} else {
							area.x = wrap(area.x, row_header.size());
						}

						area.y = wrap(area.y, col_header.size());

						area.w = ImAbs(drag_x2 - drag_x1) + 1;
						area.h = ImAbs(drag_y2 - drag_y1) + 1;

						// check if area already exists
						for (int i = 0; i < areas.size(); i++) {
							if (area.x == areas[i].x && area.y == areas[i].y
								&& area.w == areas[i].w && area.h == areas[i].h) {
								ImGui::OpenPopup("###area_already_exists");
								goto l_area_add_out;
							}
						}

						int why;
						if (!IsAreaValid(area, &why)) {
							if (why == 0) {
								ImGui::OpenPopup("###area_must_be_power_of_2");
							} else if (why == 1) {
								ImGui::OpenPopup("###area_must_contain_1s");
							}
							goto l_area_add_out;
						}

						areas.push_back(area);
						BuildResult(areas, result_lua, result_unicode, result_rank);
					}

				l_area_add_out:

					dragging = false;
					drag_id = 0;
				}
			} else {
				for (int i = 0; i < col_header.size(); i++) {
					for (int j = 0; j < row_header.size(); j++) {
						if (!show_correct_answer && cell_ids_abs[i][j] != 0 && cell_ids_abs[i][j] == ImGui::GetActiveID()) {
							dragging = true;
							drag_id = cell_ids_abs[i][j];
							drag_x1 = j;
							drag_y1 = i;
							drag_x2 = drag_x1;
							drag_y2 = drag_y1;
							goto l_break2;
						}

						if (cell_ids_abs[i][j] != 0 && cell_ids_abs[i][j] == ImGui::GetHoveredID()) {
							ImDrawList* list = ImGui::GetWindowDrawList();
							ImRect rect = cell_rects_abs[i][j];
							list->AddRect(rect.Min, rect.Max, COLOR_RED, 0, 0, 2);
						}
					}
				}

			l_break2:;

			}

			ImGui::Checkbox(u8"Показать подобранный ответ", &show_correct_answer);

			if (show_correct_answer) {
				ShowResultInfo(areas2, result2_lua, result2_unicode, result2_rank, true);
			} else {
				ShowResultInfo(areas, result_lua, result_unicode, result_rank);

				if (result_rank == result2_rank) {
					ImColor color = {30, 210, 30, 255};
					// ImColor color = {0, 255, 0, 255};
					const char* text = u8"Сложность формулы совпадает с подобранным ответом.";
					// ImVec2 cursor = ImGui::GetCursorScreenPos();
					// ImGui::GetWindowDrawList()->AddText({cursor.x + 1, cursor.y + 1}, IM_COL32(0, 0, 0, 128), text);
					ImGui::TextColored(color, "%s", text);
				} else {
					ImGui::TextColored(COLOR_RED, u8"Сложность формулы не совпадает с подобранным ответом.");
				}
			}

			// printf("result_lua capacity: %zu\n", result_lua.capacity());
			// printf("result_unicode capacity: %zu\n", result_unicode.capacity());

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

		}


	l_window_end:;

		ImGuiPopupFlags popup_flags = (ImGuiWindowFlags_NoResize
									   | ImGuiWindowFlags_NoMove
									   | ImGuiWindowFlags_NoScrollbar
									   | ImGuiWindowFlags_NoCollapse);

		// ImGui::PushItemWidth(50 * dpi_scale * ImGui::GetIO().FontGlobalScale);
		if (ImGui::BeginPopupModal(u8"Ошибка###area_must_be_in_one_area", nullptr, popup_flags)) {
			ImGui::Text(u8"Область должна находиться либо в левой, либо в правой части карты Карно.\n"
						u8"(Симметричные области покрываются автоматически).");
			if (ImGui::Button(u8"Ок")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		// ImGui::PopItemWidth();

		if (ImGui::BeginPopupModal(u8"Ошибка###area_already_exists", nullptr, popup_flags)) {
			ImGui::Text(u8"Эта область уже существует.");
			if (ImGui::Button(u8"Ок")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal(u8"Ошибка###area_must_be_power_of_2", nullptr, popup_flags)) {
			ImGui::Text(u8"Площадь области должна быть степенью двойки.");
			if (ImGui::Button(u8"Ок")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal(u8"Ошибка###area_must_contain_1s", nullptr, popup_flags)) {
			ImGui::Text(u8"Область должна покрывать только единицы.");
			if (ImGui::Button(u8"Ок")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

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
	result_lua.clear();
	result_unicode.clear();
	result_rank = 0;
	karnaugh_xoff = 0;
	karnaugh_yoff = 0;
	show_correct_answer = false;

	script_error = true;

	if (luaL_loadstring(L, lua_std_code) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		ImStrncpy(lua_err_msg, err, sizeof(lua_err_msg));
		lua_settop(L, 0);
		return;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		ImStrncpy(lua_err_msg, err, sizeof(lua_err_msg));
		lua_settop(L, 0);
		return;
	}

	if (luaL_loadstring(L, formula) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		ImStrncpy(lua_err_msg, err, sizeof(lua_err_msg));
		lua_settop(L, 0);
		return;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		const char* err = lua_tostring(L, -1);
		ImStrncpy(lua_err_msg, err, sizeof(lua_err_msg));
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
		ImFormatString(lua_err_msg, sizeof(lua_err_msg), u8"Переменная \"f\" не является функцией (%s).", tname);
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
			ImStrncpy(lua_err_msg, err, sizeof(lua_err_msg));
			lua_settop(L, 0);
			script_error = true;
			return;
		}

		bool result = lua_toboolean(L, -1);
		truth_table[i] = result;

		lua_pop(L, 1); // pop result
	}

	lua_settop(L, 0);

	BuildKarnaughMap();
}

void MinBoolFunc::SetVariableCount(int _variable_count) {
	variable_count = _variable_count;

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

	CompileScript();
}

void MinBoolFunc::BuildKarnaughMap() {
	switch (variable_count) {
		case 2: {
			row_header = {"0", "1"}; // y
			col_header = {"0", "1"}; // x
			cells = {
				truth_table[0b00], truth_table[0b01],
				truth_table[0b10], truth_table[0b11],
			};
			cell_header = "x\\y";
			row_header2 = {"0..", "1.."};
			col_header2 = {"0..", "1.."};
			break;
		}
		case 3: {
			row_header = {"00", "01", "11", "10"}; // yz
			col_header = {"0", "1"}; // x
			cells = {
				truth_table[0b000], truth_table[0b001], truth_table[0b011], truth_table[0b010],
				truth_table[0b100], truth_table[0b101], truth_table[0b111], truth_table[0b110],
			};
			cell_header = "x\\yz";
			row_header2 = {"00.", "01.", "11.", "10."};
			col_header2 = {"0..", "1.."};
			break;
		}
		case 4: {
			row_header = {"00", "01", "11", "10"}; // x3 x4
			col_header = {"00", "01", "11", "10"}; // x1 x2
			cells = {
				truth_table[0b0000], truth_table[0b0001], truth_table[0b0011], truth_table[0b0010],
				truth_table[0b0100], truth_table[0b0101], truth_table[0b0111], truth_table[0b0110],
				truth_table[0b1100], truth_table[0b1101], truth_table[0b1111], truth_table[0b1110],
				truth_table[0b1000], truth_table[0b1001], truth_table[0b1011], truth_table[0b1010],
			};
			cell_header = u8"x₁x₂\\x₃x₄";
			row_header2 = {"00.", "01.", "11.", "10."};
			col_header2 = {"00.", "01.", "11.", "10."};
			break;
		}
		case 5: {
			// 
			// x1 x2 \ x3 x4 x5
			// 
			// row_header = {"000", "001", "011", "010", "110", "111", "101", "100"}; // x3 x4 x5
			// col_header = {"00", "01", "11", "10"}; // x1 x2
			// cells = {
			// 	truth_table[0b00000], truth_table[0b00001], truth_table[0b00011], truth_table[0b00010], truth_table[0b00110], truth_table[0b00111], truth_table[0b00101], truth_table[0b00100],
			// 	truth_table[0b01000], truth_table[0b01001], truth_table[0b01011], truth_table[0b01010], truth_table[0b01110], truth_table[0b01111], truth_table[0b01101], truth_table[0b01100],
			// 	truth_table[0b11000], truth_table[0b11001], truth_table[0b11011], truth_table[0b11010], truth_table[0b11110], truth_table[0b11111], truth_table[0b11101], truth_table[0b11100],
			// 	truth_table[0b10000], truth_table[0b10001], truth_table[0b10011], truth_table[0b10010], truth_table[0b10110], truth_table[0b10111], truth_table[0b10101], truth_table[0b10100],
			// };
			// cell_header = u8"x₁x₂\\x₃x₄x₅";
			// row_header2 = {"000", "001", "011", "010", "110", "111", "101", "100"};
			// col_header2 = {"00.", "01.", "11.", "10."};

			// 
			// x3
			// x1 x2 \ x3 x4
			// 
			row_header = {"000", "001", "011", "010", "100", "101", "111", "110"}; // x3 x4 x5
			col_header = {"00", "01", "11", "10"}; // x1 x2
			cells = {
				truth_table[0b00000], truth_table[0b00001], truth_table[0b00011], truth_table[0b00010], truth_table[0b00100], truth_table[0b00101], truth_table[0b00111], truth_table[0b00110],
				truth_table[0b01000], truth_table[0b01001], truth_table[0b01011], truth_table[0b01010], truth_table[0b01100], truth_table[0b01101], truth_table[0b01111], truth_table[0b01110],
				truth_table[0b11000], truth_table[0b11001], truth_table[0b11011], truth_table[0b11010], truth_table[0b11100], truth_table[0b11101], truth_table[0b11111], truth_table[0b11110],
				truth_table[0b10000], truth_table[0b10001], truth_table[0b10011], truth_table[0b10010], truth_table[0b10100], truth_table[0b10101], truth_table[0b10111], truth_table[0b10110],
			};
			cell_header = "?";
			row_header2 = {"000", "001", "011", "010", "100", "101", "111", "110"};
			col_header2 = {"00", "01", "11", "10"};
			break;
		}
	}

	cell_rects.resize(col_header.size());
	cell_rects_abs.resize(col_header.size());
	cell_ids.resize(col_header.size());
	cell_ids_abs.resize(col_header.size());

	for (auto& v : cell_rects) v.resize(row_header.size());
	for (auto& v : cell_rects_abs) v.resize(row_header.size());
	for (auto& v : cell_ids) v.resize(row_header.size());
	for (auto& v : cell_ids_abs) v.resize(row_header.size());

	FindCorrectAnswer();
}

void MinBoolFunc::BuildResult(const std::vector<Area>& areas, std::string& result_lua, std::string& result_unicode, int& result_rank) {
	result_lua.clear();
	result_unicode.clear();
	result_rank = 0;

	for (int i = 0; i < areas.size(); i++) {
		const Area& area = areas[i];

		char v1 = row_header2[area.x][0]; // y y x3 x3
		char v2 = row_header2[area.x][1]; //   z x4 x4
		char v3 = row_header2[area.x][2]; //        x5
		char v4 = col_header2[area.y][0]; // x x x1 x1
		char v5 = col_header2[area.y][1]; //     x2 x2

		bool v1_changes = false;
		bool v2_changes = false;
		bool v3_changes = false;
		bool v4_changes = false;
		bool v5_changes = false;

		for (int _y = area.y; _y < area.y + area.h; _y++) {
			int y = wrap(_y, col_header2.size());
			for (int _x = area.x; _x < area.x + area.w; _x++) {

				int x;
				if (variable_count == 5) {
					if (area.x >= 4) {
						x = 4 + wrap(_x - 4, 4);
					} else {
						x = wrap(_x, 4);
					}
				} else {
					x = wrap(_x, row_header.size());
				}

				if (row_header2[x][0] != v1) v1_changes = true;
				if (row_header2[x][1] != v2) v2_changes = true;
				if (row_header2[x][2] != v3) v3_changes = true;
				if (col_header2[y][0] != v4) v4_changes = true;
				if (col_header2[y][1] != v5) v5_changes = true;
			}
		}

		if (variable_count == 5) {
			Area area2 = area;
			if (area.x < 4) {
				area2.x = 4 + wrap(area.x, 4);
			}
			if (area.x >= 4) {
				area2.x = wrap(area.x, 4);
			}

			if (IsAreaValid(area2)) {
				for (int _y = area2.y; _y < area2.y + area2.h; _y++) {
					int y = wrap(_y, col_header2.size());
					for (int _x = area2.x; _x < area2.x + area2.w; _x++) {

						int x;
						if (variable_count == 5) {
							if (area2.x >= 4) {
								x = 4 + wrap(_x - 4, 4);
							} else {
								x = wrap(_x, 4);
							}
						} else {
							x = wrap(_x, row_header.size());
						}

						if (row_header2[x][0] != v1) v1_changes = true;
						if (row_header2[x][1] != v2) v2_changes = true;
						if (row_header2[x][2] != v3) v3_changes = true;
						if (col_header2[y][0] != v4) v4_changes = true;
						if (col_header2[y][1] != v5) v5_changes = true;
					}
				}
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
				x_changes[0] = v4_changes;
				x_changes[1] = v1_changes;
				x[0] = v4;
				x[1] = v1;
				break;
			}
			case 3: {
				x_changes[0] = v4_changes;
				x_changes[1] = v1_changes;
				x_changes[2] = v2_changes;
				x[0] = v4;
				x[1] = v1;
				x[2] = v2;
				break;
			}
			case 4: {
				x_changes[0] = v4_changes;
				x_changes[1] = v5_changes;
				x_changes[2] = v1_changes;
				x_changes[3] = v2_changes;
				x[0] = v4;
				x[1] = v5;
				x[2] = v1;
				x[3] = v2;
				break;
			}
			case 5: {
				x_changes[0] = v4_changes;
				x_changes[1] = v5_changes;
				x_changes[2] = v1_changes;
				x_changes[3] = v2_changes;
				x_changes[4] = v3_changes;
				x[0] = v4;
				x[1] = v5;
				x[2] = v1;
				x[3] = v2;
				x[4] = v3;
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

				result_rank++;
			}
		}

		result_lua += ")";
		result_unicode += ")";
	}
}

bool MinBoolFunc::IsAreaValid(const Area& area, int* why) {
	int S = area.w * area.h;
	if (!ImIsPowerOfTwo(S)) {
		if (why) *why = 0;
		return false;
	}

	// must contain only 1's
	for (int _y = area.y; _y < area.y + area.h; _y++) {
		int y = wrap(_y, col_header.size());
		for (int _x = area.x; _x < area.x + area.w; _x++) {

			int x;
			if (variable_count == 5) {
				if (area.x >= 4) {
					x = 4 + wrap(_x - 4, 4);
				} else {
					x = wrap(_x, 4);
				}
			} else {
				x = wrap(_x, row_header.size());
			}

			int w = row_header.size();
			if (!cells[x + y * w]) {
				if (why) *why = 1;
				return false;
			}
		}
	}

	return true;
}

void MinBoolFunc::DrawArea(const Area& area, ImColor color, int area_index, int& area_label_x) {
	ImDrawList* list = ImGui::GetWindowDrawList();

	for (int _y = area.y; _y < area.y + area.h; _y++) {
		int y = wrap(_y, col_header.size());

		for (int _x = area.x; _x < area.x + area.w; _x++) {

			int x;
			if (variable_count == 5) {
				if (area.x >= 4) {
					x = 4 + wrap(_x - 4, 4);
				} else {
					x = wrap(_x, 4);
				}
			} else {
				x = wrap(_x, row_header.size());
			}

			ImRect rect = cell_rects[y][x];
			ImGuiID id = cell_ids[y][x];

			list->AddRectFilled(rect.Min, rect.Max, color);

			if (id == ImGui::GetHoveredID()) {

				int i = area_index;
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

				char buf[16];
				ImFormatString(buf, sizeof(buf), "S%s", subscript);
				ImU32 color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
				ImVec2 text_size = ImGui::CalcTextSize(buf);
				ImVec2 pos = ImGui::GetMousePos();
				pos.x += area_label_x;
				pos.y -= text_size.y + 4 * dpi_scale * ImGui::GetIO().FontGlobalScale;
				ImVec2 pmin = pos;
				ImVec2 pmax = {pos.x + text_size.x, pos.y + text_size.y};
				ImGui::GetForegroundDrawList()->AddRectFilled(pmin, pmax, IM_COL32(255, 255, 255, 220));
				ImGui::GetForegroundDrawList()->AddText(pos, color, buf);

				area_label_x += 15 * dpi_scale * ImGui::GetIO().FontGlobalScale;
			}
		}
	}
}

void MinBoolFunc::FindCorrectAnswer() {
	areas2.clear();

	auto check_for_area = [&](const Area& area, int cell_x, int cell_y) {

		for (int _y = area.y; _y < area.y + area.h; _y++) {
			int y = wrap(_y, col_header.size());

			for (int _x = area.x; _x < area.x + area.w; _x++) {

				int x;
				if (variable_count == 5) {
					if (area.x >= 4) {
						x = 4 + wrap(_x - 4, 4);
					} else {
						x = wrap(_x, 4);
					}
				} else {
					x = wrap(_x, row_header.size());
				}

				if (x == cell_x && y == cell_y) {
					return true;
				}
			}
		}
		return false;
	};

	auto is_cell_covered = [&](int cell_x, int cell_y, int area_ignore_index = -1) {

		for (int i = 0; i < areas2.size(); i++) {
			if (i == area_ignore_index) {
				continue;
			}

			const Area& area = areas2[i];

			if (check_for_area(area, cell_x, cell_y)) {
				return true;
			}

			if (variable_count == 5) {
				Area area2 = area;
				if (area.x < 4) {
					area2.x = 4 + wrap(area.x, 4);
				}
				if (area.x >= 4) {
					area2.x = wrap(area.x, 4);
				}

				if (IsAreaValid(area2)) {
					if (check_for_area(area2, cell_x, cell_y)) {
						return true;
					}
				}
			}
		}

		return false;
	};

	for (int y = 0; y < col_header.size(); y++) {
		for (int x = 0; x < row_header.size(); x++) {

			int w = row_header.size();
			if (!cells[x + y * w]) {
				continue;
			}

			if (is_cell_covered(x, y)) {
				continue;
			}

			std::vector<Area> areas_to_try;

			// S = 16
			if (variable_count >= 4) areas_to_try.push_back({x, y, 4, 4});

			// S = 8
			if (variable_count >= 3) areas_to_try.push_back({x, y, 4, 2});
			if (variable_count >= 4) areas_to_try.push_back({x, y - 1, 4, 2});
			if (variable_count >= 4) areas_to_try.push_back({x, y, 2, 4});
			if (variable_count >= 4) areas_to_try.push_back({x - 1, y, 2, 4});

			// S = 4
			if (variable_count >= 3) areas_to_try.push_back({x, y, 4, 1});
			if (variable_count >= 4) areas_to_try.push_back({x, y, 1, 4});
			areas_to_try.push_back({x, y, 2, 2});
			if (variable_count >= 3) areas_to_try.push_back({x - 1, y, 2, 2});
			if (variable_count >= 4) areas_to_try.push_back({x, y - 1, 2, 2});
			if (variable_count >= 4) areas_to_try.push_back({x - 1, y - 1, 2, 2});

			// S = 2
			areas_to_try.push_back({x, y, 2, 1});
			if (variable_count >= 3) areas_to_try.push_back({x - 1, y, 2, 1});
			areas_to_try.push_back({x, y, 1, 2});
			if (variable_count >= 4) areas_to_try.push_back({x, y - 1, 1, 2});

			// S = 1
			areas_to_try.push_back({x, y, 1, 1});

			for (Area& area : areas_to_try) {
				if (variable_count == 5) {
					if (x >= 4) {
						area.x = 4 + wrap(area.x - 4, 4);
					} else {
						area.x = wrap(area.x, 4);
					}
				} else {
					area.x = wrap(area.x, row_header.size());
				}

				area.y = wrap(area.y, col_header.size());
			}

			for (Area& area : areas_to_try) {
				if (IsAreaValid(area)) {
					areas2.push_back(area);
					break;
				}
			}


		}
	}

	// get rid of redundant areas

	for (int i = 0; i < areas2.size(); i++) {
		const Area& area = areas2[i];

		bool all_cells_covered = true;

		for (int _y = area.y; _y < area.y + area.h; _y++) {
			int y = wrap(_y, col_header.size());

			for (int _x = area.x; _x < area.x + area.w; _x++) {

				int x;
				if (variable_count == 5) {
					if (area.x >= 4) {
						x = 4 + wrap(_x - 4, 4);
					} else {
						x = wrap(_x, 4);
					}
				} else {
					x = wrap(_x, row_header.size());
				}

				all_cells_covered &= is_cell_covered(x, y, i);
			}
		}

		if (all_cells_covered) {
			areas2.erase(areas2.begin() + i);
			i--;
			continue;
		}
	}

	BuildResult(areas2, result2_lua, result2_unicode, result2_rank);
}

void MinBoolFunc::DrawAreas(const std::vector<Area>& areas) {
	int area_label_x = 0;
	for (int i = 0; i < areas.size(); i++) {
		const Area& area = areas[i];

		ImColor color = area_colors[i % IM_ARRAYSIZE(area_colors)];

		DrawArea(area, color, i, area_label_x);

		if (variable_count == 5) {
			Area area2 = area;
			if (area.x < 4) {
				area2.x = 4 + wrap(area.x, 4);
			}
			if (area.x >= 4) {
				area2.x = wrap(area.x, 4);
			}

			if (IsAreaValid(area2)) {
				DrawArea(area2, color, i, area_label_x);
			}
		}
	}
}

void MinBoolFunc::ShowResultInfo(std::vector<Area>& areas, std::string& result_lua, std::string& result_unicode, int& result_rank, bool readonly) {
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

		if (!readonly) {
			ImGui::PushID((i + 1) * 100);
			bool b = ImGui::Button(" - ");
			ImGui::PopID();

			if (b) {
				areas.erase(areas.begin() + i);
				i--;
				BuildResult(areas, result_lua, result_unicode, result_rank);
				continue;
			}

			ImGui::SameLine();
		}

		ImVec4 color = ImGui::ColorConvertU32ToFloat4(area_colors[i % IM_ARRAYSIZE(area_colors)]);
		color.w = 1;
		ImGui::TextColored(color,
						   "S%s: (%d,%d), (%dx%d)",
						   subscript, area.x, area.y, area.w, area.h);
	}

	ImGui::Text(u8"МДНФ: (ПКМ - скопировать)");

	if (!result_lua.empty()) {
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
	}

	if (!result_unicode.empty()) {
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
	}

	ImGui::Text(u8"Сложность: %d", result_rank);
}
