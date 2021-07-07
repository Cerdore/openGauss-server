/*
 * @Author: your name
 * @Date: 2021-06-04 11:15:49
 * @LastEditTime: 2021-07-05 11:25:27
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/nestLoopJoin.cpp
 */
#include "externalJoin.hpp"

namespace {
    #include "cuda.h"
    #include "cuda_runtime.h"
}


/* --- NestLoopJoin --- */
// void nestLoopJoin(void* arg)
// {
//     ereport(LOG,(errmsg("------------------Begin: nestLoopJoin")));

//     ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    
//     long allResNumSize = ejs->T_num[0] * ejs->T_num[1] * sizeof(Result);
//     ereport(LOG,(errmsg("allResNumSize is %ld\n", allResNumSize)));    
    
//     ejs->res = (struct Result*)malloc(ejs->T_num[0] * ejs->T_num[1] * sizeof(Result));
//     cudaError_t cudaStatus = cudaMalloc((void**)&ejs->d_res,ejs->T_num[0] * ejs->T_num[1] * sizeof(Result) );
    
//    // ejs->d_res = myCudaMalloc<Result>( ejs->T_num[0] * ejs->T_num[1] * sizeof(Result) );


//     nestLoopJoincu(ejs->d_tuple[0], ejs->d_tuple[1], ejs->T_num[0], ejs->T_num[1], ejs->d_res);

//     // cudaStatus = 
//     // cudaDeviceSynchronize();
//     ereport(LOG,(errmsg("------------------End: nestLoopJoin")));

// }

void moveTupletoGPU(void* arg)
{

}





/* --- Hash Join --- */
void insetTupleToTable(void *arg){
    ereport(LOG, (errmsg("------------------BEGIN: insetTupleToTable")));
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    
    ejs->pHashTable = create_hashtable();

    // KeyValue* device_kvs;
    // cudaMalloc(&device_kvs, sizeof(KeyValue) * num_kvs);
    // cudaMemcpy(device_kvs, kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyHostToDevice);

    // Have CUDA calculate the thread block size
    int mingridsize;
    int threadblocksize = 1024;
    //cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_hashtable_insert, 0, 0);

    // Create events for GPU timing
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    // Insert all the keys into the hash table
    int gridsize = ((uint32_t)ejs->T_num[0] + threadblocksize - 1) / threadblocksize;
    //gpu_hashtable_insert<<<gridsize, threadblocksize>>>(ejs->pHashTable, ejs->d_tuple[0], (uint32_t)ejs->T_num[0]);
    insertTupleToHashTable(ejs->pHashTable, ejs->d_tuple[0], (uint32_t)ejs->T_num[0], gridsize, threadblocksize);
    cudaEventRecord(stop);

    cudaEventSynchronize(stop);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    float seconds = milliseconds / 1000.0f;
    // printf("    GPU inserted %d items in %f ms (%f million keys/second)\n", 
    //     num_kvs, milliseconds, num_kvs / (double)seconds / 1000000.0f);

    //cudaFree(device_kvs);
    ereport(LOG, (errmsg("------------------END: insetTupleToTable")));
}

void probeTable(void *arg){
    ereport(LOG, (errmsg("------------------BEGIN: probeTable")));
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    
    ejs->res = (struct Resultkv*)malloc(ejs->T_num[1] * sizeof(Resultkv));
    cudaError_t cudaStatus = cudaMalloc((void**)&ejs->d_res,ejs->T_num[1] * sizeof(Resultkv));
    cudaMemset(ejs->d_res, 0xff, sizeof(KeyValue) * kHashTableCapacity);


    int threadblocksize = 1024;
    // Insert all the keys into the hash table
    int gridsize = ((uint32_t)ejs->T_num[1] + threadblocksize - 1) / threadblocksize;
   // gpu_hashtable_lookup <<<gridsize, threadblocksize >>> (ejs->pHashTable, ejs->d_tuple[1], ejs->d_res,(uint32_t)ejs->T_num[1]);
    probeTableLookup(ejs->pHashTable, ejs->d_tuple[1], ejs->d_res,(uint32_t)ejs->T_num[1], gridsize, threadblocksize);
    
    ereport(LOG, (errmsg("------------------END: probeTable")));
}

