#include "pch.h"

#include <thread>

#include <Framework/AsyncTaskProcessor.h>

class AsyncTaskMock : public netcoro::ITask
{
public:
	bool CheckResults(int expected_calls, int wait_ms = 1000)
	{
		for (int i = 0; i < wait_ms / 10; ++i) {
			if (expected_calls == call_counter_.load(std::memory_order_relaxed)) { return true; }
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return false;
	}
private:
	void Process() final {
		call_counter_.fetch_add(1, std::memory_order_relaxed);
	}
	std::atomic<int> call_counter_ = 0;
};

TEST(AsyncTaskProcessor, AsyncTaskProcessorTest) {
	const int kThreadPoolSize = 1;
	netcoro::IoContext io_context(kThreadPoolSize);
	netcoro::AsyncTaskProcessor async_task_processor(io_context);
	auto task = std::make_shared<AsyncTaskMock>();
	const int kCallsNumber = 5;
	for (int i = 0; i < kCallsNumber; ++i) {
		async_task_processor.Post(task);
	}
	ASSERT_TRUE(task->CheckResults(kCallsNumber));
}