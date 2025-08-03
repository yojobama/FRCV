#pragma once
class FrameSpec
{
public:
	FrameSpec() = default;
	FrameSpec(int height, int width, int type);
	~FrameSpec() = default;

	int GetHeight() const { return m_Height; }
	void SetHeight(int h) { m_Height = h; }

	int GetWidth() const { return m_Width; }
	void SetWidth(int w) { m_Width = w; }

	int GetType() const { return m_Type; }
	void SetType(int t) { m_Type = t; }

	bool operator== (const FrameSpec& other) const {
		return (m_Height == other.m_Height && m_Width == other.m_Width && m_Type == other.m_Type);
	}
private:
	int m_Height, m_Width, m_Type;
};
