#include "../../include/http/HttpResponse.h"

namespace http{
    void HttpResponse::appendToBuffer(Buffer* outputBuf) const{
        char buf[32] = "";
        snprintf(buf,sizeof(buf),"%s %d ",httpVersion_.c_str(),stateCode_);
        outputBuf->append(buf,strlen(buf));
        outputBuf->append(stateMessage_.c_str(),stateMessage_.length());
        outputBuf->append("\r\n",2);
        if(closeConnection_){
            std::string s("Connection: close\r\n");
            outputBuf->append(s.c_str(),s.length());
        }
        else{
            std::string s("Connection: Keep-Alive\r\n");
            outputBuf->append(s.c_str(),s.length());
        }
        for(const auto& header:headers_){
            outputBuf->append(header.first.c_str(),header.first.length());
            outputBuf->append(": ",2); 
            outputBuf->append(header.second.c_str(),header.second.length());
            outputBuf->append("\r\n",2);
        }
        if(!body_.empty() && headers_.count("Content-Length") == 0){
            outputBuf->append("Content-Length: ");
            std::string len = std::to_string(body_.length());
            outputBuf->append(len.c_str(), len.length());
            outputBuf->append("\r\n", 2);
        }
        outputBuf->append("\r\n",2);
        outputBuf->append(body_.c_str(),body_.length());
    }
}