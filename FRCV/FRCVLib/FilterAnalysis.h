#pragma once

#include <vector>
#include <memory>
#include <string>

class DrawCommand;
class Frame;

class FilterAnalysis
{
public:
	FilterAnalysis();
	~FilterAnalysis();
	
private:
	bool m_HasDrawingCommands;
	std::vector<DrawCommand> m_DrawCommands;
	int m_AnalysisTimeMillis;
};

