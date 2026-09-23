#include "middleware/AuthMiddleware.h"
#include <utility>

namespace http
{
namespace middleware
{

AuthMiddleware::AuthMiddleware(session::SessionManager* sessionManager,std::string pathPrefix)
    :sessionManager_(sessionManager),
     pathPrefix_(std::move(pathPrefix))
{
}

bool AuthMiddleware::before(HttpRequest& request,HttpResponse& response)
{
    const std::string& path = request.getPath();
    if(path.compare(0,pathPrefix_.size(),pathPrefix_) != 0){
        return true;    // 不在保护范围,放行
    }

    // 没有 cookie 时 getSession 会新建一个空会话(沿用现有会话模块的语义)
    auto session = sessionManager_->getSession(request,&response);
    if(session->getValue("isLoggedIn") != "true"){
        response.setStateCode(HttpResponse::k401Unauthorized);
        response.setStateMessage("Unauthorized");
        response.setBody("not logged in\n");
        return false;
    }
    return true;
}

} // namespace middleware
} // namespace http
