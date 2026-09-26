# myMuduo — 自研 Reactor 网络库与 HTTP 服务器

手写实现的多线程 Reactor 网络库（对照 muduo 架构参考重写），并在其上完成 HTTP/1.1 服务器：
状态机报文解析、正则路由、Cookie 会话、sendFile 零拷贝静态文件。C++17、epoll、LT 模式。

- 多线程 Reactor（one loop per thread 与线程池），连接跨线程投递、生命周期安全管理
- HTTP 报文状态机解析（半包/粘包）、Keep-Alive、MIME 映射、`/` → `index.html`
- 正则动态路由（`/users/:id` 路径参数）、Cookie 会话（登录/查询/登出演示）
- 中间件链：洋葱模型（before 正序、after 逆序，返回值短路），附访问日志、鉴权、限流三个实现
- 压测驱动打磨：定位并修复 6 项真实问题，含 syscall 级定量验证（见下文）

## 架构

```
应用      http_server_test（路由注册、会话演示、静态资源回退）
──────────────────────────────────────────────────────────────
HTTP 层   HttpServer ─┬─ Router        静态路由、正则动态路由、路径参数
                      ├─ Middleware    中间件链（访问日志、鉴权、限流）
                      ├─ HttpContext   状态机解析（半包/粘包）
                      ├─ HttpResponse  状态码、Keep-Alive、sendFile
                      └─ Session       SessionManager、MemoryStorage、Cookie
──────────────────────────────────────────────────────────────
网络层    TcpServer、Acceptor、TcpConnection、Buffer、UniqueFd
          EventLoopThreadPool（主从 Reactor，round-robin 分配连接）
          EventLoop（Reactor）─ Channel（事件处理器）
                               ├ Poller、EPollPoller（事件多路分解器，LT）
                               └ TimerQueue（timerfd 定时器，框架能力，HTTP 层暂未使用）
──────────────────────────────────────────────────────────────
内核      epoll、socket API（非阻塞 IO）
```

## 项目结构

```
.
├── muduo/                     # 网络库
│   ├── include/  src/         # EventLoop/Channel/Poller/Acceptor/TcpConnection 等
│   └── test/kill_client.c     # RST 攻击客户端（回归工具，见“完善期修复”#3）
├── HttpServer/                # HTTP 协议层
│   ├── include/{http,router,middleware,session}/
│   ├── src/
│   └── test/
│       ├── http_parse_test.cpp        # 解析层单测
│       ├── http_middleware_test.cpp   # 中间件链单测（顺序、短路、异常）
│       └── http_server_test.cpp       # 端到端 demo
├── www/                       # 静态资源演示页
└── CMakeLists.txt
```

## 构建与运行

依赖：Linux、CMake ≥ 3.16、C++17（开发环境：Docker Desktop ARM64 上的 ubuntu:22.04 容器）

```bash
cmake -B build && cmake --build build -j
./build/http_server_test 3 500   # 参数 = reactor 线程数（默认 0），每秒限流（省略 = 不限流）
```

> 静态文件根按 `./www` 解析，请在含 `www/` 的目录下启动（仓库根目录即可）；端口固定 8000。

- `./build/http_parse_test` — 解析层单测（状态机、半包粘包、请求行）
- `./build/http_middleware_test` — 中间件链单测（执行顺序、短路、after 异常隔离）
- `./build/echo_server` — 基础 echo 服务器

## 演示路由

| 路由 | 行为 |
|---|---|
| `GET /hello` | 动态响应 |
| `GET /users/:id` | 正则动态路由，取路径参数 |
| `POST /echo` | POST 演示 |
| `GET /login?user=alice&pass=123` | 验证通过建会话，Set-Cookie |
| `GET /whoami` | 凭 cookie 找回会话 |
| `GET /logout` | 销毁会话 |
| `GET /admin/panel` | 受鉴权中间件保护，未登录 401 |
| `GET /boom` | 故意抛异常，验证 500 兜底与 after 链收尾 |
| 其它路径 | 回退静态文件（`/` → index.html；含路径穿越与符号链接逃逸防护） |

```bash
curl -c jar -b jar 'http://127.0.0.1:8000/login?user=alice&pass=123'
curl -b jar http://127.0.0.1:8000/whoami
curl -i http://127.0.0.1:8000/admin/panel           # 401
curl -i -b jar http://127.0.0.1:8000/admin/panel    # 200
```

## 中间件

洋葱模型：`before` 按注册顺序正序执行，`after` 逆序收尾；`before` 返回 `false` 即短路——
后面的中间件和路由都不执行，响应由拦截者填好（如鉴权 401、限流 429）。

- 骨架 `Middleware` 与 `MiddlewareChain`（`include/middleware/`）：链上不存每请求状态
  （执行层数由调用方保管再传回收尾），多 reactor 下可安全共享
- 三个实现：`AccessLogMiddleware`（每请求一行访问日志）、`AuthMiddleware`（接 SessionManager，
  只拦指定路径前缀）、`RateLimitMiddleware`（固定窗口，超限 429）
