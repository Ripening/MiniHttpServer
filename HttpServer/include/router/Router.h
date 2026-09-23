#pragma once
#include <memory>
#include "RouterHandler.h"
#include <functional>
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <string>
#include <regex>
#include <unordered_map>
#include <vector>
namespace http
{
namespace router{

class Router
{
public:
    using HandlerPtr = std::shared_ptr<RouterHandler>;
    using HandlerCallBack = std::function<void(const HttpRequest&,HttpResponse*)>;
    //路由键
    struct RouteKey{
        HttpRequest::Method method;
        std::string path;

        bool operator==(const RouteKey& other) const{
            return method==other.method&&path==other.path;
        }
    };
    //路由哈希计算
    struct RouteKeyHash{
        
        size_t operator()(const RouteKey& that) const{
            size_t methodHash = std::hash<int>{}(static_cast<int>(that.method));
            size_t pathHash = std::hash<std::string>{}(that.path);
            return methodHash*31 + pathHash;
        }
    };

    Router() = default;
    ~Router() = default;

    void registerHandler(HttpRequest::Method method,const std::string& path,HandlerPtr handler);
    void registerCallBack(HttpRequest::Method method,const std::string& path,HandlerCallBack callback);

    void addRegexHandler(HttpRequest::Method method,const std::string& path,HandlerPtr handler){
        std::regex pathRegex = convertToRegex(path);
        std::vector<std::string> names = std::move(extractParamNames(path));
        regexHandlers_.emplace_back(method,pathRegex,handler,names);
    }
    void addRegexCallBack(HttpRequest::Method method,const std::string& path,HandlerCallBack callback){
        std::regex pathRegex = convertToRegex(path);
        std::vector<std::string> names = std::move(extractParamNames(path));
        regexCallBacks_.emplace_back(method,pathRegex,callback,names);
    }

    bool route(const HttpRequest& req,HttpResponse* resp);
private:
    struct RouteHandlerObj{
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerPtr handler_;
        std::vector<std::string> paramNames_;

        RouteHandlerObj( HttpRequest::Method method,std::regex pathRegex,HandlerPtr handler,std::vector<std::string> paramNames):
        method_(method),pathRegex_(pathRegex),handler_(handler),paramNames_(paramNames){}
    };
    struct RouterCallBackObj{
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerCallBack callback_;
        std::vector<std::string> paramNames_;

        RouterCallBackObj(HttpRequest::Method method,std::regex pathRegex,const HandlerCallBack& callback,std::vector<std::string> paramNames):
        method_(method),pathRegex_(pathRegex),callback_(callback),paramNames_(paramNames){}
    };
    //把路径转变为正则表达式
    std::regex convertToRegex(const std::string& path){
        std::string regexPettern = "^"+std::regex_replace(path,std::regex(R"(/:([^/]+))"),R"(/([^/]+))")+"$";
        return std::regex(regexPettern);
    }
    //提取出路径参数
    void extractPathParameters(const std::smatch &match, HttpRequest &request,const std::vector<std::string>& names)
    {
        for (size_t i = 1; i < match.size(); ++i)
        {
            request.setPathParameters(names[i-1], match[i].str());
        }
    }
    //在注册时提取出参数名称
    static std::vector<std::string> extractParamNames(const std::string& path){
        std::vector<std::string> names;
        std::regex pathRegex(R"(/:([^/]+))");
        std::sregex_iterator it(path.begin(),path.end(),pathRegex);
        std::sregex_iterator end;
        for(;it!=end;it++){
            names.push_back((*it)[1].str());
        }
        return names;
    }

    std::unordered_map<RouteKey,HandlerCallBack,RouteKeyHash> callbacks_;
    std::unordered_map<RouteKey,HandlerPtr,RouteKeyHash> handlers_;
    std::vector<RouteHandlerObj> regexHandlers_;
    std::vector<RouterCallBackObj> regexCallBacks_;
};

}
}