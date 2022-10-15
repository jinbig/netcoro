#include "pch.h"

#include <ClientServerProtocol/Serializer.h>

struct TestData
{
	std::string id_;
	int size_;

	bool operator==(const TestData& data) const {
		return id_ == data.id_ && size_ == data.size_;
	}

	void Serialize(Serializer& serializer) const
	{
		serializer.Serialize(id_);
		serializer.Serialize(size_);
	}
	bool Deserialize(Deserializer& serializer)
	{
		if (!serializer.Deserialize(id_)) { return false; }
		if (!serializer.Deserialize(size_)) { return false; }
		return true;
	}
};

TEST(Serializer, SerializerTest)
{
	Buffer buffer;
	Serializer serializer(buffer);
	TestData test_data{ "test_str", 777 };
	test_data.Serialize(serializer);

	Deserializer deserializer(buffer);
	TestData res_test_data;
	ASSERT_TRUE(res_test_data.Deserialize(deserializer));
	ASSERT_TRUE(test_data == res_test_data);
}