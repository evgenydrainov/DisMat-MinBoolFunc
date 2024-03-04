#include "graphic_imgui.h"

namespace tex {

	ImFont* Font_imgui::font_default;

	Font* Font::create(const std::string& file, float s)
	{
		return new Font_imgui(file, s);
	}

	sptr<Font> Font::_create(const std::string& name, int style, float size)
	{
		return sptrOf<Font_imgui>(name, style, size);
	}

	sptr<TextLayout> TextLayout::create(const std::wstring& src, const sptr<Font>& font)
	{
		sptr<Font_imgui> f = std::static_pointer_cast<Font_imgui>(font);
		return sptrOf<TextLayout_imgui>(src, f);
	}



	bool hasEnding (std::string const &fullString, std::string const &ending) {
		if (fullString.length() >= ending.length()) {
			return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
		} else {
			return false;
		}
	}

	Font_imgui::Font_imgui(const std::string& name, int style, float size)
	{
		ImGuiIO& io = ImGui::GetIO();

		this->font = font_default;
		this->size = size;
		this->style = style;
		this->name = name;
	}

	Font_imgui::Font_imgui(const std::string& file, float size)
	{
		ImGuiIO& io = ImGui::GetIO();

		// float custom_scale2 = 1;
		// if (hasEnding(file, "/fonts/latin/cmr10.ttf")) {
		// 	custom_scale = 26;
		// } else if (hasEnding(file, "/fonts/base/cmex10.ttf")) {
		// 	custom_scale = 26;
		// 	custom_scale2 = 3;
		// } else if (hasEnding(file, "/fonts/maths/cmsy10.ttf")) {
		// 	custom_scale = 26;
		// }

		if (hasEnding(file, "/fonts/base/cmex10.ttf")) {
			font = io.Fonts->AddFontFromFileTTF(file.c_str(), size * 26.0f * 3.0f);
			custom_scale = 26.0f;
		} else {
			font = io.Fonts->AddFontFromFileTTF(file.c_str(), size * 26.0f);
			custom_scale = 26.0f;
		}

		this->file = file;
		this->size = size;
	}

	sptr<Font> Font_imgui::deriveFont(int style) const
	{
		return sptr<Font>();
	}

	bool Font_imgui::operator==(const Font& _f) const {
		const Font_imgui& f = static_cast<const Font_imgui&>(_f);
		return font == f.font && size == f.size && style == f.style;
	}

	bool Font_imgui::operator!=(const Font& f) const {
		return !(*this == f);
	}



	TextLayout_imgui::TextLayout_imgui(const std::wstring& src, const sptr<Font_imgui>& font)
	{

	}

	void TextLayout_imgui::getBounds(Rect& bounds)
	{

	}

	void TextLayout_imgui::draw(Graphics2D& g2, float x, float y)
	{

	}



	Graphics2D_imgui::Graphics2D_imgui()
	{
	}

	void Graphics2D_imgui::setColor(color c)
	{
	}

	color Graphics2D_imgui::getColor() const
	{
		return color();
	}

	void Graphics2D_imgui::setStroke(const Stroke& s)
	{
	}

	const Stroke& Graphics2D_imgui::getStroke() const
	{
		return {};
	}

	void Graphics2D_imgui::setStrokeWidth(float w)
	{
	}

	const Font* Graphics2D_imgui::getFont() const
	{
		return font_current;
	}

	void Graphics2D_imgui::setFont(const Font* font)
	{
		font_current = (const Font_imgui*) font;
	}

	void Graphics2D_imgui::translate(float dx, float dy)
	{
		translation_x += dx * scale_x;
		translation_y += dy * scale_y;
	}

	void Graphics2D_imgui::scale(float sx, float sy)
	{
		scale_x *= sx;
		scale_y *= sy;
	}

	void Graphics2D_imgui::rotate(float angle)
	{
	}

	void Graphics2D_imgui::rotate(float angle, float px, float py)
	{
	}

	void Graphics2D_imgui::reset()
	{
		translation_x = 0;
		translation_y = 0;
		scale_x = 1;
		scale_y = 1;
		font_current = nullptr;

	}

	float Graphics2D_imgui::sx() const
	{
		return scale_x;
	}

	float Graphics2D_imgui::sy() const
	{
		return scale_y;
	}

	void Graphics2D_imgui::drawChar(wchar_t c, float x, float y)
	{
		std::wstring wstring;
		wstring.push_back(c);

		drawText(wstring, x, y);
	}

	void Graphics2D_imgui::drawText(const std::wstring& c, float x, float y)
	{
		std::string string = tex::wide2utf8(c);

		ImFont* font = font_current->font;
		float font_size = font->FontSize * scale_y / font_current->custom_scale;
		ImVec2 pos = {(x + translation_x), (y + translation_y)};
		// pos.y += 300.0f / scale_y;
		pos.y += 4.0f - scale_y * 0.8f;
		ImU32 color = IM_COL32(0, 0, 0, 255);
		list->AddText(font, font_size, pos, color, string.c_str());
	}

	void Graphics2D_imgui::drawLine(float x1, float y1, float x2, float y2)
	{
		ImVec2 p1 = {(x1 + translation_x) * scale_x, (y1 + translation_y) * scale_y};
		ImVec2 p2 = {(x2 + translation_x) * scale_x, (y2 + translation_y) * scale_y};
		list->AddLine(p1, p2, IM_COL32(0, 0, 0, 255));
	}

	void Graphics2D_imgui::drawRect(float x, float y, float w, float h)
	{
	}

	void Graphics2D_imgui::fillRect(float x, float y, float w, float h)
	{
	}

	void Graphics2D_imgui::drawRoundRect(float x, float y, float w, float h, float rx, float ry)
	{
	}

	void Graphics2D_imgui::fillRoundRect(float x, float y, float w, float h, float rx, float ry)
	{
	}

}
