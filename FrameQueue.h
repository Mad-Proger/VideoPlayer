#pragma once
#include "include.h"

typedef struct FrameQueue {
	uint8_t** q;
	double* ts;
	double presentationTime;
	size_t frameSize;
	size_t maxSize;
	size_t size;
	size_t l;
	size_t r;
	SDL_bool eof;

	SDL_mutex* mutex;
	SDL_cond* cond;
} FrameQueue;

FrameQueue* createFrameQueue(size_t queueSize, size_t frameSize);

void destroyFrameQueue(FrameQueue* fq);

void pushFrameQueue(FrameQueue* fq, const uint8_t* src, const double pt);

void pullFrameQueue(FrameQueue* fq, uint8_t* dst);

SDL_bool checkEndedFrameQueue(FrameQueue* fq);

void setEndedFrameQueue(FrameQueue* fq);

size_t getSizeFrameQueue(FrameQueue* fq);

double getPresentationTimeFrameQueue(FrameQueue* fq);