#pragma once

#include <memory>

#include "IoContext.h"

namespace netcoro {

class ITask : public ObjCounter<ITask>
{
public:
	virtual ~ITask() = default;
	virtual void Process() = 0;
};

using ITaskPtr = std::shared_ptr<ITask>;

class IAsyncTaskProcessor : public ObjCounter<IAsyncTaskProcessor>
{
public:
	virtual ~IAsyncTaskProcessor() = default;
	virtual void Post(ITaskPtr task) = 0;
};

using IAsyncTaskProcessorPtr = std::shared_ptr<IAsyncTaskProcessor>;

class AsyncTaskProcessor : public IAsyncTaskProcessor
{
public:
	AsyncTaskProcessor(const IoContext& io_context) : io_context_(io_context) {}
	void Post(ITaskPtr task) final;
private:
	const IoContext& io_context_;
};

}