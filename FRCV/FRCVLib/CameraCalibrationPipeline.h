#include "CameraCalibrationResult.h"
#include <vector>
#include <memory>

class Frame;
class CameraCalibrationResult;
class PreProcessor;
class FramePool;

using namespace std;

class CameraCalibrationPipeline
{
public:
	CameraCalibrationPipeline(PreProcessor* preProcessor, FramePool* framePool);
	~CameraCalibrationPipeline();
	CameraCalibrationResult getResults(vector<shared_ptr<Frame>> frames, int CHECKERBOARD[2]);
private:
	PreProcessor* preProcessor;
	FramePool* framePool;
};
