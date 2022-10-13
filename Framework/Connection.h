#pragma once

#include <boost/system/error_code.hpp>

namespace netcoro {

using Buffer = std::vector<unsigned char>;

class IConnection
{
public:
	using Result = boost::system::error_code;
	virtual ~IConnection() = default;
	virtual Result Connect(const std::string& ip_address, unsigned short port) = 0;
	virtual Result Read(Buffer& buffer) = 0;
	virtual Result ReadSome(Buffer& buffer) = 0;
	virtual Result Write(const Buffer& buffer) = 0;
	virtual Result Close() = 0;
};

using IConnectionPtr = std::shared_ptr<IConnection>;

class IConnectionHandler
{
public:
	virtual ~IConnectionHandler() = default;
	virtual void OnNewConnection(IConnectionPtr connection) = 0;
};

using IConnectionHandlerPtr = std::shared_ptr<IConnectionHandler>;

}