// Copyright 2021 GHA Test Team

#include "TimedDoor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

using ::testing::_;
using ::testing::StrictMock;

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockTimer : public Timer {
 public:
  MOCK_METHOD(void, tregister, (int, std::shared_ptr<TimerClient>), (override));
  MOCK_METHOD(void, cancel, (), (override));
  MOCK_METHOD(void, wait, (), (override));
};

class TimedDoorRealTimerTest : public ::testing::Test {
 protected:
  TimedDoor *door{nullptr};

  void SetUp() override { door = new TimedDoor(20); }

  void TearDown() override { delete door; }
};

class TimedDoorMockTimerTest : public ::testing::Test {
 protected:
  TimedDoor *door{nullptr};
  StrictMock<MockTimer> *mock_timer{nullptr};

  void SetUp() override {
    mock_timer = new StrictMock<MockTimer>();
    door = new TimedDoor(25, std::unique_ptr<Timer>(mock_timer));
  }

  void TearDown() override { delete door; }
};

TEST_F(TimedDoorRealTimerTest, DoorIsInitiallyClosed) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorRealTimerTest, UnlockOpensDoor) {
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorRealTimerTest, LockClosesDoor) {
  door->unlock();
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorRealTimerTest, GetTimeOutReturnsConstructorValue) {
  EXPECT_EQ(20, door->getTimeOut());
}

TEST_F(TimedDoorRealTimerTest, AdapterThrowsIfDoorIsStillOpened) {
  door->unlock();
  EXPECT_THROW(door->triggerTimeout(), std::runtime_error);
}

TEST_F(TimedDoorRealTimerTest, AdapterDoesNotThrowIfDoorWasClosed) {
  door->unlock();
  door->lock();
  EXPECT_NO_THROW(door->triggerTimeout());
}

TEST_F(TimedDoorRealTimerTest, WaitForTimerThrowsIfDoorRemainsOpened) {
  door->unlock();
  EXPECT_THROW(door->waitForTimer(), std::runtime_error);
}

TEST_F(TimedDoorRealTimerTest,
       WaitForTimerDoesNotThrowIfDoorClosedBeforeTimeout) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  door->lock();
  EXPECT_NO_THROW(door->waitForTimer());
}

TEST_F(TimedDoorRealTimerTest, TimerCanBeRearmedAfterCancel) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  door->lock();
  EXPECT_NO_THROW(door->waitForTimer());

  door->unlock();
  EXPECT_THROW(door->waitForTimer(), std::runtime_error);
}

TEST_F(TimedDoorMockTimerTest, UnlockRegistersTimerWithConfiguredTimeout) {
  EXPECT_CALL(*mock_timer, tregister(25, _)).Times(1);
  door->unlock();
}

TEST_F(TimedDoorMockTimerTest, LockCancelsTimer) {
  EXPECT_CALL(*mock_timer, tregister(25, _)).Times(1);
  EXPECT_CALL(*mock_timer, cancel()).Times(1);

  door->unlock();
  door->lock();
}

TEST_F(TimedDoorMockTimerTest, WaitForTimerDelegatesToTimer) {
  EXPECT_CALL(*mock_timer, wait()).Times(1);
  door->waitForTimer();
}

TEST(TimerTest, RegisterCallsClientTimeoutAfterDelay) {
  Timer timer;
  auto client = std::make_shared<StrictMock<MockTimerClient>>();

  EXPECT_CALL(*client, Timeout()).Times(1);

  timer.tregister(10, client);
  EXPECT_NO_THROW(timer.wait());
}

TEST(TimerTest, CancelPreventsClientTimeoutCall) {
  Timer timer;
  auto client = std::make_shared<StrictMock<MockTimerClient>>();

  EXPECT_CALL(*client, Timeout()).Times(0);

  timer.tregister(20, client);
  timer.cancel();
  EXPECT_NO_THROW(timer.wait());
}
