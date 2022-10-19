#include "pch.h"

#include "TokenHandler.h"

namespace proto {

class AsyncTokenHandlerTask : public netcoro::ITask
{
public:
	AsyncTokenHandlerTask(ITokenHandlerPtr&& token_handler, ITokenHandler::ClientInfo&& client_info, std::string&& token)
		: token_handler_(std::move(token_handler)), client_info_(std::move(client_info)), token_(std::move(token)) {}
	void Process(netcoro::ITaskPtr) final { token_handler_->OnTokenReceivedFrom(std::move(token_), std::move(client_info_)); }
	ITokenHandlerPtr token_handler_;
	ITokenHandler::ClientInfo client_info_;
	std::string token_;
};

TokenHandlerDispatcher::TokenHandlerDispatcher(netcoro::IAsyncTaskProcessorPtr async_task_processor, ITokenHandlerFactoryPtr factory)
	: async_task_processor_(async_task_processor), factory_(std::move(factory))
{}

void TokenHandlerDispatcher::OnTokenReceivedFrom(std::string&& token, ClientInfo&& client_info)
{
	ITokenHandlerPtr token_handler;
	{
		std::lock_guard lock(token_handlers_lock_);
		auto iter = token_handlers_.find(token);
		if (iter == token_handlers_.end()) {
			token_handler = factory_->Create();
			token_handlers_.emplace(token, token_handler);
		} else {
			token_handler = iter->second;
		}
	}
	async_task_processor_->Post(std::make_shared<AsyncTokenHandlerTask>(std::move(token_handler), std::move(client_info), std::move(token)));
}

}