#pragma once
#include <memory>
#include "session/Session.h"
#include <mutex>
#include <string>
#include <unordered_map>
namespace http
{
namespace session
{

class SessionStorage
{
public:
    virtual ~SessionStorage() = default;
    // 实现需保证线程安全:会话和 reactor 不绑定,这些方法会被多个 reactor 线程并发调用
    virtual void save(std::shared_ptr<Session> session) = 0;
    virtual std::shared_ptr<Session> load(const std::string& sessionId) = 0;
    virtual void remove(const std::string& sessionId) = 0;
};

class MemorySessionStorage: public SessionStorage
{
public:
    void save(std::shared_ptr<Session> session) override;
    std::shared_ptr<Session> load(const std::string& sessionId) override;
    void remove(const std::string& sessionId) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string,std::shared_ptr<Session> > sessions_;
};

} // namespace session
  
} // namespace http
