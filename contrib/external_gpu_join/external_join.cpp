/*
 * @Author: your name
 * @Date: 2021-05-14 03:06:57
 * @LastEditTime: 2021-05-26 08:55:33
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/external_join.cpp
 */
/*-------------------------------------------------------------------------
 *
 * external_join.c
 *
 *
 * Copyright (c) 2016, Ryuya Mitsuhashi
 *
 * IDENTIFICATION
 *	  contrib/external_join/external_join.cpp
 *
 *-------------------------------------------------------------------------
 */
// #define BEGIN_C_SPACE extern "C" {
// #define END_C_SPACE }

/* We cannot use new/delete because query cancelation may cause memory leak. */
/* We cannot use std::thread because namespace problem with C linkage [-] */
// #include <iostream>
// #include <thread>
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
//BEGIN_C_SPACE


#include "kernel.cuh"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

/* Saved hook values in case of unload */
static ExecProcNode_hook_type prev_ExecProcNode = NULL;

/* GUC variables */
/* Flag to use external join module */
static bool EnableExternalJoin = true;

/* Initialized pthread_t */
// static pthread_t InitialThread;
/* Holds currently running pthread_t, this is used when a query is cancelled */
// static pthread_t PreviousThread;

/* State for external join */
/* ExternalExecProcNode() uses PlanState->initPlan field to hold execution state. This is scamped design. */
/* If initPlan is used by original query process, ExternalExecProcNode() cannot work fine. */
static constexpr NodeTag T_ExternalJoin = static_cast<NodeTag>(65535);
enum State { INIT = 0, EXEC, FINI };
struct ExternalJoinState {
    NodeTag type;
    State state;

    /* socket to communicate with external process */
    // int sock;
    /* tuple sendeng or result receiving thread */
    pthread_t thread;

    /* send buffer queue */
    TupleBufferQueue tbq;
    // ColBufferQueue tbq;  // cxs: may need to save the desc for form tuple

    /* result buffer: double buffered */
    //    DoubleResultBuffer drb;
    /* result buffer which result processing thread currently handles */
    ResultBuffer prb;
    /* offset(cursor) to scan result buffer */
    // std::size_t poffset;
    /* base offset when an attribute sticks out of buffer */
    // std::size_t pbase;

    struct Tuplekv* d_tuple[2];  // poniter for gpu
    struct Result* d_res;      // pointer for host
    struct Result* res;

    int T_size[2];  // size of tuple length

    /* size of content in result buffer */
    long psize;
};

static ExternalJoinState* makeExternalJoinState(void);
static void FreeExternalJoinState(ExternalJoinState* ejs);
static ExternalJoinState* GetExternalJoinState(List* initPlan);
static ExternalJoinState* SetExternalJoinState(List** initPlan, ExternalJoinState* ejs);

/* main function to be called */
static TupleTableSlot* ExternalExecProcNode(PlanState* ps);

/* external join executor */
static ExternalJoinState* InitExternalJoin(PlanState* ps);
static void EndExternalJoin(PlanState* ps);
static TupleTableSlot* ExecExternalJoin(PlanState* ps);

/* tuple scanner */
static void ScanTuple(PlanState* node, ExternalJoinState* ejs);
/* tuple sender */
static void* SendTupleToExternal(void* arg);
/* result receiver */
static void* ReceiveResultFromExternal(void* arg);

static uint64_t bytesExtract(uint64_t x, int n);
/* thread cancelling function */
static void CancelPreviousSessionThread(void);
static void SetCurrentSessionThread(pthread_t thread);

/*
 * Module load callback
 */
void _PG_init(void)
{
    /* Define custom GUC variables. */
    DefineCustomBoolVariable("external_join.enable",
        "Selects whether external join is enabled.",
        NULL,
        &EnableExternalJoin,
        false,
        PGC_USERSET,
        0,
        NULL,
        NULL,
        NULL);

    EmitWarningsOnPlaceholders("external_join");

    elog(DEBUG1, "----- external join module loaded -----");
    /* Install hooks. */
    prev_ExecProcNode = ExecProcNode_hook;
    ExecProcNode_hook = ExternalExecProcNode;
}

