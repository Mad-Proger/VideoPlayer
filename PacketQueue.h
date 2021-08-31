#pragma once
#include "include.h"

typedef struct PacketQueue {
    AVPacket** q;
    size_t l;
    size_t r;
    size_t size;
    size_t capacity;
    SDL_bool eof;
    int64_t startPTS;
    int64_t endPTS;

    SDL_mutex* mutex;
    SDL_cond* cond;
} PacketQueue;

PacketQueue* createPacketQueue();

void destroyPacketQueue(PacketQueue* pq);

void pushPacketQueue(PacketQueue* pq, AVPacket* pkt);

void pullPacketQueue(PacketQueue* pq, AVPacket** pkt);

SDL_bool checkEndedPacketQueue(PacketQueue* pq);

void setEndedPacketQueue(PacketQueue* pq);

int64_t getTimePacketQueue(PacketQueue* pq);