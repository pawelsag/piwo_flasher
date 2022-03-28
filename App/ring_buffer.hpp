#pragma once
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
    int head_next;
    head_next = head;

    MODULO_INC(head_next, S);
    if (head_next == tail) {
      return false;
    }

    buf[head] = item;
    head = head_next;
    return true;
  }

  bool pop(T *item)
  {
    if(empty())
    {
      return false;
    }

    *item = buf[tail];
    MODULO_INC(tail, S);
    return true;
  }

  bool empty()
  {
    return head == tail;
  }

  void
  reset()
  {
    head = tail = 0;
  }

private:
  int head=0, tail=0;
  std::array<T,S> buf;
};
