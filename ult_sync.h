#pragma once
#include "ult_context.h"

class ULTMutex {
public:
  void lock() {
    if (!locked) { locked = true; return; }
    waiters.push_back(g_current_idx);
    ::SwitchToFiber(scheduler_fiber);
    // when we return here, the lock has been granted
  }
  void unlock() {
    if (waiters.empty()) locked = false;
    else {
      auto next = waiters.front(); waiters.pop_front();
      ready_queue.push_back(next);
    }
  }
private:
  bool locked = false;
  std::deque<std::size_t> waiters;
};

class ULTCondVar {
public:
  void wait(ULTMutex &m) {
    waiters.push_back(g_current_idx);
    m.unlock();
    ::SwitchToFiber(scheduler_fiber);
    m.lock();
  }
  void signal() {
    if (!waiters.empty()) {
      auto idx = waiters.front(); waiters.pop_front();
      ready_queue.push_back(idx);
    }
  }
  void broadcast() {
    while (!waiters.empty()) {
      auto idx = waiters.front(); waiters.pop_front();
      ready_queue.push_back(idx);
    }
  }
private:
  std::deque<std::size_t> waiters;
};
