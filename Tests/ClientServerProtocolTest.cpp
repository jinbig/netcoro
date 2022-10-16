#include "pch.h"

#include <filesystem>

#include "TcpClientServerMock.h"
#include "ClientServerProtocolMock.h"

using namespace test;

TEST(ClientServerProtocol, ClientServerProtocolTest)
{
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
	{
		const int kProcessionSleepTimeoutMs = 10;
		auto token_handler_factory = std::make_shared<TokenHandlerFactoryMock>(kProcessionSleepTimeoutMs);

		const int kThreadPoolSize = 2;
		netcoro::IoContext token_handler_io_context(kThreadPoolSize);
		auto async_task_processor = std::make_shared<netcoro::AsyncTaskProcessor>(token_handler_io_context);
		auto token_handler_dispatcher = std::make_shared<proto::TokenHandlerDispatcher>(async_task_processor, token_handler_factory);

		auto server_handler = std::make_shared<proto::Server>(token_handler_dispatcher);
		netcoro::TcpServer server(kPort, server_handler, kThreadPoolSize, kDefaultOperationTimeoutMs);

		const std::string kTokensFileName("test_tokens.bin");
		const std::string kResultsFileName("results.bin");
		const int kTokensNumber = 10;
		const int kTokenMinSize = 30;
		ASSERT_TRUE(TokenHandlerMock::GenerateTokensFile(kTokensFileName, kTokensNumber, kTokenMinSize));

		netcoro::IoContext clients_io_context(kThreadPoolSize);
		const int kClientsNumber = 10;
		const int kClientIdMinSize = 30;
		for (int i = 0; i < kClientsNumber; ++i) {
			std::stringstream client_id;
			client_id << std::string(kClientIdMinSize, '0') << i + 1000;

			auto client_handler = std::make_shared<proto::Client>(kAddress, kPort, client_id.str(), kTokensNumber, kTokensFileName);
			netcoro::TcpClient::CreateConnection(clients_io_context, std::move(client_handler), kDefaultOperationTimeoutMs);
		}

		ASSERT_TRUE(token_handler_factory->CheckResults(kTokensNumber, kClientsNumber));
		token_handler_factory->DumpInfoToFile(kResultsFileName);
		std::error_code ec;
		ASSERT_TRUE(std::filesystem::remove(kResultsFileName, ec));
		ASSERT_TRUE(std::filesystem::remove(kTokensFileName, ec));
	}
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
}