/*Copyright (c) 2018 Data Intensive Applications and Systems Laboratory (DIAS)
                   Ecole Polytechnique Federale de Lausanne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "bridge.cuh"


void print_timing_join(args *input, timingInfo *time, joinAlg *alg);
void parseInputArgs(int argc, char **argv, inputArgs *input);
int createSingleRelation_filename(inputArgs *input, args *attrs);
void createSingleRelation_data(inputArgs *input, args *attrs, uint64_t bytes);

void usage_exit(int op){
	if (op == 0)
		printf(
				"./benchmark -b <id=1(select), 2(reduce), 3(memcpy), 4(streams), 5(tpch), 6(layouts), 7(joins), 8(join on CPU), 9(sort)>\n");
	exit(1);
}

int createSingleRelation_filename(inputArgs *input, args *attrs)
{
	/*fix filename (no matter which relation store everything in S, name only needed to re-use a file)*/
	int n = 0;
	if (input->fullRange)
	{
		if (input->SelsNum)
			n = sprintf(attrs->S_filename, "pk_S%lu.bin", attrs->S_els);
		else
			n = sprintf(attrs->S_filename, "pk_R%lu.bin", attrs->S_els);
	}
	else if (input->uniqueKeys)
	{
		if (input->SelsNum)
			n = sprintf(attrs->S_filename, "unique_S%lu.bin", attrs->S_els);
		else
			n = sprintf(attrs->S_filename, "unique_R%lu.bin", attrs->S_els);
	}
	else
	{
		if (input->SelsNum)
			n = sprintf(attrs->S_filename, "nonUnique_S%lu.bin", attrs->S_els);
		else
			n = sprintf(attrs->S_filename, "nonUnique_R%lu.bin", attrs->S_els);
	}

	if (n >= 50)
	{
		fprintf(stderr, "ERROR: filename is %d characters long\n", n);
		return 1;
	}

	return 0;
}

void createSingleRelation_data(inputArgs *input, args *attrs, uint64_t bytes)
{
	if (input->fullRange)
	{
		printf("Creating relation with %lu tuples (%d MB) using non-unique keys and full range : ",
					 attrs->S_els, bytes / 1024 / 1024);
		fflush(stdout);
		create_relation_nonunique(attrs->S_filename, attrs->S, attrs->S_els, INT_MAX);
	}
	else if (input->uniqueKeys)
	{
		printf("Creating relation with %lu tuples (%d MB) using unique keys : ", attrs->S_els,
					 bytes / 1024 / 1024);
		fflush(stdout);
		create_relation_unique(attrs->S_filename, attrs->S, attrs->S_els, attrs->S_els);
	}
	else
	{
		printf("Creating relation with %lu tuples (%d MB) using non-unique keys : ", attrs->S_els,
					 bytes / 1024 / 1024);
		fflush(stdout);
		create_relation_nonunique(attrs->S_filename, attrs->S, attrs->S_els, attrs->S_els);
	}
	printf("DONE\n");
	fflush(stdout);
}

void printTimeInfo(uint64_t tuplesNum, time_st *start, time_st *end)
{
	double diff_usec = (((*end).tv_sec * 1000000L + (*end).tv_usec) - ((*start).tv_sec * 1000000L + (*start).tv_usec));

	double tuplesPerSec = tuplesNum / (diff_usec / 1000000.0);
	//	printf("%10.3f\n", tuplesPerSec);

	printf("total tuples = %10lu time  = %.3f msecs = %.3f secs\t", tuplesNum, diff_usec / 1000.0,
				 diff_usec / 1000000.0);
	if (tuplesPerSec < 1024 / sizeof(int))
		printf("throughput = %8.3lf B/sec\n", tuplesPerSec * sizeof(int));
	else if (tuplesPerSec < 1024 * 1024 / sizeof(int))
		printf("throughput = %8.3lf KB/sec\n", tuplesPerSec * sizeof(int) / 1024);
	else if (tuplesPerSec < 1024 * 1024 * 1024 / sizeof(int))
		printf("throughput = %8.3lf MB/sec\n", ((tuplesPerSec / 1024) * sizeof(int)) / 1024);
	else
		printf("throughput = %8.3lf GB/sec\n", ((tuplesPerSec / 1024 / 1024) * sizeof(int)) / 1024);
}

