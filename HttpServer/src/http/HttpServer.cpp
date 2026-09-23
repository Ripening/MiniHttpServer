#include "../include/http/HttpServer.h"
#include "../include/http/HttpRequest.h"
#include "../include/http/HttpResponse.h"
#include "../include/http/HttpContext.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unordered_map>
#include <filesystem>
#include <cstddef>

namespace fs = std::filesystem;

namespace http
{
    static const char* mimeTypeOf(const std::string& path);   // 前置声明:定义在本文件尾部
    static void setNotFound(HttpResponse* resp){
      resp->setStateCode(HttpResponse::k404NotFound);
      resp->setStateMessage("Not Found");
      resp->setCloseConnection(true);
    }
    HttpServer::HttpServer(int port,
            const std::string& name,
            TcpServer::Option option):
        listenAddr_(port),
        server_(&mainLoop_,name,listenAddr_,option),
        httpCallback_([this](const HttpRequest& req,HttpResponse* resp){handleRequest(req,resp);}){
            initialize();
    }

    void HttpServer::initialize(){
        // doc root 只解析一次并缓存;进程运行期不得 chdir,否则缓存失效
        std::error_code ec;
        docRoot_ = fs::weakly_canonical("./www", ec);
        if(ec){
            std::cerr << "[HttpServer] doc root 解析失败,静态文件将全部 404" << std::endl;
            docRoot_.clear();
        }
        server_.setConnectCallBack([this](const TcpConnectionPtr& conn){
            onConnection(conn);
        });
        server_.setMessageCallBack([this](const TcpConnectionPtr& conn,Buffer* buf){
            onMessage(conn,buf);
        });
    }

    void HttpServer::start(){
        server_.start();
        mainLoop_.loop();
    }

    void HttpServer::onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            
            conn->setTcpNoDelay(true);
            conn->setContext(HttpContext());
        }
    }

    void HttpServer::onMessage(const TcpConnectionPtr& conn,Buffer* buff){
        try
        {
            //
            HttpContext* context = std::any_cast<HttpContext>(conn->getMutableContext());
            if(!context->parseRequest(buff)){
                // 如果解析http报文过程中出错
                conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->shutdown();
                return;
            }
            if(context->gotAll()){
                onRequest(conn,context->request());
                context->reset();
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }
        
    }

    void HttpServer::onRequest(const TcpConnectionPtr& conn,const HttpRequest& req){
        const std::string& connection = req.getHeaders("Connection");
        bool close = ((connection=="close")||
                    (req.getVersion()=="HTTP/1.0"&&connection != "Keep-Alive"));
        HttpResponse resp(close);
        httpCallback_(req, &resp);

        Buffer buf;
        resp.appendToBuffer(&buf);

        if(resp.isFile()){
            //文件
            conn->send(&buf);
            conn->sendFile(resp.getFileFd(),0,resp.getFileSize());
        }
        else{
            //不是文件
            conn->send(&buf);
        }
        if(resp.closeConnection()){
            conn->shutdown();
        }
    }

    void HttpServer::handleRequest(const HttpRequest& req,HttpResponse* resp){
        HttpRequest mutableReq = req;
        middleware::MiddlewareChain::BeforeResult result;
        try
        {
            // 中间件 before 正序执行;depth = 执行到的层数,交给 after 逆序收尾
            result = middlewareChain_.processBefore(mutableReq,*resp);
            // 路由处理(被中间件拦截时响应已填好,直接跳过)
            if (!result.blocked && !router_.route(mutableReq, resp))
            {
                std::string urlPath = req.getPath();
                if(!urlPath.empty() && urlPath.back()=='/') urlPath += "index.html";
                std::string safePath;
                if(safeStaticPath(urlPath,safePath)){
                    //判断是否是文件
                    int fd = ::open(safePath.c_str(),O_RDONLY);
                    if(fd>=0){
                        //文件存在
                        struct stat st;
                        if(::fstat(fd,&st)==0&&S_ISREG(st.st_mode)){
                            resp->setStateCode(HttpResponse::k200Ok);
                            resp->setStateMessage("OK");
                            resp->setContentType(mimeTypeOf(safePath));
                            resp->setContentLength(st.st_size);
                            resp->setFileInfo(fd,st.st_size);
                        }
                        else{
                            ::close(fd);   // 目录/设备文件:关掉,防泄漏
                            setNotFound(resp);
                        }
                    }
                    else{
                        //文件不存在
                        setNotFound(resp);
                    }
                }
                else{
                    setNotFound(resp);
                }
            }
        }
        catch (const std::exception& e)
        {
            // 错误处理
            resp->setStateCode(HttpResponse::k500InternalServerError);
            resp->setStateMessage("Internal Server Error");
            resp->setBody(e.what());
        }
        // 收尾在所有路径都执行:正常路由/中间件拦截/404/静态文件/500
        middlewareChain_.processAfter(mutableReq,*resp,result.depth);
    }

    static const char* mimeTypeOf(const std::string& path){
        static const std::unordered_map<std::string,const char*> mimeTable{
            {".html", "text/html; charset=utf-8"},
            {".css",  "text/css"},
            {".js",   "application/javascript"},
            {".png",  "image/png"},
            {".jpg",  "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif",  "image/gif"},
            {".ico",  "image/x-icon"},
            {".txt",  "text/plain; charset=utf-8"},
            {".json", "application/json"},
            {".svg",  "image/svg+xml"},
        };

        auto dot = path.find_last_of('.');
        if(dot==std::string::npos)
            return "application/octet-stream";
        
        auto it = mimeTable.find(path.substr(dot));

        return it!=mimeTable.end()? it->second : "application/octet-stream";
    }

    bool HttpServer::safeStaticPath(const std::string& path,std::string& safePath) const {
        if(docRoot_.empty()) return false;

        std::error_code ec;
        std::string rel = path;
        if(!rel.empty()&&rel[0]=='/') rel.erase(0,1);

        fs::path candidate = docRoot_ / fs::path(rel);
        fs::path resolved = fs::weakly_canonical(candidate,ec);
        if(ec) return false;

        auto it = resolved.begin();
        for(auto resIt = docRoot_.begin();resIt!=docRoot_.end();it++,resIt++){
            if(it==resolved.end()||*it!=*resIt) return false;
        }
        safePath = resolved.string();

        return true;
    }
} // namespace http
