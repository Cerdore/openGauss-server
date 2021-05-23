#include "kernel.cuh"
#include "stdio.h"
#include "assert.h"

#define unsigned long ul
#define unsigned int ui
cudaError_t cudaStatus[10];
cudaDeviceProp GPUprop;
ul SupportedKBlocks, SupportedMBlocks, MaxTherPerBlk;
char SupportedBlocks[100];

struct Tuple {
    int key;
    double dval;
};

struct Result {
    int key1;
    double dval1;
    int key2;
    double dval2;
};

/*
* TODO
* add 全局数组指针
* 申请内存
* 数据拷贝至显存
* join
* 数据拷回内存

*/

void check()
{
    int NumGPUs = 0;
    cudaGetDeviceCount(&NumGPUs);
    if (!NumGPUs) {
        printf("\n No GPU Device is available\n");
        exit(EXIT_FAILURE);  // exit
    }
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        exit(EXIT_FAILURE);  // exit
    }
    cudaGetDeviceProperties(&GPUprop, 0);
    SupportedKBlocks = (ui)GPUprop.maxGridSize[0] * (ui)GPUprop.maxGridSize[1] * (ui)GPUprop.maxGridSize[2] / 1024;
    SupportedMBlocks = SupportedKBlocks / 1024;

    sprintf(SupportedBlocks,
        "%u %c",
        (SupportedMBlocks >= 5) ? SupportedMBlocks : SupportedKBlocks,
        (SupportedMBlocks >= 5) ? 'M' : 'K');
}

// 32 bit Murmur3 hash
__device__ uint32_t hash(uint32_t k)
{
    k ^= k >> 16;
    k *= 0x85ebca6b;
    k ^= k >> 13;
    k *= 0xc2b2ae35;
    k ^= k >> 16;
    return k & (kHashTableCapacity - 1);
}

void moveTupletoGPU(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);

    /* if TupleBufferQueue is finalized, TupleBufferQueue->getLength() returns -1 */
    int cnt = 0;
    struct* Tuple d_tuple[2];
    while (ejs->tbq.getLength() >= 0) {

        TupleBuffer* tb = ejs->tbq.pop();

        std::size_t size;

        /* wait for scan completion */
        // pthread_testcancel();
        // if (tb == NULL) {
        //     ::usleep(1);
        //     continue;
        // }
        size = tb->getContentSize();

        cudaError_t cudaStatus = cudaMalloc((void**)&d_tuple[cnt], tb->tupleNum * sizeof(Tuple));
        if (cudaStatus != cudaSuccess) {
            /*call error func*/
        }

        cudaStatus = cudaMemcpy(d_tuple[cnt], tb->getBufferPointer(), size, cudaMemcpyHostToDevice);
        if (cudaStatus != cudaSuccess) {
            /*call error func*/
        }

        ejs->d_tuple[cnt] = d_tuple[cnt];
        ejs->T_size[cnt] = tb->tupleNum;
        TupleBuffer::destructor(tb);

        cnt++;
    }
    return NULL;
}

__global__ void nLJ(struct Tuple* d_a, struct Tuple* d_b, long n_a, long n_b, struct Result* res)
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
    int x = threadIdx.x + blockIdx.x * blockDim.x;
    if (x >= n_b)
        return;
    for (int i = 0; i < n_a; i++) {
        if (d_b[x].key == d_a[i].key) {
            (res + i * x)->key1 = d_a[i].key;
            (res + i * x)->dval1 = d_a[i].dval;
            (res + i * x)->key2 = d_b[x].key;
            (res + i * x)->dval2 = d_b[y].dval;
        } else {
            (res + i * x)->key1 = -1;
            (res + i * x)->dval1 = -1;
            (res + i * x)->key2 = -1;
            (res + i * x)->dval2 = -1;
        }
    }
}

void moveResulttoHost(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    cudaError_t cudaStatus =
        cudaMemcpy(ejs->res, ejs->d_res, ejs->T_size[0] * ejs->T_size[1] * sizeof(Result), cudaMemcpyDeviceToHost);

    if (cudaStatus != cudaSuccess) {
        /*call error func*/
    }
    for (long i = 0; i < T_size[0] * T_size[1]; i++) {
        if ((ejs->res + i)->key1 != -1) {
            ejs->prb->put((ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res + i)->key2, (ejs->res + i)->dval2);
        }
    }
    /*put result to host, then put it to queue*/
}

void nestLoopJoin(void* arg, struct Tuple* d_a, struct Tuple* d_b, long n_a, long n_b, struct Result* res)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    //    struct Result *res, *d_res;
    ejs->res = (struct Result*)malloc(ejs->T_size[0] * ejs->T_size[1] * sizeof(Result));
    cudaError_t cudaStatus = cudaMalloc((void**)&ejs->d_res[cnt], ejs->T_size[0] * ejs->T_size[1] * sizeof(Result));

    nlJ<<<32, 1024>>>(d_a, d_b, n_a, n_b, d_res);

    cudaStatus = cudaDeviceSynchronize();

    // moveResulttoHost(ejs);
}
