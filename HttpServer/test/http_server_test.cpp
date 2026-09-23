#include <iostream>
#include <cstdlib>
#include <memory>
#include "http/HttpServer.h"
#include "session/SessionManager.h"
#include "middleware/AccessLogMiddleware.h"
#include "middleware/AuthMiddleware.h"
#include "middleware/RateLimitMiddleware.h"

using namespace http;

int main(int argc, char** argv)
{
    HttpServer server(8000, "http-demo");

    // argv[1] = reactor 线程数(0=全部在 base loop);必须在 start() 前设置
    int threads = (argc > 1) ? std::atoi(argv[1]) : 0;
    server.setThreadNum(threads);

    // 装配会话管理器（内存存储）
    server.setSessionManager(std::make_unique<session::SessionManager>(
        std::make_unique<session::MemorySessionStorage>()));

    // ===== 中间件（before 正序,after 逆序）=====
    // 访问日志:每个请求收尾打印一行
    server.addMiddleware(std::make_shared<middleware::AccessLogMiddleware>());
    // 鉴权:只保护 /admin 开头的路径,未登录 401
    server.addMiddleware(std::make_shared<middleware::AuthMiddleware>(
        server.getSessionManager(), "/admin"));
    // 限流:argv[2] 传 rps 才开启(压测时别开,会打满 429)
    if (argc > 2) {
        int rps = std::atoi(argv[2]);
        server.addMiddleware(std::make_shared<middleware::RateLimitMiddleware>(rps));
        std::cout << "限流已开启: " << rps << " req/s" << std::endl;
    }

    // 静态路由
    server.Get("/hello", [](const HttpRequest& req, HttpResponse* resp){
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setContentType("text/plain");
        resp->setBody("Hello, HTTP v1!\n");
    });

    // 动态路由：路径参数
    server.addRoute(HttpRequest::kGet, "/users/:id",
        [](const HttpRequest& req, HttpResponse* resp){
            std::string id = req.getPathParameters("id");
            resp->setStateCode(HttpResponse::k200Ok);
            resp->setStateMessage("OK");
            resp->setBody("user id = " + id + "\n");
        });

    // POST
    server.Post("/echo", [](const HttpRequest& req, HttpResponse* resp){
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setBody("posted\n");
    });

    // ===== 会话演示 =====

    // 登录：先比对（demo 用假验证），成功才建会话写状态
    // 用法: /login?user=alice&pass=123
    server.Get("/login", [&server](const HttpRequest& req, HttpResponse* resp){
        std::string user = req.getQueryParameters("user");
        std::string pass = req.getQueryParameters("pass");

        if (user != "alice" || pass != "123") {          // 真实项目：查数据库比对
            resp->setStateCode(HttpResponse::k401Unauthorized);
            resp->setStateMessage("Unauthorized");
            resp->setBody("wrong credentials\n");
            return;
        }
        auto session = server.getSessionManager()->getSession(req, resp);  // 验证通过才建会话
        session->setValue("user", user);
        session->setValue("isLoggedIn", "true");
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setBody("login ok\n");
    });

    // 查身份：凭 cookie 找回会话，看登录标记
    server.Get("/whoami", [&server](const HttpRequest& req, HttpResponse* resp){
        auto session = server.getSessionManager()->getSession(req, resp);
        if (session->getValue("isLoggedIn") != "true") {
            resp->setStateCode(HttpResponse::k401Unauthorized);
            resp->setStateMessage("Unauthorized");
            resp->setBody("not logged in\n");
            return;
        }
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setBody("user = " + session->getValue("user") + "\n");
    });

    // 故意抛异常的路由：验证 500 兜底，且中间件 after 链照样收尾打日志
    server.Get("/boom", [](const HttpRequest&, HttpResponse*){
        throw std::runtime_error("boom");
    });

    // 受保护路由：被 AuthMiddleware 拦住，登录后才放行
    server.Get("/admin/panel", [](const HttpRequest& req, HttpResponse* resp){
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setContentType("text/plain");
        resp->setBody("admin panel\n");
    });

    // 登出：销毁会话（storage 里删掉，浏览器旧 cookie 下次来查无此 id）
    server.Get("/logout", [&server](const HttpRequest& req, HttpResponse* resp){
        auto session = server.getSessionManager()->getSession(req, resp);
        server.getSessionManager()->destroySession(session->getId());
        resp->setStateCode(HttpResponse::k200Ok);
        resp->setStateMessage("OK");
        resp->setBody("logged out\n");
    });

    std::cout << "HTTP server listening on 127.0.0.1:8000 (reactor threads = "
              << threads << ")" << std::endl;
    server.start();
    return 0;
}
