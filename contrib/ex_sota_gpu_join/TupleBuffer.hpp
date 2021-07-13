/*
 * @Author: your name
 * @Date: 2021-05-14 03:08:11
 * @LastEditTime: 2021-07-09 01:59:09
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/TupleBuffer.cpp
 */

#ifndef TUPLEBUFFER_HEAD_
#define TUPLEBUFFER_HEAD_

#define ATTR_NUM 2

class TupleBuffer {
private:
    static constexpr std::size_t INITIAL_BUFSIZE = 1024UL * 1024UL * 32;

    void* buffer[ATTR_NUM];
    std::size_t content_size[ATTR_NUM];
    std::size_t buffer_size[ATTR_NUM];

    /*cxs why not use the desc*/

    // int* attrType;
    int* attrSize;
    // int* attrTotalSize;
    // int* attrIndex;

    // int totalAttr;

public:
    long tupleNum;
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
        for (int i = 0; i < ATTR_NUM; i++) {
            this->buffer[i] = palloc(TupleBuffer::INITIAL_BUFSIZE);
            this->buffer_size[i] = TupleBuffer::INITIAL_BUFSIZE;
            this->content_size[i] = 0;
        }
        this->tupleNum = 0;
        // this->totalAttr = 0;
    }
    void fini(void)
    {
        for (int i = 0; i < ATTR_NUM; i++) {
            pfree(this->buffer[i]);
        }
    }

    bool checkOverflow(std::size_t data_size, int index) const
    {
        return (this->content_size[index] + data_size >= this->buffer_size[index]);
    }

    void extendBuffer(int index)
    {
        this->buffer_size[index] *= 2;
        /* may cause memory shortage */
        this->buffer[index] = repalloc_huge(this->buffer[index], this->buffer_size[index]);
    }

    void putTupleCol(TupleTableSlot* tts)
    {

        std::size_t tuple_size = TupleBuffer::getTupleSize(tts);

        //bool nulls[ATTR_NUM];
        //Datum values[ATTR_NUM];
        
        //ereport(LOG, (errmsg("putTupleCol: sizeOf Datum :  %d", sizeof(Datum))));

        Datum* values = (Datum *)palloc(ATTR_NUM * sizeof(Datum));
        bool* nulls = (bool *)palloc(ATTR_NUM * sizeof(bool));        
        HeapTuple tuple = static_cast<HeapTuple>(tts->tts_tuple);
        TupleDesc desc = tts->tts_tupleDescriptor;
        
        heap_deform_tuple(tuple, desc, values, nulls);

        // tuple size is 16 ---- ereport(LOG,(errmsg("Tuple size is %lu\n", tuple_size)));

        for (int i = 0; i < ATTR_NUM; i++) {
            while (this->checkOverflow(4, i)) //cxs may be the 4 byte
                this->extendBuffer(i);
        }

        for (int i = 0; i < ATTR_NUM; i++) {
            // if(i == 0)
            //     ereport(LOG, (errmsg("putTupleCol: values[%d] :  %d", i, DatumGetInt32(*(values+i)))));

            std::memcpy(this->getWritePointer(i), (void*)(values+i), 4); //may be the 4 byte
            this->content_size[i]+=4;
        }
        this->tupleNum++;
    }
    
    void putTuple(TupleTableSlot* tts)
    {

    }

    void* getWritePointer(int index) const
    {
        return static_cast<void*>(static_cast<char*>(this->buffer[index]) + this->content_size[index]);
    }

    void* getBufferPointer(int index) const
    {
        return this->buffer[index];
    }

    std::size_t getContentSize(int index) const
    {
        return this->content_size[index];
    }

    static std::size_t getTupleSize(TupleTableSlot* tts)
    {
        //        return ((HeapTuple*)(tts->tts_tuple)->t_len - tts->tts_tuple->t_data->t_hoff);
        return (static_cast<HeapTuple>(tts->tts_tuple)->t_len - static_cast<HeapTuple>(tts->tts_tuple)->t_data->t_hoff);
    }

    /*存疑*/
    static void* getTupleDataPointer(TupleTableSlot* tts)
    {
        HeapTupleHeader ht = static_cast<HeapTuple>(tts->tts_tuple)->t_data;
        return static_cast<void*>(reinterpret_cast<char*>(ht) + ht->t_hoff);
    }
};

#endif  // TUPLEBUFFER_HEAD_
