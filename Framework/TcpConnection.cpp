#include "pch.h"

#include "Logger.h"
#include "TcpConnection.h"

namespace netcoro {

TcpConnection::TcpConnection(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, size_t operation_timeout_ms)
	: strand_(io_context.get_executor())
	, socket_(std::move(socket))
	, timer_(std::make_shared<boost::asio::steady_timer>(io_context, std::chrono::steady_clock::time_point::max()))
	, operation_timeout_ms_(operation_timeout_ms)
{
}

IConnection::Result TcpConnection::Connect(const std::string& ip_address, unsigned short port)
{
	boost::system::error_code ec;
	auto address = boost::asio::ip::make_address(ip_address, ec);
	if(ec) {
		return ec;
	}
	boost::asio::ip::tcp::endpoint endpoint(address, port);
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	socket_.async_connect(endpoint, yield_context_.value()[ec]);
	return ec;
}

IConnection::Result TcpConnection::Read(Buffer& buffer)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	boost::asio::async_read(socket_, boost::asio::buffer(buffer), yield_context_.value()[ec]);
	return ec;
}

IConnection::Result TcpConnection::ReadSome(Buffer& buffer)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	socket_.async_read_some(boost::asio::buffer(buffer), yield_context_.value()[ec]);
	return ec;
}

IConnection::Result TcpConnection::Write(const Buffer& buffer)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	boost::asio::async_write(socket_, boost::asio::buffer(buffer), yield_context_.value()[ec]);
	return ec;
}

IConnection::Result TcpConnection::Close()
{
	boost::system::error_code ec;
	socket_.close(ec);
	return ec;
}

void TcpConnection::Create(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, IConnectionHandlerPtr handler, size_t operation_timeout_ms)
{
	std::shared_ptr<TcpConnection> connection(new TcpConnection(io_context, std::move(socket), operation_timeout_ms));
	connection->CheckOperationTimeout(connection, connection->timer_);
	auto& tmp_connection_ref = *connection;
	boost::asio::spawn(tmp_connection_ref.strand_, [connection = std::move(connection), handler = std::move(handler)](boost::asio::yield_context yield) mutable {
		connection->Initialize(std::move(yield));
		handler->OnNewConnection(std::move(connection));
	}, boost::asio::detached);
}

void TcpConnection::Initialize(boost::asio::yield_context&& yield)
{
	yield_context_.emplace(std::move(yield));
}

void TcpConnection::CheckOperationTimeout(std::weak_ptr<TcpConnection> self, std::shared_ptr<boost::asio::steady_timer> timer)
{
	boost::asio::spawn(strand_, [self = std::move(self), timer = std::move(timer)](boost::asio::yield_context yield) mutable {
		while (timer->expires_at() > std::chrono::steady_clock::now()) {
			boost::system::error_code ec;
			timer->async_wait(yield[ec]);
		}

		auto connection = self.lock();
		if (!connection) {
			return;
		}
		boost::system::error_code ec;
		connection->socket_.close(ec);
		timer->cancel(ec);
	}, boost::asio::detached);
}

TcpConnection::OperationTimeoutInScope::OperationTimeoutInScope(boost::asio::steady_timer& timer, size_t operation_timeout_ms)
	: timer_(timer)
{
	boost::system::error_code ec;
	timer_.expires_from_now(std::chrono::milliseconds(operation_timeout_ms), ec);
}

TcpConnection::OperationTimeoutInScope::~OperationTimeoutInScope()
{
	boost::system::error_code ec;
	timer_.expires_at(std::chrono::steady_clock::time_point::max(), ec);
}

}