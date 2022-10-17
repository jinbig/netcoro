#include "pch.h"

#include "Logger.h"
#include "TcpConnection.h"

namespace netcoro {

TcpConnection::TcpConnection(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket&& socket, size_t operation_timeout_ms)
	: strand_(std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(io_context.get_executor()))
	, timer_(std::make_shared<boost::asio::steady_timer>(io_context, std::chrono::steady_clock::time_point::max()))
	, socket_(std::move(socket))
	, operation_timeout_ms_(operation_timeout_ms)
{
}

IConnection::Result TcpConnection::GetInfo(EndPointInfoType type, std::string& address, unsigned short& port)
{
	boost::system::error_code ec;
	auto end_point = (type == EndPointInfoType::kLocal ? socket_.local_endpoint(ec) : socket_.remote_endpoint(ec));
	if (!ec) {
		address = end_point.address().to_string();
		port = end_point.port();
	}
	return ec;
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
	return Read(buffer.data(), buffer.size());
}

IConnection::Result TcpConnection::Read(unsigned char* buffer, size_t buffer_size)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	boost::asio::async_read(socket_, boost::asio::buffer(buffer, buffer_size), yield_context_.value()[ec]);
	return ec;
}

IConnection::Result TcpConnection::ReadSome(Buffer& buffer)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	size_t bytes_transferred = socket_.async_read_some(boost::asio::buffer(buffer), yield_context_.value()[ec]);
	buffer.resize(bytes_transferred);
	return ec;
}

IConnection::Result TcpConnection::Write(const Buffer& buffer)
{
	return Write(buffer.data(), buffer.size());
}

IConnection::Result TcpConnection::Write(const unsigned char* buffer, size_t buffer_size)
{
	boost::system::error_code ec;
	OperationTimeoutInScope operation_timeout(*timer_, operation_timeout_ms_);
	boost::asio::async_write(socket_, boost::asio::buffer(buffer, buffer_size), yield_context_.value()[ec]);
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
	CheckOperationTimeout(connection, connection->timer_, connection->strand_);
	auto& strand_ref = *connection->strand_;
	boost::asio::spawn(strand_ref, [connection = std::move(connection), handler = std::move(handler)](boost::asio::yield_context yield) mutable {
		connection->Initialize(std::move(yield));
		handler->OnNewConnection(std::move(connection));
	});
}

void TcpConnection::Initialize(boost::asio::yield_context&& yield)
{
	yield_context_.emplace(std::move(yield));
}

void TcpConnection::CheckOperationTimeout(std::weak_ptr<TcpConnection> self, TimerPtr timer, StrandPtr strand)
{
	auto& timer_ref = *timer;
	auto& strand_ref = *strand;
	timer_ref.async_wait(boost::asio::bind_executor(strand_ref,
		[self = std::move(self), timer = std::move(timer), strand = std::move(strand)](const boost::system::error_code&) mutable {
		if(timer->expires_at() > std::chrono::steady_clock::now()) {
			CheckOperationTimeout(std::move(self), std::move(timer), std::move(strand));
			return;
		}

		auto connection = self.lock();
		if (!connection) {
			return;
		}
		boost::system::error_code ec;
		connection->socket_.close(ec);
		timer->cancel(ec);
	}));
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