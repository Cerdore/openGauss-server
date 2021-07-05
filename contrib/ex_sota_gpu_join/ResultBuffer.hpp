/*
 * @Author: your name
 * @Date: 2021-05-14 03:07:38
 * @LastEditTime: 2021-06-08 04:08:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/ResultBuffer.hpp
 */
#ifndef RESULTBUFFER_HEAD_
#define RESULTBUFFER_HEAD_

class ResultBuffer {
public:
    static constexpr std::size_t BUFSIZE = 1024UL * 1024UL * 128;
    // static constexpr std::size_t BUFSIZE = 30UL;
private:
    struct Resultkv* buffer;
    long size;
    
    std::atomic_long content_size;

public:
std::atomic_long index;
    ResultBuffer(void)
    {
        this->init();
    }
    ~ResultBuffer(void)
    {
        this->fini();
    }

    static ResultBuffer* constructor(void)
    {
        ResultBuffer* rb = static_cast<ResultBuffer*>(palloc(sizeof(*rb)));
        rb->init();
        return rb;
    }
    static void destructor(ResultBuffer* rb)
    {
        rb->fini();
        pfree(rb);
    }

    void init(void)
    {
        this->size = 3000000;
        this->index.store(0, std::memory_order_relaxed);// = 0;
        this->buffer = (struct Resultkv*)malloc(3000000 * sizeof(struct Resultkv));
        // this->buffer = palloc(ResultBuffer::BUFSIZE);
        // this->buffer = NULL;
        this->content_size.store(0, std::memory_order_relaxed);
    }
    void fini(void)
    {
        pfree(this->buffer);
        this->content_size.store(0, std::memory_order_relaxed);
    }
    void extendBuffer(void)
    {
        this->size *= 2;
        this->buffer = (struct Resultkv*)repalloc_huge(this->buffer, this->size * sizeof(struct Resultkv));
    }
    bool checkOverflow() const
    {
        return (this->index + 1 >= this->size);
    }

    // void put(unsigned int k1, unsigned int v1, unsigned int k2, unsigned int v2)
    // {
    //     while (this->checkOverflow())
    //         this->extendBuffer();
    //     //(this->buffer + this->index) = tp;
    //     (this->buffer + this->index)->key1 = k1;
    //     (this->buffer + this->index)->dval1 = v1;
    //     (this->buffer + this->index)->key2 = k2;
    //     (this->buffer + this->index)->dval2 = v2;
    //     this->index++;
    // }

    // void put(int k1, double v1, int k2, double v2)
    // {
    //     while (this->checkOverflow())
    //         this->extendBuffer();
    //     //(this->buffer + this->index) = tp;
    //     (this->buffer + this->index)->key1 = k1;
    //     (this->buffer + this->index)->dval1 = v1;
    //     (this->buffer + this->index)->key2 = k2;
    //     (this->buffer + this->index)->dval2 = v2;
    //     this->index++;
    // }
    void put(int k1, int v1, int k2, int v2)
    {
        // while (this->checkOverflow())
        //     this->extendBuffer();
        //(this->buffer + this->index) = tp;
        (this->buffer + this->index)->key1 = k1;
        (this->buffer + this->index)->val1 = v1;
        (this->buffer + this->index)->key2 = k2;
        (this->buffer + this->index)->val2 = v2;
        this->index++;
    }
    // struct Result* get()
    // {
    //     if (this->index <= 0)
    //         return NULL;
    //     this->index--;
    //     return (this->buffer + this->index);
    // }
    struct Resultkv* get()
    {
        if (this->index <= 0)
            return NULL;
        this->index--;
        return (this->buffer + this->index);
    }
    void setContentSize(long size)
    {
        this->content_size.store(size, std::memory_order_release);
    }

    long getContentSize(void) const
    {
        return this->content_size.load(std::memory_order_relaxed);
    }

    // struct Result* getBufferPointer(void) const
    // {
    //     return this->buffer;
    // }

    struct Resultkv* getBufferPointer(void) const
    {
        return this->buffer;
    }

    // void* operator[](const std::size_t off) const
    // {
    //     return static_cast<void*>(static_cast<char*>(this->buffer) + off);
    // }
};

#endif  // RESULTBUFFER_HEAD_
