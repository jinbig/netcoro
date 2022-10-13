#include "pch.h"

#include "Logger.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "IoContext.h"

namespace netcoro {

class TcpServerImpl
{
public:
	TcpServerImpl(unsigned short port, unsigned short thread_pool_size, size_t operation_timeout_ms);

	void AcceptConnection(std::shared_ptr<TcpServerImpl> server, IConnectionHandlerPtr handler);

	IoContext io_context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	const size_t operation_timeout_ms_;
};

TcpServerImpl::TcpServerImpl(unsigned short port, unsigned short thread_pool_size, size_t operation_timeout_ms)
	: io_context_(thread_pool_size)
	, acceptor_(*io_context_.GetIoContext(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	, operation_timeout_ms_(operation_timeout_ms)
{
}

void TcpServerImpl::AcceptConnection(std::shared_ptr<TcpServerImpl> server, IConnectionHandlerPtr handler)
{
	acceptor_.async_accept([this, server = std::move(server), handler = std::move(handler)](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) mutable {
		if (!ec) {
			TcpConnection::Create(*io_context_.GetIoContext(), std::move(socket), handler, operation_timeout_ms_);
		}
		if (acceptor_.is_open()) {
			AcceptConnection(std::move(server), std::move(handler));
		}
	});
}

TcpServer::TcpServer(unsigned short port, IConnectionHandlerPtr handler, unsigned short thread_pool_size, size_t operation_timeout_ms)
	: impl_(std::make_shared<TcpServerImpl>(port, thread_pool_size, operation_timeout_ms))
{
	Initialize(std::move(handler), thread_pool_size);
}

void TcpServer::Initialize(IConnectionHandlerPtr handler, short thread_pool_size)
{
	for (short i = 0; i < thread_pool_size; ++i) {
		impl_->AcceptConnection(impl_, handler);
	}
}

TcpServer::~TcpServer()
{
	LOG_DEBUG << __FUNCTION__;
	impl_->io_context_.Stop();
	impl_->acceptor_.close();
}

}