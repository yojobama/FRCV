#pragma once

#include <vector>
#include <memory>
#include <string>

class DrawCommand;
class Frame;

class FilterAnalysis
{
public:
	FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis);
	FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis, std::vector<DrawCommand> drawCommands);
	~FilterAnalysis();
	void SetDrawCommands(std::vector<DrawCommand> drawCommands);
	std::vector<DrawCommand> GetDrawCommands() const;
	std::shared_ptr<Frame> GetOriginalFrame() const;
	unsigned long GetAnalysisTimeMillis() const;
private:
	std::shared_ptr<Frame> m_OriginalFrame;
	bool m_HasDrawingCommands;
	std::vector<DrawCommand> m_DrawCommands;
	unsigned long m_AnalysisTimeMillis;
};

