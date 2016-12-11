#pragma once
#include <exception>
#include <stdexcept>
#include <atomic>
#include <mutex>
#include <condition_variable>

/*
  [ copied over from score ]
  ThreadBaton is inspired by folly's original (not fiber-based) Baton class.
  
  It's used where one or more threads need to wait on some action by another 
  (non-waiting) thread.

  In this case the action-taking thread is responsible for calling `post()` when
  it's done.  The waiters either call `wait()`, which blocks until `post()` has
  been called, or poll the status by calling `isDone()`.

  This is mainly used for synchronization in test code.
*/


namespace kclpp { namespace locks {

class ThreadBaton {
 protected:
  std::atomic<bool> posted_ {false};
  std::mutex mutex_;
  std::condition_variable condition_;
  ThreadBaton(const ThreadBaton&) = delete;
  ThreadBaton& operator=(const ThreadBaton&) = delete;

 public:
  ThreadBaton(){}
  ThreadBaton(ThreadBaton&&) = default;
  ThreadBaton& operator=(ThreadBaton&&) = default;
  class AlreadyPosted : public std::runtime_error {
   public:
    AlreadyPosted();
  };
  void post();
  void wait();
  bool isDone() const;

 protected:
  void doPost();
};

}} // kclpp::locks
