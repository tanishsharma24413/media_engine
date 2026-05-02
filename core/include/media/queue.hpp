#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace media {

template <typename T>
class ThreadQueue {
public:
  void push(T item) {
    {
      std::scoped_lock lock(mutex_);
      if (flushed_) return;
      queue_.push_back(std::move(item));
    }
    cv_.notify_one();
  }

  std::optional<T> pop() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || flushed_; });
    
    if (flushed_ && queue_.empty()) {
      return std::nullopt;
    }
    
    T item = std::move(queue_.front());
    queue_.pop_front();
    return item;
  }

  void flush() {
    {
      std::scoped_lock lock(mutex_);
      flushed_ = true;
      queue_.clear();
    }
    cv_.notify_all();
  }

  void reset() {
    std::scoped_lock lock(mutex_);
    flushed_ = false;
  }

  size_t size() const {
    std::scoped_lock lock(mutex_);
    return queue_.size();
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<T> queue_;
  bool flushed_{false};
};

} // namespace media
