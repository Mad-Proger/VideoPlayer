#include "FrameQueue.h"

FrameQueue* createFrameQueue(size_t queueSize, int pixelFormat, int width, int height) {
	FrameQueue* res = (FrameQueue*)malloc(sizeof(FrameQueue));
	if (res == NULL) {
		return NULL;
	}
	memset(res, 0, sizeof(FrameQueue));

	res->q = (AVFrame**)malloc(queueSize * sizeof(AVFrame*));
	if (res->q == NULL) {
		destroyFrameQueue(res);
		return NULL;
	}

	for (size_t i = 0; i < queueSize; ++i) {
		res->q[i] = av_frame_alloc();
		if (res->q[i] == NULL) {
			destroyFrameQueue(res);
			return NULL;
		}

		res->q[i]->format = pixelFormat;
		res->q[i]->width = width;
		res->q[i]->height = height;

		if (av_frame_get_buffer(res->q[i], 0) < 0) {
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
			av_frame_unref(fq->q[i]);
			av_frame_free(fq->q + i);
		}
	}
	free((void*)fq->q);

	SDL_DestroyMutex(fq->mutex);
	SDL_DestroyCond(fq->cond);

	free((void*)fq);
}

void pushFrameQueue(FrameQueue* fq, const AVFrame* src) {
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

	fq->q[fq->r]->pts = src->pts;
	av_frame_copy(fq->q[fq->r], src);
	++fq->r;
	if (fq->r >= fq->maxSize) {
		fq->r -= fq->maxSize;
	}
	++fq->size;

	SDL_CondBroadcast(fq->cond);
	SDL_UnlockMutex(fq->mutex);
}

void pullFrameQueue(FrameQueue* fq, SDL_Texture* dst) {
	if (fq == NULL) {
		return;
	}

	SDL_LockMutex(fq->mutex);
	while (fq->size == 0) {
		SDL_CondWait(fq->cond, fq->mutex);
	}

	void* pixels;
	int linesize;
	SDL_LockTexture(dst, NULL, &pixels, &linesize);
	memcpy(pixels, fq->q[fq->l]->data[0], linesize * fq->q[fq->l]->height);
	SDL_UnlockTexture(dst);

	fq->pts = fq->q[fq->l]->pts;
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

int64_t getPTSFrameQueue(FrameQueue* fq) {
	if (fq == NULL) {
		return 0;
	}

	SDL_LockMutex(fq->mutex);
	int64_t res = fq->pts;
	SDL_UnlockMutex(fq->mutex);
	return res;
}