void moveResulttoHostforHash(void* arg)
{
    ereport(LOG, (errmsg("------------------BEGIN: moveResulttoHostforHash")));
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    cudaError_t cudaStatus =
        cudaMemcpy(ejs->res, ejs->d_res, ejs->T_num[1] * sizeof(Resultkv), cudaMemcpyDeviceToHost);
        ereport(LOG, (errmsg("T_num(1):  %d  sizeof(Resultkv) %d\n", ejs->T_num[1], sizeof(Resultkv))));

    for (long i = 0; i <ejs->T_num[1]; i++) {

        // if ((ejs->res + i)->key1 ==  kEmpty )
        //     continue;

        if ((ejs->res + i)->key1 != kEmpty) {

            // ereport(LOG,(errmsg("Result is   %d %f %d %f\n",(ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res +
            // i)->key2, (ejs->res + i)->dval2 )));
            uint32_t k1 = (ejs->res + i)->key1;
            uint32_t d1 = (ejs->res + i)->val1;
            uint32_t k2 = (ejs->res + i)->key2;
            uint32_t d2 = (ejs->res + i)->val2;
            // ejs->prb.put((ejs->res + i)->key1, (ejs->res + i)->dval1, (ejs->res + i)->key2, (ejs->res + i)->dval2);
            ejs->prb.put(k1, d1, k2, d2);
        }
    }

    ereport(LOG, (errmsg("------------------END: moveResulttoHostforHash")));
    /*put result to host, then put it to queue*/
}

