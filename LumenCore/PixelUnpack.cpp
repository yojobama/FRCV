#include "PixelUnpack.h"

namespace PixelUnpack
{
	namespace {
		inline uint16_t ReadLe16(const uint8_t* p)
		{
			return static_cast<uint16_t>(p[0] | (p[1] << 8));
		}
	}

	size_t MinStrideY10(int width)
	{
		return static_cast<size_t>(width) * 2;
	}

	size_t MinStrideY10Packed(int width)
	{
		// a trailing partial group of 1-3 pixels still needs its bytes, rounded up to whole bytes
		return (static_cast<size_t>(width) * 10 + 7) / 8;
	}

	void Y10ToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst)
	{
		dst.create(height, width, CV_8UC1);
		for (int y = 0; y < height; y++) {
			const uint8_t* src = data + y * stride;
			uint8_t* out = dst.ptr<uint8_t>(y);
			for (int x = 0; x < width; x++) {
				// mask first: the upper 6 bits are specified as zero, but a driver leaving junk
				// there must not wrap the shifted value
				out[x] = static_cast<uint8_t>((ReadLe16(src + 2 * x) & 0x3FF) >> 2);
			}
		}
	}

	void Y16ToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst)
	{
		dst.create(height, width, CV_8UC1);
		for (int y = 0; y < height; y++) {
			const uint8_t* src = data + y * stride;
			uint8_t* out = dst.ptr<uint8_t>(y);
			// the high byte of each little-endian word is exactly the 8 MSBs
			for (int x = 0; x < width; x++) out[x] = src[2 * x + 1];
		}
	}

	void Y10PToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst)
	{
		dst.create(height, width, CV_8UC1);
		for (int y = 0; y < height; y++) {
			const uint8_t* src = data + y * stride;
			uint8_t* out = dst.ptr<uint8_t>(y);
			// the 8 MSBs are already whole bytes: copy 4 of every 5, drop the LSB byte
			for (int x = 0; x < width; x++) out[x] = src[(x / 4) * 5 + (x % 4)];
		}
	}

	void Y10BPackToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst)
	{
		dst.create(height, width, CV_8UC1);
		for (int y = 0; y < height; y++) {
			const uint8_t* src = data + y * stride;
			uint8_t* out = dst.ptr<uint8_t>(y);
			for (int x = 0; x < width; x++) {
				// pixel x occupies bits [10x, 10x+10) of the row, MSB first; its top 8 bits start
				// at bit 10x and may straddle two bytes
				size_t bit = static_cast<size_t>(x) * 10;
				size_t byte = bit / 8;
				unsigned shift = static_cast<unsigned>(bit % 8);
				unsigned hi = src[byte];
				unsigned lo = shift == 0 ? 0 : src[byte + 1];
				out[x] = static_cast<uint8_t>(((hi << 8) | lo) >> (8 - shift));
			}
		}
	}

	void Gray8Copy(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst)
	{
		cv::Mat view(height, width, CV_8UC1, const_cast<uint8_t*>(data), stride);
		view.copyTo(dst);
	}
}
