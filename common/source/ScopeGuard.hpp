#ifndef ScopeGuard_hpp
#define ScopeGuard_hpp

/*
ScopeGuard provides a quick way of implementing RAII:

void undo1();	// undoes what mayThrow1 does
void undo2(int param);	// undoes what mayThrow2 does
void undo3();	// undoes what mayThrow3 does

class SomeClass {
	void undo4(std::string);
	void undo5(std::string);

	void someMethod()
	{
		mayThrow1();
		ScopeGuard guard1 = makeGuard(undo1);

		mayThrow2();
		ScopeGuard guard2 = makeGuard(undo2, 42);

		mayThrow3();
		ON_BLOCK_EXIT(undo3);	// always calls undo3 when leaving the block, regardless of whether an exception is thrown or not

		mayThrow4();
		ScopeGuard guard4 = makeGuard(&SomeClass::undo4, this, "calls undo4 on this object, passing this string as the first parameter");

		mayThrow5();
		ON_BLOCK_EXIT(&SomeClass::undo5, this, "always calls undo5 on this object when leaving the block, regardless of whether an exception is thrown or not");

		//...
		// if all of them succeeded we dismiss the guards so they don't undo the operations:
		guard1.dismiss();
		guard2.dismiss();
		guard4.dismiss();
	}
};
*/

#include "byRef.hpp"

class ScopeGuardImplBase {
public:
	ScopeGuardImplBase() throw() :
		dismissed(false)
	{
	}

	void dismiss() const throw() 
	{
		dismissed = true;
	}

protected:
	~ScopeGuardImplBase()
	{
	}

	ScopeGuardImplBase(const ScopeGuardImplBase& other) throw() :
		dismissed(other.dismissed)
	{
		other.dismiss();
	}

	template<typename J>
	static void safeExecute(J& j) throw() 
	{
		if (!j.dismissed) {
			try {
				j.execute();
			} catch(...) {
			}
		}
	}
	
	mutable bool dismissed;

private:
	ScopeGuardImplBase& operator=(const ScopeGuardImplBase&);
};

typedef const ScopeGuardImplBase& ScopeGuard;

template<typename F>
class ScopeGuardImpl0 : public ScopeGuardImplBase {
public:
	static ScopeGuardImpl0<F> makeGuard(F fun)
	{
		return ScopeGuardImpl0<F>(fun);
	}

	~ScopeGuardImpl0() throw() 
	{
		safeExecute(*this);
	}

	void execute() 
	{
		fun();
	}

protected:
	ScopeGuardImpl0(F fun) :
		 fun(fun)
	{
	}

	F fun;
};

template<typename F>
ScopeGuardImpl0<F> makeGuard(F fun)
{
	return ScopeGuardImpl0<F>::makeGuard(fun);
}

template<typename F, typename P1>
class ScopeGuardImpl1 : public ScopeGuardImplBase {
public:
	static ScopeGuardImpl1<F, P1> makeGuard(F fun, P1 p1)
	{
		return ScopeGuardImpl1<F, P1>(fun, p1);
	}

	~ScopeGuardImpl1() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		fun(p1);
	}

protected:
	ScopeGuardImpl1(F fun, P1 p1) :
		fun(fun),
		p1(p1)
	{
	}

	F fun;
	const P1 p1;
};

template<typename F, typename P1>
ScopeGuardImpl1<F, P1> makeGuard(F fun, P1 p1)
{
	return ScopeGuardImpl1<F, P1>::makeGuard(fun, p1);
}

template<typename F, typename P1, typename P2>
class ScopeGuardImpl2 : public ScopeGuardImplBase {
public:
	static ScopeGuardImpl2<F, P1, P2> makeGuard(F fun, P1 p1, P2 p2)
	{
		return ScopeGuardImpl2<F, P1, P2>(fun, p1, p2);
	}

	~ScopeGuardImpl2() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		fun(p1, p2);
	}

protected:
	ScopeGuardImpl2(F fun, P1 p1, P2 p2) :
		fun(fun),
		p1(p1),
		p2(p2)
	{
	}

	F fun;
	const P1 p1;
	const P2 p2;
};

template<typename F, typename P1, typename P2>
ScopeGuardImpl2<F, P1, P2> makeGuard(F fun, P1 p1, P2 p2)
{
	return ScopeGuardImpl2<F, P1, P2>::makeGuard(fun, p1, p2);
}

template<typename F, typename P1, typename P2, typename P3>
class ScopeGuardImpl3 : public ScopeGuardImplBase {
public:
	static ScopeGuardImpl3<F, P1, P2, P3> makeGuard(F fun, P1 p1, P2 p2, P3 p3)
	{
		return ScopeGuardImpl3<F, P1, P2, P3>(fun, p1, p2, p3);
	}

	~ScopeGuardImpl3() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		fun(p1, p2, p3);
	}

protected:
	ScopeGuardImpl3(F fun, P1 p1, P2 p2, P3 p3) :
		fun(fun),
		p1(p1),
		p2(p2),
		p3(p3)
	{
	}

	F fun;
	const P1 p1;
	const P2 p2;
	const P3 p3;
};

