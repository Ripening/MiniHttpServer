#include <iostream>
#include <string>
#include <vector>
#include "http/HttpContext.h"
#include "http/HttpRequest.h"
#include "Buffer.h"

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

static void feed(Buffer* buf, const char* text){
    buf->append(text, strlen(text));
}

// ============ 用例 1:完整 GET,一次给全 ============
static void testCompleteGet(){
    std::cout << "--- testCompleteGet ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;
    feed(&buf, "GET /index.html HTTP/1.1\r\n"
               "Host: www.example.com\r\n"
               "\r\n");
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getMethod() == http::HttpRequest::kGet);
    CHECK(ctx.request().getPath() == "/index.html");
    CHECK(ctx.request().getVersion() == "HTTP/1.1");
    CHECK(ctx.request().getHeaders("Host") == "www.example.com");
    CHECK(buf.readAbleBytes() == 0);      // 全部被消费完
}

// ============ 用例 2:GET 带查询参数 ============
static void testGetWithQuery(){
    std::cout << "--- testGetWithQuery ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;
    feed(&buf, "GET /search?q=cpp&page=2 HTTP/1.1\r\n\r\n");
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getPath() == "/search");
    CHECK(ctx.request().getQueryParameters("q") == "cpp");
    CHECK(ctx.request().getQueryParameters("page") == "2");
}

// ============ 用例 3:POST 带 body,一次给全 ============
static void testPostWithBody(){
    std::cout << "--- testPostWithBody ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;
    feed(&buf, "POST /api/login HTTP/1.1\r\n"
               "Content-Length: 5\r\n"
               "\r\n"
               "hello");
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getMethod() == http::HttpRequest::kPost);
    CHECK(ctx.request().getBody() == "hello");
    CHECK(buf.readAbleBytes() == 0);
}

// ============ 用例 4:POST 半包,分三段喂(状态机断点续传核心场景) ============
static void testPostHalfPacket(){
    std::cout << "--- testPostHalfPacket ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;

    // 第一段:header 都没给完(缺结尾空行)
    feed(&buf, "POST /api/login HTTP/1.1\r\n"
               "Content-Length: 5\r\n");
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);            // 没凑齐不算错误
    CHECK(ctx.gotAll() == false); // 但也还没到终点

    // 第二段:空行 + 2 字节 body(还差 3 字节)
    feed(&buf, "\r\n12");
    ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == false); // body 差 3 字节,继续等

    // 第三段:补齐剩下 3 字节
    feed(&buf, "345");
    ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getBody() == "12345");
    CHECK(buf.readAbleBytes() == 0);
}

// ============ 用例 5:一次两个请求粘包,解析完第一个、剩下的原样保留 ============
static void testStickyPacket(){
    std::cout << "--- testStickyPacket ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;
    feed(&buf, "GET /a HTTP/1.1\r\n\r\n"
               "GET /b HTTP/1.1\r\n\r\n");
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getPath() == "/a");      // 只解析了第一个
    CHECK(buf.retrieveAllAsString() == "GET /b HTTP/1.1\r\n\r\n");  // 第二个分毫不差留着
}

// ============ 用例 6:解析完 reset 后复用同一 ctx(对应 onMessage 循环) ============
static void testResetReuse(){
    std::cout << "--- testResetReuse ---" << std::endl;
    http::HttpContext ctx;
    Buffer buf;
    feed(&buf, "GET /first HTTP/1.1\r\n\r\n"
               "POST /second HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc");
    ctx.parseRequest(&buf);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getPath() == "/first");

    ctx.reset();                  // 清空状态,复用
    bool ok = ctx.parseRequest(&buf);
    CHECK(ok == true);
    CHECK(ctx.gotAll() == true);
    CHECK(ctx.request().getPath() == "/second");
    CHECK(ctx.request().getBody() == "abc");
    CHECK(buf.readAbleBytes() == 0);
}

// ============ 用例 7:畸形报文,表驱动:全部必须返回 false ============
static void testMalformed(){
    std::cout << "--- testMalformed ---" << std::endl;
    struct Case { const char* name; const char* raw; };
    const Case cases[] = {
        {"request line no space",  "GET/index.html HTTP/1.1\r\n\r\n"},
        {"bad version",            "GET /a HTTP/1.2\r\n\r\n"},
        {"header line no colon",   "GET /a HTTP/1.1\r\nHost bad\r\n\r\n"},
        {"CL is not a number",     "POST /a HTTP/1.1\r\nContent-Length: abc\r\n\r\n"},
        {"CL negative",            "POST /a HTTP/1.1\r\nContent-Length: -5\r\n\r\n"},
        {"CL with garbage tail",   "POST /a HTTP/1.1\r\nContent-Length: 5abc\r\n\r\n"},
        {"CL missing on POST",     "POST /a HTTP/1.1\r\nHost: x\r\n\r\n"},
    };
    for(const Case& c : cases){
        http::HttpContext ctx;
        Buffer buf;
        feed(&buf, c.raw);
        bool ok = ctx.parseRequest(&buf);
        if(!ok){
            std::cout << "  PASS  " << c.name << std::endl;
        }else{
            ++g_failures;
            std::cout << "  FAIL  " << c.name << " : parseRequest returned true" << std::endl;
        }
        ++g_checks;
    }
}

int main(){
    testCompleteGet();
    testGetWithQuery();
    testPostWithBody();
    testPostHalfPacket();
    testStickyPacket();
    testResetReuse();
    testMalformed();
    std::cout << "---------------------------------" << std::endl;
    std::cout << "checks: " << g_checks << ", failures: " << g_failures << std::endl;
    return g_failures == 0 ? 0 : 1;
}
