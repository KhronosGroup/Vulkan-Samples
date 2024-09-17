#include "TimeMeasurements.h"

#include "json.hpp"

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

    #ifdef _WIN32
	const char* filePath = "./data.json";
    #else
	const char* filePath = "/data/data/com.khronos.vulkan_samples/files/data.json";
    #endif

	std::string outJson = rootJsonObj.dump(4); // Dump with pretty printing and a widht of 4 spaces.

    FILE *outFile = std::fopen(filePath, "w");
	std::fwrite(outJson.data(), sizeof(char), outJson.size(), outFile);
	std::fclose(outFile);
}