/**
 * 
 * Parallel implementation of Odd-Even sort algorithm
 * using only C++ standard mechanisms.
 * 
 * Each thread has a region assigned to work on,
 * it keeps sorting the same region (switching starting position based on current phase)
 * until the shared exit condition is met.
 * 
*/


#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <assert.h>
#include <thread>
#include <sched.h>
#include <algorithm>

#include "utils.cpp"

using hrclock = std::chrono::high_resolution_clock;

std::vector<Range> ranges;      // Ranges to assign work
std::atomic<int16_t> cond = 0;  // Exit condition

Barrier *bar1;                  // Even phase barrier
Barrier *bar2;                  // Odd phase barrier

int nw;                         // Number of workers

/**
 * 
 * Auxiliary function to assign ranges to workers
 * 
*/
void assignRanges(int m) {
  auto range_size = m / nw;
  auto rem = m % nw;

  for(int i=0; i<nw; i++) {
    Range range;
    range.start = (i==0 ? 0 : ranges.back().end + 1);
    range.end   = (i != (nw-1) ? range.start+range_size - 1 : m-1);
    
    if(range.end%2 == 0) range.end++;
    ranges.push_back(range);
  }
}


/**
 * 
 * Initializes vector to sort with padding,
 * based on the number of workers and on cache line size.
 * It initializes also the ranges to assign to each worker with the indexes to access
 * the vector.
 * @param vec    vector to initialize
 * @param seed   seed for random number generation
 * @param max    max value to be present in the initialized vector
 * @param c_size cache size (in bytes) used for padding
 * 
*/
void initializeVector(std::vector<int16_t> *vec, int seed, int max, int c_size) {
  srand(seed);
  
  for (int i = 0; i < nw; i++)
  {
    // Assigning new start based on padding
    int inter_size = ranges[i].end - ranges[i].start + 1;
    ranges[i].size = (i==nw-1 ? inter_size-1 : inter_size);
    ranges[i].l_start = vec->size();
    int pad = ((c_size - (2*(inter_size+1)%c_size))/2)%32;

    int16_t back;
    if(i!=0) {
      vec->push_back(back);
      inter_size--;
    }
    for (int k = 0; k <= inter_size-1; k++)
    {
      int16_t el = (int16_t)(rand() % max);
      vec->push_back(el);
    }
    // If not last worker, save the next element in the current region
    if(i!=nw-1) {
      back = (int16_t)(rand() % max);
      vec->push_back(back);
      // Add padding
      for (int i = 0; i < pad; i++)
      {
        vec->push_back(-1);
      } 
    }
    
  }
}


/**
 * 
 * Auxiliary function, used for debugging.
 * Prints all the elements in a vector.
 * @param vec vector to print
 * 
*/
void printVector(std::vector<int16_t> *vec) {
  for (int i = 0; i < (*vec).size(); i++)
  {
    std::cout << (*vec)[i] << " ";
  }
  std::cout << "" << std::endl;
}


/**
 * 
 * Thread function used for sorting the region specified in the assigned range.
 * @param to_sort vector to sort
 * @param range   range assigned to this thread
 * @param id      id of this thread
 * 
*/
void oddEvenSort(std::vector<int16_t> *to_sort, Range range, int id) {
  int l_start = range.l_start;
  int size = range.size;

  auto local_vec = &(*to_sort)[l_start];
  auto &vec = *to_sort;

  auto& b1 = *bar1;
  auto& b2 = *bar2;

  while(true) {

    int16_t test = 0;   // Auxiliary variable to check swaps.

    // Prepare for even phase, updates first/last element
    if(id!=0) {
      local_vec[0] = vec[ranges[id-1].l_start + ranges[id-1].size];
    }

    // Phase 1: even phase
    #pragma GCC ivdep
    for (int i = 0; i < size; i+=2)
    {
      int16_t first = local_vec[i];
      int16_t second = local_vec[i+1];

      // Swapping values
      int16_t temp = first;
      first = ( (first > second) ? second : first );
      second = ( (temp > second) ? temp : second);

      local_vec[i] = first;
      local_vec[i+1] = second;

    }
    b1.dec_wait();

    
    if(id != nw-1) {
      int16_t el2 = vec[ranges[id+1].l_start];
      local_vec[size] = el2;
    }

    // Phase 2: odd phase
    #pragma GCC ivdep
    for (int i = 1; i < size; i+=2)
    {
      int16_t first = local_vec[i];
      int16_t second = local_vec[i+1];

      int16_t temp = first;
      first = ( (first > second) ? second : first );
      second = ( (temp > second) ? temp : second);

      local_vec[i] = first;
      local_vec[i+1] = second;
      
      test = test | (temp > first);
    }
    cond += test;
    b2.dec_wait();

    if(cond == 0) break;

    // Reset barriers
    b1.inc_wait();
    b2.inc_wait();

    cond=0;
  }
}

int main(int argc, char const *argv[])
{
  
  if(argc < 5) {
    std::cout << "Usage: " << argv[0] << " seed len nw cache-line-bytes [max-value]" << std::endl;
    return -1;
  }

  int seed = atoi(argv[1]);
  const int m = atoi(argv[2]);
  nw = atoi(argv[3]);
  int size = atoi(argv[4]);

  // Optional value to specify the maximum value
  // that can be present in the array to be sorted.
  // Assuming to use only positive integers just for simplicity
  int max = INT16_MAX;
  if(argc == 6)
    max = atoi(argv[argc-1]);

  bar1 = new Barrier(nw);
  bar2 = new Barrier(nw);

  std::vector<std::thread> tids;
  std::vector<int16_t> *to_sort = new std::vector<int16_t>();
  assignRanges(m);
  initializeVector(to_sort, seed, max, size);

  int max_threads = std::thread::hardware_concurrency();

  auto start = hrclock::now();
#ifdef DEBUG
  printVector(to_sort);
#endif 
  for (int i = 0; i < nw; i++) {
    tids.push_back(std::thread(oddEvenSort, to_sort, ranges[i], i));
    // Thread pinning
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i % max_threads, &cpuset);  // without % those in excess are free to move
    int rc = pthread_setaffinity_np(tids[i].native_handle(),sizeof(cpu_set_t), &cpuset);
  }

  for(std::thread& t: tids) {
    t.join();
  }
#ifdef DEBUG
  printVector(to_sort);
#endif
  auto elapsed = hrclock::now() - start;
  auto usec    = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

  std::cout << "Simulation spent: " << usec << " usecs\n";

  // Building sorted vector
  std::vector<int16_t> sorted;
  for (int i = 0; i < nw; i++)
  {
    for (int j = ranges[i].l_start; j < ranges[i].l_start+ranges[i].size; j++)
    {
      sorted.push_back((*to_sort)[j]);
      if(i==nw-1 && j==(ranges[i].l_start+ranges[i].size-1)) sorted.push_back((*to_sort)[j+1]);
    }
  }
  #ifdef DEBUG
    std::cout << "Final sorted vector is: ";
    printVector(&sorted);
  #endif

  delete(bar1);
  delete(bar2);
  delete(to_sort);

  // Checking if it is really sorted
  assert(std::is_sorted(std::begin(sorted), std::end(sorted)));

  return 0;
}
