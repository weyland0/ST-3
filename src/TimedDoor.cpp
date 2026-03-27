// Copyright 2021 GHA Test Team

#include "TimedDoor.h"

#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor &timedDoor) : _door(timedDoor) {}

void DoorTimerAdapter::Timeout() {
  if (_door.isDoorOpened()) {
    _door.throwState();
  }
}

TimedDoor::TimedDoor(int timeout, std::unique_ptr<Timer> timer)
    : _timeout(timeout), _is_opened(false) {
  _adapter_ptr = std::make_shared<DoorTimerAdapter>(*this);
  _timer_ptr = timer ? std::move(timer) : std::make_unique<Timer>();
}

bool TimedDoor::isDoorOpened() { return _is_opened; }

void TimedDoor::unlock() {
  _is_opened = true;
  _timer_ptr->tregister(_timeout, _adapter_ptr);
}

void TimedDoor::lock() {
  _is_opened = false;
  _timer_ptr->cancel();
}

int TimedDoor::getTimeOut() const { return _timeout; }

void TimedDoor::throwState() {
  throw std::runtime_error("Please close the door");
}

void TimedDoor::triggerTimeout() { _adapter_ptr->Timeout(); }

void TimedDoor::waitForTimer() { _timer_ptr->wait(); }

Timer::~Timer() {
  cancel();
  joinWorker();
}

void Timer::sleep(int timeout) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);

  while (!_is_canceled.load() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void Timer::start(int timeout) {
  _worker = std::thread([timeout, this] {
    try {
      sleep(timeout);

      std::shared_ptr<TimerClient> client;
      {
        std::lock_guard<std::mutex> lock(_mutex);
        client = _client_ptr;
      }

      if (client && !_is_canceled.load()) {
        client->Timeout();
      }
    } catch (...) {
      std::lock_guard<std::mutex> lock(_mutex);
      _error_ptr = std::current_exception();
    }
  });
}

void Timer::tregister(int timeout, std::shared_ptr<TimerClient> client) {
  cancel();
  joinWorker();
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _client_ptr = client;
    _error_ptr = nullptr;
  }

  _is_canceled = false;

  if (client) {
    start(timeout);
  }
}

void Timer::cancel() { _is_canceled = true; }

void Timer::wait() {
  joinWorker();

  std::exception_ptr error;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    error = _error_ptr;
    _error_ptr = nullptr;
  }

  if (error != nullptr) {
    std::rethrow_exception(error);
  }
}

void Timer::joinWorker() {
  if (_worker.joinable()) {
    _worker.join();
  }
}
