/*
 * @Author: cxs
 * @Date: 2021-05-14 07:08:17
 * @LastEditTime: 2021-05-16 12:33:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/gpu_join.cpp
 */

#define BEGIN_C_SPACE extern "C" {
#define END_C_SPACE }

#include <atomic>
#include <cstdio>
#include <cstring>

// Need to point extern "C"?
BEGIN_C_SPACE
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>

#include <errno.h>

#include "postgres.h"

#include <limits.h>
#include "executor/executor.h"
#include "utils/guc.h"

#include "access/htup_details.h"
#include "utils/memutils.h"
#include "miscadmin.h"
#include "catalog/pg_type.h"
#include "nodes/print.h"

/* ExecNode Module */
#include "knl/knl_variable.h"

#include "executor/executor.h"
#include "executor/nodeAgg.h"
#include "executor/nodeAppend.h"
#include "executor/nodeBitmapAnd.h"
#include "executor/nodeBitmapHeapscan.h"
#include "executor/nodeBitmapIndexscan.h"
#include "executor/nodeBitmapOr.h"
#include "executor/nodeCtescan.h"
#include "executor/nodeExtensible.h"
#include "executor/nodeForeignscan.h"
#include "executor/nodeFunctionscan.h"
#include "executor/nodeGroup.h"
#include "executor/nodeHash.h"
#include "executor/nodeHashjoin.h"
#include "executor/nodeIndexonlyscan.h"
#include "executor/nodeIndexscan.h"
#include "executor/nodeLimit.h"
#include "executor/nodeLockRows.h"
#include "executor/nodeMaterial.h"
#include "executor/nodeMergeAppend.h"
#include "executor/nodeMergejoin.h"
#include "executor/nodeModifyTable.h"
#include "executor/nodeNestloop.h"
#include "executor/nodePartIterator.h"
#include "executor/nodeRecursiveunion.h"
#include "executor/nodeResult.h"
#include "executor/nodeSeqscan.h"
#include "executor/nodeSetOp.h"
#include "executor/nodeSort.h"
#include "executor/nodeStub.h"
#include "executor/nodeSubplan.h"
#include "executor/nodeSubqueryscan.h"
#include "executor/nodeTidscan.h"
#include "executor/nodeUnique.h"
#include "executor/nodeValuesscan.h"
#include "executor/nodeWindowAgg.h"
#include "executor/nodeWorktablescan.h"
#include "executor/execStream.h"
#include "optimizer/clauses.h"
#include "optimizer/encoding.h"
#include "optimizer/ml_model.h"
#include "vecexecutor/vecstream.h"
#include "miscadmin.h"
#include "vecexecutor/vecnodecstorescan.h"
#include "vecexecutor/vecnodecstoreindexscan.h"
#include "vecexecutor/vecnodedfsindexscan.h"
#include "vecexecutor/vecnodevectorow.h"
#include "vecexecutor/vecnodecstoreindexctidscan.h"
#include "vecexecutor/vecnodecstoreindexheapscan.h"
#include "vecexecutor/vecnodecstoreindexand.h"
#include "vecexecutor/vecnodecstoreindexor.h"
#include "vecexecutor/vecnoderowtovector.h"
#include "vecexecutor/vecnodeforeignscan.h"
#include "vecexecutor/vecremotequery.h"
#include "vecexecutor/vecnoderesult.h"
#include "vecexecutor/vecsubqueryscan.h"
#include "vecexecutor/vecnodesort.h"
#include "vecexecutor/vecmodifytable.h"
#include "vecexecutor/vechashjoin.h"
#include "vecexecutor/vechashagg.h"
#include "vecexecutor/vecpartiterator.h"
#include "vecexecutor/vecappend.h"
#include "vecexecutor/veclimit.h"
#include "vecexecutor/vecsetop.h"
#ifdef ENABLE_MULTIPLE_NODES
#include "vecexecutor/vectsstorescan.h"
#include "tsdb/cache/tags_cachemgr.h"
#endif /* ENABLE_MULTIPLE_NODES */
#include "vecexecutor/vecgroup.h"
#include "vecexecutor/vecunique.h"
#include "vecexecutor/vecnestloop.h"
#include "vecexecutor/vecmaterial.h"
#include "vecexecutor/vecmergejoin.h"
#include "vecexecutor/vecwindowagg.h"
#include "utils/aes.h"
#include "utils/builtins.h"
#ifdef PGXC
#include "pgxc/execRemote.h"
#endif
#include "distributelayer/streamMain.h"
#include "pgxc/pgxc.h"
#include "securec.h"
#include "gstrace/gstrace_infra.h"
#include "gstrace/executer_gstrace.h"

// cxs
#include "external_Node.h"

// #include "TupleBuffer.hpp"
// #include "TupleBufferQueue.hpp"
// #include "ResultBuffer.hpp"
// #include "socket_lapper.hpp"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

/* Saved hook values in case of unload */
static ExecProcNode_hook_type prev_ExecProcNode = NULL;

/* GUC variables */
/* Flag to use external join module */
static bool EnableExternalJoin = false;

/* State for external join */

enum State { INIT = 0, EXEC, FINI };