/*
 * Module unload callback
 */
void _PG_fini(void)
{
    elog(DEBUG1, "-----external join module unloaded-----");
    /* Uninstall hooks. */
    ExecProcNode_hook = prev_ExecProcNode;
}

static inline ExternalJoinState* makeExternalJoinState(void)
{
    ExternalJoinState* ejs;

    ejs = static_cast<ExternalJoinState*>(palloc(sizeof(*ejs)));
    ejs->type = T_ExternalJoin;
    ejs->state = State::INIT;

    ejs->tbq.init();
    ejs->prb.init();

    ejs->T_size[0] = 0;
    ejs->T_size[1] = 0;
    //   ejs->drb.init();
    // ejs->prb = ejs->drb.getCurrentResultBuffer();
    // ejs->poffset = ResultBuffer::BUFSIZE;
    // ejs->pbase = 0;

    // ejs->psize = 0;

    return ejs;
}

static inline void FreeExternalJoinState(ExternalJoinState* ejs)
{
    ejs->tbq.fini();
   // ejs->drb.fini();
    pfree(ejs);
}

static inline ExternalJoinState* GetExternalJoinState(List* initPlan)
{
    return reinterpret_cast<ExternalJoinState*>(initPlan);
}

static inline ExternalJoinState* SetExternalJoinState(List** initPlan, ExternalJoinState* ejs)
{
    *reinterpret_cast<ExternalJoinState**>(initPlan) = ejs;
    return ejs;
}


void moveTupletoGPU(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);

    /* if TupleBufferQueue is finalized, TupleBufferQueue->getLength() returns -1 */
    int cnt = 0;
    struct Tuplekv* d_tuple[2];
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

        cuda::cudaError_t cudaStatus = cuda::cudaMalloc((void**)&d_tuple[cnt], tb->tupleNum * sizeof(Tuplekv));
        if (cudaStatus != cuda::cudaSuccess) {
            /*call error func*/
        }

        cudaStatus = cuda::cudaMemcpy(d_tuple[cnt], tb->getBufferPointer(), size, cuda::cudaMemcpyHostToDevice);
        if (cudaStatus != cuda::cudaSuccess) {
            /*call error func*/
        }

        ejs->d_tuple[cnt] = d_tuple[cnt];
        ejs->T_size[cnt] = tb->tupleNum;
        TupleBuffer::destructor(tb);

        cnt++;
    }
//    return NULL;
}

void moveResulttoHost(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    cuda::cudaError_t cudaStatus =
        cuda::cudaMemcpy(ejs->res, ejs->d_res, ejs->T_size[0] * ejs->T_size[1] * sizeof(Result), cuda::cudaMemcpyDeviceToHost);

    if (cudaStatus != cuda::cudaSuccess) {
        /*call error func*/
    }
    for (long i = 0; i < ejs->T_size[0] * ejs->T_size[1]; i++) {
        if ((ejs->res + i)->key1 != -1) {
            ejs->prb.put((ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res + i)->key2, (ejs->res + i)->dval2);
        }
    }
    /*put result to host, then put it to queue*/
}

void nestLoopJoin(void* arg, struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* d_res)
{
    
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    //    struct Result *res, *d_res;
    ejs->res = (struct Result*)malloc(ejs->T_size[0] * ejs->T_size[1] * sizeof(Result));
    cuda::cudaError_t cudaStatus = cuda::cudaMalloc((void**)&ejs->d_res, ejs->T_size[0] * ejs->T_size[1] * sizeof(Result));


    nestLoopJoincu(d_a, d_b, n_a, n_b, d_res);

    cudaStatus = cuda::cudaDeviceSynchronize();

}


