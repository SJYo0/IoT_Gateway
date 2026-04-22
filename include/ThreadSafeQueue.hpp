#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

public:
    // 센서 스레드에 활용할 push
    // 뮤텍스와 조건변수 활용 무결성 보장
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        cond_var_.notify_one(); // 데이터가 들어오면 통신 스레드 알림
    } // 함수 종료시 자동 Unlock

    // 통신 스레드가 활용할 pop
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 데이터가 들어올때까지 통신 스레드 대기
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });
        
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }
};

#endif