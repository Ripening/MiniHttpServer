#pragma once
#include "middleware/Middleware.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace http
{
namespace middleware
{

class MiddlewareChain
{
public:
    // blocked 必须和 depth 分开报:最后一层拦截时 depth 恰好等于 size(),
    // 单看层数分不清"全部放行"还是"被最后一层拦下"
    struct BeforeResult{
        std::size_t depth = 0;      // 已执行 before 的层数,也是 after 要收尾的层数
        bool blocked = false;       // 是否被某层短路拦截
    };

    void addMiddleware(std::shared_ptr<Middleware> middleware);
    std::size_t size() const{
        return middlewares_.size();
    }

    // 正序执行 before。结果由调用方保管再传回 processAfter:
    // 链是跨 reactor 共享的,不能把每请求状态存在链上。
    BeforeResult processBefore(HttpRequest& request,HttpResponse& response);

    // 逆序执行前 depth 层的 after,和 processBefore 的返回值配对使用
    void processAfter(HttpRequest& request,HttpResponse& response,std::size_t depth);

private:
    std::vector<std::shared_ptr<Middleware>> middlewares_;
};

} // namespace middleware
} // namespace http
