#include "../../include/http/HttpRequest.h"
#include <cassert>
namespace http{
    bool HttpRequest::setMethod(const char* start,const char* end){
        assert(method_ == kInvalid);
        std::string m(start,end);
        if(m == "GET")
            method_ = kGet;
        else if(m == "POST")
            method_ = kPost;
        else if(m == "PUT")
            method_ = kPut;
        else if(m == "DELETE")
            method_ = kDelete;
        else if(m == "OPTIONS")
            method_ = kOptions;
        else if(m == "HEAD")
            method_ = kHead;
        else 
            method_ = kInvalid;

        return method_ != kInvalid;
    }

    std::string HttpRequest::getPathParameters(const std::string& key) const{
        auto it = pathParameters_.find(key);
        if(it!=pathParameters_.end()){
            return it->second;
        }
        return "";
    }

    void HttpRequest::setQueryParameters(const char* start, const char* end){
        std::string argumentStr(start,end);
        std::string::size_type pos = 0;
        std::string::size_type prev = 0;
        while((pos = argumentStr.find('&',prev)) != std::string::npos)
        {
            std::string pair = argumentStr.substr(prev,pos-prev);
            std::string::size_type equalPos = pair.find('=');
            if(equalPos!=std::string::npos){
                std::string key = pair.substr(0,equalPos);
                std::string value = pair.substr(equalPos+1);
                queryParameters_[key] = value;
            }
            prev = pos+1;
        }
        //最后一个参数
        std::string lastPair = argumentStr.substr(prev);
        std::string::size_type equalPos = lastPair.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = lastPair.substr(0, equalPos);
            std::string value = lastPair.substr(equalPos + 1);
            queryParameters_[key] = value;
        }
    }

    std::string HttpRequest::getQueryParameters(const std::string& key) const{
        auto it = queryParameters_.find(key);
        if(it!=queryParameters_.end()){
            return it->second;
        }
        return "";
    }

    void HttpRequest::setHeaders(const char* start,const char* colon,const char* end){
        std::string key(start,colon);
        colon++;
        while(colon<end&&isspace(static_cast<unsigned char>(*colon))){
            colon++;
        }
        std::string value(colon,end);
        while(!value.empty()&&isspace(static_cast<unsigned char>(value.back()))){
            value.resize(value.size()-1);
        }
        headers_[key] = value;
    }

    std::string HttpRequest::getHeaders(const std::string& field) const{
        auto it = headers_.find(field);
        if(it!=headers_.end()){
            return it->second;
        }
        return "";
    }

    void HttpRequest::swap(HttpRequest& that){
        std::swap(method_,that.method_);
        std::swap(version_,that.version_);
        std::swap(path_,that.path_);
        std::swap(pathParameters_,that.pathParameters_);
        std::swap(queryParameters_,that.queryParameters_);
        std::swap(headers_,that.headers_);
        std::swap(content_,that.content_);
        std::swap(contentLength_,that.contentLength_);

    }
}