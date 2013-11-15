#include "Stopwatch.hpp"
#include <stdexcept>
#include <string>

using namespace std;

Stopwatch::Stopwatch() :
	startTime(0),
	stopTime(0),
	running(false)
{
	#if defined(_WIN32)
		if (!frequency.QuadPart) {
			QueryPerformanceFrequency(&frequency);
		}
		if (!frequency.QuadPart) {
			throw runtime_error("cannot initialize Stopwatch");
		}
	#endif
}

void Stopwatch::start()
{
	if (running) {
		throw runtime_error("Stopwatch already running");
	}
	startTime = getCurrentSystemTime() - (stopTime - startTime);
	running = true;
}

void Stopwatch::stop()
{
	if (!running) {
		throw runtime_error("Stopwatch not running");
	}
	stopTime = getCurrentSystemTime();
	running = false;
}

Duration Stopwatch::read() const
{
	return (running ? getCurrentSystemTime() : stopTime) - startTime;
}

void Stopwatch::set(const Duration& elapsedTime)
{
	stopTime = getCurrentSystemTime();
	startTime = stopTime - elapsedTime;
}

double Stopwatch::getCurrentSystemTime()
{
	#if defined(_WIN32)
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return static_cast<double>(now.QuadPart) / static_cast<double>(frequency.QuadPart);
	#else
		timeval now;
		gettimeofday(&now, NULL);
		return (double)now.tv_sec + (double)now.tv_usec / (1000*1000);
	#endif
}

#if defined(_WIN32)
	LARGE_INTEGER Stopwatch::frequency;
#endif
