#ifndef _PARALLEL_H
#define _PARALLEL_H

// cilkarts cilk++
#if defined(CILK)
#include <cilk.h>
#include <cassert>
#define parallel_main cilk_main
#define parallel_for cilk_for
#define parallel_for_1 _Pragma("cilk_grainsize = 1") cilk_for
#define parallel_for_256 _Pragma("cilk_grainsize = 256") cilk_for

static int getWorkers() { return -1; }
static void setWorkers(int n) { }

// intel cilk+
#elif defined(CILKP)
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#define parallel_for cilk_for
#define parallel_main main
#define parallel_for_1 _Pragma("cilk grainsize = 1") parallel_for
#define parallel_for_256 _Pragma("cilk grainsize = 256") parallel_for


static int getWorkers() {
  return __cilkrts_get_nworkers();
}
static void setWorkers(int n) {
  __cilkrts_end_cilk();
  //__cilkrts_init();
  std::stringstream ss; ss << n;
  if (0 != __cilkrts_set_param("nworkers", ss.str().c_str())) {
    std::cerr << "failed to set worker count!" << std::endl;
    std::abort();
  }
}

// openmp
#elif defined(OPENMP)
#include <omp.h>
#define cilk_spawn
#define cilk_sync
#define parallel_main main
#define parallel_for _Pragma("omp parallel for") for
#define parallel_for_1 _Pragma("omp parallel for schedule (static,1)") for
#define parallel_for_256 _Pragma("omp parallel for schedule (static,256)") for

static int getWorkers() { return omp_get_max_threads(); }
static void setWorkers(int n) { omp_set_num_threads(n); }

// c++
#else
#define cilk_spawn
#define cilk_sync
#define parallel_main main
#define parallel_for for
#define parallel_for_1 for
#define parallel_for_256 for
#define cilk_for for

static int getWorkers() { return 1; }
static void setWorkers(int n) { }

#endif

#include <limits.h>

//#if defined(LONG)
typedef long intT;
typedef unsigned long uintT;
#define INT_T_MAX LONG_MAX
#define UINT_T_MAX ULONG_MAX
//#else
//typedef int intT;
//typedef unsigned int uintT;
//#define INT_T_MAX INT_MAX
//#define UINT_T_MAX UINT_MAX
//#endif

#endif // _PARALLEL_H