void print_timing_join(args *input, timingInfo *time, joinAlg *alg)
{
	unsigned int blocksNum = (input->R_els + input->threadsNum - 1) / input->threadsNum;
	unsigned int memElsNum = (input->sharedMem + sizeof(int) - 1) / sizeof(int);
	unsigned int shareMemoryBlocksNum = (input->S_els + memElsNum - 1) / memElsNum;
	uint64_t tuplesNum = input->S_els + input->R_els; //if alg==0

	if (strcmp(alg->name, algs[2].name) != 0 && strcmp(alg->name, algs[3].name) != 0 && strcmp(alg->name, algs[4].name) != 0)
	{

#if defined(NLJ_SIMPLE)
		tuplesNum = input->R_els + input->R_els * input->S_els;
#elif defined(SHAREDMEM_LOOPIN)
#if defined(MEM_S_DEVICE)
		tuplesNum = input->R_els + input->S_els;
#else
		tuplesNum = input->R_els + input->S_els * blocksNum;
#endif
#endif
	}

	printf("blocksNum=%lu\tmemElsNum=%lu\tshareMemBlocksNum=%lu\ttuplesNum=%lu\n", blocksNum, memElsNum,
				 shareMemoryBlocksNum, tuplesNum);

	if (strcmp(alg->name, algs[1].name) == 0)
	{
		/*SMJ*/
		printf("SORT:\t");
		printTimeInfo(tuplesNum, &(time->start[1]), &(time->end[1]));
		// } else if (strcmp(alg->name, algs[4].name) == 0) {
		// 	/*HJ*/
		// 	printf("INDEX:\t");
		// 	printTimeInfo(tuplesNum, &(time->start[1]), &(time->end[1]));
	}

	if (strcmp(alg->name, algs[2].name) == 0 || strcmp(alg->name, algs[3].name) == 0 || strcmp(alg->name, algs[4].name) == 0)
	{
		printf("BUILD:\t");
		printTimeInfo(input->R_els, &(time->start[1]), &(time->end[1]));
		printf("PROBE:\t");
		printTimeInfo(input->S_els, &(time->start[0]), &(time->end[0]));
	}
	else
	{
		printf("JOIN:\t");
		printTimeInfo(tuplesNum, &(time->start[0]), &(time->end[0]));
	}

	printf("AGGR:\t");
	printTimeInfo(tuplesNum, &(time->start[time->n - 2]), &(time->end[time->n - 2]));
	printf("TOTAL:\t");
	printTimeInfo(tuplesNum, &(time->start[time->n - 1]), &(time->end[time->n - 1]));
}

