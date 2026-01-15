
#include "sem.h"

#include <cassert>
#include <cerrno>
#include <chrono>

#include "result.h"

namespace fio {

void Sem::Init(int32_t v) { value_ = v; }

void Sem::Up() {
  assert(magic_ == kFioSemMagic);
  std::lock_guard<std::mutex> g(lock_);
  /*
   * If value is currently 0, there might be someone waiting on it.
   * If so, we need to wake up a waiter.
   */
  if (!value_ && waiters_) {
    cond_.notify_one();
  }
  value_++;
}

void Sem::Down() {
  assert(magic_ == kFioSemMagic);
  std::unique_lock<std::mutex> g(lock_);
  while (!value_) {
    waiters_++;
    cond_.wait(g);
    waiters_--;
  }
  value_--;
}

bool Sem::DownTryLock() {
  assert(magic_ == kFioSemMagic);
  std::lock_guard<std::mutex> g(lock_);
  if (value_) {
    value_--;
    return true;
  }
  return false;
}

Status Sem::DownTimeout(int32_t msecs) {
  assert(magic_ == kFioSemMagic);
  std::unique_lock<std::mutex> g(lock_);

  waiters_++;
  bool got_it =
      cond_.wait_for(g, std::chrono::milliseconds(msecs), [this] { return value_ > 0; });
  waiters_--;

  if (got_it) {
    value_--;
    return {};
  }

  return TimedOut("timedout");
}

}  // namespace fio
