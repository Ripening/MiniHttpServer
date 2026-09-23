#include "session/Session.h"

namespace http
{
namespace session
{
    Session::Session(const std::string& sessionId,
                    SessionManager* sessionManage,
                    int maxAge):
                    sessionId_(sessionId),
                    sessionManage_(sessionManage),
                    maxAge_(maxAge)
    {
        refresh();
    }

    void Session::refresh(){
        std::lock_guard<std::mutex> lock(mutex_);
        expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
    }

    bool Session::isExpiry() const{
        std::lock_guard<std::mutex> lock(mutex_);
        return std::chrono::system_clock::now() > expiryTime_;
    }

    void Session::setValue(const std::string& key,const std::string& value){
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
    }
    std::string Session::getValue(const std::string& key) const{
        // 返回拷贝,不让内部的值的引用逃出锁域
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        return it!=data_.end()?it->second:std::string();
    }
    void Session::remove(const std::string& key){
        std::lock_guard<std::mutex> lock(mutex_);
        data_.erase(key);
    }
    void Session::clear(){
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
    }
} // namespace session

} // namespace http
