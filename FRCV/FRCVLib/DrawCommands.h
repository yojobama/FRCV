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

	// TODO: Add all other commands like DrawRectangleCommand, DrawLineCommand, PutTextCommand, etc.
}