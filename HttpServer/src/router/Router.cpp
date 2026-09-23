#include "../../include/router/Router.h"

namespace http
{
namespace router
{
    void Router::registerHandler(HttpRequest::Method method,const std::string& path,HandlerPtr handler){
        RouteKey key{method,path};
        handlers_[key] = std::move(handler);
    }

    void Router::registerCallBack(HttpRequest::Method method,const std::string& path,HandlerCallBack callback){
        RouteKey key{method,path};
        callbacks_[key] = std::move(callback);
    }

    bool Router::route(const HttpRequest& req,HttpResponse* resp){
        RouteKey key{req.getMethod(),req.getPath()};

        auto handlerIt = handlers_.find(key);
        if(handlerIt!=handlers_.end()){
            handlerIt->second->handle(req,resp);
            return true;
        }

        auto callbackIt = callbacks_.find(key);
        if(callbackIt!=callbacks_.end()){
            callbackIt->second(req,resp);
            return true;
        }

        std::string pathStr(req.getPath());

        for(const auto& [method,pathRegex,handler,names]:regexHandlers_){
            std::smatch match;
            if(method==req.getMethod()&&std::regex_match(pathStr,match,pathRegex)){
                HttpRequest newReq(req);
                this->extractPathParameters(match,newReq,names);
                handler->handle(newReq,resp);
                return true;
            }
        }

        for(const auto& [method,pathRegex,callback,names]:regexCallBacks_){
            std::smatch match;
            if(method==req.getMethod()&&std::regex_match(pathStr,match,pathRegex)){
                HttpRequest newReq(req);
                this->extractPathParameters(match,newReq,names);
                callback(newReq,resp);
                return true;
            }
        }

        return false;
    }
    
} // namespace router

}