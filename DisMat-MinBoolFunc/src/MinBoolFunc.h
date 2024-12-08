#pragma once

#include "imgui.h"
#include "imgui_internal.h"

extern "C" {
#include "lua.h"
}

#include <vector>
#include <string>

#define PROGRAM_VERSION "1.1.1"

#define MAX_VARIABLE_COUNT 5

#define COLOR_WHITE (ImColor{0xFF, 0xFF, 0xFF, 0xFF})
#define COLOR_BLACK (ImColor{0x00, 0x00, 0x00, 0xFF})
#define COLOR_RED   (ImColor{0xFF, 0x00, 0x00, 0xFF})
#define COLOR_GREEN (ImColor{0x00, 0xFF, 0x00, 0xFF})
#define COLOR_BLUE  (ImColor{0x00, 0x00, 0xFF, 0xFF})

// #define SHOW_IMGUI_DEMO

enum InputMethod {
	INPUT_METHOD_VECTOR,
	INPUT_METHOD_FORMULA,
};

struct MinBoolFunc {
	char formula[1024];
	char function_vector[256];
	lua_State* L;
	int variable_count;
	int var_count_formula;
	int var_count_vector;
	bool script_error;
	char lua_err_msg[256];
	bool truth_table[1 << MAX_VARIABLE_COUNT]; // TODO: pack bits
											   // No, actual TODO: rewrite the whole program.
	InputMethod input_method;

	bool dragging;
	ImGuiID drag_id;
	int drag_x1;
	int drag_y1;
	int drag_x2;
	int drag_y2;

	struct Area {
		int x;
		int y;
		int w;
		int h;
	};
	std::vector<Area> areas;
	std::vector<Area> areas2;

	std::string result_lua;
	std::string result2_lua;
	std::string result_unicode;
	std::string result2_unicode;
	int result_rank = 0;
	int result2_rank = 0;
	std::vector<std::string> results;
	std::vector<std::string> results2;
	std::vector<bool> result_show;
	std::vector<bool> result2_show;
	bool show_mdnf;
	bool show_rank;

	int karnaugh_xoff;
	int karnaugh_yoff;

	std::vector<const char*> var_names_lua;
	std::vector<const char*> var_names_unicode;

	std::vector<const char*> row_header;
	std::vector<const char*> col_header;
	std::vector<const char*> row_header2;
	std::vector<const char*> col_header2;
	std::vector<bool> cells;
	const char* cell_header = "?";

	std::vector<std::vector<ImRect>> cell_rects;
	std::vector<std::vector<ImRect>> cell_rects_abs;
	std::vector<std::vector<ImGuiID>> cell_ids;
	std::vector<std::vector<ImGuiID>> cell_ids_abs;

	bool show_correct_answer;

	float dpi_scale = 1;

	ImFont* fnt_main;
	ImFont* fnt_mono;
	ImFont* fnt_math;
	ImFont* fnt_main_bold;

	void Init();
	void Quit();

	void ImGuiStep();

	bool lua_load_string_and_pcall(const char* string, bool with_hook = true);
	void CompileScript();
	void BuildTruthTable();
	void SetVariableCount(int _variable_count);
	void BuildKarnaughMap();
	void BuildResult(const std::vector<Area>& areas, std::string& result_lua,
					 std::string& result_unicode, int& result_rank,
					 std::vector<std::string>& results, std::vector<bool>& result_show);
	bool IsAreaValid(const Area& area, int* why = nullptr);
	void DrawArea(const Area& area, ImColor color, int area_index, int& area_label_x);
	int area_wrap_x(const Area& area, int _x);
	int area_wrap_y(const Area& area, int _y);
	bool is_cell_covered_by_area(const Area& area, int cell_x, int cell_y);
	bool is_cell_covered(const std::vector<Area>& areas, int cell_x, int cell_y, int area_ignore_index = -1);
	bool is_area_covered(const std::vector<Area>& areas, const Area& area, int area_ignore_index = -1);
	void FindCorrectAnswer();
	void DrawAreas(const std::vector<Area>& areas);
	void ShowResultInfo(std::vector<Area>& areas, std::string& result_lua,
						std::string& result_unicode, int& result_rank,
						std::vector<std::string>& results, std::vector<bool>& result_show,
						bool readonly = false);
};
