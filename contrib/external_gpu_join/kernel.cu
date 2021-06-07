#include "kernel.cuh"
#include "stdio.h"
#include "assert.h"
#include "cuda.h"
#include "cuda_runtime.h"

cudaDeviceProp GPUprop;


__global__ void nLJ(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res)
{
    // int x = threadIdx.x + blockIdx.x * blockDim.x;
    // int y = threadIdx.y + blockIdx.y * blockDim.y;
    // if (x < n_a && y < n_b) {
    //     if (d_a[x].key == d_b[y].key) {
    //         (*res)->key1 = d_b[x].key;
    //         res->dval1 = d_a[x].dval;
    //         res->key2 = d_a[y].key;
    //         res->dval2 = d_a[y].dval;
    //     } else
    //         res = NULL;
    // }
    long x = threadIdx.x + blockIdx.x * blockDim.x;
    if (x >= n_b)
        return;
    for (long i = 0; i < n_a; i++) {
        if (d_b[x].key == d_a[i].key) {
            (res + i + n_a * x)->key1 = d_a[i].key;
            (res + i + n_a * x)->dval1 = d_a[i].dval;
            (res + i + n_a * x)->key2 = d_b[x].key;
            (res + i + n_a * x)->dval2 = d_b[x].dval;
        }
    }
}


void nestLoopJoincu(struct Tuplekv* d_a, struct Tuplekv* d_b, long n_a, long n_b, struct Result* res)
{
    nLJ<<<128, 1024>>>(d_a, d_b, n_a, n_b, res);
    cudaError_t cudaStatus = cudaDeviceSynchronize();
}


/*Hash Join -- simple*/
#define p 334214459
#define TABLESIZE 1000000
#define maxiterations 10
#define KEYEMPTY -1
#define NOTFOUND -100

__device__
struct Tuplekv table[TABLESIZE];



__device__
Tuplekv make_entry(int key, double value){
  //printf("key : %d, value : %d",key , value);
  struct Tuplekv ans = {key, value};
  //printf ("ans : %d ", (int)ans>>32);
  return ans;
}

__device__ int getkey(Tuplekv entry){
  return entry.key;
}

// __device__ unsigned getvalue(unsigned long long entry){
//   return (entry & 0xffffffff) ;
// }

__device__
unsigned hash_function_1(unsigned key){
   int a1 = 5;
   int b1 = 2;
   return (((a1*key+b1)%p)%TABLESIZE);
}

__device__
unsigned hash_function_2(unsigned key){
   int a1 = 13;
   int b1 = 7;
   return (((a1*key+b1)%p)%TABLESIZE);
}

__global__
void hash(struct Tuplekv* d_a, int width, int height){
  
    int index = blockIdx.x * blockDim.x +threadIdx.x;
  //  unsigned long key = Table_A[index*width+0];
  //  unsigned long value = Table_A[index*width+1]; 

    int key = d_a[index].key;
    double value = d_a[index].dval;
    
    Tuplekv entry = make_entry(key,value);
    //printf("entry: %d",entry);
    unsigned location = hash_function_1(key);
    unsigned k = key;
    for (int its = 0; its<maxiterations; its++){
      entry = atomicExch(&table[location], entry);
      
      key = getkey(entry);
      if (key == 0) {
        //printf("key: %lu table: %llu \n",k,table[location]);
        return;
      }
      unsigned location1 = hash_function_1(key);
      unsigned location2 = hash_function_2(key);
      if (location == location1)
      location = location2;
      else if (location == location2)
      location = location1;
    }
    printf("chain was too long");
    return ;
}


// __global__
// void join(int *Table_B,int *Table_C,int width_c,int width,int height){
//   int index = blockIdx.x * blockDim.x +threadIdx.x;
//   unsigned long primkey = Table_B[index*width+0];
//    //printf("primkey : %lu \n",primkey);
//   unsigned long value = Table_B[index*width+1];
//   unsigned location_1 = hash_function_1(primkey);
//   unsigned location_2 = hash_function_2(primkey);
//   unsigned long long entry;
//   if (getkey(entry = table[location_1])!= primkey)
//     if (getkey(entry = table[location_2])!= primkey){
//         entry = make_entry(0,NOTFOUND);
//     }
//  // printf("entry of primkey %lu:%llu \n",primkey,entry);
//   //printf("key from hash table of primkey %lu: %d\n",primkey,getkey(entry));
//   Table_C[index*width_c+0]=getkey(entry);
//   Table_C[index*width_c+1]=getvalue(entry);
//   //printf("key from hash table of primkey %lu: %d\n",primkey,getvalue(entry));
//   Table_C[index*width_c+2] = value;
//   //printf("value from hash table of primkey %lu: %d\n",primkey,value); 
//   for(int l =0 ;l<3 ;l++){
//     //printf("index : %d,Table: %d  ",index,Table_C[index*width_c+l]);
//   }
// }



