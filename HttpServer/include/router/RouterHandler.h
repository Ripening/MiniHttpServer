#pragma once
#include "../../include/http/HttpRequest.h"
#include "../../include/http/HttpResponse.h"
namespace http
{
namespace router{

class RouterHandler
{
public:
    virtual ~RouterHandler() = default;
    virtual void handle(const HttpRequest& req,HttpResponse* resp) = 0;

};
}
}