/* main */
TupleTableSlot* ExternalExecProcNode(PlanState* ps)
{
    TupleTableSlot* tts = NULL;
    ExternalJoinState* ejs = GetExternalJoinState(ps->initPlan);

    elog(DEBUG5, "*****ExternalExecProcNode()*****");
    if (EnableExternalJoin == false)
        return ExecProcNode(ps);

    if (!(ejs == NULL || (ejs->state >= State::INIT && ejs->state <= State::FINI))) {
        ereport(ERROR,
            (errcode(ERRCODE_EXTERNAL_ROUTINE_INVOCATION_EXCEPTION),
                errmsg("PlanState->initPlan is set, cannot apply external join to this query.")));
    }

    for (;;) {
        if (ejs == NULL) {
            elog(DEBUG5, "BEGIN: Init");
            ejs = InitExternalJoin(ps);

            //            ejs->poffset = 0;
            // while ((ejs->psize = ejs->prb.getContentSize()) == 0)
            //     ::usleep(1);

            nestLoopJoin(ejs, ejs->d_tuple[0], ejs->d_tuple[1], ejs->T_size[0], ejs->T_size[1], ejs->d_res);
            moveResulttoHost(ejs);
            ejs->state = State::EXEC;
            elog(DEBUG5, "END: Init");
        } else if (ejs->state == State::EXEC) {

            /*开始执行之后，第一次返回一个tuple，然后break，返回单个tuple；下次再调用ExternalExecProcNode还是进入EXEC*/
            elog(DEBUG5, "BEGIN: Exec");

            tts = ExecExternalJoin(ps);
            if (tts == NULL) {
                ejs->state = State::FINI;
                continue;
            }

            elog(DEBUG5, "END: Exec");
            break;
        } else if (ejs->state == State::FINI) {
            elog(DEBUG5, "END: Begin");

            /* join result receiving thread */
            // ::pthread_join(ejs->thread, NULL);
            // CancelPreviousSessionThread();

            EndExternalJoin(ps);

            elog(DEBUG5, "END: End");
            break;
        } else {
            ereport(
                FATAL, (errcode(ERRCODE_UNDEFINED_PSTATEMENT), errmsg("unknown state in ExternalExecProcNode()\n")));
            break;
        }
    };

    return tts;
}

static inline ExternalJoinState* InitExternalJoin(PlanState* ps)
{
    cuda::cudaError_t cudaStatus = cuda::cudaSetDevice(0);
    ExternalJoinState* ejs = SetExternalJoinState(&ps->initPlan, makeExternalJoinState());

    ScanTuple(ps, ejs);

    moveTupletoGPU(ejs);

    while (ejs->tbq.getLength() > 0)
        ::usleep(1);
    ejs->tbq.fini();

    ExecAssignExprContext(ps->state, ps);
    /* init result tuple */
    ExecInitResultTupleSlot(ps->state, ps);
    ExecAssignResultTypeFromTL(ps);
    ExecAssignProjectionInfo(ps, NULL);
    ps->ps_TupFromTlist = false;

    // ::pthread_join(ejs->thread, NULL);
    // CancelPreviousSessionThread();

    return ejs;
}

static inline void EndExternalJoin(PlanState* ps)
{
    ExternalJoinState* ejs = GetExternalJoinState(ps->initPlan);

    ExecFreeExprContext(ps);
    ExecClearTuple(ps->ps_ResultTupleSlot);
    //    ::close(ejs->sock);
    FreeExternalJoinState(ejs);
}

/* prev 应该保存的是上次最后的位置
如果 prev 不是 size 的倍数，就从下个整数倍的size位置开始
是的话，返回当前位置
*/
static inline std::size_t GetAlignedOffset(std::size_t prev, std::size_t size)
{
    return (prev % size) ? (prev / size + 1) * size : prev;
}

