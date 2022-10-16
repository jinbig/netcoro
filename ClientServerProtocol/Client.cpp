#include "pch.h"

#include <fstream>

#include <Framework/Logger.h>

#include "ProtocolData.h"
#include "Client.h"

namespace proto {

Client::Client(std::string address, unsigned short port, std::string client_id, int tokens_number, std::string tokens_file_path)
	: address_(address), port_(port), client_id_(client_id), tokens_number_(tokens_number), tokens_file_path_(tokens_file_path)
{}

void Client::OnNewConnection(netcoro::IConnectionPtr connection)
{
	ConnectToServer(connection) &&
	SendGreetingPacket(connection) &&
	ReceiveReadyPacket(connection) &&
	SendTokensPacket(connection);
}

bool Client::ConnectToServer(netcoro::IConnectionPtr& connection)
{
	auto ec = connection->Connect(address_, port_);
	if (ec) {
		LOG_ERROR << __FUNCTION__ << " Failed to connect to [address:port]: " << address_ << ":" << port_ << " with error : " << ec.value();
	}
	return !ec;
}

bool Client::SendGreetingPacket(netcoro::IConnectionPtr& connection)
{
	netcoro::Buffer buffer(sizeof(PacketHeader));
	Serializer serializer(buffer);
	GreetingPacket{ client_id_, tokens_number_ }.Serialize(serializer);

	PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
	*packet_header = PacketHeader{};
	packet_header->packet_type_ = PacketType::kGreeting;
	packet_header->payload_size_ = (int)buffer.size() - sizeof(PacketHeader);

	if (auto ec = connection->Write(buffer); ec) {
		LOG_DEBUG << __FUNCTION__ << " Failed to send packet with error: " << ec.value();
		return false;
	}
	return true;
}

bool Client::ReceiveReadyPacket(netcoro::IConnectionPtr& connection)
{
	netcoro::Buffer buffer(sizeof(PacketHeader) + sizeof(ReadyPacket));
	if (auto ec = connection->Read(buffer); ec) {
		LOG_DEBUG << __FUNCTION__ << " Failed to read packet with error: " << ec.value();
		return false;
	}
	PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
	if (!packet_header->CheckHeader(PacketType::kReady) || packet_header->payload_size_ != sizeof(ReadyPacket)) {
		LOG_WARNING << __FUNCTION__ << " Unexpected packet with header: " << packet_header;
		return false;
	}
	return true;
}

bool Client::SendTokensPacket(netcoro::IConnectionPtr& connection)
{
	std::ifstream tokens_file(tokens_file_path_, std::ios::binary);
	tokens_file.seekg(0, std::ios::end);
	size_t file_size = tokens_file.tellg();
	netcoro::Buffer buffer(sizeof(PacketHeader) + file_size);
	tokens_file.seekg(0);
	tokens_file.read((char*)buffer.data() + sizeof(PacketHeader), file_size);

	PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
	*packet_header = PacketHeader{};
	packet_header->packet_type_ = PacketType::kTokens;
	packet_header->payload_size_ = (int)buffer.size() - sizeof(PacketHeader);

	if (auto ec = connection->Write(buffer); ec) {
		LOG_DEBUG << __FUNCTION__ << " Failed to send packet with error: " << ec.value();
		return false;
	}
	return true;
}

}