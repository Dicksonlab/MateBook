#ifndef debug_hpp
#define debug_hpp

#include <cassert>

void breakIf(bool condition = true);
void initMemoryLeakDetection();
long breakAlloc(long allocNumber);
void crashReport(bool enable);

// use assert_cast for downcasts when you know at compile-time that the cast is valid, but want to check in Debug mode
template<class TO, class FROM>
class AssertCaster;

template<class TO, class FROM>
class AssertCaster<TO*, FROM*> {
public:
	TO* operator()(FROM* from)
	{
		assert(dynamic_cast<TO*>(from));
		return static_cast<TO*>(from);
	}
};

template<class TO, class FROM>
TO assert_cast(FROM from)
{
	return AssertCaster<TO, FROM>()(from);
}

#endif
