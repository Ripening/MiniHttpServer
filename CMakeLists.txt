cmake_minimum_required(VERSION 3.16)
project(myMuduoHttp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(muduo/include HttpServer/include)

# 框架库（不含 main.cpp）
set(MUDUO_SRC
    muduo/src/Acceptor.cpp
    muduo/src/Buffer.cpp
    muduo/src/Channel.cpp
    muduo/src/CurrentThread.cpp
    muduo/src/DefaultPoller.cpp
    muduo/src/EPollPoller.cpp
    muduo/src/EventLoop.cpp
    muduo/src/EventLoopThread.cpp
    muduo/src/EventLoopThreadPool.cpp
    muduo/src/InetAddress.cpp
    muduo/src/Poller.cpp
    muduo/src/Socket.cpp
    muduo/src/TcpConnection.cpp
    muduo/src/TcpServer.cpp
    muduo/src/Thread.cpp
    muduo/src/TimerQueue.cpp
)
add_library(myMuduoLib ${MUDUO_SRC})

# HTTP 层
add_library(httpServerLib
    HttpServer/src/http/HttpContext.cpp
    HttpServer/src/http/HttpRequest.cpp
    HttpServer/src/http/HttpResponse.cpp
    HttpServer/src/http/HttpServer.cpp
    HttpServer/src/router/Router.cpp
    HttpServer/src/middleware/MiddlewareChain.cpp
    HttpServer/src/middleware/AccessLogMiddleware.cpp
    HttpServer/src/middleware/AuthMiddleware.cpp
    HttpServer/src/middleware/RateLimitMiddleware.cpp
    HttpServer/src/session/Session.cpp
    HttpServer/src/session/SessionManager.cpp
    HttpServer/src/session/SessionStorage.cpp
)
target_link_libraries(httpServerLib myMuduoLib)

# 解析层测试（Mac/容器都能跑）
add_executable(http_parse_test HttpServer/test/http_parse_test.cpp)
target_link_libraries(http_parse_test httpServerLib pthread)

# 中间件链测试（Mac/容器都能跑）
add_executable(http_middleware_test HttpServer/test/http_middleware_test.cpp)
target_link_libraries(http_middleware_test httpServerLib pthread)

# HTTP 端到端 demo（容器里跑，curl 验证）
add_executable(http_server_test HttpServer/test/http_server_test.cpp)
target_link_libraries(http_server_test httpServerLib pthread)

# echo 服务器（原有，保留）
add_executable(echo_server muduo/src/main.cpp)
target_link_libraries(echo_server myMuduoLib pthread)
