#ifndef __OK_CUH
#define __OK_CUH



/* cut line*/
#include "TupleResult.hpp"
namespace {
    #include "cuda.h"
    #include "cuda_runtime.h"
}
// namespace {
// template< typename T >
// T* myCudaMalloc( int size )
// {
//     T* loc          = NULL;
//     const int space = size;// * sizeof( T );
//     cudaMalloc( &loc, space );
//     return loc;
// }
// template <typename T>
// T* cudaMemcpyToDevice(const T* device_array, const T* host_array, std::size_t count)
// {
//     // some static_assert for allowed types: pod and built-in.
// //    T* device_array = nullptr;
//   //  gpuErrchk(cudaMalloc(&device_array, count * sizeof(T)));  
//     gpuErrchk(cudaMemcpy(device_array, host_array, count, cudaMemcpyHostToDevice));
//     return device_array;
// }

// template <typename T>
// T* cudaMemcpyToHost(const T* device_array, const T* host_array, std::size_t count)
// {
//     // some static_assert for allowed types: pod and built-in.
// //    T* device_array = nullptr;
//   //  gpuErrchk(cudaMalloc(&device_array, count * sizeof(T)));  
//     gpuErrchk(cudaMemcpy(device_array, host_array, count, cudaMemcpyDeviceToHost));
//     return device_array;
// }

// }


bool cudaMallocu(void ** p, size_t s);
__global__ void nLJ(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);
void nestLoopJoincu(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res);

#endif