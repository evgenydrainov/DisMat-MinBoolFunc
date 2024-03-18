#pragma once

#include "imgui.h"
#include "imgui_internal.h"

extern "C" {
#include "lua.h"
}

#include <vector>
#include <string>

#define MAX_VARIABLE_COUNT 5

#define COLOR_WHITE (ImColor{0xFF, 0xFF, 0xFF, 0xFF})
#define COLOR_BLACK (ImColor{0x00, 0x00, 0x00, 0xFF})
#define COLOR_RED   (ImColor{0xFF, 0x00, 0x00, 0xFF})
#define COLOR_GREEN (ImColor{0x00, 0xFF, 0x00, 0xFF})
#define COLOR_BLUE  (ImColor{0x00, 0x00, 0xFF, 0xFF})

// #define SHOW_IMGUI_DEMO

struct MinBoolFunc {
	char formula[1024];
	lua_State* L;
	int variable_count;
	bool script_error;
	char lua_err_msg[256];
	bool truth_table[1 << MAX_VARIABLE_COUNT]; // TODO: pack bits

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

	void Init();
	void Quit();

	void ImGuiStep();

	void CompileScript();
	void BuildTruthTable();
	void SetVariableCount(int _variable_count);
	void BuildKarnaughMap();
	void BuildResult(const std::vector<Area>& areas, std::string& result_lua, std::string& result_unicode, int& result_rank);
	bool IsAreaValid(const Area& area);
	void DrawArea(const Area& area, ImColor color, int area_index, int& area_label_x);
	void FindCorrectAnswer();
	void DrawAreas(const std::vector<Area>& areas);
	void ShowResultInfo(std::vector<Area>& areas, std::string& result_lua, std::string& result_unicode, int& result_rank, bool readonly = false);
};
