#pragma once
#include "middleware/Middleware.h"
#include <chrono>
#include <cstddef>
#include <mutex>

namespace http
{
namespace middleware
{

// 限流:固定窗口,每秒最多放过 maxPerSecond 个请求,超出的直接 429 短路
class RateLimitMiddleware : public Middleware
{
public:
    explicit RateLimitMiddleware(std::size_t maxPerSecond);

    bool before(HttpRequest& request,HttpResponse& response) override;

private:
    std::size_t maxPerSecond_;
    std::size_t count_ = 0;
    std::chrono::steady_clock::time_point windowStart_;
    std::mutex mutex_;
};

} // namespace middleware
} // namespace http
