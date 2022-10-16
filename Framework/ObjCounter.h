#pragma once

#include <mutex>
#include <unordered_set>

namespace netcoro {

// It actually works in DEBUG compilation mode only.
template<typename T>
class ObjCounter
{
public:
#ifndef NDEBUG
	ObjCounter() { GetCounter().fetch_add(1, std::memory_order_relaxed); AddOrRemove(true); }
	~ObjCounter() { GetCounter().fetch_sub(1, std::memory_order_relaxed); AddOrRemove(false); }
	static int GetValue() { return GetCounter().load(std::memory_order_relaxed); }
	static bool IsEmpty()
	{
		if (netcoro::ObjCounter<T>::GetValue()) {
			auto& objs = netcoro::ObjCounter<T>::GetObjects();
			T* obj = nullptr;
			if (!objs.empty()) {
				obj = static_cast<T*>(*objs.begin());
			}
			return false;
		}
		return true;
	}
private:
	static std::atomic<int>& GetCounter()
	{
		static std::atomic<int> counter = 0;
		return counter;
	}
	using Ptrs = std::unordered_set<ObjCounter<T>*>;
	static Ptrs& GetObjects()
	{
		static Ptrs objects;
		return objects;
	}
	void AddOrRemove(bool add)
	{
		add;
#ifdef INVESTIGATION
		static std::mutex lock;
		std::lock_guard guard(lock);
		if (add) {
			GetObjects().insert(this);
		} else {
			GetObjects().erase(this);
		}
#endif
	}
#else
	static int GetValue() { return 0; }
#endif
};

}