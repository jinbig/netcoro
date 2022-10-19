#include "pch.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

#include "AsyncTaskProcessor.h"

namespace netcoro {

void AsyncTaskProcessor::Post(ITaskPtr task)
{
	boost::asio::post(*io_context_.GetIoContext(), [task = std::move(task)]() mutable {
		task->Process(std::move(task));
	});
}

void AsyncTaskProcessor::PostDeferred(ITaskPtr task, size_t deferred_for_ms)
{
	boost::asio::steady_timer timer(*io_context_.GetIoContext(), std::chrono::milliseconds(deferred_for_ms));
	timer.async_wait([timer = std::move(timer), task = std::move(task)](const boost::system::error_code&) mutable {
		task->Process(std::move(task));
	});
}

}