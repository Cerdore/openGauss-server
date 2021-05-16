
#include <atomic>
#include <cstdio>
#include <cstring>

// Need to point extern "C"?

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

#include "external_Node.h"

TupleTableSlot* exeternal_ExecNestLoop(NestLoopState* node)
{
    TupleTableSlot* outer_tuple_slot = NULL;
    TupleTableSlot* inner_tuple_slot = NULL;

    // cxs
    /*try to move the tuple to gpu, need to add the info of gpu_tuple
     *
     * 下方代码是 先获取一个 outer tuple and then a inner tuple
     *
     */

    ListCell* lc = NULL;

    /*
     * get information from the node
     */
    ENL1_printf("getting info from node");

    NestLoop* nl = (NestLoop*)node->js.ps.plan;
    List* joinqual = node->js.joinqual;
    List* otherqual = node->js.ps.qual;
    PlanState* outer_plan = outerPlanState(node);
    PlanState* inner_plan = innerPlanState(node);
    ExprContext* econtext = node->js.ps.ps_ExprContext;

    /*
     * Check to see if we're still projecting out tuples from a previous join
     * tuple (because there is a function-returning-set in the projection
     * expressions).  If so, try to project another one.
     */
    if (node->js.ps.ps_TupFromTlist) {
        ExprDoneCond is_done;

        TupleTableSlot* result = ExecProject(node->js.ps.ps_ProjInfo, &is_done);
        if (is_done == ExprMultipleResult)
            return result;
        /* Done with that source tuple... */
        node->js.ps.ps_TupFromTlist = false;
    }

    /*
     * Reset per-tuple memory context to free any expression evaluation
     * storage allocated in the previous tuple cycle.  Note this can't happen
     * until we're done projecting out tuples from a join tuple.
     */
    ResetExprContext(econtext);

    /*
     * Ok, everything is setup for the join so now loop until we return a
     * qualifying join tuple.
     */
    ENL1_printf("entering main loop");

    if (node->nl_MaterialAll) {
        MaterialAll(inner_plan);
        node->nl_MaterialAll = false;
    }

    for (;;) {
        /*
         * If we don't have an outer tuple, get the next one and reset the
         * inner scan.
         */
        if (node->nl_NeedNewOuter) {
            ENL1_printf("getting new outer tuple");
            outer_tuple_slot = ExternalExecProcNode(outer_plan);
            /*
             * if there are no more outer tuples, then the join is complete..
             */
            if (TupIsNull(outer_tuple_slot)) {
                ExecEarlyFree(inner_plan);
                ExecEarlyFree(outer_plan);

                EARLY_FREE_LOG(elog(LOG,
                    "Early Free: NestLoop is done "
                    "at node %d, memory used %d MB.",
                    (node->js.ps.plan)->plan_node_id,
                    getSessionMemoryUsageMB()));

                ENL1_printf("no outer tuple, ending join");

                return NULL;
            }

            ENL1_printf("saving new outer tuple information");
            econtext->ecxt_outertuple = outer_tuple_slot;
            node->nl_NeedNewOuter = false;
            node->nl_MatchedOuter = false;

            /*
             * fetch the values of any outer Vars that must be passed to the
             * inner scan, and store them in the appropriate PARAM_EXEC slots.
             */
            foreach (lc, nl->nestParams) {
                NestLoopParam* nlp = (NestLoopParam*)lfirst(lc);
                int paramno = nlp->paramno;
                ParamExecData* prm = NULL;

                prm = &(econtext->ecxt_param_exec_vals[paramno]);
                /* Param value should be an OUTER_VAR var */
                Assert(IsA(nlp->paramval, Var));
                Assert(nlp->paramval->varno == OUTER_VAR);
                Assert(nlp->paramval->varattno > 0);
                Assert(outer_tuple_slot != NULL && outer_tuple_slot->tts_tupleDescriptor != NULL);
                /* Get the Table Accessor Method*/
                prm->value = tableam_tslot_getattr(outer_tuple_slot, nlp->paramval->varattno, &(prm->isnull));
                /*
                 * the following two parameters are called when there exist
                 * join-operation with column table (see ExecEvalVecParamExec).
                 */
                prm->valueType = outer_tuple_slot->tts_tupleDescriptor->tdtypeid;
                prm->isChanged = true;
                /* Flag parameter value as changed */
                inner_plan->chgParam = bms_add_member(inner_plan->chgParam, paramno);
            }

            /*
             * now rescan the inner plan
             */
            ENL1_printf("rescanning inner plan");
            ExecReScan(inner_plan);
        }

        /*
         * we have an outerTuple, try to get the next inner tuple.
         */
        ENL1_printf("getting new inner tuple");

        /*
         * If inner plan is mergejoin, which does not cache data,
         * but will early free the left and right tree's caching memory.
         * When rescan left tree, may fail.
         */
        bool orig_value = inner_plan->state->es_skip_early_free;
        if (!IsA(inner_plan, MaterialState))
            inner_plan->state->es_skip_early_free = true;

        inner_tuple_slot = ExternalExecProcNode(inner_plan);

        inner_plan->state->es_skip_early_free = orig_value;
        econtext->ecxt_innertuple = inner_tuple_slot;

        if (TupIsNull(inner_tuple_slot)) {
            ENL1_printf("no inner tuple, need new outer tuple");

            node->nl_NeedNewOuter = true;

            if (!node->nl_MatchedOuter && (node->js.jointype == JOIN_LEFT || node->js.jointype == JOIN_ANTI ||
                                              node->js.jointype == JOIN_LEFT_ANTI_FULL)) {
                /*
                 * We are doing an outer join and there were no join matches
                 * for this outer tuple.  Generate a fake join tuple with
                 * nulls for the inner tuple, and return it if it passes the
                 * non-join quals.
                 */
                econtext->ecxt_innertuple = node->nl_NullInnerTupleSlot;

                ENL1_printf("testing qualification for outer-join tuple");

                if (otherqual == NIL || ExecQual(otherqual, econtext, false)) {
                    /*
                     * qualification was satisfied so we project and return
                     * the slot containing the result tuple using
                     * function ExecProject.
                     */
                    ExprDoneCond is_done;

                    ENL1_printf("qualification succeeded, projecting tuple");

                    TupleTableSlot* result = ExecProject(node->js.ps.ps_ProjInfo, &is_done);

                    if (is_done != ExprEndResult) {
                        node->js.ps.ps_TupFromTlist = (is_done == ExprMultipleResult);
                        return result;
                    }
                } else
                    InstrCountFiltered2(node, 1);
            }

            /*
             * Otherwise just return to top of loop for a new outer tuple.
             */
            continue;
        }

        /*
         * at this point we have a new pair of inner and outer tuples so we
         * test the inner and outer tuples to see if they satisfy the node's
         * qualification.
         *
         * Only the joinquals determine MatchedOuter status, but all quals
         * must pass to actually return the tuple.
         */
        ENL1_printf("testing qualification");

        if (ExecQual(joinqual, econtext, false)) {
            node->nl_MatchedOuter = true;

            /* In an antijoin, we never return a matched tuple */
            if (node->js.jointype == JOIN_ANTI || node->js.jointype == JOIN_LEFT_ANTI_FULL) {
                node->nl_NeedNewOuter = true;
                continue; /* return to top of loop */
            }

            /*
             * In a semijoin, we'll consider returning the first match, but
             * after that we're done with this outer tuple.
             */
            if (node->js.jointype == JOIN_SEMI)
                node->nl_NeedNewOuter = true;

            if (otherqual == NIL || ExecQual(otherqual, econtext, false)) {
                /*
                 * qualification was satisfied so we project and return the
                 * slot containing the result tuple using ExecProject().
                 */
                ExprDoneCond is_done;

                ENL1_printf("qualification succeeded, projecting tuple");

                TupleTableSlot* result = ExecProject(node->js.ps.ps_ProjInfo, &is_done);

                if (is_done != ExprEndResult) {
                    node->js.ps.ps_TupFromTlist = (is_done == ExprMultipleResult);
                    /*
                     * @hdfs
                     * Optimize plan by informational constraint.
                     */
                    if (((NestLoop*)(node->js.ps.plan))->join.optimizable) {
                        node->nl_NeedNewOuter = true;
                    }

                    return result;
                }
            } else
                InstrCountFiltered2(node, 1);
        } else
            InstrCountFiltered1(node, 1);

        /*
         * Tuple fails qual, so free per-tuple memory and try again.
         */
        ResetExprContext(econtext);

        ENL1_printf("qualification failed, looping");
    }
}