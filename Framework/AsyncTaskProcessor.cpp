#include "pch.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

#include "AsyncTaskProcessor.h"

namespace netcoro {

void AsyncTaskProcessor::Post(ITaskPtr task)
{
	boost::asio::post(*io_context_.GetIoContext(), [task = std::move(task)]() {
		task->Process();
	});
}

}