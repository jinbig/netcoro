#include "pch.h"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

#include "IoContext.h"

namespace netcoro {

class IoContextImpl
{
public:
	IoContextImpl(short thread_pool_size);
	~IoContextImpl();
	void Stop();

	std::shared_ptr<boost::asio::io_context> io_context_;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
	boost::thread_group threads_;
};

IoContextImpl::IoContextImpl(short thread_pool_size)
	: io_context_(std::make_shared<boost::asio::io_context>())
	, work_guard_(boost::asio::make_work_guard(*io_context_))
{
	for (short i = 0; i < thread_pool_size; ++i) {
		threads_.create_thread([io_context = io_context_]() {
			boost::system::error_code ec;
			io_context->run(ec);
			});
	}
}

IoContextImpl::~IoContextImpl()
{
	Stop();
}

void IoContextImpl::Stop()
{
	work_guard_.reset();
	io_context_->stop();
	threads_.join_all();
}

IoContext::IoContext(short thread_pool_size)
	: impl_(std::make_shared<IoContextImpl>(thread_pool_size))
{}

void IoContext::Stop()
{
	impl_->Stop();
}

const std::shared_ptr<boost::asio::io_context>& IoContext::GetIoContext() const
{
	return impl_->io_context_;
}

}