#include "pch.h"

#include <mutex>

#include <Framework/Logger.h>
#include <Framework/TcpClient.h>
#include <Framework/TcpServer.h>

const std::string kAddress = "127.0.0.1";
const unsigned short kPort = 1111;
const size_t kDefaultOperationTimeoutMs = 1000;

class ClientServerTest
{
public:
	class ServerHandler : public netcoro::IConnectionHandler
	{
	public:
		ServerHandler(ClientServerTest& test) : test_(test) {}
		void OnNewConnection(netcoro::IConnectionPtr connection) final
		{
			netcoro::Buffer buffer(100, 0);
			bool result = !connection->ReadSome(buffer) && !connection->Write(buffer);
			if (result) {
				test_.AddServerResult();
			}
		}
		ClientServerTest& test_;
	};
	class ClientHandler : public netcoro::IConnectionHandler
	{
	public:
		ClientHandler(ClientServerTest& test) : test_(test) {}
		void OnNewConnection(netcoro::IConnectionPtr connection) final
		{
			bool result = !connection->Connect(kAddress, kPort);
			if (test_.add_sleep_for_timeot_operation_ms_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(test_.add_sleep_for_timeot_operation_ms_));
			}
			netcoro::Buffer buffer(10, 0);
			result &= !connection->Write(buffer) && !connection->Read(buffer);
			if (result) {
				test_.AddClientResult();
			}
		}
		ClientServerTest& test_;
	};
	ClientServerTest(short thread_pool_size, size_t operation_timeout_ms = kDefaultOperationTimeoutMs)
		: server_handler_(std::make_shared<ServerHandler>(*this))
		, server_(kPort, server_handler_, thread_pool_size, operation_timeout_ms)
		, client_handler_(std::make_shared<ClientHandler>(*this))
		, client_io_context_(thread_pool_size)
	{ }

	void StartClients(int clients_number, size_t operation_timeout_ms = kDefaultOperationTimeoutMs)
	{
		for (int i = 0; i < clients_number; ++i) {
			netcoro::TcpClient::CreateConnection(client_io_context_, client_handler_, operation_timeout_ms);
		}
	}

	void SetSleepForTimeoutOperation(size_t ms) { add_sleep_for_timeot_operation_ms_ = ms; }
	bool CheckServerResults(int result, int wait_ms = 1000) { return CheckResults(server_results_, result, wait_ms); }
	bool CheckClientResults(int result, int wait_ms = 1000) { return CheckResults(client_results_, result, wait_ms); }
private:
	void AddServerResult() { server_results_.fetch_add(1, std::memory_order_relaxed); }
	void AddClientResult() { client_results_.fetch_add(1, std::memory_order_relaxed); }
	bool CheckResults(std::atomic<int>& results, int result, int wait_ms)
	{
		for (int i = 0; i < wait_ms / 10; ++i) {
			if (result == results.load(std::memory_order_relaxed)) { return true; }
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return false;
	}

	size_t add_sleep_for_timeot_operation_ms_ = 0;
	std::shared_ptr<ServerHandler> server_handler_;
	netcoro::TcpServer server_;
	std::atomic<int> server_results_ = 0;

	std::shared_ptr<ClientHandler> client_handler_;
	netcoro::IoContext client_io_context_;
	std::atomic<int> client_results_ = 0;
};

TEST(Connection, ClientServerTest) {
	const int kThreadPoolSize = 2;
	ClientServerTest client_srv_test(kThreadPoolSize);

	const int kClientNumber = 100;
	client_srv_test.StartClients(kClientNumber);
	EXPECT_TRUE(client_srv_test.CheckClientResults(kClientNumber));
	EXPECT_TRUE(client_srv_test.CheckServerResults(kClientNumber));
}

TEST(Connection, ClientServerTimeoutTest) {
	const int kThreadPoolSize = 1;
	const size_t kOperationTimeoutMs = 10;
	ClientServerTest client_srv_test(kThreadPoolSize, kOperationTimeoutMs);

	const int kClientNumber = 1;
	client_srv_test.SetSleepForTimeoutOperation(kDefaultOperationTimeoutMs + 100);
	client_srv_test.StartClients(kClientNumber, kOperationTimeoutMs);
	EXPECT_TRUE(client_srv_test.CheckClientResults(0));
	EXPECT_TRUE(client_srv_test.CheckServerResults(0));
}