#include <cstdint>
#include <string>

// This will do two things
// Measurements will stop after sMaxNumberOfDataPoints of frames measured.
// Initialize all the arrays in the background with sMaxNumberOfDataPoints empty slots. If more than these data points are collected all additional ones are ignored.
namespace HackConstants
{
#ifdef _WIN32
constexpr size_t MaxNumberOfDataPoints = 10000;
#else // Android produces much less frames and we should have a good idea of performance after just a fraction of the desktop frames
constexpr size_t MaxNumberOfDataPoints = 1000;
#endif
}

enum class MeasurementPoints : uint16_t
{
	FullDrawCall = 0,
	PrepareFrame,
	QueueFillingOperations,
	QueueVkQueueSubmitOperation,
	SubmitFrame,
	HackRenderFunction,
	HackPrepareFunction
};

struct MeasurementPointsUtils
{
	static std::string MeasurementPointsToString(MeasurementPoints measurementPoint)
	{
		switch (measurementPoint)
		{
			case MeasurementPoints::FullDrawCall:
				return "FullDrawCall";
			case MeasurementPoints::PrepareFrame:
				return "PrepareFrame";
			case MeasurementPoints::QueueFillingOperations:
				return "QueueFillingOperations";
			case MeasurementPoints::QueueVkQueueSubmitOperation:
				return "QueueVkQueueSubmitOperation";
			case MeasurementPoints::SubmitFrame:
				return "SubmitFrame";
			case MeasurementPoints::HackRenderFunction:
				return "HackRenderFunction";
			case MeasurementPoints::HackPrepareFunction:
				return "HackPrepareFunction";
			default:
				return "Unknown MeasurementPoint";
		}
	}

	static std::string MeasurementPointsToString(uint16_t measurementPoint)
	{
		return MeasurementPointsToString((MeasurementPoints) measurementPoint);
	}
};