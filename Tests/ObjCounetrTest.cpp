#include "pch.h"

#include <Framework/ObjCounter.h>

class TestObjBase : public netcoro::ObjCounter<TestObjBase>
{
};

class TestObjDerived : public netcoro::ObjCounter<TestObjDerived>, TestObjBase
{
};

TEST(ObjCounter, ObjCounterTest)
{
	// It actually works in DEBUG compilation mode only.
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjBase>::GetValue());
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjDerived>::GetValue());
	ASSERT_EQ(0, netcoro::ObjCounter<std::string>::GetValue());
	{
		TestObjBase obj; obj;
#ifndef NDEBUG
		ASSERT_EQ(1, netcoro::ObjCounter<TestObjBase>::GetValue());
#else
		ASSERT_EQ(0, netcoro::ObjCounter<TestObjBase>::GetValue());
#endif
		ASSERT_EQ(0, netcoro::ObjCounter<TestObjDerived>::GetValue());
	}
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjBase>::GetValue());
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjDerived>::GetValue());
	{
		std::vector<TestObjDerived> objs(5);
#ifndef NDEBUG
		ASSERT_EQ(objs.size(), netcoro::ObjCounter<TestObjBase>::GetValue());
		ASSERT_EQ(objs.size(), netcoro::ObjCounter<TestObjDerived>::GetValue());
#else
		ASSERT_EQ(0, netcoro::ObjCounter<TestObjBase>::GetValue());
		ASSERT_EQ(0, netcoro::ObjCounter<TestObjDerived>::GetValue());
#endif
	}
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjBase>::GetValue());
	ASSERT_EQ(0, netcoro::ObjCounter<TestObjDerived>::GetValue());
}