#pragma once

#include "../../../muduo/include/Buffer.h"
#include <map>
#include <string>
#include <cstring>
#include <filesystem>
namespace http{
class HttpResponse
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k429TooManyRequests = 429,
        k500InternalServerError = 500,
    }; 
    HttpResponse(bool close = true):stateCode_(kUnknown),
                                    closeConnection_(close),
                                    httpVersion_("HTTP/1.1"){}
    ~HttpResponse() = default;
    
    void setHttpVersion(std::string v){
        httpVersion_ = std::move(v);
    }
    void setStateCode(HttpStatusCode code){
        stateCode_ = code;
    }
    HttpStatusCode getStateCode() const{
        return stateCode_;
    }
    void setStateMessage(std::string message){
        stateMessage_ = std::move(message);
    }
    void setCloseConnection(bool on){
        closeConnection_ = on;
    }
    bool closeConnection()const{
        return closeConnection_;
    }
    void setBody(std::string body){
        body_ = std::move(body);
    }
    void addHeaders(const std::string& key,const std::string& value){
        headers_[key] = value;
    }
    void setContentType(const std::string& contentType){
        addHeaders("Content-Type",contentType);
    }
    void setContentLength(uint64_t contentLength){
        addHeaders("Content-Length",std::to_string(contentLength));
    }

    void setFileInfo(int fd,uint64_t fileSize){
        fd_ = fd;
        fileSize_ = fileSize;
        isFile_ = true;
        
    }
    bool isFile() const{
        return isFile_;
    }
    int getFileFd() const{
        return fd_;
    }
    uint64_t getFileSize() const{
        return fileSize_;
    }

    void appendToBuffer(Buffer* outBuf) const;

private:
    std::string httpVersion_;                   //http版本号
    HttpStatusCode stateCode_;                  //响应码
    std::string stateMessage_;                  //响应码对应的文字
    bool closeConnection_;                      //是否关闭连接
    std::map<std::string,std::string> headers_; //请求头
    std::string body_;                          //响应正文
    bool isFile_{false};                        //是否为文件
    int fd_{-1};                                //文件描述符(-1=无)
    uint64_t fileSize_{0};                      //文件大小
};

}