void parseInputArgs(int argc, char **argv, inputArgs *input)
{
	/* flags */
	int uniqueKeys_flag = input->uniqueKeys;
	int fullRange_flag = input->fullRange;
	int file_flag = input->fileInput;

	int c;
	int option_index = 0;

	printf("INPUT: ");

	static struct option long_options[] = {
			/*These options set a flag.*/
			{"file", no_argument, &file_flag, 1},
			{"non-unique", no_argument, &uniqueKeys_flag, 0},
			{"full-range", no_argument, &fullRange_flag, 1},
			/* These options don't set a flag. We distinguish them by their indices. */
			{"benchmark", required_argument, 0, 'b'},
			{"alg", required_argument, 0, 'a'},
			{"SelsNum",
			 required_argument, 0, 'S'},
			{"RelsNum", required_argument, 0, 'R'},
			{"skew",
			 required_argument, 0, 's'},
			{"threadsNum", required_argument, 0, 't'},
			{"values",
			 required_argument, 0, 'v'},
			{"memory", required_argument, 0, 'm'},
			{"pivotsNum",
			 required_argument, 0, 'p'},
			{"OneToMany", required_argument, 0, 'w'},
			{"XSelsMultiplier",
			 required_argument, 0, 'x'},
			{"YRelsMultiplier", required_argument, 0, 'y'},
			{"R_filename",
			 required_argument, 0, 'k'},
			{"S_filename", required_argument, 0, 'l'},
			{0, 0, 0, 0}};

	while ((c = getopt_long(argc, argv, "b:a:S:R:s:t:v:m:p:x:y:k:l:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 0:
			printf("%s\t", long_options[option_index].name);
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;
		case 'b':
			input->option = atoi(optarg);
			printf("option = %d\t", input->option);
			break;
		case 'a':
		{
			int i = 0;
			while (algs[i].joinAlg)
			{
				if (strcmp(optarg, algs[i].name) == 0)
				{
					strcpy(input->alg.name, algs[i].name);
					input->alg.joinAlg = algs[i].joinAlg;
					break;
				}
				i++;
			}
			printf("joinAlg = %s\t", input->alg.name);
		}
		break;
		case 'k':
			input->R_filename = optarg;
			printf("R filename = %s\t", input->R_filename);
			break;
		case 'l':
			input->S_filename = optarg;
			printf("S filename = %s\t", input->S_filename);
			break;
		case 'S':
		{
			uint64_t p = atol(optarg);
			if (p > ULONG_MAX / sizeof(int))
			{
				fprintf(stderr,
								"WARNING: SelsNun is too big (%lu). Setting SelsNum to maximum supported value %lu\n",
								p, ULONG_MAX / sizeof(int));
				input->SelsNum = ULONG_MAX / sizeof(int);
			}
			else
				input->SelsNum = p;
		}
			printf("||S|| = %lu\t", input->SelsNum);
			break;
		case 'R':
		{
			uint64_t p = atol(optarg);
			if (p > ULONG_MAX / sizeof(int))
			{
				fprintf(stderr,
								"WARNING: RelsNun is too big (%lu). Setting RelsNum to maximum supported value %lu\n",
								p, ULONG_MAX / sizeof(int));
				input->RelsNum = ULONG_MAX / sizeof(int);
			}
			else
				input->RelsNum = p;
		}
			printf("||R|| = %lu\t", input->RelsNum);
			break;
		case 's':
			input->skew = atof(optarg);
			printf("skew = %f\t", input->skew);
			break;
		case 't':
			input->threadsNum = atoi(optarg);
			printf("#threads = %d\t", input->threadsNum);
			break;
		case 'v':
			input->valuesPerThread = atoi(optarg);
			printf("values per thread= %d\t", input->valuesPerThread);
			break;
		case 'm':
			input->sharedMem = atoi(optarg); // << 10;
			printf("sharedMem = %d\t", input->sharedMem);
			break;
		case 'p':
			input->pivotsNum = atoi(optarg);
			printf("pivotsNum = %d\t", input->pivotsNum);
			break;
		case 'w':
			input->one_to_many = atoi(optarg);
			printf("OneToMany = %d\t", input->one_to_many);
			break;
		case 'x':
			input->SelsMultiplier = atol(optarg);
			printf("SelsMultiplier = %d\t", input->SelsMultiplier);
			break;
		case 'y':
			input->RelsMultiplier = atol(optarg);
			printf("RelsMultiplier = %d\t", input->RelsMultiplier);
			break;
		}
	}

	input->uniqueKeys = uniqueKeys_flag;
	input->fullRange = fullRange_flag;
	input->fileInput = file_flag;
	printf("uniqueKeys_flag: %d\t",uniqueKeys_flag);
	printf("funRange_flag: %d\t",fullRange_flag);
	printf("fileInput: %d\t", file_flag);
	printf("\n");

	if (input->option < 1 || (input->option > 9 && input->option < 100) || input->option > 101)
		usage_exit(0);
}
