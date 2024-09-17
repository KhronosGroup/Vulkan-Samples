#include "TimeMeasurements.h"

// That is from https://github.com/nlohmann/json
// We might need to include a license for that, but as it is only for hacking and internal things I think it is fine for now.
// If we can't use it we have to find an alternative json lib or write it ourselves
#include "json.hpp"

// Converts the full TimeMeasurements to a json object and writes it to a file called "data.json"
void TimeMeasurements::writeToJsonFile()
{
	nlohmann::json rootJsonObj;
	nlohmann::json measurementsJsonArray = nlohmann::json::array();

	for (auto iter = mTimes.begin(); iter != mTimes.end(); iter++)
	{
		iter->second->calculateSummarizations();

		nlohmann::json timingsOfTypeJsonObj;
		timingsOfTypeJsonObj["Name"]            = MeasurementPointsUtils::MeasurementPointsToString(iter->first);
		timingsOfTypeJsonObj["DataPointsCount"] = iter->second->mDataPoints.size();

		nlohmann::json dataPointsJsonArray = nlohmann::json::array();
		std::copy(iter->second->mDataPoints.begin(), iter->second->mDataPoints.end(), std::back_inserter(dataPointsJsonArray));
		timingsOfTypeJsonObj["DataPoints"] = dataPointsJsonArray;

		nlohmann::json summaryJsonObj;
		summaryJsonObj["min"]           = iter->second->mSummary.mMin;
		summaryJsonObj["avg"]           = iter->second->mSummary.mAverage;
		summaryJsonObj["mean"]          = iter->second->mSummary.mMean;
		summaryJsonObj["variance"]      = iter->second->mSummary.mVariance;
		summaryJsonObj["p90"]           = iter->second->mSummary.mP90;
		summaryJsonObj["p95"]           = iter->second->mSummary.mP95;
		summaryJsonObj["p99"]           = iter->second->mSummary.mP99;
		summaryJsonObj["max"]           = iter->second->mSummary.mMax;
		timingsOfTypeJsonObj["Summary"] = summaryJsonObj;

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