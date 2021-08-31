#pragma once
#include "include.h"

typedef struct AudioQueue {
    uint8_t* q;
    double* ts;
    double presentationTime;
    size_t l;
    size_t r;
    size_t sz;
    size_t maxSize;
    SDL_bool eof;

    SDL_mutex* mutex;
    SDL_cond* cond;
} AudioQueue;

AudioQueue* createAudioQueue(const size_t bufferSize);

void destroyAudioQueue(AudioQueue* audioq);

void pushAudioQueue(AudioQueue* audioq, const uint8_t* restrict src, size_t len, const double pt);

void pullAudioQueue(AudioQueue* audioq, uint8_t* restrict dst, size_t len);

SDL_bool checkEndedAudioQueue(AudioQueue* audioq);

void setEndedAudioQueue(AudioQueue* audioq);

size_t getSizeAudioQueue(AudioQueue* audioq);

double getPresentationTimeAudioQueue(AudioQueue* audioq);