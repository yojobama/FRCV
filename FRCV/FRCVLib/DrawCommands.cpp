#include "DrawCommands.h"
#include <opencv2/imgproc.hpp>

namespace DrawCommands {

void DrawCircleCommand::Execute(std::shared_ptr<Frame> frame)
{
	if (!frame) return;
	if (frame->empty()) return;
	cv::circle(*frame, center, radius, colour, thickness);
}

void DrawLineCommand::Execute(std::shared_ptr<Frame> frame)
{
	if (!frame) return;
	if (frame->empty()) return;
	cv::line(*frame, p1, p2, colour, thickness);
}

void DrawRectCommand::Execute(std::shared_ptr<Frame> frame)
{
	if (!frame) return;
	if (frame->empty()) return;
	cv::rectangle(*frame, rect, colour, thickness);
}

void PutTextCommand::Execute(std::shared_ptr<Frame> frame)
{
	if (!frame) return;
	if (frame->empty()) return;
	cv::putText(*frame, text, org, fontFace, fontScale, colour, thickness);
}

} // namespace DrawCommands
