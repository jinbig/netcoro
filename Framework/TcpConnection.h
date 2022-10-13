#pragma once

#include <optional>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

#include "Connection.h"

namespace netcoro {

class TcpConnection : public IConnection
{
public:
	static void Create(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, IConnectionHandlerPtr handler, size_t operation_timeout_ms);

	Result Connect(const std::string& address, unsigned short port) final;
	Result Read(Buffer& buffer) final;
	Result ReadSome(Buffer& buffer) final;
	Result Write(const Buffer& buffer) final;
	Result Close() final;

private:
	TcpConnection(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, size_t operation_timeout_ms);

	void Initialize(boost::asio::yield_context&& yield);
	void CheckOperationTimeout(std::weak_ptr<TcpConnection> self, std::shared_ptr<boost::asio::steady_timer> timer);

	struct OperationTimeoutInScope
	{
		OperationTimeoutInScope(boost::asio::steady_timer& timer, size_t operation_timeout_ms);
		~OperationTimeoutInScope();
		boost::asio::steady_timer& timer_;
	};

	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	boost::asio::ip::tcp::socket socket_;
	std::optional<boost::asio::yield_context> yield_context_;
	std::shared_ptr<boost::asio::steady_timer> timer_;

	const size_t operation_timeout_ms_;
};

}