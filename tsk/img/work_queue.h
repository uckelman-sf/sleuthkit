#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

class WorkQueue {
public:
  WorkQueue() {
    std::thread t([this] {
      while (true) {
        std::packaged_task<void()> task;
        {
          std::unique_lock<std::mutex> lock(m);
          c.wait(lock, [this]{ return !q.empty(); });
          task = std::move(q.back());
          q.pop_back();
        }
        task();
      }
    });

    t.detach();
  }

  template <class F, class... Args>
  std::future<typename std::result_of<F(Args...)>::type>
  enqueue(F&& f, Args&&... args) {
    std::packaged_task<typename std::result_of<F(Args...)>::type()> task(
      std::bind(f, args...)
    );
    auto fut = task.get_future();

    std::lock_guard<std::mutex> lock(m);
    q.emplace(q.begin(), std::move(task));
    c.notify_one();

    return fut;
  }

  template <class F, class... Args>
  std::future<typename std::result_of<F(Args...)>::type>
  enqueue_urgently(F&& f, Args&&... args) {
    std::packaged_task<typename std::result_of<F(Args...)>::type()> task(
      std::bind(f, args...)
    );
    auto fut = task.get_future();

    std::lock_guard<std::mutex> lock(m);
    q.emplace_back(std::move(task));
    c.notify_one();

    return fut;
  }

private:
  std::vector<std::packaged_task<void()>> q;
  std::mutex m;
  std::condition_variable c;
};
