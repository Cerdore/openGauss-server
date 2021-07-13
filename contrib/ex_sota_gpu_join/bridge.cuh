#include <unistd.h>
#include <cstdio>
#include <limits.h> /*INT_MAX*/
#include <getopt.h>

#include "generator_ETHZ.cuh"
#include "common.h"
#include "common-host.h"
#include "TupleResult.hpp"

unsigned int hashJoinClusteredProbe(args *inputAttrs, timingInfo *time, Resultkv * &res);

typedef struct joinAlg
{
	char name[4];
	unsigned int (*joinAlg)(args *, timingInfo *, Resultkv * &);
} joinAlg;

typedef struct inputArgs
{
	short option = 0;
	joinAlg alg
#ifndef __CUDACC__
			{"HJC", hashJoinClusteredProbe};//= {"NLJ", nestedLoopsJoin}; // does not play well along --expt-relaxed-constexpr
#else
			;
#endif
	uint64_t SelsNum = 0;
	uint64_t RelsNum = 0;
	int uniqueKeys = 1;
	int fullRange = 0;
	float skew = 0.0;
	int threadsNum = 32;
	//	int selectivity = 1;
	int valuesPerThread = 2;
	int sharedMem = 30 << 10; 
	unsigned int pivotsNum = 1;
	int one_to_many = 0;
	int RelsMultiplier = 1;
	int SelsMultiplier = 1;
	const char *R_filename = NULL;
	const char *S_filename = NULL;
	int fileInput = 0;
} inputArgs;

static joinAlg algs[]{
		{"HJC", hashJoinClusteredProbe}
		//		{"HJ", hashIndexJoin}
};

//void usage_exit(int op);
