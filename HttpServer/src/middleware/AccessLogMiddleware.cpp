#include "middleware/AccessLogMiddleware.h"
#include <iostream>

namespace http
{
namespace middleware
{

static const char* methodName(HttpRequest::Method method){
    switch(method){
        case HttpRequest::kGet:     return "GET";
        case HttpRequest::kPost:    return "POST";
        case HttpRequest::kHead:    return "HEAD";
        case HttpRequest::kPut:     return "PUT";
        case HttpRequest::kDelete:  return "DELETE";
        case HttpRequest::kOptions: return "OPTIONS";
        default:                    return "INVALID";
    }
}

void AccessLogMiddleware::after(HttpRequest& request,HttpResponse& response)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[access] " << methodName(request.getMethod()) << " "
              << request.getPath() << " -> " << response.getStateCode() << std::endl;
}

} // namespace middleware
} // namespace http
