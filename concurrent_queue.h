//
// Created by fourstring on 2020/10/20.
//

#ifndef LAB1_CONCURRENT_QUEUE_H
#define LAB1_CONCURRENT_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

using cv_ptr = std::unique_ptr<std::condition_variable>;
using mutex_ptr = std::unique_ptr<std::mutex>;

using unique_t = std::unique_lock<std::mutex>;

template<typename T>
class concurrent_queue {
private:
    std::queue<T> _queue{};
    mutex_ptr update_mutex;
    cv_ptr wait_content_cv;
public:
    concurrent_queue();

    concurrent_queue(concurrent_queue &&rhs) noexcept;

    void enqueue(const T &data);

    void enqueue(T &&data);

    T dequeue();
};

template<typename T>
void concurrent_queue<T>::enqueue(const T &data) {
    update_mutex->lock();
    _queue.push(data);
    if (_queue.size() == 1) {
        update_mutex->unlock();
        wait_content_cv->notify_one();
    } else {
        update_mutex->unlock();
    }
}

template<typename T>
T concurrent_queue<T>::dequeue() {
    unique_t u{*update_mutex};
    while (_queue.empty()) {
        wait_content_cv->wait(u);
    }
    auto v = _queue.front();
    _queue.pop();
    return v;
}

template<typename T>
void concurrent_queue<T>::enqueue(T &&data) {
    update_mutex->lock();
    _queue.push(std::forward<T>(data));
    if (_queue.size() == 1) {
        update_mutex->unlock();
        wait_content_cv->notify_one();
    } else {
        update_mutex->unlock();
    }
}

template<typename T>
concurrent_queue<T>::concurrent_queue()
        : _queue{}, update_mutex{new std::mutex{}}, wait_content_cv{new std::condition_variable{}} {}

template<typename T>
concurrent_queue<T>::concurrent_queue(concurrent_queue &&rhs) noexcept:
        _queue{std::move(rhs._queue)}, update_mutex{std::move(rhs.update_mutex)},
        wait_content_cv{std::move(rhs.wait_content_cv)} {

}

#endif //LAB1_CONCURRENT_QUEUE_H
