#include "debug.hpp"

/*
Provides a hardcoded breakpoint.
If the function is inlined, changing *(char*)eip,x from 0xcc to 0x90 in the watch window will disable it.

Since Windows 7: Make sure "Action Center / Change Action Center settings / Problem reporting Settings" is set to "Each time a problem occurs, ask me before checking for solutions"
*/

#if defined(WIN32)
	#include <intrin.h>

	void breakIf(bool condition)
	{
		if (condition) {
			__debugbreak();
		}
	}

	// for now we're using the same code in both Debug and Release builds
	// if we ever want to change that, we can branch in the preprocessor like this:
	#if defined(_DEBUG)
	#else
	#endif
#else
	#include <cassert>
	
	void breakIf(bool condition)
	{
		assert(!condition);
	}
#endif

/*
Prints memory leaks to the Output window when the program exits.
The allocation number is in curly brackets.
See breakAlloc() below for how to find the allocations corresponding to a certain number.
*/

#if defined(_MSC_VER) && defined(WIN32) && defined(_DEBUG)
	#include <crtdbg.h>

	void initMemoryLeakDetection()
	{
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flag |= _CRTDBG_ALLOC_MEM_DF;	// Enable debug heap allocations and use of memory block type identifiers, such as _CLIENT_BLOCK.
		flag &= ~_CRTDBG_CHECK_ALWAYS_DF;	// Don't call _CrtCheckMemory at every allocation and deallocation request.
		flag &= ~_CRTDBG_CHECK_CRT_DF;	// Ignore memory used internally by the run-time library.
		flag &= ~_CRTDBG_DELAY_FREE_MEM_DF; // Don't keep freed memory blocks in the heap's linked list, assign them the _FREE_BLOCK type, and fill them with the byte value 0xDD.
		flag |= _CRTDBG_LEAK_CHECK_DF;	// Perform automatic leak checking at program exit.
		_CrtSetDbgFlag(flag);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	}
#else
	#include <iostream>
	
	void initMemoryLeakDetection()
	{
		std::cerr << "Memory leak detection is not supported in this build configuration." << std::endl;
	}
#endif

/*
Call breakAlloc(42); to break on allocation 42.
You can also assign a value to the global variable _crtBreakAlloc (or {,,msvcr90d.dll}_crtBreakAlloc) in the debugger
*/

#if defined(_MSC_VER) && defined(WIN32) && defined(_DEBUG)
	#include <crtdbg.h>

	long breakAlloc(long allocNumber)
	{
		return _CrtSetBreakAlloc(allocNumber);
	}
#else
	#include <iostream>
	
	long breakAlloc(long allocNumber)
	{
		std::cerr << "Memory leak detection is not supported in this build configuration." << std::endl;
		return 0;
	}
#endif

/*
Enable or disable the modal application crash popup on windows.
The popup causes a process to stay in memory until the user has dealt with it.
*/

#if defined(_WIN32)
	#include <Windows.h>

	void crashReport(bool enable)
	{
		if (enable) {
			SetErrorMode(0);
		} else {
			SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
		}
	}
#else
	#include <iostream>
	
	void crashReport(bool enable)
	{
		std::cerr << "Crash reports are not supported in this build configuration." << std::endl;
	}
#endif
