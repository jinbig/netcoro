#include "pch.h"

#include <filesystem>

#include "TcpClientServerMock.h"
#include "ClientServerProtocolMock.h"

using namespace test;

TEST(ClientServerProtocol, ClientServerProtocolTest)
{
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());

	const std::string kTokensFileName("test_tokens.bin");
	const std::string kDumpFileName("dump.bin");

	{
		const int kProcessingSleepTimeoutMs = 10;
		const int kThreadPoolSize = 2;
		netcoro::IoContext token_handler_io_context(kThreadPoolSize);
		auto async_task_processor = std::make_shared<netcoro::AsyncTaskProcessor>(token_handler_io_context);
		auto token_handler_factory = std::make_shared<TokenHandlerFactoryMock>(kProcessingSleepTimeoutMs);

		const int kPeriodicalDumpTimeoutMs = 100;
		auto dump_tokens_info_to_file = std::make_shared<DumpTokenHandlersInfoToFileTaskMock>(
			async_task_processor, token_handler_factory, kDumpFileName, kPeriodicalDumpTimeoutMs);
		async_task_processor->PostDeferred(dump_tokens_info_to_file, kPeriodicalDumpTimeoutMs);

		auto token_handler_dispatcher = std::make_shared<proto::TokenHandlerDispatcher>(async_task_processor, token_handler_factory);

		auto server_handler = std::make_shared<proto::Server>(token_handler_dispatcher);
		netcoro::TcpServer server(kPort, server_handler, kThreadPoolSize, kDefaultOperationTimeoutMs);

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
	}

	std::error_code ec;
	ASSERT_TRUE(std::filesystem::remove(kDumpFileName, ec));
	ASSERT_TRUE(std::filesystem::remove(kTokensFileName, ec));
	ASSERT_TRUE(ClientServerBase::IsObjCountersNull());
}