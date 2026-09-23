#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>

namespace http
{
namespace session
{

class SessionManager;

class Session:public std::enable_shared_from_this<Session>
{

public:
    Session(const std::string& sessionId,SessionManager* sessionManage,int maxAge = 3600);
    ~Session() = default;

    const std::string& getId() const{
        return sessionId_;
    }
    bool isExpiry() const;
    void refresh();

    void setManager(SessionManager* manager){
        std::lock_guard<std::mutex> lock(mutex_);
        sessionManage_ = manager;
    }
    SessionManager* getManager() const{
        std::lock_guard<std::mutex> lock(mutex_);
        return sessionManage_;
    }

    void setValue(const std::string& key,const std::string& value);
    std::string getValue(const std::string& key) const;
    void remove(const std::string& key);
    void clear();

private:
    std::string sessionId_;  //会话ID(构造后不变,读它不用锁)
    // 连接和 reactor 绑定,但同一个会话可能被多条连接、多个 reactor 线程碰,
    // 所以 data_/expiryTime_ 的每次访问都要先拿这把锁
    mutable std::mutex mutex_;
    std::unordered_map<std::string,std::string> data_;
    std::chrono::system_clock::time_point        expiryTime_;
    int maxAge_;
    SessionManager* sessionManage_;
};

} // namespace session
    
} // namespace http

