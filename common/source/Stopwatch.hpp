#ifndef Stopwatch_hpp
#define Stopwatch_hpp

#if defined(_WIN32)
	#include <windows.h>
#else
	#include <sys/time.h>
#endif

typedef double Duration;

class Stopwatch {
public:
	Stopwatch();

	void start();
	void stop();
	Duration read() const;
	void set(const Duration& elapsedTime = 0);

private:
	double startTime;
	double stopTime;
	bool running;

	static double getCurrentSystemTime();
	#if defined(_WIN32)
		static LARGE_INTEGER frequency;
	#endif
};

#endif
