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
    // [센서 스레드용] 큐에 데이터를 안전하게 집어넣음
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_); // 자물쇠 찰칵
        queue_.push(std::move(item));
        cond_var_.notify_one(); // "데이터 들어왔다!" 하고 통신 스레드를 깨움
    } // 함수가 끝나면 자동으로 자물쇠가 풀림

    // [통신 스레드용] 큐에서 데이터를 안전하게 꺼내감
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 큐가 비어있으면, 데이터가 들어올 때까지 통신 스레드를 재움 (CPU 점유율 최소화)
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });
        
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }
};

#endif