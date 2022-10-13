#pragma once

#include "Connection.h"

namespace netcoro {

class TcpServerImpl;

class TcpServer
{
public:
	TcpServer(unsigned short port, IConnectionHandlerPtr handler, unsigned short thread_pool_size, size_t operation_timeout_ms);
	~TcpServer();

private:
	void Initialize(IConnectionHandlerPtr handler, short thread_pool_size);
	std::shared_ptr<TcpServerImpl> impl_;
};

}