#include "PacketQueue.h"

const size_t START_PACKET_QUEUE_SIZE = 1000;

PacketQueue* createPacketQueue() {
	PacketQueue* res = (PacketQueue*)malloc(sizeof(PacketQueue));
	if (res == NULL) {
		return NULL;
	}
	memset(res, 0, sizeof(PacketQueue));

	res->q = (AVPacket**)malloc(START_PACKET_QUEUE_SIZE * sizeof(AVPacket*));
	if (res->q == NULL) {
		destroyPacketQueue(res);
		return NULL;
	}

	res->maxSize = START_PACKET_QUEUE_SIZE;

	res->mutex = SDL_CreateMutex();
	if (res->mutex == NULL) {
		destroyPacketQueue(res);
		return NULL;
	}

	res->cond = SDL_CreateCond();
	if (res->cond == NULL) {
		destroyPacketQueue(res);
		return NULL;
	}

	return res;
}

void destroyPacketQueue(PacketQueue* pq) {
	if (pq == NULL) {
		return;
	}

	if (pq->q != NULL) {
		free((void*)pq->q);
	}

	if (pq->cond != NULL) {
		SDL_DestroyCond(pq->cond);
	}
	if (pq->mutex != NULL) {
		SDL_DestroyMutex(pq->mutex);
	}

	free((void*)pq);
}

void pushPacketQueue(PacketQueue* pq, AVPacket* pkt) {
	if (pq == NULL) {
		return;
	}
	if (pq->eof) {
		return;
	}

	SDL_LockMutex(pq->mutex);

	if (pq->sz == pq->maxSize) {
		AVPacket** nq = (AVPacket**)malloc(2 * pq->maxSize * sizeof(AVPacket*));
		size_t ind = 0;
		for (size_t i = 0, pos = pq->l; i < pq->sz; ++i) {
			nq[ind++] = pq->q[pos];
			++pos;
			if (pos >= pq->maxSize) {
				pos -= pq->maxSize;
			}
		}

		free((void*)pq->q);
		pq->q = nq;
		pq->l = 0;
		pq->r = pq->sz = ind;
		pq->maxSize *= 2;
	}

	pq->q[pq->r] = pkt;
	++pq->r;
	if (pq->r >= pq->maxSize)
		pq->r -= pq->maxSize;
	++pq->sz;

	pq->endPTS = pkt->pts;
	if (pq->sz == 1) {
		pq->startPTS = pq->endPTS;
	}

	SDL_CondBroadcast(pq->cond);
	SDL_UnlockMutex(pq->mutex);
}

void pullPacketQueue(PacketQueue* pq, AVPacket** pkt) {
	if (pq == NULL) {
		*pkt = NULL;
		return;
	}

	SDL_LockMutex(pq->mutex);
	while (pq->sz == 0 && !pq->eof) {
		SDL_CondWait(pq->cond, pq->mutex);
	}

	if (pq->sz > 0) {
		*pkt = pq->q[pq->l];
		++pq->l;
		if (pq->l >= pq->maxSize) {
			pq->l -= pq->maxSize;
		}
		--pq->sz;

		if (pq->sz == 0) {
			pq->startPTS = 0;
			pq->endPTS = 0;
		} else {
			pq->startPTS = pq->q[pq->l]->pts;
		}
	} else {
		*pkt = NULL;
		pq->startPTS = 0;
		pq->endPTS = 0;
	}

	SDL_UnlockMutex(pq->mutex);
}

SDL_bool checkEndedPacketQueue(PacketQueue* pq) {
	if (pq == NULL) {
		return 1;
	}

	SDL_LockMutex(pq->mutex);
	SDL_bool res = pq->eof;
	SDL_UnlockMutex(pq->mutex);
	return res;
}

void setEndedPacketQueue(PacketQueue* pq) {
	if (pq == NULL) {
		return;
	}

	SDL_LockMutex(pq->mutex);
	pq->eof = SDL_TRUE;
	SDL_UnlockMutex(pq->mutex);
	SDL_CondBroadcast(pq->cond);
}

int64_t getTimePacketQueue(PacketQueue* pq) {
	if (pq == NULL) {
		return 0;
	}

	SDL_LockMutex(pq->mutex);
	int64_t res = pq->endPTS - pq->startPTS;
	SDL_UnlockMutex(pq->mutex);
	return res;
}