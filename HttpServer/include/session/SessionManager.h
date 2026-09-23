#pragma once
#include "session/SessionStorage.h"
#include "session/Session.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <memory>
namespace http
{
namespace session
{
class SessionManager
{

public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);
    std::shared_ptr<Session> getSession(const HttpRequest& req,HttpResponse* resp);
    //销毁会话
    void destroySession(const std::string& sessionId);
    //清理过期会话
    void cleanExpiredSessions();
    //更新会话
    void updateSession(std::shared_ptr<Session> session){
        storage_->save(session);
    }
private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);
private:
    std::unique_ptr<SessionStorage> storage_;
};

} // namespace session 
} // namespace http

