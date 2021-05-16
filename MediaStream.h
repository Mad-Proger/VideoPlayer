#pragma once
#include "include.h"
#include "PacketQueue.h"

typedef struct MediaStream {
	double baseTime;
	unsigned int streamIndex;
	const AVCodec* pStreamCodec;
	AVCodecContext* pStreamCodecContext;
	PacketQueue* pq;
} MediaStream;

void destroyMediaStream(MediaStream* pMediaStream);

MediaStream* createMediaStream(AVStream* stream, unsigned int idx);