#pragma once
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

namespace http
{
namespace middleware
{

// 中间件基类:before 按注册顺序正序执行,after 逆序执行(洋葱模型)
class Middleware
{
public:
    virtual ~Middleware() = default;

    // 请求进入路由前调用。返回 false = 拦截该请求:后面的中间件和路由都不再执行,
    // 响应要由拦截者自己填好(如 401/429)。
    virtual bool before(HttpRequest& request,HttpResponse& response){
        return true;
    }

    // 响应发出前调用,逆序执行。无论前面是正常路由、404 还是 500 都会走到这里。
    virtual void after(HttpRequest& request,HttpResponse& response){}
};

} // namespace middleware
} // namespace http
