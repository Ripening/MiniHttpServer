#pragma once
#include "middleware/Middleware.h"
#include "session/SessionManager.h"
#include <string>

namespace http
{
namespace middleware
{

// 鉴权:只拦 pathPrefix 开头的路径(如 "/admin"),未登录直接 401 短路
class AuthMiddleware : public Middleware
{
public:
    // sessionManager 必须比本中间件活得久(HttpServer 里两者同生命周期)
    AuthMiddleware(session::SessionManager* sessionManager,std::string pathPrefix);

    bool before(HttpRequest& request,HttpResponse& response) override;

private:
    session::SessionManager* sessionManager_;
    std::string pathPrefix_;
};

} // namespace middleware
} // namespace http
