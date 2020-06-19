/**
 * 
 * Parallel implementation of Odd-Even sort algorithm
 * using FastFlow library.
 * 
 * The implementation is a Master-Worker structure,
 * in which the Master node schedules "tasks" represented by 
 * the current phase and a local test condition.
 * Workers updates task test condition and for each task received
 * sort the assigned region until EOS is received.
 * 
*/


#include <iostream>
#include <vector>
#include <chrono>
#include <assert.h>
#include <algorithm>

#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include "utils.cpp"

using namespace ff;
using hrclock = std::chrono::high_resolution_clock;

std::vector<Range> ranges;      // Region assigned to workers
std::vector<Task*> *tasks;      // Tasks being assigned to workers
std::vector<int16_t> *to_sort;  // Vector to sort

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
 * @param c_size cache line size (in bytes) used for padding
 * 
*/
void initializeVector(std::vector<int16_t> *vec, int seed, int max, int c_size) {
  srand(seed);
  
  for (int i = 0; i < nw; i++)
  {
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


struct Master: ff_monode_t<Task> {

  int ntask = 0;        // Variable to check current active workers
  int16_t test = 0;     // Variable to check termination
  ff_loadbalancer *lb;  // Load balancer to retrieve channel id

  Master(ff_loadbalancer* const lb): lb(lb) {}

  Task* svc(Task* task) {

    // Farm is starting, null task
    // Send to all workers the assigned task
    if(task == nullptr) {
      for (int i = 0; i < nw; i++) {
        ntask++;
        (*tasks)[i] = new Task(0,0);
        ff_send_out_to((*tasks)[i], i);
      }
      return GO_ON;
    }
    
    test += task->test;
    ntask--;
    
    // Reached exit condition
    if(task->phase == 1 && test == 0 && ntask == 0)  {
      for (int i = 0; i < nw; i++)
      {
        delete((*tasks)[i]);
      }
      
      return EOS;
    }    

    // Reached end of phase, send new task to all the workers
    if(ntask == 0) {
      test = 0;
      for (int i = 0; i < nw; i++)
      {
        ntask++;
        (*tasks)[i]->test = 0;
        (*tasks)[i]->phase = ((*tasks)[i]->phase ? 0 : 1);
        ff_send_out_to((*tasks)[i], i);
      }
    }
    return GO_ON;
  }
};


struct Worker: ff_node_t<Task> {

  int size, l_start, l_end, id;
  std::vector<int16_t> *vec_to_sort;

  Worker(int id) : id(id) {
    size = ranges[id].size;
    l_start = ranges[id].l_start;
    l_end = l_start+ranges[id].size;

    // Create local vector to sort
    vec_to_sort = new std::vector<int16_t>(size+1);
    std::copy_n(std::begin(*to_sort)+l_start, size+1, std::begin(*vec_to_sort));
  }

  Task* svc(Task* task) {

    Task &t = *task;
    auto &vec = *to_sort;
    auto &local_vec = *vec_to_sort;
    int16_t test = 0;

    // Prepare next phase, updates first/last element
    if(id!=0 && (task->phase == 0)) {
      local_vec[0] = vec[ranges[id-1].l_start + ranges[id-1].size];
    }

    if(id != nw-1 && (task->phase == 1)) {
      local_vec[size] = vec[ranges[id+1].l_start];
    }

    if(task->phase == 0){
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
    }
    else {
      #pragma GCC ivdep
      for (int i = 1; i < size; i+=2)
      {
        int16_t first = local_vec[i];
        int16_t second = local_vec[i+1];

        // Swapping values
        int16_t temp = first;
        first = ( (first > second) ? second : first );
        second = ( (temp > second) ? temp : second);

        local_vec[i] = first;
        local_vec[i+1] = second;

        test = test | (temp > first);
      }
    }
    // Updates border elements
    vec[l_start] = local_vec[0];
    vec[l_end] = local_vec[size];

    task->test = test;
    return task;

  }

  void svc_end() {
    std::copy_n(std::begin(*vec_to_sort), size, std::begin(*to_sort)+l_start);
    delete(vec_to_sort);
  }

};


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
  // Assuming to use only
  // positive integers just for simplicity
  int max = INT16_MAX;
  if(argc == 6)
    max = atoi(argv[argc-1]);

  to_sort = new std::vector<int16_t>();
  tasks = new std::vector<Task*>(nw);
  assignRanges(m);
  initializeVector(to_sort, seed, max, size);

  auto start = hrclock::now();
#ifdef DEBUG
  printVector(to_sort);
#endif

  std::vector<std::unique_ptr<ff_node> > W;
  for(int i=0; i<nw; i++) W.push_back(make_unique<Worker>(i));

  ff_Farm<Task> farm(std::move(W));
  farm.remove_collector();

  Master  master(farm.getlb());
  farm.add_emitter(master);
  farm.wrap_around();

  if (farm.run_and_wait_end()<0) {
    error("running farm");
    return -1;
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

  delete(to_sort);
  delete(tasks);

  // Checking if it is really sorted
  assert(std::is_sorted(std::begin(sorted), std::end(sorted)));

  return 0;
}