void _PG_init(void)
{
    elog(DEBUG1, "----gpu join module loaded ----");

    prev_ExecProcNode = ExecProcNode_hook;
    ExecProcNode_hook = ExternalExecProcNode;
}

void _PG_fini(void)
{
    elog(DEBUG1, "-----external join module unloaded-----");
    /* Uninstall hooks. */
    ExecProcNode_hook = prev_ExecProcNode;
}

/* main */
TupleTableSlot* ExternalExecProcNode(PlanState* ps)
{
    TupleTableSlot* result = NULL;

    CHECK_FOR_INTERRUPTS();
    MemoryContext old_context;

    /* Response to stop or cancel signal. */
    if (unlikely(executorEarlyStop())) {
        return NULL;
    }

    /* Switch to Node Level Memory Context */
    old_context = MemoryContextSwitchTo(node->nodeContext);

    if (node->chgParam != NULL) { /* something changed */
        ExecReScan(node);         /* let ReScan handle this */
    }

    if (node->instrument != NULL) {
        InstrStartNode(node->instrument);
    }

    if (unlikely(planstate_need_stub(node))) {
        result = ExecProcNodeStub(node);
    } else {
        result = external_ExecProcNodeByType(node);
    }

    if (node->instrument != NULL) {
        ExecProcNodeInstr(node, result);

        MemoryContextSwitchTo(old_context);

        node->ps_rownum++;

        return result;
    }
}

TupleTableSlot* external_ExecProcNodeByType(PlanState* node)
{
    TupleTableSlot* result = NULL;
    switch (nodeTag(node)) {
        case T_ResultState:
            return ExecResult((ResultState*)node);
        case T_ModifyTableState:
        case T_DistInsertSelectState:
            return ExecModifyTable((ModifyTableState*)node);
        case T_AppendState:
            return ExecAppend((AppendState*)node);
        case T_MergeAppendState:
            return ExecMergeAppend((MergeAppendState*)node);
        case T_RecursiveUnionState:
            return ExecRecursiveUnion((RecursiveUnionState*)node);
        case T_SeqScanState:
            return ExecSeqScan((SeqScanState*)node);
        case T_IndexScanState:
            return ExecIndexScan((IndexScanState*)node);
        case T_IndexOnlyScanState:
            return ExecIndexOnlyScan((IndexOnlyScanState*)node);
        case T_BitmapHeapScanState:
            return ExecBitmapHeapScan((BitmapHeapScanState*)node);
        case T_TidScanState:
            return ExecTidScan((TidScanState*)node);
        case T_SubqueryScanState:
            return ExecSubqueryScan((SubqueryScanState*)node);
        case T_FunctionScanState:
            return ExecFunctionScan((FunctionScanState*)node);
        case T_ValuesScanState:
            return ExecValuesScan((ValuesScanState*)node);
        case T_CteScanState:
            return ExecCteScan((CteScanState*)node);
        case T_WorkTableScanState:
            return ExecWorkTableScan((WorkTableScanState*)node);
        case T_ForeignScanState:
            return ExecForeignScan((ForeignScanState*)node);
        case T_ExtensiblePlanState:
            return ExecExtensiblePlan((ExtensiblePlanState*)node);
            /*
             * join nodes
             */
        case T_NestLoopState:
            return exeternal_ExecNestLoop((NestLoopState*)node);
        case T_MergeJoinState:
            return ExecMergeJoin((MergeJoinState*)node);
        case T_HashJoinState:
            return ExecHashJoin((HashJoinState*)node);

            /*
             * partition iterator node
             */
        case T_PartIteratorState:
            return ExecPartIterator((PartIteratorState*)node);
            /*
             * materialization nodes
             */
        case T_MaterialState:
            return ExecMaterial((MaterialState*)node);
        case T_SortState:
            return ExecSort((SortState*)node);
        case T_GroupState:
            return ExecGroup((GroupState*)node);
        case T_AggState:
            return ExecAgg((AggState*)node);
        case T_WindowAggState:
            return ExecWindowAgg((WindowAggState*)node);
        case T_UniqueState:
            return ExecUnique((UniqueState*)node);
        case T_HashState:
            return ExecHash();
        case T_SetOpState:
            return ExecSetOp((SetOpState*)node);
        case T_LockRowsState:
            return ExecLockRows((LockRowsState*)node);
        case T_LimitState:
            return ExecLimit((LimitState*)node);
        case T_VecToRowState:
            return ExecVecToRow((VecToRowState*)node);
#ifdef PGXC
        case T_RemoteQueryState:
            t_thrd.pgxc_cxt.GlobalNetInstr = node->instrument;
            result = ExecRemoteQuery((RemoteQueryState*)node);
            t_thrd.pgxc_cxt.GlobalNetInstr = NULL;
            return result;
#endif
        case T_StreamState:
            t_thrd.pgxc_cxt.GlobalNetInstr = node->instrument;
            result = ExecStream((StreamState*)node);
            t_thrd.pgxc_cxt.GlobalNetInstr = NULL;
            return result;

        default:
            ereport(ERROR,
                (errmodule(MOD_EXECUTOR),
                    errcode(ERRCODE_UNRECOGNIZED_NODE_TYPE),
                    errmsg("unrecognized node type: %d when executing executor node.", (int)nodeTag(node))));
            return NULL;
    }
}

END_C_SPACE