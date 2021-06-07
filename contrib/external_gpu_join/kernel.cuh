#ifndef __OK_CUH
#define __OK_CUH



/* cut line*/
#include "TupleResult.hpp"
#include "linearprobing.h"

namespace {
    #include "cuda.h"
    #include "cuda_runtime.h"
}

bool cudaMallocu(void ** p, size_t s);
__global__ void nLJ(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);
void nestLoopJoincu(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);

#endif