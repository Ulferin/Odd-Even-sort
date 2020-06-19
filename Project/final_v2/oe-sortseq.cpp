/**
 * 
 * Sequential implementation of Odd-Even sort algorithm.
 * 
*/


#include <iostream>
#include <vector>
#include <chrono>
#include <assert.h>
#include <algorithm>

using hrclock = std::chrono::high_resolution_clock;
using now = std::chrono::_V2::system_clock::time_point;

// Stats counters
  int phase1 = 0;
  int n_1 = 0;
  int phase2 = 0;
  int n_2 = 0;
  int overhead = 0;
  now time_s, time_e;


/**
 * 
 * Initializes vector to sort.
 * @param vec    vector to initialize
 * @param seed   seed for random number generation
 * @param m      vector length
 * @param max    max value to be present in the initialized vector
 * 
*/
void initializeVector(std::vector<int16_t> *vec, int seed, int m, int max) {
  srand(seed);

  for (int i = 0; i < m; i++)
  {
    (*vec)[i] = (int16_t)(rand() % max);
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
 * Actual implementation of the Odd-Even sort algorithm.
 * @param to_sort vector to sort
 * @param m       vector length
 * 
*/
void oddEvenSort(std::vector<int16_t> *to_sort, int m) {
  auto &vec = *to_sort;

  now start = hrclock::now();
  while(true) {
    int16_t test = 0;   // Auxiliary variable to check swaps.

    // Phase 1: even phase
    time_s = hrclock::now();
    for (int i = 0; i < m-1; i+=2)
    {
      int16_t first = vec[i];
      int16_t second = vec[i+1];

      // Swapping values
      int16_t temp = first;
      first = ( (first > second) ? second : first );
      second = ( (temp > second) ? temp : second);

      vec[i] = first;
      vec[i+1] = second;
    }
    time_e = hrclock::now();
    phase1 += std::chrono::duration_cast<std::chrono::microseconds>(time_e-time_s).count();
    n_1++;
    
    // Phase 2: odd phase
    time_s = hrclock::now();
    for (int i = 1; i < m-1; i+=2)
    {
      int16_t first = vec[i];
      int16_t second = vec[i+1];

      int16_t temp = first;
      first = ( (first > second) ? second : first );
      second = ( (temp > second) ? temp : second);

      vec[i] = first;
      vec[i+1] = second;

      // Compatible with SIMD, avoiding type conversion
      test = test | (temp > first);
    }
    time_e = hrclock::now();
    phase2 += std::chrono::duration_cast<std::chrono::microseconds>(time_e-time_s).count();
    n_2++;

    if(!test) break;   

  }
  now end = hrclock::now();
  overhead += std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
  overhead = overhead - phase2 - phase1;
  std::cout << "Overhead: " << overhead << std::endl;

}

int main(int argc, char const *argv[])
{
  
  if(argc < 3) {
    std::cout << "Usage: " << argv[0] << " seed len [max-value]" << std::endl;
    return -1;
  }

  int seed = atoi(argv[1]);
  const int m = atoi(argv[2]);
  
  // Optional value to specify the maximum value
  // that can be present in the array to be sorted.
  // Assuming to use only
  // positive integers just for simplicity
  int max = INT16_MAX;
  if(argc == 4)
    max = atoi(argv[3]);

  std::vector<int16_t> *to_sort = new std::vector<int16_t>(m);
  initializeVector(to_sort, seed, m, max);

  auto start = hrclock::now();
#ifdef DEBUG
  printVector(to_sort);
#endif  
  oddEvenSort(to_sort, m);
#ifdef DEBUG
  printVector(to_sort);
#endif
  auto elapsed = hrclock::now() - start;
  auto usec    = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

  std::cout << "Simulation spent: " << usec << " usecs\n";
  std::cout << "Average phase1 spent: " << phase1 << " usecs, with a total of: " << n_1 << " phases." << " That is: " << (float)phase1/(float)n_1 << " usecs per phase." << "\n";
  std::cout << "Average phase2 spent: " << phase2 << " usecs, with a total of: " << n_2 << " phases." << " That is: " << (float)phase2/(float)n_2 << " usecs per phase." << "\n";
  std::cout << "OH per cicle: " << (float)overhead/(float)(n_1*2) << std::endl;

  // Checking if it is really sorted
  assert(std::is_sorted(std::begin(*to_sort), std::end(*to_sort)));

  delete(to_sort);

  return 0;
}
