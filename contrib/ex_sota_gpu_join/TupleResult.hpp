/*
 * @Author: your name
 * @Date: 2021-05-26 12:27:32
 * @LastEditTime: 2021-07-04 13:10:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/TupleResult.hpp
 */

#ifndef TUPLERESULT_
#define TUPLERESULT_

struct Tuplekv {
    int key;
    double dval;
};


struct Result {
    int key1;
    double dval1;
    int key2;
    double dval2;
};

struct KeyValue
{
    unsigned int key;
    unsigned int value;
};

struct Resultkv {
    unsigned int key1;
    unsigned int val1;
    unsigned int key2;
    unsigned int val2;
};

/*hash join 用的是 uint32 uint32*/

#endif