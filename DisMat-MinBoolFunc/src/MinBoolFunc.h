#pragma once

#include "imgui.h"

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

struct MinBoolFunc {
	char formula[1024];
	lua_State* L;
	int variable_count = 3;
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

	std::string result_lua;
	std::string result_unicode;

	int karnaugh_xoff;
	int karnaugh_yoff;

	ImFont* fnt_main;
	ImFont* fnt_mono;
	ImFont* fnt_math;

	void Init();
	void Quit();

	void ImGuiStep();

	void CompileScript();
	void BuildTruthTable();
};
