#pragma once

#include "Serializer.h"

namespace proto {

const int kMagic = *reinterpret_cast<const int*>("tedp");
const int kProtocolVersion = 1;

enum class PacketType : int
{
	kNone = 0,
	kGreeting = 1,
	kReady = 2,
	kTokens = 3,
};

#pragma pack(push, 1)
struct PacketHeader
{
	int protocol_type_ = kMagic;
	int protocol_version_ = kProtocolVersion;
	PacketType packet_type_ = PacketType::kNone;
	int payload_size_ = 0;

	bool CheckHeader(PacketType expected_packet_type) const
	{
		return protocol_type_ == kMagic && protocol_version_ == kProtocolVersion &&
			packet_type_ == expected_packet_type;
	}
};
#pragma pack(pop)

struct GreetingPacket
{
	std::string client_id_;
	int tokens_number_ = 0;

	void Serialize(Serializer& serializer)
	{
		serializer.Serialize(client_id_);
		serializer.Serialize(tokens_number_);
	}

	bool Deserialize(Deserializer& deserializer)
	{
		return deserializer.Deserialize(client_id_) &&
			deserializer.Deserialize(tokens_number_);
	}
};

#pragma pack(push, 1)
struct ReadyPacket
{
	int result_ = 0;

	void Serialize(Serializer& serializer) { serializer.Serialize(result_); }
	bool Deserialize(Deserializer& deserializer) { return deserializer.Deserialize(result_); }
};
#pragma pack(pop)

inline std::ostream& operator<<(std::ostream& os, const PacketHeader& header)
{
	return os << "ProtocolType: " << header.protocol_type_
		<< "ProtocolVersion: " << header.protocol_version_
		<< "PacketType: " << (int)header.packet_type_
		<< "PaoloadSize: " << header.payload_size_;
}

}