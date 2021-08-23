#include "FrameQueue.h"

FrameQueue* createFrameQueue(size_t queueSize, size_t frameSize) {
	FrameQueue* res = (FrameQueue*)malloc(sizeof(FrameQueue));
	if (res == NULL) {
		return NULL;
	}
	memset(res, 0, sizeof(FrameQueue));
	res->frameSize = frameSize;

	res->q = (uint8_t**)malloc(queueSize * sizeof(uint8_t*));
	if (res->q == NULL) {
		destroyFrameQueue(res);
		return NULL;
	}

	res->ts = (double*)malloc(queueSize * sizeof(double));
	if (res->ts == NULL) {
		destroyFrameQueue(res);
		return NULL;
	}

	for (size_t i = 0; i < queueSize; ++i) {
		res->q[i] = (uint8_t*)malloc(frameSize);
		if (res->q[i] == NULL) {
			destroyFrameQueue(res);
			return NULL;
		}
	}

	res->maxSize = queueSize;
	res->l = res->r = res->size = 0;
	res->eof = SDL_FALSE;

	res->mutex = SDL_CreateMutex();
	if (res->mutex == NULL) {
		destroyFrameQueue(res);
		return NULL;
	}

	res->cond = SDL_CreateCond();
	if (res->cond == NULL) {
		destroyFrameQueue(res);
		return NULL;
	}

	return res;
}

void destroyFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return;
	}

	if (fq->q != NULL) {
		for (size_t i = 0; i < fq->maxSize; ++i) {
			free((void*)fq->q[i]);
		}
	}
	free((void*)fq->q);
	free((void*)fq->ts);

	SDL_DestroyMutex(fq->mutex);
	SDL_DestroyCond(fq->cond);

	free((void*)fq);
}

void pushFrameQueue(FrameQueue* fq, const uint8_t* src, const double pt) {
	if (fq == NULL) {
		return;
	}

	SDL_LockMutex(fq->mutex);
	while (fq->size == fq->maxSize && !fq->eof) {
		SDL_CondWait(fq->cond, fq->mutex);
	}

	if (fq->eof) {
		return;
	}
	memcpy(fq->q[fq->r], src, fq->frameSize);
	fq->ts[fq->r] = pt;
	++fq->r;
	if (fq->r >= fq->maxSize) {
		fq->r -= fq->maxSize;
	}
	++fq->size;

	SDL_CondBroadcast(fq->cond);
	SDL_UnlockMutex(fq->mutex);
}

void pullFrameQueue(FrameQueue* fq, uint8_t* dst) {
	if (fq == NULL) {
		return;
	}

	SDL_LockMutex(fq->mutex);
	while (fq->size == 0) {
		SDL_CondWait(fq->cond, fq->mutex);
	}

	memcpy(dst, fq->q[fq->l], fq->frameSize);
	fq->presentationTime = fq->ts[fq->l];
	++fq->l;
	if (fq->l >= fq->maxSize) {
		fq->l -= fq->maxSize;
	}
	--fq->size;

	SDL_CondBroadcast(fq->cond);
	SDL_UnlockMutex(fq->mutex);
}

SDL_bool checkEndedFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return SDL_TRUE;
	}

	SDL_LockMutex(fq->mutex);
	SDL_bool res = fq->eof;
	SDL_UnlockMutex(fq->mutex);
	return res;
}

void setEndedFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return;
	}

	SDL_LockMutex(fq->mutex);
	fq->eof = SDL_TRUE;
	SDL_CondBroadcast(fq->cond);
	SDL_UnlockMutex(fq->mutex);
}

size_t getSizeFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return 0;
	}

	SDL_LockMutex(fq->mutex);
	size_t res = fq->size;
	SDL_UnlockMutex(fq->mutex);
	return res;
}

double getPresentationTimeFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return 0.0;
	}

	SDL_LockMutex(fq->mutex);
	double res = fq->presentationTime;
	SDL_UnlockMutex(fq->mutex);
	return res;
}