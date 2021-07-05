/*
 * @Author: your name
 * @Date: 2021-06-07 11:18:28
 * @LastEditTime: 2021-07-04 13:23:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/linearprobing.h
 */
#pragma once
#include "TupleResult.hpp"
 #include <vector>

// struct KeyValue
// {
//     unsigned int key;
//     unsigned int value;
// };
// struct Resultkv {
//     unsigned int key1;
//     unsigned int val1;
//     unsigned int key2;
//     unsigned int val2;
// };


const unsigned int kHashTableCapacity = 128 * 1024 * 1024;

const unsigned int kNumKeyValues = kHashTableCapacity / 2;

const unsigned int kEmpty = 0xffffffff;

KeyValue* create_hashtable();

void insert_hashtable(KeyValue* hashtable, const KeyValue* kvs, unsigned int num_kvs);

void lookup_hashtable(KeyValue* hashtable, KeyValue* kvs, unsigned int num_kvs);

void delete_hashtable(KeyValue* hashtable, const KeyValue* kvs, unsigned int num_kvs);

std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable);

void destroy_hashtable(KeyValue* hashtable);

void insertTupleToHashTable(KeyValue* hashtable, KeyValue* tuple, unsigned int num, int gridsize, int threadblocksize);

void probeTableLookup(KeyValue* hashtable, KeyValue* tuple, Resultkv* res, unsigned int num, int gridsize, int threadblocksize);