#include <cstdint>
#include <string>

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