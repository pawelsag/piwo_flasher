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
struct ring_buffer{

  bool push_no_wait(T item)
  {
    sem_acquire_blocking(&sem);
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
    sem_acquire_blocking(&sem);
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

  bool empty()
  {
    return head == tail;
  }

  void
  reset()
  {
    sem_acquire_blocking(&sem);
    head = tail = 0;
    sem_release(&sem);
  }

  ring_buffer()
  {
    sem_init(&sem, 1, 1);
  }

private:
  int head=0, tail=0;
  std::array<T,S> buf;
  semaphore_t sem;
};

inline ring_buffer<100, uint8_t> rx_queue;
