// Copyright 2014, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef CPP_CHANNEL_H
#define CPP_CHANNEL_H

#include <deque>
#include <mutex>
#include <vector>
#include <limits>
#include <random>
#include <memory>
#include <thread>
#include <cstddef>
#include <cassert>
#include <functional>
#include <type_traits>
#include <condition_variable>

namespace cpp
{

namespace internal
{

#if __cplusplus <= 201103L
// since C++14 in std, see Herb Sutter's blog
template<class T, class ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
  using std::make_unique;
#endif

template<class T>
struct _is_exception_safe :
  std::integral_constant<bool,
    std::is_nothrow_copy_constructible<T>::value or
    std::is_nothrow_move_constructible<T>::value>
{};

// Note that currently handshakes between send/receives inside selects
// have higher priority compared to sends/receives outside selects.

// TODO: investigate and ideally also discuss other scheduling algorithms
template<class T, std::size_t N>
class _channel
{
static_assert(N < std::numeric_limits<std::size_t>::max(),
  "N must be strictly less than the largest possible size_t value");

private:
  std::mutex m_mutex;
  std::condition_variable m_send_begin_cv;
  std::condition_variable m_send_end_cv;
  std::condition_variable m_recv_cv;

  // FIFO order
  std::deque<std::pair</* sender */ std::thread::id, T>> m_queue;

  bool m_is_send_done;
  bool m_is_try_send_done;
  bool m_is_recv_ready;
  bool m_is_try_send_ready;
  bool m_is_try_recv_ready;

  bool is_full() const
  {
    return m_queue.size() > N;
  }

  // Is nonblocking receive and nonblocking send ready for handshake?
  bool is_try_ready() const
  {
    return m_is_try_recv_ready && m_is_try_send_ready;
  }

  // Block calling thread until queue becomes nonempty. While waiting
  // (i.e. queue is empty), give try_send() a chance to succeed.
  //
  // \pre: calling thread owns lock
  // \post: queue is nonempty and calling thread still owns lock
  void _pre_blocking_recv(std::unique_lock<std::mutex>& lock)
  {
    m_is_recv_ready = true;
    m_recv_cv.wait(lock, [this]{ return !m_queue.empty(); });

    // TODO: support the case where both ends of a channel are inside a select
    assert(!is_try_ready());
  }

  // Pop front of queue and unblock one _send() (if any)
  //
  // \pre: calling thread must own lock and queue is nonempty
  // \post: calling thread doesn't own lock anymore, and protocol with
  //    try_send() and try_recv() is fulfilled
  void _post_blocking_recv(std::unique_lock<std::mutex>& lock)
  {
    // If queue is full, then there exists either a _send() waiting
    // for m_send_end_cv, or try_send() has just enqueued an element.
    //
    // In general, the converse is false: if there exists a blocking send,
    // then a nonblocking receive might have just dequeued an element,
    // i.e. queue is not full.
    assert(!is_full() || !m_is_send_done || !m_is_try_send_done);

    // blocking and nonblocking send can never occur simultaneously
    assert(m_is_try_send_done || m_is_send_done);

    m_queue.pop_front();
    assert(!is_full());

    // protocol with nonblocking calls
    m_is_try_send_done = true;
    m_is_recv_ready = false;
    m_is_try_recv_ready = false;
    m_is_try_send_ready = false;

    // Consider two concurrent _send() calls denoted by s and s'.
    //
    // Suppose s is waiting to enqueue an element (i.e. m_send_begin_cv),
    // whereas s' is waiting for an acknowledgment (i.e. m_send_end_cv)
    // that its previously enqueued element has been dequeued. Since s'
    // is waiting and the flag m_is_send_done is only modified by _send(),
    // m_is_send_done is false. Hence, we notify m_send_end_cv. This
    // causes s' to notify s, thereby allowing s to proceed if possible.
    //
    // Now suppose there is no such s' (say, due to the fact that the
    // queue never became full). Then, m_is_send_done == true. Thus,
    // m_send_begin_cv is notified and s proceeds if possible. Note
    // that if we hadn't notified s this way, then it could deadlock
    // in case that it waited on m_is_try_send_done to become true.
    if (m_is_send_done)
    {
      // unlock before notifying threads; otherwise, the
      // notified thread would unnecessarily block again
      lock.unlock();

      // nonblocking, see also note below about notifications
      m_send_begin_cv.notify_one();
    }
    else
    {
      // unlock before notifying threads; otherwise, the
      // notified thread would unnecessarily block again
      lock.unlock();

      // we rely on the following semantics of notify_one():
      //
      // if a notification n is issued to s (i.e. s is chosen from among
      // a set of threads waiting on a condition variable associated with
      // mutex m) but another thread t locks m before s wakes up (i.e. s
      // has not owned yet m after n had been issued), then n is retried
      // as soon as t unlocks m. The retries repeat until n arrives at s
      // meaning that s actually owns m and checks its wait guard.
      m_send_end_cv.notify_one();
    }
  }

