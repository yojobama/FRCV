#pragma once
class FrameSpec
{
public:
	FrameSpec(int height, int width, int type);
	~FrameSpec() = default;

	int getHeight() const { return height; }
	void setHeight(int h) { height = h; }

	int getWidth() const { return width; }
	void setWidth(int w) { width = w; }

	int getType() const { return type; }
	void setType(int t) { type = t; }

	bool operator== (const FrameSpec& other) const {
		return (height == other.height && width == other.width && type == other.type);
	}
private:
	int height, width, type;
};
