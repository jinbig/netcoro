#include "pch.h"

#include "IoContext.h"
#include "TcpClient.h"
#include "TcpConnection.h"

namespace netcoro {

void TcpClient::CreateConnection(const IoContext& io_context, IConnectionHandlerPtr handler, size_t operation_timeout_ms)
{
	TcpConnection::Create(
		*io_context.GetIoContext(),
		boost::asio::ip::tcp::socket(*io_context.GetIoContext(), boost::asio::ip::tcp::v4()),
		std::move(handler),
		operation_timeout_ms);
}

}