#include "pch.h"

#include <filesystem>

#include <Framework/TcpClient.h>
#include <Framework/TcpServer.h>

#include <ClientServerProtocol/Client.h>
#include <ClientServerProtocol/Server.h>
#include <ClientServerProtocol/TokenHandler.h>

const std::string kAddress = "127.0.0.1";
const unsigned short kPort = 11111;
const size_t kDefaultOperationTimeoutMs = 100000;

class TokensHandlerTest : public proto::ITokenHandler
{
public:
	TokensHandlerTest(int procession_sleep_timeout_ms) : procession_sleep_timeout_ms_(procession_sleep_timeout_ms) {}

	static bool GenerateTokensFile(const std::string& file_name, int tokens_number, size_t min_token_size)
	{
		Buffer buffer;
		Serializer serializer(buffer);

		serializer.Serialize(tokens_number);
		for (int i = 0; i < tokens_number; ++i) {
			std::stringstream token;
			token << std::string(min_token_size, '0') << i + 100000;
			serializer.Serialize(token.str());
		}
		std::ofstream file(file_name, std::ios::binary);
		file.write((char*)buffer.data(), buffer.size());
		return true;
	}
	void DumpClientsDataToFile(const std::string& file_name)
	{
		std::stringstream data;
		{
			std::lock_guard lock(tokens_data_lock_);
			data << "TokensNumber: " << tokens_data_.size() << std::endl;
			for (const auto& [token, token_data] : tokens_data_) {
				data << '\t' << "Token: " << token << std::endl;
				for (const auto& [client_id, client_data] : token_data) {
					data << "\t\t" << "ClientId: " << client_id << " ReceivedTokens: " << client_data.received_tokens_ << std::endl;
				}
				data << std::endl;
			}
		}
		std::ofstream file(file_name, std::ios::binary);
		file << data.rdbuf();
	}

	bool CheckResults(int tokens_number, int clients_number, int wait_ms = 5000)
	{
		for (int i = 0; i < wait_ms / 10; ++i) {
			if (CheckDataResults(tokens_number, clients_number)) { return true; }
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return false;
	}
private:
	bool CheckDataResults(int tokens_number, int clients_number)
	{
		std::lock_guard lock(tokens_data_lock_);
		if (tokens_number != tokens_data_.size()) { return false; }
		for (const auto& [token, token_data] : tokens_data_) {
			if (clients_number != token_data.size()) { return false; }
			for (const auto& [client_id, client_data] : token_data) {
				if (client_data.received_tokens_ != 1) { return false; }
			}
		}
		return true;
	}

	struct ClientData
	{
		size_t expected_tokens_ = 0;
		size_t received_tokens_ = 0;
	};

	using ClientsData = std::unordered_map<std::string, ClientData>;
	using TokensData = std::unordered_map<std::string, ClientsData>;


	void OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info) final
	{
		if (procession_sleep_timeout_ms_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(procession_sleep_timeout_ms_));
		}
		std::stringstream id;
		id << '[' << client_info.client_id_ << ']' << client_info.address_ << ':' << client_info.port_;
		std::lock_guard lock(tokens_data_lock_);
		auto& token_data = tokens_data_[std::move(token)];
		auto& client_data = token_data[id.str()];
		++client_data.received_tokens_;
	}
	int procession_sleep_timeout_ms_ = 0;
	TokensData tokens_data_;
	std::mutex tokens_data_lock_;
};

class SingleTokenHandlerFactoryTest : public proto::ITokenHandlerFactory
{
public:
	SingleTokenHandlerFactoryTest(proto::ITokenHandlerPtr handler) : handler_(std::move(handler)) {}
	proto::ITokenHandlerPtr Create() final { return handler_; }
private:
	proto::ITokenHandlerPtr handler_;
};

TEST(ClientServerProtocol, ClientServerProtocolTest)
{
	const int kProcessionSleepTimeoutMs = 10;
	auto tokens_handler = std::make_shared<TokensHandlerTest>(kProcessionSleepTimeoutMs);
	auto single_token_handler_factory = std::make_shared<SingleTokenHandlerFactoryTest>(tokens_handler);

	const int kThreadPoolSize = 2;
	netcoro::IoContext token_handler_io_context(kThreadPoolSize);
	auto async_task_processor = std::make_shared<netcoro::AsyncTaskProcessor>(token_handler_io_context);
	auto token_handler_dispatcher = std::make_shared<proto::TokenHandlerDispatcher>(async_task_processor, single_token_handler_factory);

	auto server_handler = std::make_shared<proto::Server>(token_handler_dispatcher);
	netcoro::TcpServer server(kPort, server_handler, kThreadPoolSize, kDefaultOperationTimeoutMs);

	const std::string kTokensFileName("test_tokens.bin");
	const std::string kResultsFileName("results.bin");
	const int kTokensNumber = 10;
	const int kTokenMinSize = 30;
	ASSERT_TRUE(TokensHandlerTest::GenerateTokensFile(kTokensFileName, kTokensNumber, kTokenMinSize));

	netcoro::IoContext clients_io_context(kThreadPoolSize);
	const int kClientsNumber = 10;
	const int kClientIdMinSize = 30;
	for (int i = 0; i < kClientsNumber; ++i) {
		std::stringstream client_id;
		client_id << std::string(kClientIdMinSize, '0') << i + 1000;

		auto client_handler = std::make_shared<proto::Client>(kAddress, kPort, client_id.str(), kTokensNumber, kTokensFileName);
		netcoro::TcpClient::CreateConnection(clients_io_context, std::move(client_handler), kDefaultOperationTimeoutMs);		
	}

	ASSERT_TRUE(tokens_handler->CheckResults(kTokensNumber, kClientsNumber));
	tokens_handler->DumpClientsDataToFile(kResultsFileName);
	std::error_code ec;
	ASSERT_TRUE(std::filesystem::remove(kResultsFileName, ec));
	ASSERT_TRUE(std::filesystem::remove(kTokensFileName, ec));
}