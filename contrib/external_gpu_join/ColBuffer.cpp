/*
 * @Author: your name
 * @Date: 2021-05-14 03:08:11
 * @LastEditTime: 2021-05-14 03:08:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/TupleBuffer.cpp
 */

#ifndef ColBUFFER_HEAD_
#define ColBUFFER_HEAD_
#include <vector>
#include "access/tuptoaster.h"

class ColBuffer {
private:
    static constexpr std::size_t INITIAL_BUFSIZE = 1024UL * 1024UL * 32;

    void* buffer;
    std::size_t content_size;
    std::size_t buffer_size;

    /*cxs*/

    // std::vector<Datum> col[10];  // Datum 用的到底对不对？

    Datum* col[10];

    int* attrType;
    int* attrSize;
    int* attrTotalSize;
    int* attrIndex;
    long tupleNum;
    int totalAttr;

public:
    TupleBuffer(void)
    {
        this->init();
    }
    ~TupleBuffer(void)
    {
        this->fini();
    }

    static TupleBuffer* constructor(void)
    {
        TupleBuffer* tb = static_cast<TupleBuffer*>(palloc(sizeof(*tb)));
        tb->init();
        return tb;
    }
    static void destructor(TupleBuffer* tb)
    {
        tb->fini();
        pfree(tb);
    }

    void init(void)
    {
        this->buffer = palloc(TupleBuffer::INITIAL_BUFSIZE);
        this->buffer_size = TupleBuffer::INITIAL_BUFSIZE;
        this->content_size = 0;

        for (int i = 0; i < 10; i++) {
            this->col[i] = palloc(TupleBuffer::INITIAL_BUFSIZE);
        }
        this->tupleNum = 0;
        this->totalAttr = 0;
    }
    void fini(void)
    {
        pfree(this->buffer);
    }

    bool checkOverflow(std::size_t data_size) const
    {
        return (this->content_size + data_size >= this->buffer_size);
    }

    void extendBuffer(void)
    {
        this->buffer_size *= 2;
        /* may cause memory shortage */
        this->buffer = repalloc_huge(this->buffer, this->buffer_size);
    }

    void putTuple(TupleTableSlot* tts)
    {
        // std::size_t tuple_size = TupleBuffer::getTupleSize(tts);

        // while (this->checkOverflow(tuple_size))
        //     this->extendBuffer();

        int ncolumns = tts->tts_tupleDescriptor->natts;
        Datum* values = (Datum*)palloc(ncolumns * sizeof(Datum));
        bool* nulls = (bool*)palloc(ncolumns * sizeof(bool));

        heap_deform_tuple(&tts->tts_tuple, tts->tts_tupleDescriptor, values, nulls);

        for (int i = 0; i < ncolumns; i++) {
            col[i].push_back(values[i]);
        }

        if (this->tupleNum = 0) {
            this->totalAttr = ncolumns;

            // this->attrSize = (int*)palloc(sizeof(int) * natts);
            // this->attrType = (int*)palloc(sizeof(int) * natts);
            // this->attrIndex = (int*)palloc(sizeof(int) * natts);
        }

        pfree(values);
        pfree(nulls);
        this->tupleNum++;
    }

    void* getWritePointer(void) const
    {
        return static_cast<void*>(static_cast<char*>(this->buffer) + this->content_size);
    }

    void* getBufferPointer(void) const
    {
        return this->buffer;
    }

    std::size_t getContentSize(void) const
    {
        return this->content_size;
    }

    static std::size_t getTupleSize(TupleTableSlot* tts)
    {
        return (tts->tts_tuple->t_len - tts->tts_tuple->t_data->t_hoff);
    }

    /*存疑*/
    static void* getTupleDataPointer(TupleTableSlot* tts)
    {
        HeapTupleHeader ht = tts->tts_tuple->t_data;
        return static_cast<void*>(reinterpret_cast<char*>(ht) + ht->t_hoff);
    }
};

#endif  // TUPLEBUFFER_HEAD_
