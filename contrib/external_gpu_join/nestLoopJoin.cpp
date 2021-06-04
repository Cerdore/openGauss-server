/*
 * @Author: your name
 * @Date: 2021-06-04 11:15:49
 * @LastEditTime: 2021-06-04 14:22:27
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/nestLoopJoin.cpp
 */
#include "externalJoin.hpp"


/* --- NestLoopJoin --- */
void nestLoopJoin(void* arg)
{
    ereport(LOG,(errmsg("------------------Begin: nestLoopJoin")));

    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    
    long allResNumSize = ejs->T_size[0] * ejs->T_size[1] * sizeof(Result);
    ereport(LOG,(errmsg("allResNumSize is %ld\n", allResNumSize)));    
    
    ejs->res = (struct Result*)malloc(ejs->T_size[0] * ejs->T_size[1] * sizeof(Result));
    cudaError_t cudaStatus = cudaMalloc((void**)&ejs->d_res,ejs->T_size[0] * ejs->T_size[1] * sizeof(Result) );
    
   // ejs->d_res = myCudaMalloc<Result>( ejs->T_size[0] * ejs->T_size[1] * sizeof(Result) );


    nestLoopJoincu(ejs->d_tuple[0], ejs->d_tuple[1], ejs->T_size[0], ejs->T_size[1], ejs->d_res);

    // cudaStatus = 
    // cudaDeviceSynchronize();
    ereport(LOG,(errmsg("------------------End: nestLoopJoin")));

}

void moveTupletoGPU(void* arg)
{
    ereport(LOG, (errmsg("------------------BEGIN: moveTupletoGPU")));
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);

    /* if TupleBufferQueue is finalized, TupleBufferQueue->getLength() returns -1 */
    int cnt = 0;
    struct Tuplekv* d_tuple[2];

    ereport(LOG, (errmsg("tbq.getlength():  %d\n", ejs->tbq.getLength())));

    while (ejs->tbq.getLength() > 0) {

        TupleBuffer* tb = ejs->tbq.pop();

        std::size_t size;
        /* wait for scan completion */
        // pthread_testcancel();
        // if (tb == NULL) {
        //     ::usleep(1);
        //     continue;
        // }
        size = tb->getContentSize();

        ereport(LOG, (errmsg("tuple num is %ld\n size is %lu\n", tb->tupleNum, size)));

        cudaError_t cudaStatus = cudaMalloc((void**)&d_tuple[cnt], tb->tupleNum * sizeof(Tuplekv));
        // if (!cudaStatus) {
        //     /*call error func*/
        // }

        cudaStatus = cudaMemcpy(d_tuple[cnt], tb->getBufferPointer(), size, cudaMemcpyHostToDevice);

        // if (cudaStatus != cudaSuccess) {
        //     /*call error func*/
        // }

        ejs->d_tuple[cnt] = d_tuple[cnt];

        ejs->T_size[cnt] = tb->tupleNum;
        TupleBuffer::destructor(tb);

        cnt++;
    }
    ereport(LOG, (errmsg("------------------End: moveTupletoGPU")));
    //    return NULL;
}

void moveResulttoHost(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    cudaError_t cudaStatus =
        cudaMemcpy(ejs->res, ejs->d_res, ejs->T_size[0] * ejs->T_size[1] * sizeof(Result), cudaMemcpyDeviceToHost);
    // cudaMemcpyToHost(ejs->res, ejs->d_res, ejs->T_size[0] * ejs->T_size[1] * sizeof(Result), cudaMemcpyDeviceToHost);

    // if (cudaStatus != cudaSuccess) {
    //     /*call error func*/
    // }
    long sum = 0;
    for (long i = 0; i < ejs->T_size[0] * ejs->T_size[1]; i++) {

        if ((ejs->res + i)->key1 == 0 && ((ejs->res + i)->key2 == 0))
            continue;

        if ((ejs->res + i)->key1 != -1) {

            // ereport(LOG,(errmsg("Result is   %d %f %d %f\n",(ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res +
            // i)->key2, (ejs->res + i)->dval2 )));
            sum++;
            int k1 = (ejs->res + i)->key1;
            double d1 = (ejs->res + i)->dval1;
            int k2 = (ejs->res + i)->key2;
            double d2 = (ejs->res + i)->dval2;
            // ejs->prb.put((ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res + i)->key2, (ejs->res + i)->dval2);
            ejs->prb.put(k1, d1, k2, d2);
        }
    }
    ereport(LOG, (errmsg("count(Result) is   %d\n", sum)));

    /*put result to host, then put it to queue*/
}

/* --- NestLoopJoin --- */





/* --- Hash Join --- */