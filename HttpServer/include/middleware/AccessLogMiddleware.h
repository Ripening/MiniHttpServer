#pragma once
#include "middleware/Middleware.h"
#include <mutex>

namespace http
{
namespace middleware
{

// 访问日志:请求收尾时打印 方法 路径 状态码
class AccessLogMiddleware : public Middleware
{
public:
    void after(HttpRequest& request,HttpResponse& response) override;

private:
    std::mutex mutex_;  // 多 reactor 下保证日志整行输出
};

} // namespace middleware
} // namespace http
