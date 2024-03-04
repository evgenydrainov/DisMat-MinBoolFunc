#pragma once

#include "C:\Users\Username\source\repos\DisMat-MinBoolFunc\MicroTex\src\config.h"
#include "C:\Users\Username\source\repos\DisMat-MinBoolFunc\MicroTex\src\common.h"
#include "C:\Users\Username\source\repos\DisMat-MinBoolFunc\MicroTex\src\graphic\graphic.h"

#include "C:\Users\Username\source\repos\DisMat-MinBoolFunc\DisMat-MinBoolFunc\src\imgui\imgui.h"

namespace tex {

	class Font_imgui : public Font
	{
	public:
		Font_imgui(const std::string& name, int style, float size);
		Font_imgui(const std::string& file, float size);

		float getSize() const { return size; }
		sptr<Font> deriveFont(int style) const;

		bool operator==(const Font& f) const;
		bool operator!=(const Font& f) const;

		static ImFont* font_default;

	private:
		ImFont* font = nullptr;
		float size = 0;
		int style = 0;
		std::string name;
		std::string file;
		float custom_scale = 1;

		friend class Graphics2D_imgui;
	};

	class TextLayout_imgui : public TextLayout
	{
	public:
		TextLayout_imgui(const std::wstring& src, const sptr<Font_imgui>& font);

		void getBounds(Rect& bounds);
		void draw(Graphics2D& g2, float x, float y);

	private:

	};

	class Graphics2D_imgui : public Graphics2D
	{
	public:
		Graphics2D_imgui();

		void setColor(color c);
		color getColor() const;
		void setStroke(const Stroke& s);
		const Stroke& getStroke() const;
		void setStrokeWidth(float w);
		const Font* getFont() const;
		void setFont(const Font* font);
		void translate(float dx, float dy);
		void scale(float sx, float sy);
		void rotate(float angle);
		void rotate(float angle, float px, float py);
		void reset();
		float sx() const;
		float sy() const;
		void drawChar(wchar_t c, float x, float y);
		void drawText(const std::wstring& c, float x, float y);
		void drawLine(float x1, float y1, float x2, float y2);
		void drawRect(float x, float y, float w, float h);
		void fillRect(float x, float y, float w, float h);
		void drawRoundRect(float x, float y, float w, float h, float rx, float ry);
		void fillRoundRect(float x, float y, float w, float h, float rx, float ry);

		ImDrawList* list = nullptr;

	private:
		float translation_x = 0;
		float translation_y = 0;
		float scale_x = 1;
		float scale_y = 1;
		const Font_imgui* font_current = nullptr;

	};

}
