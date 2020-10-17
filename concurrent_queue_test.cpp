//
// Created by fourstring on 2020/10/16.
//

#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <unistd.h>

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


concurrent_queue<int> cq;

void producer() {
    int i[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (auto l:i) {
        cq.enqueue(l);
    }
}

void consumer() {
    while (true) {
        std::cout << cq.dequeue() << ' ' << std::flush;
    }
}

int main() {
    std::thread t2{consumer}, t1{producer};

    t1.detach();
    t2.detach();
    while (true) {
        sleep(100);
    }
    return 0;
}