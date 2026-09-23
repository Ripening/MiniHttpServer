#include "../../include/http/HttpContext.h"

namespace http{
    bool HttpContext::parseRequest(Buffer* buf){
        bool ok = true;
        bool hasMore = true;
        while(hasMore){
            if(state_ == kExpectRequestLine){
                const char* crlf = buf->findCRLF();
                if(crlf){
                    ok = this->processRequestLine(buf->peek(),crlf);
                    if(ok){
                        buf->retrieveUntil(crlf+2);
                        state_ = kExpectHeaders;
                    }else{
                        hasMore = false;
                    }
                }
                else{
                    hasMore = false;
                }
            }
            else if(state_ == kExpectHeaders){
                const char* crlf = buf->findCRLF();
                if(crlf){
                    const char* colon = std::find(buf->peek(),crlf,':');
                    //正常情况
                    if(colon<crlf){
                        request_.setHeaders(buf->peek(),colon,crlf);
                    }
                    //请求行的最后一行空行
                    else if(buf->peek()==crlf){
                        //根据请求方法判断是否有请求体
                        if(request_.getMethod()==HttpRequest::kPost||
                           request_.getMethod()==HttpRequest::kPut){
                            std::string length = request_.getHeaders("Content-Length");
                            if(!length.empty()){
                                size_t used = 0;
                                bool valid = false;
                                uint64_t contentLength = 0;
                                try{
                                    long long cl = std::stoll(length, &used);
                                    valid = (cl >= 0) && (used == length.size());
                                    if(valid) contentLength = static_cast<uint64_t>(cl);
                                }catch(const std::exception&){
                                    valid = false;
                                }
                                if(valid){
                                    request_.setContentLength(contentLength);
                                    if(contentLength > 0) state_ = kExpectBodys;
                                    else{                       // valid 保证非负,这里只剩 ==0 一种可能
                                        state_ = kGotAll;
                                        hasMore = false;
                                    }
                                }else{
                                    ok = false;
                                    hasMore = false;
                                }
                            }
                            else{
                                //post和put 请求方法没有这个字段本身就是错的
                                ok = false;
                                hasMore = false;
                            }
                        }
                        else{
                            //其他的请求方法没有请求体内容，解析直接结束
                            state_ = kGotAll;
                            hasMore = false;
                        }
                    }
                    //请求行格式错误
                    else{
                        ok = false;
                        hasMore = false;
                    }
                    buf->retrieveUntil(crlf+2);
                }else{
                    hasMore = false;
                }
            }
            else if(state_ == kExpectBodys){
                //检查缓冲区中是否有足够的数据
                if(buf->readAbleBytes()<request_.getContentLength()){
                    //数据不完整
                    return true;
                }
                std::string body(buf->peek(),buf->peek()+request_.getContentLength());
                request_.setBody(body);
                //移动读指针
                buf->retrieve(request_.getContentLength());
                hasMore = false;
                state_ = kGotAll;
            }
            else{
                ok = false;
                hasMore = false;
            }
        }
        return ok;
    }

    bool HttpContext::processRequestLine(const char* start,const char* end){
        bool succeed = false;
        const char* begin = start;
        const char* space = std::find(begin,end,' ');
        if((space!=end)&&request_.setMethod(begin,space)){
            begin = space+1;
            space = std::find(begin,end,' ');
            if(space!=end){
                const char* argumentStart = std::find(begin,space,'?');
                if(argumentStart!=space){
                    //有查询参数
                    request_.setPath(begin,argumentStart); //查询路径
                    request_.setQueryParameters(argumentStart+1,space);
                }
                else{
                    //没有查询参数
                    request_.setPath(begin,argumentStart);
                }
                begin = space+1;
                succeed = ((end-begin==8)&&std::equal(begin,end-1,"HTTP/1."));
                if(succeed){
                    if(*(end-1)=='1') request_.setVersion("HTTP/1.1");
                    else if(*(end-1)=='0') request_.setVersion("HTTP/1.0");
                    else succeed = false;
                }
            }
        }
        return succeed;
    }

}