int invokeICDE(void* arg)
{
    ExternalJoinState* ejs = static_cast<ExternalJoinState*>(arg);
    
	timingInfo time;
	inputArgs input;
//	parseInputArgs(argc, argv, &input);

    input.option = 7;
    input.SelsMultiplier = 1;
    input.RelsMultiplier = 1;
    input.RelsNum = ejs->T_num[0];
    input.SelsNum = ejs->T_num[1];
    input.uniqueKeys = 1;
    input.fullRange = 0;
    strcpy(input.alg.name, "HJC");

	int dev = 0;

	switch (input.option)
	{
	case 7:
	case 8:
	{
		//set up device
		cudaDeviceProp deviceProp;
		CHK_ERROR(cudaGetDeviceProperties(&deviceProp, dev));
		CHK_ERROR(cudaSetDevice(dev));

		int *Q_r = NULL;
		size_t Q_els_r = input.RelsNum;
		size_t Q_bytes_r = Q_els_r * sizeof(int);

		int *Q_s = NULL;
		size_t Q_els_s = input.SelsNum;
		size_t Q_bytes_s = Q_els_s * sizeof(int);

		if (input.SelsMultiplier > 1 || input.RelsMultiplier > 1)
		{
			input.SelsNum = input.SelsNum * input.SelsMultiplier;
			input.RelsNum = input.RelsNum * input.RelsMultiplier;

			Q_r = (int *)malloc(Q_bytes_r);
			Q_s = (int *)malloc(Q_bytes_s);
		}

		args joinArgs;
		joinArgs.S_els = input.SelsNum;
		joinArgs.R_els = input.RelsNum;
		uint64_t S_bytes = joinArgs.S_els * sizeof(int);
		uint64_t R_bytes = joinArgs.R_els * sizeof(int);

		/*fix filenames*/
		if (input.fileInput)
		{
		}
		else if (input.fullRange)
		{
			int n = 0;
			if ((n = sprintf(joinArgs.S_filename, "fk_S%lu_pk_R%lu.bin", joinArgs.S_els, joinArgs.R_els)) >= 50)
			{
				fprintf(stderr, "ERROR: S_filename is %d characters long\n", n);
				return 1;
			}
			if ((n = sprintf(joinArgs.R_filename, "pk_R%lu.bin", joinArgs.R_els)) >= 50)
			{
				fprintf(stderr, "ERROR: R_filename is %d characters long\n", n);
				return 1;
			}
		}
		else if (input.uniqueKeys)
		{
			int n = 0;

			if ((n = sprintf(joinArgs.R_filename, "unique_%lu.bin", (input.RelsMultiplier > 1) ? Q_els_r : joinArgs.R_els)) >= 50)
			{
				fprintf(stderr, "ERROR: R_filename is %d characters long\n", n);
				return 1;
			}

			if (input.skew > 0)
				n = sprintf(joinArgs.S_filename, "unique_skew%.2f_S%lu.bin", joinArgs.S_els);
			else
				n = sprintf(joinArgs.S_filename, "unique_%lu.bin", (input.SelsMultiplier > 1) ? Q_els_s : joinArgs.S_els);

			if (n >= 50)
			{
				fprintf(stderr, "ERROR: S_filename is %d characters long\n", n);
				return 1;
			}
		}
		else
		{
			int n = 0;
			if ((n = sprintf(joinArgs.S_filename, "nonUnique_S%lu.bin", joinArgs.S_els)) >= 50)
			{
				fprintf(stderr, "ERROR: S_filename is %d characters long\n", n);
				return 1;
			}
			if ((n = sprintf(joinArgs.R_filename, "nonUnique_R%lu.bin", joinArgs.R_els)) >= 50)
			{
				fprintf(stderr, "ERROR: R_filename is %d characters long\n", n);
				return 1;
			}
		}

		/*create relations*/
#if defined(MEM_DEVICE)
		joinArgs.S = (int *)malloc(S_bytes);
		joinArgs.R = (int *)malloc(R_bytes);
		if (!joinArgs.S || !joinArgs.R)
		{
			fprintf(stderr, "Problem allocating space for the relations\n");
			if (joinArgs.S)
				free(joinArgs.S);
			if (joinArgs.R)
				free(joinArgs.R);
			return 0;
		}
#elif defined(MEM_S_DEVICE)
		joinArgs.S = (int *)malloc(S_bytes);
		if (!joinArgs.S)
		{
			fprintf(stderr, "Problem allocating space for the relations\n");
			return 0;
		}
		CHK_ERROR(cudaHostAlloc((void **)&joinArgs.R, R_bytes, cudaHostAllocMapped));
#elif defined(MEM_MANAGED)
		CHK_ERROR(cudaMallocManaged((void **)&joinArgs.S, S_bytes));
		CHK_ERROR(cudaMallocManaged((void **)&joinArgs.R, R_bytes));
#elif defined(MEM_HOST)
		//CHK_ERROR(cudaHostAlloc((void **)&joinArgs.S, S_bytes, cudaHostAllocMapped)); //cxs 主机内存到设备内存的映射，设备端不需要额外开辟内存
		//CHK_ERROR(cudaHostAlloc((void **)&joinArgs.R, R_bytes, cudaHostAllocMapped));
        TupleBuffer* tbl = ejs->tbq.pop();
        TupleBuffer* tbr = ejs->tbq.pop();
#endif

		if (input.fileInput)
		{
			printf("Reading from files\n");
			readFromFile(input.R_filename, joinArgs.R, joinArgs.R_els);
			readFromFile(input.S_filename, joinArgs.S, joinArgs.S_els);
		}
		else if (input.fullRange)
		{
			printf("Creating relation R with %lu tuples (%d MB) using non-unique keys and full range : ",
						 joinArgs.R_els, R_bytes / 1024 / 1024);
			fflush(stdout);
			create_relation_nonunique(joinArgs.R_filename, joinArgs.R, joinArgs.R_els, INT_MAX);

			printf("Creating relation S with %lu tuples (%d MB) using non-unique keys and full range : ",
						 joinArgs.S_els, S_bytes / 1024 / 1024);
			fflush(stdout);
			create_relation_fk_from_pk(joinArgs.S_filename, joinArgs.S, joinArgs.S_els, joinArgs.R,
																 joinArgs.R_els);
			fflush(stdout);
		}
		else if (input.uniqueKeys)
		{
			printf("Creating relation R with %lu tuples (%d MB) using unique keys : ", joinArgs.R_els,
						 R_bytes / 1024 / 1024);
			//cxs fflush(stdout);

			if (Q_r == NULL)
			{
				//create_relation_unique(joinArgs.R_filename, joinArgs.R, joinArgs.R_els, joinArgs.R_els); //cxs step into there to create relation
                joinArgs.R = (int *)tbr->getBufferPointer(0);
            }
			else
			{
				create_relation_unique(joinArgs.R_filename, Q_r, Q_els_r, Q_els_r);
				create_relation_n(Q_r, joinArgs.R, Q_els_r, input.RelsMultiplier);
			}

			if (Q_s == NULL)
			{
				if (input.skew > 0)
				{
					/* S is skewed */
					printf("Creating relation S with %lu tuples (%d MB) using unique keys and skew %f : ",
								 joinArgs.S_els, S_bytes / 1024 / 1024, input.skew);
					//cxs fflush(stdout);
					create_relation_zipf(joinArgs.S_filename, joinArgs.S, joinArgs.S_els, joinArgs.R_els,
															 input.skew);
				}
				else
				{
					/* S is uniform foreign key */
					printf("Creating relation S with %lu tuples (%d MB) using unique keys : ", joinArgs.S_els,
								 S_bytes / 1024 / 1024);
					fflush(stdout);																	//cxs step into there
					create_relation_unique(joinArgs.S_filename, joinArgs.S, joinArgs.S_els, joinArgs.R_els);
                    joinArgs.S = (int *)tbl->getBufferPointer(0);
                }
			}
			else
			{
				if (input.skew > 0)
				{
					/* S is skewed */
					printf("Creating relation S with %lu tuples (%d MB) using unique keys and skew %f : ",
								 joinArgs.S_els, S_bytes / 1024 / 1024, input.skew);
					fflush(stdout);
					create_relation_zipf(joinArgs.S_filename, Q_s, Q_els_s, Q_els_s, input.skew);
				}
				else
				{
					/* S is uniform foreign key */
					printf("Creating relation S with %lu tuples (%d MB) using unique keys : ", joinArgs.S_els,
								 S_bytes / 1024 / 1024);
					fflush(stdout);
					create_relation_unique(joinArgs.S_filename, Q_s, Q_els_s, Q_els_s);
				}

				create_relation_n(Q_s, joinArgs.S, Q_els_s, input.SelsMultiplier);

				fflush(stdout);
			}

			fflush(stdout);
		}
		else
		{
			printf("Creating relation R with %lu tuples (%d MB) using non-unique keys : ", joinArgs.R_els,
						 R_bytes / 1024 / 1024);
			fflush(stdout);
			create_relation_nonunique(joinArgs.R_filename, joinArgs.R, joinArgs.R_els, joinArgs.R_els / 2); // |R|/2 to get on average 2entries/value

			printf("Creating relation S with %lu tuples (%d MB) using non-unique keys : ", joinArgs.S_els,
						 S_bytes / 1024 / 1024);
			fflush(stdout);
			create_relation_nonunique(joinArgs.S_filename, joinArgs.S, joinArgs.S_els, joinArgs.R_els / 2); // |R|/2 and not |S|/2 to get the same range
			fflush(stdout);
		}

		if (input.option == 7)
		{
			joinArgs.sharedMem = input.sharedMem;
			joinArgs.threadsNum = input.threadsNum;
			printf("%s : shareMemory = %ld\t#threads = %d\n", input.alg.name, joinArgs.sharedMem,
						 joinArgs.threadsNum);
			fflush(stdout);

#if defined(MEM_DEVICE)
			printf("memory alloc done\n");
			int *S_host = joinArgs.S;
			int *R_host = joinArgs.R;

			cudaDeviceSynchronize();

			CHK_ERROR(cudaMalloc((int **)&joinArgs.S, S_bytes));
			CHK_ERROR(cudaMalloc((int **)&joinArgs.R, R_bytes));
			CHK_ERROR(cudaMemcpy(joinArgs.S, S_host, S_bytes, cudaMemcpyHostToDevice));
			CHK_ERROR(cudaMemcpy(joinArgs.R, R_host, R_bytes, cudaMemcpyHostToDevice));

			/*free(S_host); free(R_host);*/
#elif defined(MEM_S_DEVICE)
			int *S_host = joinArgs.S;
			CHK_ERROR(cudaMalloc((int **)&joinArgs.S, S_bytes));
			CHK_ERROR(cudaMemcpy(joinArgs.S, S_host, S_bytes, cudaMemcpyHostToDevice));
			free(S_host);
#endif
			recordTime(&time.start[time.n - 1]);
			uint64_t joinsNum = input.alg.joinAlg(&joinArgs, &time); //cxs join there
			recordTime(&time.end[time.n - 1]);

			cudaDeviceReset();
#if defined(MEM_HOST)
//cxs			cudaFreeHost(joinArgs.S);
//cxs			cudaFreeHost(joinArgs.R);
#else
			cudaFree(joinArgs.S);
			cudaFree(joinArgs.R);
#endif
		}
	}

	break;
	default:
		
		printf(
				"./benchmark -b <id=1(select), 2(reduce), 3(memcpy), 4(streams), 5(tpch), 6(layouts), 7(joins), 8(join on CPU), 9(sort)>\n");
	    exit(1);
		break;
	}
}