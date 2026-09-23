#include "middleware/RateLimitMiddleware.h"

namespace http
{
namespace middleware
{

RateLimitMiddleware::RateLimitMiddleware(std::size_t maxPerSecond)
    :maxPerSecond_(maxPerSecond),
     windowStart_(std::chrono::steady_clock::now())
{
}

bool RateLimitMiddleware::before(HttpRequest&,HttpResponse& response)
{
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    if(now - windowStart_ >= std::chrono::seconds(1)){
        windowStart_ = now;     // 进入新窗口,重新计数
        count_ = 0;
    }
    if(++count_ > maxPerSecond_){
        response.setStateCode(HttpResponse::k429TooManyRequests);
        response.setStateMessage("Too Many Requests");
        response.setBody("rate limit exceeded\n");
        return false;
    }
    return true;
}

} // namespace middleware
} // namespace http
