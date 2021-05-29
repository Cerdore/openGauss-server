/*
 * @Author: your name
 * @Date: 2021-05-14 03:08:11
 * @LastEditTime: 2021-05-29 08:28:05
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/TupleBuffer.cpp
 */

#ifndef TUPLEBUFFER_HEAD_
#define TUPLEBUFFER_HEAD_

class TupleBuffer {
private:
    static constexpr std::size_t INITIAL_BUFSIZE = 1024UL * 1024UL * 32;

    void* buffer;
    std::size_t content_size;
    std::size_t buffer_size;

    /*cxs why not use the desc*/

    // int* attrType;
    // int* attrSize;
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
        this->buffer = palloc(TupleBuffer::INITIAL_BUFSIZE);
        this->buffer_size = TupleBuffer::INITIAL_BUFSIZE;
        this->content_size = 0;

        this->tupleNum = 0;
        // this->totalAttr = 0;
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
        
        std::size_t tuple_size = TupleBuffer::getTupleSize(tts);
        //tuple size is 16 ---- ereport(LOG,(errmsg("Tuple size is %lu\n", tuple_size)));
         
        while (this->checkOverflow(tuple_size))
            this->extendBuffer();

        /*把数据全拷过去，没考虑解析*/
        std::memcpy(this->getWritePointer(), TupleBuffer::getTupleDataPointer(tts), tuple_size);
        this->content_size += tuple_size;

        /* add attr, ref to gpu/tableScan.c
        这是一个buffer,所以应该是对每个tuple一个这个属性？
        并不需要，因为一个buffer中的tuple的属性是一样的
        ，但是size都一样吗！！？
        有了类型不就应该有size了吗...*/
        // if (this->tupleNum = 0) {
        //     //    this->totalAttr = get_relnatts(tts->tts_tuple->t_tableOid);
        //     int natts = tts->tts_tupleDescriptor->natts;
        //     this->totalAttr = natts;

        //     this->attrSize = (int*)palloc(sizeof(int) * natts);
        //     this->attrType = (int*)palloc(sizeof(int) * natts);
        //     this->attrIndex = (int*)palloc(sizeof(int) * natts);
        // }
        // this->attrType[this->tupleNum] = this->tupleNum++;
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
