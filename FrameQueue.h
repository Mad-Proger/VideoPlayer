#pragma once
#include "include.h"

typedef struct FrameQueue {
	AVFrame** q;
	int64_t pts;
	size_t maxSize;
	size_t size;
	size_t l;
	size_t r;
	SDL_bool eof;

	SDL_mutex* mutex;
	SDL_cond* cond;
} FrameQueue;

FrameQueue* createFrameQueue(size_t queueSize, int pixelFormat, int width, int height);

void destroyFrameQueue(FrameQueue* fq);

void pushFrameQueue(FrameQueue* fq, const AVFrame* src);

void pullFrameQueue(FrameQueue* fq, SDL_Texture* dst);

SDL_bool checkEndedFrameQueue(FrameQueue* fq);

void setEndedFrameQueue(FrameQueue* fq);

size_t getSizeFrameQueue(FrameQueue* fq);

int64_t getPTSFrameQueue(FrameQueue* fq);