#include "TimeMeasurements.h"

nlohmann::json TimingsOfType::toJson()
{
	calculateSummarizations();

	nlohmann::json timingsOfTypeJsonObj;
	timingsOfTypeJsonObj["DataPointsCount"] = mNextIdxToFill;

	nlohmann::json dataPointsJsonArray = nlohmann::json::array();
    for (int i = 0; i < mNextIdxToFill; ++i)
    {
		dataPointsJsonArray.push_back(mDataPoints[i]);
    }
	timingsOfTypeJsonObj["DataPoints"] = dataPointsJsonArray;

	nlohmann::json summaryJsonObj;
	summaryJsonObj["min"]           = mSummary.mMin;
	summaryJsonObj["avg"]           = mSummary.mAverage;
	summaryJsonObj["mean"]          = mSummary.mMean;
	summaryJsonObj["variance"]      = mSummary.mVariance;
	summaryJsonObj["p90"]           = mSummary.mP90;
	summaryJsonObj["p95"]           = mSummary.mP95;
	summaryJsonObj["p99"]           = mSummary.mP99;
	summaryJsonObj["max"]           = mSummary.mMax;
	timingsOfTypeJsonObj["Summary"] = summaryJsonObj;

	return timingsOfTypeJsonObj;
}

// Converts the full TimeMeasurements to a json object and writes it to a file called "data.json"
void TimeMeasurements::writeToJsonFile()
{
	nlohmann::json rootJsonObj;
	nlohmann::json measurementsJsonArray = nlohmann::json::array();

	for (auto iter = mTimes.begin(); iter != mTimes.end(); iter++)
	{
		nlohmann::json timingsOfTypeJsonObj = iter->second->toJson();
		timingsOfTypeJsonObj["Name"] = MeasurementPointsUtils::MeasurementPointsToString(iter->first);
		measurementsJsonArray.push_back(timingsOfTypeJsonObj);
	}

	rootJsonObj["Measurements"] = measurementsJsonArray;

    // Paths differ between windows and android
    // I have no idea whether the android path will be the same between different android devices, but this one did work at one point on a Quest 3
    #ifdef _WIN32
	const char* filePath = "./data.json";
    #else
	const char* filePath = "/data/data/com.khronos.vulkan_samples/files/data.json";
    #endif

	std::string outJson = rootJsonObj.dump(4); // Dump with pretty printing and a width of 4 spaces.

    // That will spike our CPU hard, but as we should stop measuring afterwards it's fine.
    FILE *outFile = std::fopen(filePath, "w");
	std::fwrite(outJson.data(), sizeof(char), outJson.size(), outFile);
	std::fclose(outFile);
}
