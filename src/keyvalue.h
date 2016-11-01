/**
 * @file   keyvalue.h
 * @Author Tao Gao (taogao@udel.edu)
 * @date   September 1st, 2016
 * @brief  This file includes <Key,Value> object.
 *
 *
 */
#ifndef KEY_VALUE_H
#define KEY_VALUE_H

#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

#include "dataobject.h"
#include "mapreduce.h"

namespace MIMIR_NS {

class KeyValue : public DataObject{
public:
    KeyValue(int, int,
        int64_t pagesize=1,
        int maxpages=4);

    ~KeyValue();

    void set_kv_type(enum KVType, int, int);

    // Scan KVs one by one
    //int start_scan();
    int getNextKV(char **, int &, char **, int &);
    //int stop_scan();

    // Add KVs one by one
    int addKV(char *, int, char *, int);

    void set_combiner(MapReduce *_mr, UserCombiner _combiner);

    // Insert a KV at a position
    //int insert_kv(char *, char *, int, char *, int);
    //int64_t getNextKV(int, int64_t, char **, int &, char **, int &,
    //    int *kff=NULL, int *vff=NULL);

    //int addKV(int, char *, int &, char *, int &);

    uint64_t get_local_count();
    uint64_t get_global_count();
    uint64_t set_global_count(uint64_t _count){
        global_kvs_count = _count;
    }

    /* used for debug */
    void print(int type=0, FILE *fp=stdout, int format=0);

public:
    enum KVType kvtype;
    int    ksize, vsize;
    uint64_t local_kvs_count, global_kvs_count; 

    MapReduce *mr;
    UserCombiner combiner;
    std::unordered_map<void*, int> slices;
};

}

#endif
