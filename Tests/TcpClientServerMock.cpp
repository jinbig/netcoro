#include "pch.h"

#include <Framework/AsyncTaskProcessor.h>

#include "TcpClientServerMock.h"

namespace test {

bool ClientServerBase::CheckResults(int result, int wait_ms)
{
	for (int i = 0; i < wait_ms / 10; ++i) {
		if (result == results_.load(std::memory_order_relaxed)) { return true; }
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

bool ClientServerBase::IsObjCountersNull()
{
	bool result = netcoro::ObjCounter<netcoro::TcpServer>::IsEmpty();
	result &= netcoro::ObjCounter<netcoro::IConnection>::IsEmpty();
	result &= netcoro::ObjCounter<netcoro::IConnectionHandler>::IsEmpty();
	result &= netcoro::ObjCounter<netcoro::IoContext>::IsEmpty();
	result &= netcoro::ObjCounter<netcoro::ITask>::IsEmpty();
	result &= netcoro::ObjCounter<netcoro::IAsyncTaskProcessor>::IsEmpty();
	return result;
}

ClientMock::ClientMock(short thread_pool_size)
	: ClientServerBase(std::make_shared<Handler>(*this))
	, client_io_context_(thread_pool_size)
{ }

void ClientMock::StartClients(int clients_number, size_t operation_timeout_ms)
{
	for (int i = 0; i < clients_number; ++i) {
		netcoro::TcpClient::CreateConnection(client_io_context_, handler_, operation_timeout_ms);
	}
}

void ClientMock::Handler::OnNewConnection(netcoro::IConnectionPtr connection)
{
	bool result = !connection->Connect(kAddress, kPort);
	if (test_.GetSleepForTimeoutOperation()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(test_.GetSleepForTimeoutOperation()));
	}
	netcoro::Buffer buffer(10, 0);
	result &= !connection->Write(buffer) && !connection->Read(buffer);
	if (result) {
		test_.AddResult();
	}
}

ServerMock::ServerMock(short thread_pool_size, size_t operation_timeout_ms)
	: ClientServerBase(std::make_shared<Handler>(*this))
	, server_(kPort, handler_, thread_pool_size, operation_timeout_ms)
{ }

void ServerMock::Handler::OnNewConnection(netcoro::IConnectionPtr connection)
{
	netcoro::Buffer buffer(100, 0);
	bool result = !connection->ReadSome(buffer) && !connection->Write(buffer);
	if (result) {
		test_.AddResult();
	}
}

}