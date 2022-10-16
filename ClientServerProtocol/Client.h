#pragma once

#include <Framework/Logger.h>
#include <Framework/Connection.h>

namespace proto {

class Client : public netcoro::IConnectionHandler
{
public:
	Client(std::string address, unsigned short port, std::string client_id, int tokens_number, std::string tokens_file_path);

private:
	void OnNewConnection(netcoro::IConnectionPtr connection) final;

	bool ConnectToServer(netcoro::IConnectionPtr& connection);
	bool SendGreetingPacket(netcoro::IConnectionPtr& connection);
	bool ReceiveReadyPacket(netcoro::IConnectionPtr& connection);
	bool SendTokensPacket(netcoro::IConnectionPtr& connection);

	std::string address_;
	unsigned short port_;

	std::string client_id_;
	int tokens_number_;
	std::string tokens_file_path_;
};

}