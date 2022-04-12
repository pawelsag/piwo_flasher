#pragma once
extern "C"
{
#include "pico/sem.h"
}
#include <array>
#include <cmath>

#define MODULO_INC(val, max)                                      \
	{	                                                              \
		val = (val == max-1) ? 0 : val + 1;                         \
	}

template<int S, typename T>
struct ring_buffer {

  bool push_no_wait(T item)
  {
    if(try_sem_acquire(10) == false)
      assert(false);

    int head_next;
    head_next = head;

    MODULO_INC(head_next, S);
    if (head_next == tail) {
      sem_release(&sem);
      return false;
    }

    buf[head] = item;
    head = head_next;
    sem_release(&sem);
    return true;
  }

  bool pop(T *item)
  {
    if(try_sem_acquire(10) == false)
      assert(false);

    if(empty())
    {
      sem_release(&sem);
      return false;
    }

    *item = buf[tail];
    MODULO_INC(tail, S);
    sem_release(&sem);
    return true;
  }

  void
  reset()
  {
    if(try_sem_acquire(10) == false)
      assert(false);

    head = tail = 0;
    sem_release(&sem);
  }

  ring_buffer()
  {
    sem_init(&sem, 1, 1);
  }
private:
  bool try_sem_acquire(int attempts)
  {
    constexpr int us_to_ms= 1000;
    while(sem_acquire_timeout_us(&sem, 100*us_to_ms) == false && attempts > 0)
      attempts--;

    return !(attempts == 0);
  }

  bool empty()
  {
    return head == tail;
  }

private:
  int head=0, tail=0;
  std::array<T,S> buf;
  semaphore_t sem;
};

inline ring_buffer<100, uint8_t> rx_queue;