  template<class U>
  void _send(U&&);

public:
  // \pre: calling thread must own mutex()
  // \post: calling thread doesn't own mutex() anymore
  template<class U>
  bool try_send(std::unique_lock<std::mutex>&, U&&);

  // \pre: calling thread must own mutex()
  // \post: calling thread doesn't own mutex() anymore
  std::pair<bool, std::unique_ptr<T>> try_recv_ptr(
    std::unique_lock<std::mutex>&);

  _channel(const _channel&) = delete;

  // Propagates exceptions thrown by std::condition_variable constructor
  _channel()
  : m_mutex(),
    m_send_begin_cv(),
    m_send_end_cv(),
    m_recv_cv(),
    m_queue(),
    m_is_send_done(true),
    m_is_try_send_done(true),
    m_is_recv_ready(false),
    m_is_try_send_ready(false),
    m_is_try_recv_ready(false) {}

  // channel lock
  std::mutex& mutex()
  {
    return m_mutex;
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    _send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    _send(std::move(t));
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv();

  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T&);

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr();
};

}

template<class T, std::size_t N> class ichannel;
template<class T, std::size_t N> class ochannel;

/// Go-style concurrency

/// Thread synchronization mechanism as in the Go language.
/// As in Go, cpp::channel<T, N> are first-class values.
///
/// Unlike Go, however, cpp::channels<T, N> cannot be nil
/// not closed. This simplifies the usage of the library.
///
/// The template arguments are as follows:
///
/// * T -- type of data to be communicated over channel
/// * N is zero -- synchronous channel
/// * N is positive -- asynchronous channel with queue size N
///
/// Note that cpp::channel<T, N>::recv() is only supported if T is
/// exception safe. This is automatically checked at compile time.
/// If T is not exception safe, use any of the other receive member
/// functions.
///
/// \see http://golang.org/ref/spec#Channel_types
/// \see http://golang.org/ref/spec#Send_statements
/// \see http://golang.org/ref/spec#Receive_operator
/// \see http://golang.org/doc/effective_go.html#chan_of_chan
template<class T, std::size_t N = 0>
class channel
{
static_assert(N < std::numeric_limits<std::size_t>::max(),
  "N must be strictly less than the largest possible size_t value");

private:
  friend class ichannel<T, N>;
  friend class ochannel<T, N>;

  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  channel(const channel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  // Propagates exceptions thrown by std::condition_variable constructor
  channel()
  : m_channel_ptr(std::make_shared<internal::_channel<T, N>>()) {}

  channel& operator=(const channel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const channel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const channel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  bool operator==(const ichannel<T, N>&) const noexcept;
  bool operator!=(const ichannel<T, N>&) const noexcept;

  bool operator==(const ochannel<T, N>&) const noexcept;
  bool operator!=(const ochannel<T, N>&) const noexcept;

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    m_channel_ptr->send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    m_channel_ptr->send(std::move(t));
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv()
  {
    static_assert(internal::_is_exception_safe<T>::value,
      "Cannot guarantee exception safety, use another recv operator");

    return m_channel_ptr->recv();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr()
  {
    return m_channel_ptr->recv_ptr();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T& t)
  {
    m_channel_ptr->recv(t);
  }
};

class select;

/// Can only be used to receive elements of type T
template<class T, std::size_t N = 0>
class ichannel
{
private:
  friend class select;
  friend class channel<T, N>;
  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  ichannel(const channel<T, N>& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ichannel(const ichannel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ichannel(ichannel&& other) noexcept
  : m_channel_ptr(std::move(other.m_channel_ptr)) {}

  ichannel& operator=(const ichannel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const ichannel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const ichannel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  T recv()
  {
    static_assert(internal::_is_exception_safe<T>::value,
      "Cannot guarantee exception safety, use another recv operator");

    return m_channel_ptr->recv();
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void recv(T& t)
  {
    m_channel_ptr->recv(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  std::unique_ptr<T> recv_ptr()
  {
    return m_channel_ptr->recv_ptr();
  }

};

/// Can only be used to send elements of type T
template<class T, std::size_t N = 0>
class ochannel
{
private:
  friend class select;
  friend class channel<T, N>;
  std::shared_ptr<internal::_channel<T, N>> m_channel_ptr;

public:
  ochannel(const channel<T, N>& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ochannel(const ochannel& other) noexcept
  : m_channel_ptr(other.m_channel_ptr) {}

  ochannel(ochannel&& other) noexcept
  : m_channel_ptr(std::move(other.m_channel_ptr)) {}

  ochannel& operator=(const ochannel& other) noexcept
  {
    m_channel_ptr = other.m_channel_ptr;
    return *this;
  }

  bool operator==(const ochannel& other) const noexcept
  {
    return m_channel_ptr == other.m_channel_ptr;
  }

  bool operator!=(const ochannel& other) const noexcept
  {
    return m_channel_ptr != other.m_channel_ptr;
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(const T& t)
  {
    m_channel_ptr->send(t);
  }

  // Propagates exceptions thrown by std::condition_variable::wait()
  void send(T&& t)
  {
    m_channel_ptr->send(std::move(t));
  }
};

/// Go's select statement

/// \see http://golang.org/ref/spec#Select_statements
///
/// \warning select objects must not be shared between threads
///
// TODO: investigate and ideally discuss pseudo-random distribution
class select
{
private:
  template<class T, std::size_t N, class NullaryFunction>
  class try_send_nullary
  {
  private:
    template<class U, class V = typename std::remove_reference<U>::type>
    static bool _run(ochannel<V, N>& c, U&& u, NullaryFunction f)
    {
      internal::_channel<V, N>& _c = *c.m_channel_ptr;
      std::unique_lock<std::mutex> lock(_c.mutex(), std::defer_lock);
      if (lock.try_lock() && _c.try_send(lock, std::forward<U>(u)))
      {
        assert(!lock.owns_lock());
        f();
        return true;
      }

      return false;
    }

  public:
    bool operator()(ochannel<T, N>& c, const T& t, NullaryFunction f)
    {
      return _run(c, t, f);
    }

    bool operator()(ochannel<T, N>& c, T&& t, NullaryFunction f)
    {
      return _run(c, std::move(t), f);
    }
  };

  template<class T, std::size_t N, class NullaryFunction>
  struct try_recv_nullary
  {
    bool operator()(ichannel<T, N>& c, T& t, NullaryFunction f)
    {
      internal::_channel<T, N>& _c = *c.m_channel_ptr;
      std::unique_lock<std::mutex> lock(_c.mutex(), std::defer_lock);
      if (lock.try_lock())
      {
        std::pair<bool, std::unique_ptr<T>> pair = _c.try_recv_ptr(lock);
        if (pair.first)
        {
          assert(!lock.owns_lock());
          t = *pair.second;
          f();
          return true;
        }
      }

      return false;
    }
  };

  template<class T, std::size_t N, class UnaryFunction>
  struct try_recv_unary
  {
    bool operator()(ichannel<T, N>& c, UnaryFunction f)
    {
      internal::_channel<T, N>& _c = *c.m_channel_ptr;
      std::unique_lock<std::mutex> lock(_c.mutex(), std::defer_lock);
      if (lock.try_lock())
      {
        std::pair<bool, std::unique_ptr<T>> pair = _c.try_recv_ptr(lock);
        if (pair.first)
        {
          assert(!lock.owns_lock());
          f(std::move(*pair.second));
          return true;
        }
      }

      return false;
    }
  };

  typedef std::function<bool()> try_function;
  typedef std::vector<try_function> try_functions;
  try_functions m_try_functions;

  std::mt19937 random_gen;

public:
  select()
  : m_try_functions(),
    random_gen(std::time(nullptr)) {}

  /* send cases */

  template<class T, std::size_t N,
    class U = typename std::remove_reference<T>::type>
  select& send_only(channel<U, N> c, T&& t)
  {
    return send_only(ochannel<U, N>(c), std::forward<T>(t));
  }

  template<class T, std::size_t N,
    class U = typename std::remove_reference<T>::type>
  select& send_only(ochannel<U, N> c, T&& t)
  {
    return send(c, std::forward<T>(t), [](){ /* skip */ });
  }

  template<class T, std::size_t N, class NullaryFunction,
    class U = typename std::remove_reference<T>::type>
  select& send(channel<U, N> c, T&& t, NullaryFunction f)
  {
    return send(ochannel<U, N>(c), std::forward<T>(t), f);
  }

  template<class T, std::size_t N, class NullaryFunction,
    class U = typename std::remove_reference<T>::type>
  select& send(ochannel<U, N> c, T&& t, NullaryFunction f)
  {
    m_try_functions.push_back(std::bind(
      try_send_nullary<U, N, NullaryFunction>(), c, std::forward<T>(t), f));
    return *this;
  }

  /* receive cases */

  template<class T, std::size_t N>
  select& recv_only(channel<T, N> c, T& t)
  {
    return recv_only(ichannel<T, N>(c), t);
  }

  template<class T, std::size_t N>
  select& recv_only(ichannel<T, N> c, T& t)
  {
    return recv(c, t, [](){ /* skip */ });
  }

  template<class T, std::size_t N, class NullaryFunction>
  select& recv(channel<T, N> c, T& t, NullaryFunction f)
  {
    return recv(ichannel<T, N>(c), t, f);
  }

  template<class T, std::size_t N, class NullaryFunction>
  select& recv(ichannel<T, N> c, T& t, NullaryFunction f)
  {
    m_try_functions.push_back(std::bind(
      try_recv_nullary<T, N, NullaryFunction>(), c, std::ref(t), f));
    return *this;
  }

  template<class T, std::size_t N, class UnaryFunction>
  select& recv(channel<T, N> c, UnaryFunction f)
  {
    return recv(ichannel<T, N>(c), f);
  }

  template<class T, std::size_t N, class UnaryFunction>
  select& recv(ichannel<T, N> c, UnaryFunction f)
  {
    m_try_functions.push_back(std::bind(
      try_recv_unary<T, N, UnaryFunction>(), c, f));
    return *this;
  }

  /// Nonblocking like Go's select statement with default case

  /// Returns true if and only if exactly one case succeeded
  bool try_once()
  {
    const try_functions::size_type n = m_try_functions.size(), i = random_gen();
    for(try_functions::size_type j = 0; j < n; j++)
    {
      if (m_try_functions.at((i + j) % n)())
        return true;
    }
    return false;
  }

  void wait()
  {
    const try_functions::size_type n = m_try_functions.size();
    try_functions::size_type i = random_gen();
    for(;;)
    {
      i = (i + 1) % n;
      if (m_try_functions.at(i)())
        break;
    }
  }

  // Propagates any exception thrown by std::this_thread::sleep_for
  template<class Rep, class Period>
  void wait(const std::chrono::duration<Rep, Period>& sleep)
  {
    const try_functions::size_type n = m_try_functions.size();
    try_functions::size_type i = random_gen();
    for(;;)
    {
      i = (i + 1) % n;
      if (m_try_functions.at(i)())
        break;

      std::this_thread::sleep_for(sleep);
    }
  }
};

template<class T, std::size_t N>
bool channel<T, N>::operator==(const ichannel<T, N>& other) const noexcept
{
  return m_channel_ptr == other.m_channel_ptr;
}

template<class T, std::size_t N>
bool channel<T, N>::operator!=(const ichannel<T, N>& other) const noexcept
{
  return m_channel_ptr != other.m_channel_ptr;
}

template<class T, std::size_t N>
bool channel<T, N>::operator==(const ochannel<T, N>& other) const noexcept
{
  return m_channel_ptr == other.m_channel_ptr;
}

template<class T, std::size_t N>
bool channel<T, N>::operator!=(const ochannel<T, N>& other) const noexcept
{
  return m_channel_ptr != other.m_channel_ptr;
}

template<class T, std::size_t N>
template<class U>
bool internal::_channel<T, N>::try_send(
  std::unique_lock<std::mutex>& lock, U&& u)
{
  m_is_try_send_ready = true;

  // TODO: support the case where both ends of a channel are inside a select
  assert(!is_try_ready());

  if ((!m_is_send_done || !m_is_try_send_done || is_full() ||
       (0 == N - m_queue.size() && !m_is_recv_ready)))
  {
    // TODO: investigate potential LLVM libc++ RAII unlocking issue
    lock.unlock();
    return false;
  }

  assert(m_is_send_done);
  assert(m_is_try_send_done);
  assert(!is_full());

  // if enqueue should block, there must be a receiver waiting
  m_is_try_send_done = 0 < N - m_queue.size();
  assert(m_is_try_send_done || m_is_recv_ready);

  m_queue.emplace_back(std::this_thread::get_id(), std::forward<U>(u));

  // Let v be the value enqueued by try_send(). If m_is_try_send_done
  // is false, no other sender (whether blocking or not) can enqueue a
  // value until a receiver has dequeued v, thereby ensuring the channel
  // FIFO order when the queue is filled up by try_send(). Moreover, in
  // that case, since m_is_try_send_done implies m_is_recv_ready, such a
  // receiver is guaranteed to exist, and it will reset m_is_try_send_done
  // to true so that other senders can make progress after v has been
  // dequeued. And by notifying m_recv_cv, other receivers waiting for
  // the queue to become nonempty can make progress as well.
  lock.unlock();
  m_recv_cv.notify_one();
  return true;
}

template<class T, std::size_t N>
std::pair<bool, std::unique_ptr<T>> internal::_channel<T, N>::try_recv_ptr(
  std::unique_lock<std::mutex>& lock)
{
  m_is_try_recv_ready = true;

  if (m_queue.empty())
    return std::make_pair(false, std::unique_ptr<T>(nullptr));

  // If queue is full, then there exists either a _send() waiting
  // for m_send_end_cv, or try_send() has just enqueued an element.
  //
  // In general, the converse is false: if there exists a blocking send,
  // then a nonblocking receive might have just dequeued an element,
  // i.e. queue is not full.
  assert(!is_full() || !m_is_send_done || !m_is_try_send_done);

  // blocking and nonblocking send can never occur simultaneously
  assert(m_is_try_send_done || m_is_send_done);

  std::pair<std::thread::id, T> pair(std::move(m_queue.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  // move/copy before pop_front() to ensure strong exception safety
  std::unique_ptr<T> t_ptr(make_unique<T>(std::move(pair.second)));

  m_queue.pop_front();
  assert(!is_full());

  // protocol with nonblocking calls
  m_is_try_send_done = true;
  m_is_try_recv_ready = false;
  m_is_try_send_ready = false;

  // see also explanation in _channel::_post_blocking_recv()
  if (m_is_send_done)
  {
    lock.unlock();
    m_send_begin_cv.notify_one();
  }
  else
  {
    lock.unlock();
    m_send_end_cv.notify_one();
  }

  return std::make_pair(true, std::move(t_ptr));
}

template<class T, std::size_t N>
template<class U>
void internal::_channel<T, N>::_send(U&& u)
{
  // unlock before notifying threads; otherwise, the
  // notified thread would unnecessarily block again
  {
    // wait (if necessary) until queue is no longer full and any
    // previous _send() has successfully enqueued element
    std::unique_lock<std::mutex> lock(m_mutex);
    m_send_begin_cv.wait(lock, [this]{ return m_is_send_done &&
      m_is_try_send_done && !is_full(); });

    assert(m_is_send_done);
    assert(m_is_try_send_done);
    assert(!is_full());

    // TODO: support the case where both ends of a channel are inside a select
    assert(!is_try_ready());

    m_queue.emplace_back(std::this_thread::get_id(), std::forward<U>(u));
    m_is_send_done = false;
  }

  // nonblocking
  m_recv_cv.notify_one();

  // wait (if necessary) until u has been received by another thread
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    // It is enough to check !is_full() because m_is_send_done == false.
    // Therefore, no other thread could have caused the queue to fill up
    // during the brief time we didn't own the lock.
    //
    // Performance note: unblocks after at least N successful recv calls
    m_send_end_cv.wait(lock, [this]{ return !is_full(); });
    m_is_send_done = true;
  }

  // see also explanation in _channel<T, N>::recv()
  m_send_begin_cv.notify_one();
}

template<class T, std::size_t N>
T internal::_channel<T, N>::recv()
{
  static_assert(internal::_is_exception_safe<T>::value,
    "Cannot guarantee exception safety, use another recv operator");

  std::unique_lock<std::mutex> lock(m_mutex);
  _pre_blocking_recv(lock);

  std::pair<std::thread::id, T> pair(std::move(m_queue.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  _post_blocking_recv(lock);
  return std::move(pair.second);
}

template<class T, std::size_t N>
void internal::_channel<T, N>::recv(T& t)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  _pre_blocking_recv(lock);

  std::pair<std::thread::id, T> pair(std::move(m_queue.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  // assignment before pop_front() to ensure strong exception safety
  t = std::move(pair.second);
  _post_blocking_recv(lock);
}

template<class T, std::size_t N>
std::unique_ptr<T> internal::_channel<T, N>::recv_ptr()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  _pre_blocking_recv(lock);

  std::pair<std::thread::id, T> pair(std::move(m_queue.front()));
  assert(!is_full() || std::this_thread::get_id() != pair.first);

  // move/copy before pop_front() to ensure strong exception safety
  std::unique_ptr<T> t_ptr(make_unique<T>(std::move(pair.second)));
  _post_blocking_recv(lock);
  return t_ptr;
}

}

#endif