#pragma once

#include "../../../muduo/include/Buffer.h"
#include "HttpRequest.h"

namespace http{

class HttpContext
{
public:
    enum HttpRequestParseState{
        kExpectRequestLine,     //等待解析请求行
        kExpectHeaders,     //等待解析请求头
        kExpectBodys,       //等待解析请求体
        kGotAll     //解析完成
    };
    HttpContext():state_(kExpectRequestLine){}
    ~HttpContext() = default;

    bool parseRequest(Buffer* buf);
    bool gotAll() const {return state_ == kGotAll;}

    void reset(){
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }

    const HttpRequest& request() const{return request_;}
    HttpRequest& request() {return request_;}
private:
    bool processRequestLine(const char* start,const char* end);
private:
    HttpRequestParseState state_;   //状态机
    HttpRequest request_;   //请求类
};

}