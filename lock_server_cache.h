#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include "handle.h"
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>

enum class RLockStatus {
    Granted,
    Free,
    Revoking,
    Retrying
};

struct RLockState {
    RLockStatus status;
    std::string owned_handle;
};


struct RevokeTask {
    lock_protocol::lockid_t lid{};
    std::string requester_handle;

    RevokeTask(unsigned long long int lid, std::string requesterHandle);

    RevokeTask(RevokeTask &&rhs) noexcept;

    RevokeTask(RevokeTask &rhs);
};

using lock_store_t = std::map<lock_protocol::lockid_t, RLockState>;
using unique_t = std::unique_lock<std::mutex>;

template<typename T>
class concurrent_queue {
private:
    std::queue<T> _queue{};
    std::mutex update_mutex;
    std::condition_variable wait_content_cv;
public:
    void enqueue(const T &data);

    void enqueue(T &&data);

    T dequeue();
};

template<typename T>
void concurrent_queue<T>::enqueue(const T &data) {
    update_mutex.lock();
    _queue.push(data);
    if (_queue.size() == 1) {
        update_mutex.unlock();
        wait_content_cv.notify_one();
    } else {
        update_mutex.unlock();
    }
}

template<typename T>
T concurrent_queue<T>::dequeue() {
    unique_t u{update_mutex};
    while (_queue.empty()) {
        wait_content_cv.wait(u);
    }
    auto v = _queue.front();
    _queue.pop();
    return v;
}

template<typename T>
void concurrent_queue<T>::enqueue(T &&data) {
    update_mutex.lock();
    _queue.push(std::forward<T>(data));
    if (_queue.size() == 1) {
        update_mutex.unlock();
        wait_content_cv.notify_one();
    } else {
        update_mutex.unlock();
    }
}


class lock_server_cache {
private:
    int nacquire{};
    lock_store_t lock_store;
    std::mutex access_lock_mutex;
    concurrent_queue<RevokeTask> revoke_queue;
    bool _stop = false;
public:
    lock_server_cache();

    lock_protocol::status stat(lock_protocol::lockid_t, int &);

    int acquire(lock_protocol::lockid_t lid, std::string &id, int &result);

    int release(lock_protocol::lockid_t lid, std::string &id, int &result);

    void revoker();
};

#endif
