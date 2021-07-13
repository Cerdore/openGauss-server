/*
 * @Author: your name
 * @Date: 2021-07-05 12:14:00
 * @LastEditTime: 2021-07-08 10:38:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/ex_sota_gpu_join/common-host.h
 */
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

#ifndef COMMON_HOST_H_
#define COMMON_HOST_H_

#include <chrono>
#include <string>
#include <iostream>


#define PAGESIZE 65568 //16384 * sizeof(int) + 32  //
//#define PAGESIZE 16416 //16384 - 32
//#define PAGESIZE  4096

#include <cstdlib>

//nsight complains about cstdint (?)
typedef unsigned int      uint32_t;
typedef unsigned long int uint64_t;

typedef struct args {
	int *S;
    int *S_v;
	size_t S_els;
	char S_filename[50];
	int *R;
    int *R_v;
	size_t R_els;
	char R_filename[50];
	int threadsNum;
//	int blocksNum_max;
//	int valuesPerThread;
	unsigned int sharedMem;
	unsigned int pivotsNum;

} args;

/* Timing */
double cpuSeconds();

/* Benchmarking */
void initializeSeq(int *in, size_t size);
void initializeUniform(int *in, size_t size);
void initializeUniform(int *in, size_t size, int seed);
void initializeUniform(int *in, size_t size, int maxNo, int seed);
void initializeZero(int *in, size_t size);

/* Bitmap Ops */
int NumberOfSetBits(int i);

class time_block{
private:
    std::chrono::time_point<std::chrono::system_clock> start;
    std::string                                        text ;
public:
    inline time_block(std::string text = ""): 
                        text(text), start(std::chrono::system_clock::now()){}

    inline ~time_block(){
        auto end   = std::chrono::system_clock::now();
        std::cout << text;
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }
};

#endif /* COMMON_HOST_H_ */