- 相对参考实现 Kama-HTTPServer 的两处改进：
  - 短路用返回值表达，不用 `throw HttpResponse` 拿异常做控制流
  - `after` 链在异常路径同样执行——Kama 把收尾放在 `try` 内，路由一抛异常整条 `after`
    链被跳过（访问日志会漏掉全部 500）；本实现把收尾提到 `catch` 之后，并让每层 `after`
    各自隔离异常

## 压测数据（wrk 4.1.0）

环境：Docker Desktop ARM64、ubuntu:22.04、3 vCPU、loopback、keep-alive（除非注明）。
注意：回环网络与共享 VM 会放大误差，数字只反映量级与倍数关系；同机 wrk 与内核协议栈会和服务器抢 CPU。
数据在中间件接入前测得；复现时注释掉 `http_server_test.cpp` 里注册 `AccessLogMiddleware`
的一行——逐请求打日志会明显拉低 QPS。

| 场景 | QPS | 说明 |
|---|---|---|
| /index.html 1.1 KB（c=15） | 109,761 | P50 126.9 µs |
| /index.html 1.1 KB（c=100） | 117,785 | P50 0.85 ms |
| /hello（c=100） | 406,494 | 动态响应 |
| /users/123（c=100） | 386,956 | 正则动态路由 |
| /big.html 135 KB | 86,273 | sendfile 零拷贝，约 10.9 GB/s |
| Connection: close | 111,810 | 每请求新建连接 |
| 长跑 15 s（c=100） | 116,504 | 174.9 万请求、2.0 GB，零错误 |

多 Reactor 扩展性（`./http_server_test 1` 与 `./http_server_test 3`）：

| 场景 | 1 线程 | 3 线程 | 扩展比 |
|---|---|---|---|
| 静态 index.html（c=100） | 102,036 | 182,687 | 1.79 倍 |
| 动态 /hello（c=100） | 约 38 万 | 约 45 万～49 万 | 1.2～1.3 倍 |

> 3 vCPU 被 wrk、服务器、内核回环栈三方分走（ksoftirqd 软中断占 60%～70%），
> 测到的是整机回环吞吐上限（/hello 约 45 万～50 万 QPS），因此扩展比是下界；
> 真实部署（客户端-服务端分离）扩展性会更好。

## 完善期修复（均已验证）

| # | 问题 | 根因 | 修复与验证 |
|---|---|---|---|
| 1 | 小静态文件恒定 41 ms，QPS 仅 337 | Nagle 与延迟 ACK 互锁（响应头与正文分两次写） | 连接级 TCP_NODELAY → **10 万 QPS，延迟 119 µs（约 297 倍）** |
| 2 | 每响应泄漏 1 个 fd，最终打瘫容器 | sendFile 各出口手工 close 不全（所有权未定义） | UniqueFd RAII，所有权写进函数签名 → **179 万请求后 fd 恒 6** |
| 3 | 压测中服务无声消失（无日志无 core） | 向 RST 连接 write 或 sendfile → SIGPIPE 默认杀进程 | 启动全局忽略 SIGPIPE（exit=141 取证复现）→ **RST 攻击 4 次、长跑全程存活** |
| 4 | 三进程共绑 8000，会话状态“灵异丢失” | Acceptor 无视 Option 硬编码 REUSEPORT | 参数化，并让 bind/listen 失败致命化（不再隐式绑随机端口）→ **第二实例 exit=1 拒绝共绑** |
| 5 | 每请求重复解析 doc root（热路径上重算不变量） | canonical 每请求约 11 个 syscall | 启动时缓存 docRoot_ → **32.5 → 27.6 次 syscall/请求**，穿越与 symlink 防护语义不变 |
| 6 | 会话在多 reactor 下有数据竞争 | 连接与 reactor 绑定但会话不绑定：storage 共享 map 无锁、`Session::data_`、过期时间、随机数引擎未保护 | 双层锁（storage 锁 → Session 锁，单向锁序），引擎改 `thread_local` → **TSan 下 3 reactor × 30 并发登录与登出，0 告警** |

其中 #5 的 syscall 级定量（strace，200 固定静态请求）：

| syscall | 改前 | 改后 |
|---|---|---|
| getcwd | 1 次/请求 | 0（仅启动 1 次） |
| newfstatat | 约 3 次/请求 | 约 2 次/请求 |
| readlinkat | 约 7 次/请求 | 约 4 次/请求 |
| **合计** | **32.5 次/请求** | **27.6 次/请求（降低 15%）** |

## 已知限制

- 压测在回环网络上同机进行（MSS 64 KB、无真实网络 RTT），绝对 QPS 不代表公网表现
- 暂无 TLS

## 参考

- 陈硕《Linux 多线程服务端编程》及 muduo 源码 —— 架构参考（参考重写，非魔改）
- Kama-HTTPServer —— HTTP 层模块划分参考（报文解析、路由、中间件、会话）
