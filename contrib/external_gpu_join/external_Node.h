/*
 * @Author: your name
 * @Date: 2021-05-16 12:17:33
 * @LastEditTime: 2021-05-16 13:03:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /openGauss-server/contrib/external_gpu_join/external_Node.h
 */
#ifndef EXECUTOR_H
#define EXECUTOR_H

TupleTableSlot* exeternal_ExecNestLoop(NestLoopState* node){};

static inline long sendStrong(const int sock, void* data, const long size)
{
    long send_byte;
    long cumulative_byte = 0;

    for (;;) {
        send_byte =
            ::send(sock, static_cast<void*>(static_cast<char*>(data) + cumulative_byte), size - cumulative_byte, 0);
        if (send_byte <= 0) {
            if (cumulative_byte > 0)
                break;
            else
                return send_byte;
        }
        cumulative_byte += send_byte;
        if (cumulative_byte == size)
            break;
    }
    return cumulative_byte;
}

static inline long receiveStrong(const int sock, void* buf, const long size)
{
    long rec_byte;
    long cumulative_byte = 0;

    for (;;) {
        rec_byte =
            ::recv(sock, static_cast<void*>(static_cast<char*>(buf) + cumulative_byte), size - cumulative_byte, 0);
        if (rec_byte <= 0) {
            if (cumulative_byte > 0)
                break;
            else
                return rec_byte;
        }
        cumulative_byte += rec_byte;
        if (cumulative_byte == size)
            break;
    }
    return cumulative_byte;
}

#endif /* EXECUTOR_H  */