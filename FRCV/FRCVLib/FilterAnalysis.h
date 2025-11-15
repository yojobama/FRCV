#pragma once

#include <vector>
#include <memory>
#include <string>

namespace DrawCommands 
{
	class DrawCommandBase;
};

class Frame;

class FilterAnalysis
{
public:
	FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis);
	FilterAnalysis(std::shared_ptr<Frame> orgFrame, unsigned long analysisTimeMillis, std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> drawCommands);
	~FilterAnalysis();
	void SetDrawCommands(std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> drawCommands);
	std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> GetDrawCommands() const;
	std::shared_ptr<Frame> GetOriginalFrame() const;
	unsigned long GetAnalysisTimeMillis() const;
	void SetJsonData(const std::string& json) { jsonData = json; }
	std::string GetJsonData() const { return jsonData; }
private:
	std::string jsonData;
	std::shared_ptr<Frame> m_OriginalFrame;
	bool m_HasDrawingCommands;
	std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> m_DrawCommands;
	unsigned long m_AnalysisTimeMillis;
};

