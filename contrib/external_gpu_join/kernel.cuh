#ifndef __OK_CUH
#define __OK_CUH



/* cut line*/
namespace cuda{
    #include <cuda.h>
    #include <cuda_runtime.h>

}
#include "TupleBuffer.hpp"
#include "TupleBufferQueue.hpp"
#include "ResultBuffer.hpp"

struct Tuplekv {
    int key;
    double dval;
};


__global__ void nLJ(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);
void nestLoopJoincu(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);

#endif