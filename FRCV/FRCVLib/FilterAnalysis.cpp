#include "FilterAnalysis.h"
#include "DrawCommands.h"

FilterAnalysis::FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis)
{
	m_OriginalFrame = orgFrame;
	m_AnalysisTimeMillis = analysisTimeMillis;
	m_HasDrawingCommands = false;
}

FilterAnalysis::FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis, std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> drawCommands)
{
	FilterAnalysis(orgFrame, analysisTimeMillis);
	m_HasDrawingCommands = true;
	m_DrawCommands = drawCommands;
}

// TODO: Implement the bloody destructor
FilterAnalysis::~FilterAnalysis()
{
}

void FilterAnalysis::SetDrawCommands(std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> drawCommands)
{
	m_DrawCommands = drawCommands;
}

std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> FilterAnalysis::GetDrawCommands() const
{
	return m_DrawCommands;
}

std::shared_ptr<Frame> FilterAnalysis::GetOriginalFrame() const
{
	return m_OriginalFrame;
}
unsigned long FilterAnalysis::GetAnalysisTimeMillis() const
{
	return m_AnalysisTimeMillis;
}
