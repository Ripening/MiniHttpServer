#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "middleware/MiddlewareChain.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) do {                                                   \
    ++g_checks;                                                            \
    if(!(cond)){                                                           \
        ++g_failures;                                                      \
        std::cout << "  FAILED " << __FILE__ << ":" << __LINE__            \
                  << "  " << #cond << std::endl;                           \
    }                                                                      \
} while(0)

// 往 log 里记调用顺序的测试中间件;block=true 表示在 before 里拦截
class Recorder : public http::middleware::Middleware
{
public:
    Recorder(std::string name,bool block,std::string* log)
        :name_(std::move(name)),block_(block),log_(log){}

    bool before(http::HttpRequest&,http::HttpResponse&) override{
        *log_ += name_ + ".before;";
        return !block_;
    }
    void after(http::HttpRequest&,http::HttpResponse&) override{
        *log_ += name_ + ".after;";
    }

private:
    std::string name_;
    bool block_;
    std::string* log_;
};

// after 里抛异常,验证不拖累其他层
class ThrowingAfter : public http::middleware::Middleware
{
public:
    void after(http::HttpRequest&,http::HttpResponse&) override{
        throw std::runtime_error("after 里故意抛的");
    }
};

// ============ 用例 1:全部放行,after 逆序 ============
static void testOrder(){
    std::cout << "--- testOrder ---" << std::endl;
    std::string log;
    http::middleware::MiddlewareChain chain;
    chain.addMiddleware(std::make_shared<Recorder>("A",false,&log));
    chain.addMiddleware(std::make_shared<Recorder>("B",false,&log));

    http::HttpRequest req;
    http::HttpResponse resp;
    auto result = chain.processBefore(req,resp);
    CHECK(result.blocked == false);
    CHECK(result.depth == 2);
    chain.processAfter(req,resp,result.depth);

    CHECK(log == "A.before;B.before;B.after;A.after;");
}

// ============ 用例 2:第一层拦截,后面的 before 不执行 ============
static void testBlockAtFirst(){
    std::cout << "--- testBlockAtFirst ---" << std::endl;
    std::string log;
    http::middleware::MiddlewareChain chain;
    chain.addMiddleware(std::make_shared<Recorder>("A",true,&log));
    chain.addMiddleware(std::make_shared<Recorder>("B",false,&log));

    http::HttpRequest req;
    http::HttpResponse resp;
    auto result = chain.processBefore(req,resp);
    CHECK(result.blocked == true);
    CHECK(result.depth == 1);
    chain.processAfter(req,resp,result.depth);

    CHECK(log == "A.before;A.after;");
}

// ============ 用例 3:最后一层拦截(depth 恰好等于 size,别把拦截误判成放行)============
static void testBlockAtLast(){
    std::cout << "--- testBlockAtLast ---" << std::endl;
    std::string log;
    http::middleware::MiddlewareChain chain;
    chain.addMiddleware(std::make_shared<Recorder>("A",false,&log));
    chain.addMiddleware(std::make_shared<Recorder>("B",true,&log));

    http::HttpRequest req;
    http::HttpResponse resp;
    auto result = chain.processBefore(req,resp);
    CHECK(result.blocked == true);
    CHECK(result.depth == chain.size());     // 只看层数会误判,靠 blocked 区分
    chain.processAfter(req,resp,result.depth);

    CHECK(log == "A.before;B.before;B.after;A.after;");
}

// ============ 用例 4:某层 after 抛异常,其余层的 after 照常执行 ============
static void testAfterThrows(){
    std::cout << "--- testAfterThrows ---" << std::endl;
    std::string log;
    http::middleware::MiddlewareChain chain;
    chain.addMiddleware(std::make_shared<Recorder>("A",false,&log));
    chain.addMiddleware(std::make_shared<ThrowingAfter>());
    chain.addMiddleware(std::make_shared<Recorder>("B",false,&log));

    http::HttpRequest req;
    http::HttpResponse resp;
    auto result = chain.processBefore(req,resp);
    chain.processAfter(req,resp,result.depth);   // 不应往外抛

    CHECK(log == "A.before;B.before;B.after;A.after;");
}

int main(){
    testOrder();
    testBlockAtFirst();
    testBlockAtLast();
    testAfterThrows();

    std::cout << "---------------------------------" << std::endl;
    std::cout << "checks: " << g_checks << ", failures: " << g_failures << std::endl;
    return g_failures == 0 ? 0 : 1;
}
