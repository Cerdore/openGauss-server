#include "kernel.cuh"
#include "stdio.h"
#include "assert.h"

#define unsigned long ul
#define unsigned int ui
cudaError_t cudaStatus[10];
cudaDeviceProp GPUprop;
ul SupportedKBlocks, SupportedMBlocks, MaxTherPerBlk;
char SupportedBlocks[100];

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

std::size_t* p_size[2];
Datum* GpuColBuffer[10];

void moveTupletoGPU(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);

    /* if TupleBufferQueue is finalized, TupleBufferQueue->getLength() returns -1 */
    int cnt = 0;
    while (ejs->tbq.getLength() >= 0) {

        ColBuffer* tb = ejs->tbq.pop();
        std::size_t size;

        /* wait for scan completion */
        //        pthread_testcancel();
        if (tb == NULL) {
            ::usleep(1);
            continue;
        }
        size = tb->getContentSize();
        /* send tuple buffer size to external */
        // sendStrong(sock, &size, sizeof(size));

        int ncol = tb->totalAttr;
        for (int i = 0; i < ncol; i++) {
            // for (int j = 0; j < col[i].size(); j++) {
            //     /*copy data to device*/
            // }
            
            /*暂时忽略类型， Datum 貌似可用， 参见heaptuple.cpp*/
            cudaStatus[i] = cudaMalloc(&GpuColBuffer[i], col[i].size());
            if (cudaStatus[i]) {
            }
            cuda
        }

        /*build hash table here?

        然后在Exec：   join 的时候 一条一条探测。
        */
        cudaStatus = cudaMalloc((void**)&p_size[cnt], sizeof(std::size_t));

        /* send tuples to external */
        //    sendStrong(sock, tb->getBufferPointer(), size);

        cudaStatus2 = cudaMalloc((void**)&GpuBuffer[cnt], sizeof(size));
        TupleBuffer::destructor(tb);
        cnt++;
    }
    return NULL;
}

void moveTupletoHost()
{}
