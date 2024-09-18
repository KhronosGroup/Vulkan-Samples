#pragma once

#include "utils.h"

#include "json.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

struct SummarizedTimings
{
	long long mMin;
	long long mAverage;
	long long mMean;
	long long mVariance;
	long long mP90;
	long long mP95;
	long long mP99;
	long long mMax;
};

struct TimingsOfType
{
	SummarizedTimings mSummary;

	TimingsOfType() :
	    mSummary(SummarizedTimings())
	{}

	void addTiming(long long value)
	{
		// Just ignore all values that go over the measurement limit
		if (mNextIdxToFill >= mDataPoints.size())
			return;

		mDataPointsLock.lock();
		mDataPoints[mNextIdxToFill] = value;
		mNextIdxToFill++;
		mDataPointsLock.unlock();
	}

	void calculateSummarizations()
	{
		// Copy the original vector, as it is faster than sorting and summarizing, to not block the data collection to much and not alter the original data.
		mDataPointsLock.lock();
		std::vector<long long> copy;
		std::copy(mDataPoints.begin(), mDataPoints.end(), std::back_inserter(copy));
		mDataPointsLock.unlock();

		std::sort(copy.begin(), copy.end());

		if (copy.size() >= 1)
		{
			mSummary.mMin     = copy[0];
			mSummary.mAverage = std::accumulate(copy.begin(), copy.end(), 0ll) / copy.size();
			mSummary.mMean    = copy[copy.size() / 2];

			long long varianceTemp = 0;
			for (auto iter = copy.begin(); iter != copy.end(); iter++)
			{
				varianceTemp += ((*iter - mSummary.mMean) * (*iter - mSummary.mMean));
			}
			mSummary.mVariance = varianceTemp * (1 / copy.size());

			mSummary.mP90 = copy[copy.size() * 0.9];
			mSummary.mP95 = copy[copy.size() * 0.95];
			mSummary.mP99 = copy[copy.size() * 0.99];
			mSummary.mMax = copy[copy.size() - 1];
		}
	}

	nlohmann::json toJson();

  private:
	std::mutex                                                  mDataPointsLock;
	std::array<long long, HackConstants::MaxNumberOfDataPoints> mDataPoints;
	size_t                                                      mNextIdxToFill = 0;
};

class TimeMeasurements
{
  public:
	void addTime(MeasurementPoints label, long long value)
	{
		if (!mEnabled)
			return;

		if (mTimes.count(label) == 0)
		{
			mTimesLock.lock();
			if (mTimes.count(label) == 0)
			{
				mTimes.try_emplace(label, std::unique_ptr<TimingsOfType>(new TimingsOfType()));
			}
			mTimesLock.unlock();
		}

		mTimes[label]->addTiming(value);
	}
	void writeToJsonFile();
	void disable()
	{
		mEnabled = false;
	}
	bool isEnabled() const
	{
		return mEnabled;
	}

  private:
	bool                                                        mEnabled = true;
	std::mutex                                                  mTimesLock;
	std::map<MeasurementPoints, std::unique_ptr<TimingsOfType>> mTimes;
};

class ScopedTiming
{
  public:
	ScopedTiming(TimeMeasurements &sw, MeasurementPoints label) :
	    mSw(sw), mLabel(label), mStartTime(std::chrono::high_resolution_clock::now()){};

	~ScopedTiming()
	{
		long long duration = (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - mStartTime)).count();
		mSw.addTime(mLabel, duration);
	}

  private:
	TimeMeasurements                     &mSw;
	MeasurementPoints                     mLabel;
	std::chrono::steady_clock::time_point mStartTime;
};
