/*
 * @Author: your name
 * @Date: 2021-05-26 12:27:32
 * @LastEditTime: 2021-05-26 12:28:52
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

#endif