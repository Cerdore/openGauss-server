/*
 * @Author: your name
 * @Date: 2021-05-14 03:08:36
 * @LastEditTime: 2021-05-14 03:08:36
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/GpuJoin/ColBufferQueue.hpp
 */
#ifndef ColBUFFERQUEUE_HEAD_
#define ColBUFFERQUEUE_HEAD_

// #include <atomic>

class ColBufferQueue {
private:
    static constexpr int QUEUE_LENGTH = 10;
    ColBuffer* queue[QUEUE_LENGTH];
    std::atomic_int head;
    std::atomic_int tail;
    std::atomic_int length;

public:
    ColBufferQueue(void)
    {
        this->init();
    }
    ~ColBufferQueue(void)
    {
        this->fini();
    }

    static ColBufferQueue* constructor(void)
    {
        ColBufferQueue* tbq = static_cast<ColBufferQueue*>(palloc(sizeof(*tbq)));
        tbq->init();
        return tbq;
    }
    static void destructor(ColBufferQueue* tbq)
    {
        tbq->fini();
        pfree(tbq);
    }

    void init(void)
    {
        this->head.store(-1, std::memory_order_relaxed);
        this->tail.store(-1, std::memory_order_relaxed);
        this->length.store(0, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        // this->head = this->length = -1;
        // this->length = 0;
    }
    void fini(void)
    {
        this->length.store(-1, std::memory_order_release);
    }

    int push(ColBuffer* tb)
    {
        int offset = ++(this->head);

        if (offset > QUEUE_LENGTH) {
            --(this->head);
            return -1;
        }
        queue[offset] = tb;
        ++(this->length);
        return offset;
    }

    ColBuffer* pop(void)
    {
        int offset;

        if (this->length.load(std::memory_order_relaxed) < 1)
            return NULL;
        offset = ++(this->tail);
        --(this->length);
        return queue[offset];
    }

    int getLength(void) const
    {
        return this->length.load(std::memory_order_relaxed);
    }
};
#endif  // ColBUFFERQUEUE_HEAD_
