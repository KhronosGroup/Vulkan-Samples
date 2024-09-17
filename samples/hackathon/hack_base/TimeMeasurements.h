#pragma once

#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

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
	std::mutex             mDataPointsLock;
	std::vector<long long> mDataPoints;
	SummarizedTimings      mSummarize;

	TimingsOfType() :
	    mSummarize(SummarizedTimings())
	{}

	void calculateSummarizations()
	{
		// Copy the original vector, as it is faster than sorting and summarizing, to not block the data collection to much and not alter the original data.
		mDataPointsLock.lock();
		std::vector<long long> copy;
		std::copy(mDataPoints.begin(), mDataPoints.end(), std::back_inserter(copy));
		mDataPointsLock.unlock();

		std::sort(copy.begin(), copy.end());

		if (copy.size() > 1)
		{
			mSummarize.mMin     = copy[0];
			mSummarize.mAverage = std::accumulate(copy.begin(), copy.end(), 0ll) / copy.size();
			mSummarize.mMean    = copy[copy.size() / 2];
			mSummarize.mP90     = copy[copy.size() * 0.9];
			mSummarize.mP95     = copy[copy.size() * 0.95];
			mSummarize.mP99     = copy[copy.size() * 0.99];
			mSummarize.mMax     = copy[copy.size() - 1];
		}
	}
};

class TimeMeasurements
{
  public:
	void addTime(MeasurementPoints label, long long value)
	{
		if (mTimes.count(label) == 0)
		{
			mTimesLock.lock();
			if (mTimes.count(label) == 0)
			{
				mTimes.try_emplace(label, std::unique_ptr<TimingsOfType>(new TimingsOfType()));
			}
			mTimesLock.unlock();
		}

		mTimes[label]->mDataPointsLock.lock();
		mTimes[label]->mDataPoints.push_back(value);
		mTimes[label]->mDataPointsLock.unlock();
	}

  private:
	std::mutex                                         mTimesLock;
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
