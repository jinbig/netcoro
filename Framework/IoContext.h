#pragma once

namespace boost::asio { class io_context; }

namespace netcoro {
	
class IoContextImpl;

class IoContext
{
public:
	IoContext(short thread_pool_size);
	void Stop();
	const std::shared_ptr<boost::asio::io_context>& GetIoContext() const;
private:
	std::shared_ptr<IoContextImpl> impl_;
};

}