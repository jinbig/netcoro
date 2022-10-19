#pragma once

#include <memory>

#include "IoContext.h"

namespace netcoro {

class ITask;
using ITaskPtr = std::shared_ptr<ITask>;

class ITask : public ObjCounter<ITask>
{
public:
	virtual ~ITask() = default;
	virtual void Process(ITaskPtr self_task) = 0;
};

class IAsyncTaskProcessor : public ObjCounter<IAsyncTaskProcessor>
{
public:
	virtual ~IAsyncTaskProcessor() = default;
	virtual void Post(ITaskPtr task) = 0;
	virtual void PostDeferred(ITaskPtr task, size_t deffered_for_ms) = 0;
};

using IAsyncTaskProcessorPtr = std::shared_ptr<IAsyncTaskProcessor>;

class AsyncTaskProcessor : public IAsyncTaskProcessor
{
public:
	AsyncTaskProcessor(const IoContext& io_context) : io_context_(io_context) {}
	void Post(ITaskPtr task) final;
	void PostDeferred(ITaskPtr task, size_t deffered_for_ms) final;
private:
	const IoContext& io_context_;
};

}