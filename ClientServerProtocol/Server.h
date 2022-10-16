#pragma once

#include <Framework/Connection.h>

#include "ProtocolData.h"
#include "TokenHandler.h"

namespace proto {

class Server : public netcoro::IConnectionHandler
{
public:
	Server(ITokenHandlerPtr tokens_handler);

private:
	struct Session
	{
		GreetingPacket greeting_packet_;
	};

	void OnNewConnection(netcoro::IConnectionPtr connection) final;

	bool ReceiveGreetingPacket(const netcoro::IConnectionPtr& connection, Session& session);
	bool SendReadyPacket(const netcoro::IConnectionPtr& connection);
	bool ReceiveTokensPacket(const netcoro::IConnectionPtr& connection, const Session& session);
	bool DeserializeTokens(const netcoro::IConnectionPtr& connection, const Session& session, const Buffer& buffer);

	ITokenHandlerPtr tokens_handler_;
};

}