template<typename F, typename P1, typename P2, typename P3>
ScopeGuardImpl3<F, P1, P2, P3> makeGuard(F fun, P1 p1, P2 p2, P3 p3)
{
	return ScopeGuardImpl3<F, P1, P2, P3>::makeGuard(fun, p1, p2, p3);
}

template<class Obj, typename MemFun>
class ObjScopeGuardImpl0 : public ScopeGuardImplBase {
public:
	static ObjScopeGuardImpl0<Obj, MemFun> makeObjGuard(Obj& obj, MemFun memFun)
	{
		return ObjScopeGuardImpl0<Obj, MemFun>(obj, memFun);
	}

	~ObjScopeGuardImpl0() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		(obj.*memFun)();
	}

protected:
	ObjScopeGuardImpl0(Obj& obj, MemFun memFun) :
		obj(obj),
		memFun(memFun)
	{
	}

	Obj& obj;
	MemFun memFun;
};

template<typename Ret, class Obj1, class Obj2>
ObjScopeGuardImpl0<Obj1, Ret(Obj2::*)()> makeGuard(Ret(Obj2::*memFun)(), Obj1 &obj)
{
	return ObjScopeGuardImpl0<Obj1, Ret(Obj2::*)()>::makeObjGuard(obj, memFun);
}

template<typename Ret, class Obj1, class Obj2>
ObjScopeGuardImpl0<Obj1, Ret(Obj2::*)()> makeGuard(Ret(Obj2::*memFun)(), Obj1 *obj)
{
	return ObjScopeGuardImpl0<Obj1, Ret(Obj2::*)()>::makeObjGuard(*obj, memFun);
}

template<class Obj, typename MemFun, typename P1>
class ObjScopeGuardImpl1 : public ScopeGuardImplBase {
public:
	static ObjScopeGuardImpl1<Obj, MemFun, P1> makeObjGuard(Obj& obj, MemFun memFun, P1 p1)
	{
		return ObjScopeGuardImpl1<Obj, MemFun, P1>(obj, memFun, p1);
	}

	~ObjScopeGuardImpl1() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		(obj.*memFun)(p1);
	}

protected:
	ObjScopeGuardImpl1(Obj& obj, MemFun memFun, P1 p1) :
		obj(obj),
		memFun(memFun),
		p1(p1)
	{
	}

	Obj& obj;
	MemFun memFun;
	const P1 p1;
};

template<typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
ObjScopeGuardImpl1<Obj1, Ret(Obj2::*)(P1a), P1b> makeGuard(Ret(Obj2::*memFun)(P1a), Obj1 &obj, P1b p1)
{
	return ObjScopeGuardImpl1<Obj1, Ret(Obj2::*)(P1a), P1b>::makeObjGuard(obj, memFun, p1);
}

template<typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
ObjScopeGuardImpl1<Obj1, Ret(Obj2::*)(P1a), P1b> makeGuard(Ret(Obj2::*memFun)(P1a), Obj1 *obj, P1b p1)
{
	return ObjScopeGuardImpl1<Obj1, Ret(Obj2::*)(P1a), P1b>::makeObjGuard(*obj, memFun, p1);
}

template<class Obj, typename MemFun, typename P1, typename P2>
class ObjScopeGuardImpl2 : public ScopeGuardImplBase {
public:
	static ObjScopeGuardImpl2<Obj, MemFun, P1, P2> makeObjGuard(Obj& obj, MemFun memFun, P1 p1, P2 p2)
	{
		return ObjScopeGuardImpl2<Obj, MemFun, P1, P2>(obj, memFun, p1, p2);
	}

	~ObjScopeGuardImpl2() throw()
	{
		safeExecute(*this);
	}

	void execute()
	{
		(obj.*memFun)(p1, p2);
	}

protected:
	ObjScopeGuardImpl2(Obj& obj, MemFun memFun, P1 p1, P2 p2) :
		obj(obj),
		memFun(memFun),
		p1(p1),
		p2(p2)
	{
	}

	Obj& obj;
	MemFun memFun;
	const P1 p1;
	const P2 p2;
};

template<typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b>
ObjScopeGuardImpl2<Obj1, Ret(Obj2::*)(P1a,P2a), P1b, P2b> makeGuard(Ret(Obj2::*memFun)(P1a,P2a), Obj1 &obj, P1b p1, P2b p2)
{
	return ObjScopeGuardImpl2<Obj1, Ret(Obj2::*)(P1a,P2a), P1b, P2b>::makeObjGuard(obj, memFun, p1, p2);
}

template<typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b>
ObjScopeGuardImpl2<Obj1, Ret(Obj2::*)(P1a,P2a), P1b, P2b> makeGuard(Ret(Obj2::*memFun)(P1a,P2a), Obj1 *obj, P1b p1, P2b p2)
{
	return ObjScopeGuardImpl2<Obj1, Ret(Obj2::*)(P1a,P2a), P1b, P2b>::makeObjGuard(*obj, memFun, p1, p2);
}

#include "macro.h"
#define ON_BLOCK_EXIT ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = makeGuard

#endif
