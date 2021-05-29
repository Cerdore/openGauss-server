#include "kernel.cuh"
#include "stdio.h"
#include "assert.h"
#include "cuda.h"
#include "cuda_runtime.h"

cudaDeviceProp GPUprop;
/*
* TODO
* add 全局数组指针
* 申请内存
* 数据拷贝至显存
* join
* 数据拷回内存

*/

// void check()
// {
//     int NumGPUs = 0;
//     cudaGetDeviceCount(&NumGPUs);
//     if (!NumGPUs) {
//         printf("\n No GPU Device is available\n");
//         exit(EXIT_FAILURE);  // exit
//     }
//     cudaError_t cudaStatus = cudaSetDevice(0);
//     if (cudaStatus != cudaSuccess) {
//         exit(EXIT_FAILURE);  // exit
//     }
//     cudaGetDeviceProperties(&GPUprop, 0);
// }

// 32 bit Murmur3 hash
// __device__ uint32_t hash(uint32_t k)
// {
//     k ^= k >> 16;
//     k *= 0x85ebca6b;
//     k ^= k >> 13;
//     k *= 0xc2b2ae35;
//     k ^= k >> 16;
//     return k & (kHashTableCapacity - 1);
// }


// bool tupleKvcudaMallocu(Tuplekv * p, size_t t){
//     cudaError_t cudaStatus = cudaMalloc((void**)&p, t);
//     if (cudaStatus != cudaSuccess) {
//         /*call error func*/
//         return true;
//     }
//     return false;
// }
// bool cudaMemcpytoDevice(){

// }
// bool cudaMemcpytoHost(){

// }

__global__ void nLJ(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res)
{
    // int x = threadIdx.x + blockIdx.x * blockDim.x;
    // int y = threadIdx.y + blockIdx.y * blockDim.y;
    // if (x < n_a && y < n_b) {
    //     if (d_a[x].key == d_b[y].key) {
    //         (*res)->key1 = d_b[x].key;
    //         res->dval1 = d_a[x].dval;
    //         res->key2 = d_a[y].key;
    //         res->dval2 = d_a[y].dval;
    //     } else
    //         res = NULL;
    // }
    long x = threadIdx.x + blockIdx.x * blockDim.x;
    if (x >= n_b)
        return;
    for (long i = 0; i < n_a; i++) {
        if (d_b[x].key == d_a[i].key) {
            (res + i + n_a * x)->key1 = d_a[i].key;
            (res + i + n_a * x)->dval1 = d_a[i].dval;
            (res + i + n_a * x)->key2 = d_b[x].key;
            (res + i + n_a * x)->dval2 = d_b[x].dval;
        } else {
            (res + i + n_a * x)->key1 = -1;
            (res + i + n_a * x)->dval1 = -1;
            (res + i + n_a * x)->key2 = -1;
            (res + i + n_a * x)->dval2 = -1;
        }
    }
}



void nestLoopJoincu(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res)
{
    
    nLJ<<<32, 1024>>>(d_a, d_b, n_a, n_b, res);

    cudaError_t cudaStatus = cudaDeviceSynchronize();

}