static inline TupleTableSlot* ExecExternalJoin(PlanState* ps)
{
    ExternalJoinState* ejs = GetExternalJoinState(ps->initPlan);
    TupleTableSlot* tts = ps->ps_ProjInfo->pi_slot;
    TupleDesc td = tts->tts_tupleDescriptor;

    if (ejs->prb.index <= 0)
        return NULL;
    struct Result* res = ejs->prb.get();
    void* tptr = static_cast<void*>(res);
    if (res == NULL)
        return NULL;

    /* check cancel request */
    CHECK_FOR_INTERRUPTS();

    /* fill result tuple */
    ExecClearTuple(tts);

    // void* re = static_cast<void*> res;
    std::size_t off = 0;
    for (int col = 0; col < td->natts; col++) {
        void* ptr;
        switch (td->attrs[col]->atttypid) {
            case INT8OID:
            case FLOAT8OID:
                // ejs->poffset = GetAlignedOffset(ejs->poffset, sizeof(double));
                ptr = static_cast<void*>(static_cast<char*>(tptr) + off);
                off += sizeof(double);
                //                ejs->poffset += sizeof(double);
                break;
            case INT4OID:
            case FLOAT4OID:
            case OIDOID:
                ptr = static_cast<void*>(static_cast<char*>(tptr) + off);
                off += sizeof(float);
                break;
            case INT2OID:
                ptr = static_cast<void*>(static_cast<char*>(tptr) + off);
                off += sizeof(short);
                break;
            case BOOLOID:
                ptr = static_cast<void*>(static_cast<char*>(tptr) + off);
                off += sizeof(bool);
                break;
            default:
                ereport(ERROR,
                    (errcode(ERRCODE_EXTERNAL_ROUTINE_INVOCATION_EXCEPTION),
                        errmsg("unsupported result type %d.\n", td->attrs[col]->atttypid)));
        };

        /* put column data to result tuple */
        switch (td->attrs[col]->atttypid) {
            case BOOLOID:
                tts->tts_values[col] = BoolGetDatum(*reinterpret_cast<bool*>(ptr));
                break;
            case INT8OID:
                tts->tts_values[col] = Int64GetDatum(*reinterpret_cast<int64_t*>(ptr));
                break;
            case INT2OID:
                tts->tts_values[col] = Int16GetDatum(*reinterpret_cast<int16_t*>(ptr));
                break;
            case INT4OID:
                tts->tts_values[col] = Int32GetDatum(*reinterpret_cast<int32_t*>(ptr));
                break;
            case FLOAT4OID:
                tts->tts_values[col] = Float4GetDatum(*reinterpret_cast<float*>(ptr));
                break;
            case FLOAT8OID:
                tts->tts_values[col] = Float8GetDatum(*reinterpret_cast<double*>(ptr));
                break;
            case OIDOID:
                tts->tts_values[col] = ObjectIdGetDatum(*reinterpret_cast<uint32_t*>(ptr));
                break;
        };
    }
    /* set null flags to false */
    ::bzero(static_cast<void*>(tts->tts_isnull), sizeof(bool) * td->natts);

    return ExecStoreVirtualTuple(tts);
}

static inline void ScanTuple(PlanState* node, ExternalJoinState* ejs)
{
    if (node == NULL)
        return;
    if (node->type >= T_ScanState &&
        node->type <= T_ForeignScanState) {  // scandate ranges T_ScanState from T_ForeignScanState
        TupleBuffer* tb = TupleBuffer::constructor();
        // ColBuffer* tb = ColBuffer::constructor();

        elog(DEBUG5, "----- ScanNode [%p] -----", node);
        elog_node_display(DEBUG5, "ScanNode->plan", node->plan, true);

        /* scan tuple */
        for (TupleTableSlot* tts = ExecProcNode(node); tts->tts_isempty == false; tts = ExecProcNode(node)) {
            /* copy tuple to buffer */
            tb->putTuple(tts);
            ResetExprContext(node->ps_ExprContext);
        }

        /* scan is complete for this ScanNode, put buffer into queue */
        ejs->tbq.push(tb);
    }
    /* look for other ScanNode */
    ScanTuple(outerPlanState(node), ejs);
    ScanTuple(innerPlanState(node), ejs);
}

static inline uint64_t bytesExtract(uint64_t x, int n)
{
    static constexpr uint64_t TABLE[] = {0x00000000000000FF,
        0x000000000000FFFF,
        0x0000000000FFFFFF,
        0x00000000FFFFFFFF,
        0x000000FFFFFFFFFF,
        0x0000FFFFFFFFFFFF,
        0x00FFFFFFFFFFFFFF,
        0xFFFFFFFFFFFFFFFF};
    return x & TABLE[n];
}
