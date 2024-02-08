#include <iostream>
#include <atomic>


// Used to assign ranges to workers
struct Range {
  int start;
  int end;

  int l_start;
  int size;
};


// Used in the Master-Worker version to
// schedule work
struct Task {
  Task(int phase, int16_t test) : phase(phase), test(test) {}

  int phase;
  int16_t test;
};


// Active wait barrier
class Barrier {
  private:
    std::atomic<int> k;
    int n;

  public:
    Barrier(int in) : k(in), n(in) {}

    void inc_wait() {
      k++;
      while(k != n) {}
    }

    void dec_wait() {
      k--;
      while(k != 0) {}
    }
};