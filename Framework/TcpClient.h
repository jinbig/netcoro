#pragma once

#include "IoContext.h"
#include "Connection.h"

namespace netcoro {

class TcpClient
{
public:
	static void CreateConnection(const IoContext& io_context, IConnectionHandlerPtr handler, size_t operation_timeout_ms);
};

}