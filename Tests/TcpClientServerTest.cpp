#include "pch.h"

#include "TcpClientServerMock.h"

using namespace test;

TEST(Connection, ClientServerTest)
{
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
	{
		const int kThreadPoolSize = 2;
		ServerMock server_mock(kThreadPoolSize);

		const int kClientNumber = 100;
		ClientMock client_mock(kThreadPoolSize);
		client_mock.StartClients(kClientNumber);
		EXPECT_TRUE(server_mock.CheckResults(kClientNumber));
		EXPECT_TRUE(client_mock.CheckResults(kClientNumber));
		ASSERT_FALSE(ClientServerBase::IsObjCountersNull());
	}
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
}

TEST(Connection, ClientServerTimeoutTest)
{
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
	{
		const int kThreadPoolSize = 1;
		const size_t kOperationTimeoutMs = 10;
		ServerMock server_mock(kThreadPoolSize, kOperationTimeoutMs);

		const int kClientNumber = 1;
		ClientMock client_mock(kThreadPoolSize);
		client_mock.SetSleepForTimeoutOperation(kDefaultOperationTimeoutMs + 100);
		client_mock.StartClients(kClientNumber, kOperationTimeoutMs);
		EXPECT_TRUE(server_mock.CheckResults(0));
		EXPECT_TRUE(client_mock.CheckResults(0));
	}
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
}