#pragma once
#include "TcpServer.h"
#include "InetAddress.h"
#include <functional>
#include <string>
#include <filesystem>
#include "../router/Router.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "session/SessionManager.h"
#include "middleware/MiddlewareChain.h"
namespace http
{
class HttpRequest;
class HttpResponse;

class HttpServer
{
public:
    using HttpCallBack = std::function<void(const HttpRequest&,HttpResponse*)>;
    HttpServer(int port,
            const std::string& name,
            TcpServer::Option option = TcpServer::Option::kNoReusePort);
    ~HttpServer() = default;
    
    void start();

    void setThreadNum(int numThreads){
        server_.setNumThreads(numThreads);
    }
    EventLoop* getLoop() const{
        return server_.getLoop();
    }
    //设置回调函数
    void setHttpCallBack(const HttpCallBack& callback){
        httpCallback_ = callback;
    }
    //注册get路由
    void Get(const std::string& path,const HttpCallBack& httpCallback){
        router_.registerCallBack(HttpRequest::kGet,path,httpCallback);
    }
    void Get(const std::string& path,router::Router::HandlerPtr handler){
        router_.registerHandler(HttpRequest::kGet,path,handler);
    }
    //注册Post路由
    void Post(const std::string& path,const HttpCallBack& httpCallback){
        router_.registerCallBack(HttpRequest::kPost,path,httpCallback);
    }
    void Post(const std::string& path,router::Router::HandlerPtr handler){
        router_.registerHandler(HttpRequest::kPost,path,handler);
    }

    //注册动态路由处理函数
    void addRoute(HttpRequest::Method method,const std::string& path,const router::Router::HandlerCallBack& callback){
        router_.addRegexCallBack(method,path,callback);
    }
    void addRoute(HttpRequest::Method method,const std::string& path,router::Router::HandlerPtr handler){
        router_.addRegexHandler(method,path,handler);
    }

    //注册中间件(启动前调用)。执行顺序 = 注册顺序的 before,after 逆序
    void addMiddleware(std::shared_ptr<middleware::Middleware> middleware){
        middlewareChain_.addMiddleware(std::move(middleware));
    }

    //设置会话管理器
    void setSessionManager(std::unique_ptr<session::SessionManager> sessionManager){
        sessionManager_ = std::move(sessionManager);
    }
    //返回会话管理器
    session::SessionManager* getSessionManager() const{
        return sessionManager_.get();
    }
private:
    void initialize();
    
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buff);
    void onRequest(const TcpConnectionPtr& conn,const HttpRequest& req);
    void handleRequest(const HttpRequest& req,HttpResponse* resp);

    bool safeStaticPath(const std::string& path,std::string& safePath) const;

private:
    InetAddress     listenAddr_;    //监听连接请求
    EventLoop       mainLoop_;      //主循环
    TcpServer       server_;        //服务器

    HttpCallBack    httpCallback_;  //http回调函数
    router::Router  router_ ;       //路由
    middleware::MiddlewareChain middlewareChain_;  //中间件链
    std::unique_ptr<session::SessionManager> sessionManager_;
    std::filesystem::path docRoot_; //静态文件根目录,启动时解析一次(见 .cpp initialize)
};

} // namespace http
