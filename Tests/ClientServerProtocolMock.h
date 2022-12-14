#pragma once

#include "pch.h"

#include <Framework/TcpClient.h>
#include <Framework/TcpServer.h>

#include <ClientServerProtocol/Client.h>
#include <ClientServerProtocol/Server.h>
#include <ClientServerProtocol/TokenHandler.h>

namespace test {

class TokenHandlerMock : public proto::ITokenHandler
{
public:
	static bool GenerateTokensFile(const std::string& file_name, int tokens_number, size_t min_token_size);

	TokenHandlerMock(int procession_sleep_timeout_ms);

	bool CheckResults(int clients_number);
	friend std::ostream& operator<<(std::ostream& os, const TokenHandlerMock& token_handler);

private:
	void OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info) final;

	struct ClientData { size_t received_tokens_ = 0; };
	using ClientIdToClientData = std::unordered_map<std::string, ClientData>;

	std::string token_;
	int procession_sleep_timeout_ms_ = 0;
	ClientIdToClientData clients_data_;
	mutable std::mutex clients_data_lock_;
};

class TokenHandlerFactoryMock : public proto::ITokenHandlerFactory
{
public:
	TokenHandlerFactoryMock(int procession_sleep_timeout_ms);

	void DumpInfoToFile(const std::string& file_name);
	bool CheckResults(int tokens_number, int clients_number, int wait_ms = 5000);

private:
	proto::ITokenHandlerPtr Create() final;
	bool CheckTokensResults(int tokens_number, int clients_number);

	int procession_sleep_timeout_ms_;
	std::vector<std::shared_ptr<TokenHandlerMock>> handlers_;
	std::mutex handlers_lock_;
};

class DumpTokenHandlersInfoToFileTaskMock : public netcoro::ITask
{
public:
	DumpTokenHandlersInfoToFileTaskMock(
		const netcoro::IAsyncTaskProcessorPtr& async_task_processor,
		std::shared_ptr<TokenHandlerFactoryMock> token_factory,
		const std::string& file_name, size_t timeout_ms);
private:
	void Process(netcoro::ITaskPtr self_task) final;

	std::weak_ptr<netcoro::IAsyncTaskProcessor> async_task_processor_;
	std::shared_ptr<TokenHandlerFactoryMock> token_factory_;
	std::string file_name_;
	size_t timeout_ms_;
};

}