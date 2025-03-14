#include "GameTimer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <profileapi.h>

GameTimer::GameTimer() :
	secondsPerCount_(0.0),
	deltaTime_(-1.0),
	baseTime_(0),
	pausedTime_(0),
	stopTime_(0),
	previousTime_(0),
	currentTime_(0),
	stopped_(false)
{
	__int64 countsPerSecond;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSecond));
}

float GameTimer::getTotalTime() const
{
	if (stopped_)
	{
		return (float)(((stopTime_ - pausedTime_) - baseTime_) * secondsPerCount_);
	}
	else
	{
		return (float)(((currentTime_ - pausedTime_) - baseTime_) * secondsPerCount_);
	}
}

float GameTimer::getDeltaTime() const
{
	return (float)deltaTime_;
}

void GameTimer::reset()
{
	__int64 currentTime = {};
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));

	baseTime_		= currentTime;
	previousTime_	= currentTime;
	stopTime_		= 0;
	stopped_		= false;
}

void GameTimer::start()
{
	__int64 startTime = {};
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	// If we are resuming the timer from a stopped state
	if (stopped_)
	{
		// Accumulate the paused time
		pausedTime_		+= (startTime - stopTime_);
		previousTime_	= startTime;
		stopTime_		= 0;
		stopped_		= false;
	}
}

void GameTimer::stop()
{
	if (!stopped_)
	{
		__int64 currentTime = {};
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));

		stopTime_	= currentTime;
		stopped_	= true;
	}
}

void GameTimer::tick()
{
	if (stopped_)
	{
		deltaTime_ = 0.0;
		return;
	}

	// Get the time this frame.
	__int64 currentTime = {};
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));
	currentTime_ = currentTime;

	// Time difference between this frame and the previous frame.
	deltaTime_ = (currentTime_ - previousTime_) * secondsPerCount_;

	// Prepare for the next frame.
	previousTime_ = currentTime_;

	/// Force nonnegative deltaTime_.
	/// If the processor goes into power save mode, the time difference will be 
	/// negative.
	if (deltaTime_ < 0.0)
	{
		deltaTime_ = 0.0;
	}

}
