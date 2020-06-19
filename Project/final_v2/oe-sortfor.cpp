/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 */
/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   April 2019
 *
 */

#include <iostream>
#include <vector>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
using namespace ff;

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


int main(int argc, char *argv[]) {    
  if(argc < 3) {
    std::cout << "Usage: " << argv[0] << " seed len [max-value]" << std::endl;
    return -1;
  }
  
  int seed = atoi(argv[1]);
  const int m = atoi(argv[2]);
  int nworkers = atoi(argv[3]);
  int grain = atoi(argv[4]);
  
  // Optional value to specify the maximum value
  // that can be present in the array to be sorted.
  // Assuming to use only
  // positive integers just for simplicity
  int max = INT16_MAX;
  if(argc == 6)
    max = atoi(argv[5]);
    
  std::vector<int16_t> *to_sort = new std::vector<int16_t>(m);
  initializeVector(to_sort, seed, m, max);

  // printVector(to_sort);
  ffTime(START_TIME);
  if (nworkers>1) {
      ParallelFor pf(nworkers,true);
      std::vector<float> V(m);
      auto &vec = *to_sort;
      int test = 0;

      auto even = [&](const long i) {
        int16_t first = vec[i];
        int16_t second = vec[i+1];

        // Swapping values
        int16_t temp = first;
        first = ( (first > second) ? second : first );
        second = ( (temp > second) ? temp : second);

        vec[i] = first;
        vec[i+1] = second;
      };      

      auto odd = [&](const long i) {
        int16_t first = vec[i];
        int16_t second = vec[i+1];

        // Swapping values
        int16_t temp = first;
        first = ( (first > second) ? second : first );
        second = ( (temp > second) ? temp : second);

        vec[i] = first;
        vec[i+1] = second;

        test = test | (temp > first);
      };

      int count = 0;

      while(true)
      {
        test = 0;
        pf.parallel_for(0, m-1, 2, grain, even, nworkers );
        pf.parallel_for(1, m-1, 2, grain, odd, nworkers );

        if(!test) break;
      }
  }

  ffTime(STOP_TIME);

  // printVector(to_sort);

  std::cout << "Time: " << ffTime(GET_TIME) << "\n";
  return 0;
}
