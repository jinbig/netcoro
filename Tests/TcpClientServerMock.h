#pragma once

#include "pch.h"

#include <mutex>

#include <Framework/Logger.h>
#include <Framework/TcpClient.h>
#include <Framework/TcpServer.h>

namespace test {

const std::string kAddress = "127.0.0.1";
const unsigned short kPort = 1111;
const size_t kDefaultOperationTimeoutMs = 1000;

class ClientServerBase
{
public:
	ClientServerBase(netcoro::IConnectionHandlerPtr handler) : handler_(std::move(handler)) { }

	size_t GetSleepForTimeoutOperation() const { return sleep_for_timeot_operation_ms_; }
	void SetSleepForTimeoutOperation(size_t ms) { sleep_for_timeot_operation_ms_ = ms; }

	void AddResult() { results_.fetch_add(1, std::memory_order_relaxed); }
	bool CheckResults(int result, int wait_ms = 1000);

protected:
	size_t sleep_for_timeot_operation_ms_ = 0;
	netcoro::IConnectionHandlerPtr handler_;
	std::atomic<int> results_ = 0;
};

class ClientMock : public ClientServerBase
{
public:
	class Handler : public netcoro::IConnectionHandler
	{
	public:
		Handler(ClientServerBase& test) : test_(test) {}
		void OnNewConnection(netcoro::IConnectionPtr connection) final;
		ClientServerBase& test_;
	};

	ClientMock(short thread_pool_size);
	void StartClients(int clients_number, size_t operation_timeout_ms = kDefaultOperationTimeoutMs);

private:
	netcoro::IoContext client_io_context_;
};

class ServerMock : public ClientServerBase
{
public:
	class Handler : public netcoro::IConnectionHandler
	{
	public:
		Handler(ClientServerBase& test) : test_(test) {}
		void OnNewConnection(netcoro::IConnectionPtr connection) final;
		ClientServerBase& test_;
	};

	ServerMock(short thread_pool_size, size_t operation_timeout_ms = kDefaultOperationTimeoutMs);

private:
	netcoro::TcpServer server_;
};

}