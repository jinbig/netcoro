#pragma once

#include <unordered_map>

#include <Framework/AsyncTaskProcessor.h>

namespace proto {

class ITokenHandler
{
public:
	struct ClientInfo
	{
		std::string client_id_;
		std::string address_;
		unsigned short port_ = 0;
	};

	virtual ~ITokenHandler() = default;
	virtual void OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info) = 0;
};

using ITokenHandlerPtr = std::shared_ptr<ITokenHandler>;

class ITokenHandlerFactory
{
public:
	virtual ~ITokenHandlerFactory() = default;
	virtual ITokenHandlerPtr Create() = 0;
};

using ITokenHandlerFactoryPtr = std::shared_ptr<ITokenHandlerFactory>;

class TokenHandlerDispatcher : public ITokenHandler
{
public:
	TokenHandlerDispatcher(netcoro::IAsyncTaskProcessorPtr async_task_processor, ITokenHandlerFactoryPtr factory);
	void OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info) final;
private:
	netcoro::IAsyncTaskProcessorPtr async_task_processor_;
	ITokenHandlerFactoryPtr factory_;

	using TokenHandlers = std::unordered_map<std::string, ITokenHandlerPtr>;
	TokenHandlers token_handlers_;
	std::mutex token_handlers_lock_;
};

}