#pragma once

#include <string>
#include <unordered_map>
#include <map>
namespace http{

class HttpRequest
{
public:
    //请求方法
    enum Method{
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions
    };
    HttpRequest():method_(kInvalid)
                , version_("Unknown")
    {}
    ~HttpRequest() = default;
    //存取请求方法
    bool setMethod(const char* start,const char* end);
    Method getMethod()const {return method_;}
    //http版本
    void setVersion(std::string v){version_ = std::move(v);}
    std::string getVersion()const{return version_;}
    //请求路径
    void setPath(const char* start,const char* end){path_.assign(start,end);}
    std::string getPath()const{return path_;}
    //请求路径参数
    void setPathParameters(const std::string& key,const std::string& value){
        pathParameters_[key] = value;
    }
    std::string getPathParameters(const std::string& key) const;
    //查询路径参数
    void setQueryParameters(const char* start, const char* end);
    std::string getQueryParameters(const std::string& key) const;
    //请求头
    void setHeaders(const char* start,const char* colon,const char* end);
    std::string getHeaders(const std::string& field) const;

    const std::map<std::string,std::string>& headers() const{
        return headers_;
    }
    //请求体内容
    void setBody(std::string body){
        content_ = std::move(body);
    }
    void setBody(const char* start,const char* end){
        if(end>=start){
            content_.assign(start,end-start);
        }
    }
    std::string getBody() const{
        return content_;
    }
    //设置请求体长度
    void setContentLength(uint64_t length){
        contentLength_ = length;
    }
    uint64_t getContentLength() const{
        return contentLength_;
    }

    void swap(HttpRequest& that);

private:
    Method                                      method_; //方法名
    std::string                                 version_; //版本号
    std::string                                 path_; //查询路径
    std::unordered_map<std::string,std::string> pathParameters_; //路径参数
    std::unordered_map<std::string,std::string> queryParameters_; //查询参数
    std::map<std::string,std::string>           headers_; //请求头
    std::string                                 content_; //请求体
    uint64_t                                    contentLength_{0}; //请求体长度

};

}