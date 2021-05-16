#include "AudioQueue.h"

AudioQueue* createAudioQueue(const size_t bufferSize) {
	AudioQueue* res = (AudioQueue*)malloc(sizeof(AudioQueue));
	if (res == NULL) {
		return NULL;
	}
	memset(res, 0, sizeof(AudioQueue));
	res->maxSize = bufferSize;

	res->q = (uint8_t*)malloc(bufferSize);
	if (res->q == NULL) {
		destroyAudioQueue(res);
		return NULL;
	}

	res->ts = (double*)malloc(bufferSize * sizeof(double));
	if (res->ts == NULL) {
		destroyAudioQueue(res);
		return NULL;
	}

	res->mutex = SDL_CreateMutex();
	if (res->mutex == NULL) {
		destroyAudioQueue(res);
		return NULL;
	}

	res->cond = SDL_CreateCond();
	if (res->cond == NULL) {
		destroyAudioQueue(res);
		return NULL;
	}

	return res;
}

void destroyAudioQueue(AudioQueue* audioq) {
	if (audioq == NULL) {
		return;
	}

	if (audioq->q != NULL) {
		free((void*)audioq->q);
	}
	if (audioq->ts != NULL) {
		free((void*)audioq->ts);
	}
	if (audioq->cond != NULL) {
		SDL_DestroyCond(audioq->cond);
	}
	if (audioq->mutex != NULL) {
		SDL_DestroyMutex(audioq->mutex);
	}

	free((void*)audioq);
}

void pushAudioQueue(AudioQueue* audioq, const uint8_t* src, size_t len, const double pt) {
	if (audioq == NULL) {
		return;
	}

	SDL_LockMutex(audioq->mutex);
	while (audioq->sz + len > audioq->maxSize && !audioq->eof) {
		SDL_CondWait(audioq->cond, audioq->mutex);
	}

	if (audioq->eof) {
		return;
	}

	if (audioq->r + len >= audioq->maxSize) {
		size_t b1 = audioq->maxSize - audioq->r;
		size_t b2 = len - b1;
		memcpy(audioq->q + audioq->r, src, b1);
		memcpy(audioq->q, src + b1, b2);
		for (size_t i = audioq->r; i != b2;) {
			audioq->ts[i] = pt;
			++i;
			if (i >= audioq->maxSize) {
				i -= audioq->maxSize;
			}
		}
		audioq->sz += len;
		audioq->r = b2;
	} else {
		memcpy(audioq->q + audioq->r, src, len);
		for (size_t i = 0; i < len; ++i) {
			audioq->ts[audioq->r + i] = pt;
		}
		audioq->r += len;
		if (audioq->r >= audioq->maxSize) {
			audioq->r -= audioq->maxSize;
		}
		audioq->sz += len;
	}

	SDL_CondBroadcast(audioq->cond);
	SDL_UnlockMutex(audioq->mutex);
}

void pullAudioQueue(AudioQueue* audioq, uint8_t* dst, size_t len) {
	if (audioq == NULL) {
		return;
	}

	SDL_LockMutex(audioq->mutex);
	while (audioq->sz < len && !audioq->eof) {
		SDL_CondWait(audioq->cond, audioq->mutex);
	}

	audioq->presentationTime = audioq->ts[audioq->l];
	if (audioq->l + len >= audioq->maxSize) {
		size_t b1 = audioq->maxSize - audioq->l;
		size_t b2 = len - b1;
		memcpy(dst, audioq->q + audioq->l, b1);
		memcpy(dst + b1, audioq->q, b2);
		audioq->l = b2;
		audioq->sz -= len;
	} else if (len <= audioq->sz) {
		memcpy(dst, audioq->q + audioq->l, len);
		audioq->l += len;
		if (audioq->l >= audioq->maxSize) {
			audioq->l -= audioq->maxSize;
		}
		audioq->sz -= len;
	} else {
		memset(dst, 0, len);
		if (audioq->r >= audioq->l) {
			memcpy(dst, audioq->q + audioq->l, audioq->r - audioq->l);
		} else {
			size_t block = audioq->maxSize - audioq->l;
			memcpy(dst, audioq->q + audioq->l, block);
			memcpy(dst + block, audioq->q, audioq->r);
		}
		audioq->sz = 0;
		audioq->l = audioq->r;
	}

	SDL_CondBroadcast(audioq->cond);
	SDL_UnlockMutex(audioq->mutex);
}

SDL_bool checkEndedAudioQueue(AudioQueue* audioq) {
	if (audioq == NULL) {
		return SDL_TRUE;
	}

	SDL_LockMutex(audioq->mutex);
	SDL_bool res = audioq->eof;
	SDL_CondBroadcast(audioq->cond);
	SDL_UnlockMutex(audioq->mutex);

	return res;
}

void setEndedAudioQueue(AudioQueue* audioq) {
	if (audioq == NULL) {
		return;
	}

	SDL_LockMutex(audioq->mutex);
	audioq->eof = SDL_TRUE;
	SDL_CondBroadcast(audioq->cond);
	SDL_UnlockMutex(audioq->mutex);
}

size_t getSizeAudioQueue(AudioQueue* audioq) {
	if (audioq == NULL) {
		return 0;
	}

	SDL_LockMutex(audioq->mutex);
	size_t res = audioq->sz;
	SDL_UnlockMutex(audioq->mutex);
	return res;
}

double getPresentationTimeAudioQueue(AudioQueue* audioq) {
	if (audioq == NULL) {
		return 0.0;
	}

	SDL_LockMutex(audioq->mutex);
	double res = audioq->presentationTime;
	SDL_UnlockMutex(audioq->mutex);
	return res;
}
