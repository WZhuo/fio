#pragma once

#include <condition_variable>
#include <mutex>

#include "fio/result.h"

namespace fio {

constexpr int32_t kFioSemMagic = 0x4d555445U;

class Sem {
 public:
  enum class State : int32_t { LOCKED = 0, UNLOCKED = 1 };

  void Init(int32_t value);
  void Up();
  void Down();
  bool DownTryLock();
  Status DownTimeout(int32_t timeout);

 private:
  std::mutex lock_;
  std::condition_variable cond_;
  int32_t value_ = 0;
  int32_t waiters_ = 0;
  int32_t magic_ = kFioSemMagic;
};

}  // namespace fio
