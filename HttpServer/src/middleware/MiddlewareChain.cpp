#include "middleware/MiddlewareChain.h"
#include <algorithm>
#include <iostream>
#include <utility>

namespace http
{
namespace middleware
{

void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware)
{
    if(middleware){
        middlewares_.push_back(std::move(middleware));
    }
}

MiddlewareChain::BeforeResult MiddlewareChain::processBefore(HttpRequest& request,HttpResponse& response)
{
    for(std::size_t i = 0;i < middlewares_.size();++i){
        if(!middlewares_[i]->before(request,response)){
            // 被第 i 层拦截;它自己这层的 after 仍然要执行
            return {i + 1,true};
        }
    }
    return {middlewares_.size(),false};
}

void MiddlewareChain::processAfter(HttpRequest& request,HttpResponse& response,std::size_t depth)
{
    depth = std::min(depth,middlewares_.size());
    while(depth-- > 0){
        try{
            middlewares_[depth]->after(request,response);
        }
        catch(const std::exception& e){
            // 单个中间件收尾失败,不拖累其余中间件
            std::cerr << "[middleware] after 异常: " << e.what() << std::endl;
        }
    }
}

} // namespace middleware
} // namespace http
