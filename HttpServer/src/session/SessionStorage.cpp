#include "session/SessionStorage.h"

namespace http
{
namespace session
{

    void MemorySessionStorage::save(std::shared_ptr<Session> session){
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[session->getId()] = session;
    }
    void MemorySessionStorage::remove(const std::string& sessionId){
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(sessionId);
    }
    std::shared_ptr<Session> MemorySessionStorage::load(const std::string& sessionId){
        // 锁序:只有 storage 锁包着 Session 锁这一种嵌套(下面 isExpiry 会拿 Session 锁),
        // Session 的方法不回调 storage,单向无环
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if(it!=sessions_.end()){
            if(!it->second->isExpiry()){
                return it->second;
            }
            else{
                sessions_.erase(sessionId);
            }
        }
        return nullptr;
    }
} // namespace session
} // namespace http
