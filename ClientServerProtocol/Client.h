#pragma once

#include <sstream>
#include <fstream>

#include <Framework/TcpClient.h>
#include <Framework/Logger.h>

#include "ProtocolData.h"

namespace proto {

class Client : public netcoro::IConnectionHandler
{
public:
	Client(std::string address, unsigned short port, std::string client_id, int tokens_number, std::string tokens_file_path)
		: address_(address), port_(port), client_id_(client_id), tokens_number_(tokens_number), tokens_file_path_(tokens_file_path) {}

private:
	void OnNewConnection(netcoro::IConnectionPtr connection) final
	{
		ConnectToServer(connection) &&
		SendGreetingPacket(connection) &&
		ReceiveReadyPacket(connection) &&
		SendTokensPacket(connection);
	}

	bool ConnectToServer(netcoro::IConnectionPtr& connection)
	{
		auto ec = connection->Connect(address_, port_);
		if (ec) {
			LOG_ERROR << __FUNCTION__ << " Failed to connect to [address:port]: " << address_ << ":" << port_ << " with error : " << ec.value();
		}
		return !ec;
	}

	bool SendGreetingPacket(netcoro::IConnectionPtr& connection)
	{
		netcoro::Buffer buffer(sizeof(PacketHeader));
		Serializer serializer(buffer);
		GreetingPacket{ client_id_, tokens_number_ }.Serialize(serializer);

		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		*packet_header = PacketHeader{};
		packet_header->packet_type_ = kPacketTypeGreeting;
		packet_header->payload_size_ = (int)buffer.size() - sizeof(PacketHeader);

		if (auto ec = connection->Write(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to send packet with error: " << ec.value();
			return false;
		}
		return true;
	}

	bool ReceiveReadyPacket(netcoro::IConnectionPtr& connection)
	{
		netcoro::Buffer buffer(sizeof(PacketHeader) + sizeof(ReadyPacket));
		if (auto ec = connection->Read(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to read packet with error: " << ec.value();
			return false;
		}
		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		if (!packet_header->CheckHeader(kPacketTypeReady) || packet_header->payload_size_ != sizeof(ReadyPacket)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected packet with header: " << packet_header;
			return false;
		}
		return true;
	}

	bool SendTokensPacket(netcoro::IConnectionPtr& connection)
	{
		std::ifstream tokens_file(tokens_file_path_, std::ios::binary);
		tokens_file.seekg(0, std::ios::end);
		size_t file_size = tokens_file.tellg();
		netcoro::Buffer buffer(sizeof(PacketHeader) + file_size);
		tokens_file.seekg(0);
		tokens_file.read((char*)buffer.data() + sizeof(PacketHeader), file_size);

		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		*packet_header = PacketHeader{};
		packet_header->packet_type_ = kPacketTypeTokens;
		packet_header->payload_size_ = (int)buffer.size() - sizeof(PacketHeader);

		if (auto ec = connection->Write(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to send packet with error: " << ec.value();
			return false;
		}
		return true;
	}

	std::string address_;
	unsigned short port_;

	std::string client_id_;
	int tokens_number_;
	std::string tokens_file_path_;
};

}