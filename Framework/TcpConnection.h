#pragma once

#include <optional>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

#include "Connection.h"
#include "ObjCounter.h"

namespace netcoro {

class TcpConnection : public IConnection
{
public:
	static void Create(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, IConnectionHandlerPtr handler, size_t operation_timeout_ms);

	Result GetInfo(EndPointInfoType type, std::string& address, unsigned short& port) final;
	Result Connect(const std::string& address, unsigned short port) final;
	Result Read(Buffer& buffer) final;
	Result Read(unsigned char* buffer, size_t buffer_size) final;
	Result ReadSome(Buffer& buffer) final;
	Result Write(const Buffer& buffer) final;
	Result Write(const unsigned char* buffer, size_t buffer_size) final;
	Result Close() final;

private:
	using StrandPtr = std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>>;
	using TimerPtr = std::shared_ptr<boost::asio::steady_timer>;

	TcpConnection(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, size_t operation_timeout_ms);

	void Initialize(boost::asio::yield_context&& yield);
	static void CheckOperationTimeout(std::weak_ptr<TcpConnection> self, TimerPtr timer, StrandPtr strand);

	struct OperationTimeoutInScope
	{
		OperationTimeoutInScope(boost::asio::steady_timer& timer, size_t operation_timeout_ms);
		~OperationTimeoutInScope();
		boost::asio::steady_timer& timer_;
	};

	StrandPtr strand_;
	TimerPtr timer_;

	boost::asio::ip::tcp::socket socket_;
	std::optional<boost::asio::yield_context> yield_context_;

	const size_t operation_timeout_ms_;
};

}