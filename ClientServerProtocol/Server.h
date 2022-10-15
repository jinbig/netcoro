#pragma once

#include <sstream>
#include <fstream>

#include <Framework/TcpClient.h>
#include <Framework/Logger.h>

#include "ProtocolData.h"

namespace proto {

class TokensHandler
{
public:
	struct ClientInfo
	{
		std::string client_id_;
		std::string address_;
		unsigned short port_ = 0;
	};

	virtual void OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info) = 0;
};

using TokensHandlerPtr = std::shared_ptr<TokensHandler>;

class Server : public netcoro::IConnectionHandler
{
public:
	Server(TokensHandlerPtr tokens_handler) : tokens_handler_(std::move(tokens_handler)) {}

private:
	struct Session
	{
		GreetingPacket greeting_packet_;
	};

	void OnNewConnection(netcoro::IConnectionPtr connection) final
	{
		Session session;
		ReceiveGreetingPacket(connection, session) &&
		SendReadyPacket(connection) &&
		ReceiveTokensPacket(connection, session);
	}

	bool ReceiveGreetingPacket(const netcoro::IConnectionPtr& connection, Session& session)
	{
		const size_t kExpectedMaxPacketSize = 512;
		netcoro::Buffer buffer(kExpectedMaxPacketSize);
		if (auto ec = connection->ReadSome(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to read packet with error: " << ec.value();
			return false;
		}
		if (buffer.size() < sizeof(PacketHeader)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected header packet size: " << buffer.size();
			return false;
		}
		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		if (!packet_header->CheckHeader(kPacketTypeGreeting)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected packet with header: " << packet_header;
			return false;
		}
		if (packet_header->payload_size_ > kExpectedMaxPacketSize - sizeof(PacketHeader)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected payload size: " << packet_header->payload_size_ << " Ñlose connection.";
			return false;
		}

		Deserializer deserializer(buffer.data() + sizeof(PacketHeader), buffer.size() - sizeof(PacketHeader));
		if (!session.greeting_packet_.Deserialize(deserializer)) {
			LOG_WARNING << __FUNCTION__ << " Failed to deserialize packet!";
			return false;
		}
		return true;
	}

	bool SendReadyPacket(const netcoro::IConnectionPtr& connection)
	{
		netcoro::Buffer buffer(sizeof(PacketHeader));
		Serializer serializer(buffer);
		const int kErrorCodeSuccess = 1;
		ReadyPacket{ kErrorCodeSuccess }.Serialize(serializer);

		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		*packet_header = PacketHeader{};
		packet_header->packet_type_ = kPacketTypeReady;
		packet_header->payload_size_ = (int)buffer.size() - sizeof(PacketHeader);

		if (auto ec = connection->Write(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to send packet with error: " << ec.value();
			return false;
		}
		return true;
	}

	bool ReceiveTokensPacket(const netcoro::IConnectionPtr& connection, const Session& session)
	{
		const size_t kExpectedPacketMaxSize = 1024 * 16;
		netcoro::Buffer buffer(kExpectedPacketMaxSize);
		if (auto ec = connection->ReadSome(buffer); ec) {
			LOG_DEBUG << __FUNCTION__ << " Failed to read packet with error: " << ec.value();
			return false;
		}
		if (buffer.size() < sizeof(PacketHeader)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected header packet size: " << buffer.size();
			return false;
		}
		PacketHeader* packet_header = reinterpret_cast<PacketHeader*>(buffer.data());
		if (!packet_header->CheckHeader(kPacketTypeTokens)) {
			LOG_WARNING << __FUNCTION__ << " Unexpected packet with header: " << packet_header;
			return false;
		}

		size_t remain_data_size = packet_header->payload_size_ - (buffer.size() - sizeof(PacketHeader));
		if (remain_data_size > 0) {
			Buffer remain_buffer(remain_data_size);
			if (auto ec = connection->Read(remain_buffer); ec) {
				LOG_DEBUG << __FUNCTION__ << " Failed to read packet with error: " << ec.value();
				return false;
			}
			buffer.insert(buffer.end(), remain_buffer.begin(), remain_buffer.end());
		}
		return DeserializeTokens(connection, session, buffer);
	}

	bool DeserializeTokens(const netcoro::IConnectionPtr& connection, const Session& session, const Buffer& buffer)
	{
		Deserializer deserializer(buffer.data() + sizeof(PacketHeader), buffer.size() - sizeof(PacketHeader));
		int tokens_number = 0;
		if (!deserializer.Deserialize(tokens_number)) {
			LOG_ERROR << __FUNCTION__ << " Failed to reserialize tokens number!";
			return false;
		}
		if (tokens_number != session.greeting_packet_.tokens_number_) {
			LOG_ERROR << __FUNCTION__ << " Unexpected received tokens number: " << tokens_number << " Expected: " << session.greeting_packet_.tokens_number_;
			return false;
		}
		
		for (int i = 0; i < session.greeting_packet_.tokens_number_; ++i) {
			std::string token;
			if (!deserializer.Deserialize(token)) {
				return false;
			}
			TokensHandler::ClientInfo client_info{ std::move(session.greeting_packet_.client_id_) };
			connection->GetInfo(netcoro::IConnection::EndPointInfoType::kRemote, client_info.address_, client_info.port_);
			tokens_handler_->OnTokenReceivedFrom(std::move(token), std::move(client_info));
		}
		return true;
	}

	TokensHandlerPtr tokens_handler_;
};

}