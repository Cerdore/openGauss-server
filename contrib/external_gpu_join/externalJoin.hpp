/*
 * @Author: your name
 * @Date: 2021-06-04 11:16:29
 * @LastEditTime: 2021-06-07 11:32:19
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/externalJoin.hpp
 */
#include <atomic>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>

#include <errno.h>

#include "postgres.h"

#include <limits.h>
#include "executor/executor.h"
#include "executor/tuptable.h"
#include "utils/guc.h"

#include "access/htup.h"

#include "utils/memutils.h"
#include "miscadmin.h"
#include "catalog/pg_type.h"
#include "nodes/print.h"
// BEGIN_C_SPACE

#include "TupleResult.hpp"
#include "TupleBuffer.hpp"
#include "TupleBufferQueue.hpp"
#include "ResultBuffer.hpp"

#include "kernel.cuh"


/* State for external join */
/* ExternalExecProcNode() uses PlanState->initPlan field to hold execution state. This is scamped design. */
/* If initPlan is used by original query process, ExternalExecProcNode() cannot work fine. */
static constexpr NodeTag T_ExternalJoin = static_cast<NodeTag>(65535);
enum State { INIT = 0, EXEC, FINI };
enum exJoinState { nlJ = 0, hashJ };
struct ExternalJoinState {
    NodeTag type;
    State state;
    exJoinState joinState;
    /* socket to communicate with external process */
    // int sock;
    /* tuple sendeng or result receiving thread */
    pthread_t thread;

    /* send buffer queue */
    TupleBufferQueue tbq;  // bytebuffer

    // ColBufferQueue tbq;  // cxs: may need to save the desc for form tuple

    /* result buffer: double buffered */
    //    DoubleResultBuffer drb;
    /* result buffer which result processing thread currently handles */
    ResultBuffer prb;

    struct Tuplekv* d_tuple[2];  // poniter for gpu

    struct Result* d_res;  // pointer for host
    struct Result* res;

    long T_size[2];  // size of tuple length

    /* size of content in result buffer */
    long psize;
};


/*for NestLoop*/
void nestLoopJoin(void *args);
void moveTupletoGPU(void* arg);
void moveResulttoHost(void* arg);


/*for hasjJoin*/
void insetTupleToTable(void *args);