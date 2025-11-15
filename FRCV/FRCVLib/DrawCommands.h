#pragma once
#include <memory>

#include <opencv2/opencv.hpp>

#include "Frame.h"

class Frame;

namespace DrawCommands
{
	class DrawCommandBase
	{
	public:
		virtual ~DrawCommandBase() = default;
		virtual void Execute(std::shared_ptr<Frame> frame) = 0;
	};

	class DrawCircleCommand : public DrawCommandBase
	{
		cv::Point center;
		int radius;
		cv::Scalar colour;
		int thickness;
	public:
		DrawCircleCommand(cv::Point center, int radius, cv::Scalar colour, int thickness)
			: center(center), radius(radius), colour(colour), thickness(thickness) {
		}
		void Execute(std::shared_ptr<Frame> frame) override;
	};

	class DrawLineCommand : public DrawCommandBase
	{
		cv::Point p1;
		cv::Point p2;
		cv::Scalar colour;
		int thickness;
	public:
		DrawLineCommand(cv::Point a, cv::Point b, cv::Scalar colour, int thickness)
			: p1(a), p2(b), colour(colour), thickness(thickness) {}
		void Execute(std::shared_ptr<Frame> frame) override;
	};

	class DrawRectCommand : public DrawCommandBase
	{
		cv::Rect rect;
		cv::Scalar colour;
		int thickness;
	public:
		DrawRectCommand(cv::Rect rect, cv::Scalar colour, int thickness)
			: rect(rect), colour(colour), thickness(thickness) {}
		void Execute(std::shared_ptr<Frame> frame) override;
	};

	class PutTextCommand : public DrawCommandBase
	{
		std::string text;
		cv::Point org;
		int fontFace;
		double fontScale;
		cv::Scalar colour;
		int thickness;
	public:
		PutTextCommand(const std::string& text, cv::Point org, double fontScale, cv::Scalar colour, int thickness, int fontFace = cv::FONT_HERSHEY_SIMPLEX)
			: text(text), org(org), fontFace(fontFace), fontScale(fontScale), colour(colour), thickness(thickness) {}
		void Execute(std::shared_ptr<Frame> frame) override;
	};

	// TODO: Add all other commands like DrawRectangleCommand, DrawLineCommand, PutTextCommand, etc.
}