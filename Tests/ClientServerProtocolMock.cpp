#include "pch.h"

#include <fstream>

#include "ClientServerProtocolMock.h"

namespace test {

TokenHandlerMock::TokenHandlerMock(int procession_sleep_timeout_ms)
	: procession_sleep_timeout_ms_(procession_sleep_timeout_ms)
{}

bool TokenHandlerMock::GenerateTokensFile(const std::string& file_name, int tokens_number, size_t min_token_size)
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

std::ostream& operator<<(std::ostream& os, const TokenHandlerMock& token_handler)
{
	std::lock_guard lock(token_handler.clients_data_lock_);
	os << '\t' << "Token: " << token_handler.token_ << std::endl;
	for (const auto& [client_id, client_data] : token_handler.clients_data_) {
		os << "\t\t" << "ClientId: " << client_id << " ReceivedTokens: " << client_data.received_tokens_ << std::endl;
	}
	return os;
}

bool TokenHandlerMock::CheckResults(int clients_number)
{
	std::lock_guard lock(clients_data_lock_);
	if (clients_number != clients_data_.size()) { return false; }
	for (const auto& [client_id, client_data] : clients_data_) {
		if (client_data.received_tokens_ != 1) { return false; }
	}
	return true;
}

void TokenHandlerMock::OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info)
{
	if (token_.empty()) {
		token_ = std::move(token);
	} else {
		assert(token_ == token);
	}

	if (procession_sleep_timeout_ms_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(procession_sleep_timeout_ms_));
	}
	std::stringstream id;
	id << '[' << client_info.client_id_ << ']' << client_info.address_ << ':' << client_info.port_;
	std::lock_guard lock(clients_data_lock_);
	auto& client_data = clients_data_[id.str()];
	++client_data.received_tokens_;
}

TokenHandlerFactoryMock::TokenHandlerFactoryMock(int procession_sleep_timeout_ms)
	: procession_sleep_timeout_ms_(procession_sleep_timeout_ms)
{}

proto::ITokenHandlerPtr TokenHandlerFactoryMock::Create()
{
	auto token_handler = std::make_shared<TokenHandlerMock>(procession_sleep_timeout_ms_);
	std::lock_guard lock(handlers_lock_);
	handlers_.push_back(token_handler);
	return token_handler;
}

void TokenHandlerFactoryMock::DumpInfoToFile(const std::string& file_name)
{
	std::stringstream data;
	{
		std::lock_guard lock(handlers_lock_);
		data << "TokensNumber: " << handlers_.size() << std::endl;
		for (const auto& token_handler : handlers_) {
			data << *token_handler << std::endl;
		}
	}
	std::ofstream file(file_name, std::ios::binary);
	file << data.rdbuf();
}

bool TokenHandlerFactoryMock::CheckResults(int tokens_number, int clients_number, int wait_ms)
{
	for (int i = 0; i < wait_ms / 10; ++i) {
		if (CheckTokensResults(tokens_number, clients_number)) { return true; }
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

bool TokenHandlerFactoryMock::CheckTokensResults(int tokens_number, int clients_number)
{
	std::lock_guard lock(handlers_lock_);
	if (tokens_number != handlers_.size()) { return false; }
	for (const auto& token_handler : handlers_) {
		if (!token_handler->CheckResults(clients_number)) { return false; }
	}
	return true;
}

}