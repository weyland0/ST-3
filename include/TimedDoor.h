// Copyright 2021 GHA Test Team

#ifndef INCLUDE_TIMEDDOOR_H_
#define INCLUDE_TIMEDDOOR_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>

class DoorTimerAdapter;
class Timer;
class Door;
class TimedDoor;

class TimerClient {
 public:
  virtual ~TimerClient() = default;
  virtual void Timeout() = 0;
};

class Door {
 public:
  virtual ~Door() = default;
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool isDoorOpened() = 0;
};

class DoorTimerAdapter : public TimerClient {
 private:
  TimedDoor &_door;

 public:
  explicit DoorTimerAdapter(TimedDoor &);
  void Timeout() override;
};

class TimedDoor : public Door {
 private:
  std::shared_ptr<DoorTimerAdapter> _adapter_ptr;
  std::unique_ptr<Timer> _timer_ptr;
  int _timeout;
  bool _is_opened;

 public:
  explicit TimedDoor(int, std::unique_ptr<Timer> = nullptr);
  ~TimedDoor() = default;

  bool isDoorOpened();
  void unlock();
  void lock();
  int getTimeOut() const;
  void throwState();
  void triggerTimeout();
  void waitForTimer();
};

class Timer {
 private:
  std::shared_ptr<TimerClient> _client_ptr = nullptr;
  std::thread _worker;
  std::atomic_bool _is_canceled = false;
  std::exception_ptr _error_ptr = nullptr;
  mutable std::mutex _mutex;

  void sleep(int);
  void start(int);
  void joinWorker();

 public:
  explicit Timer() = default;
  virtual ~Timer();
  virtual void tregister(int, std::shared_ptr<TimerClient>);
  virtual void cancel();
  virtual void wait();
};

#endif // INCLUDE_TIMEDDOOR_H_
