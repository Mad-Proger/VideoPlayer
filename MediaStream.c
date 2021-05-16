#include "MediaStream.h"

void destroyMediaStream(MediaStream* pMediaStream) {
	if (pMediaStream == NULL) {
		return;
	}

	avcodec_close(pMediaStream->pStreamCodecContext);
	avcodec_free_context(&pMediaStream->pStreamCodecContext);
	destroyPacketQueue(pMediaStream->pq);
	free((void*)pMediaStream);
}

MediaStream* createMediaStream(AVStream* stream, unsigned int idx) {
	MediaStream* ms = (MediaStream*)malloc(sizeof(MediaStream));
	if (ms == NULL) {
		return NULL;
	}
	memset(ms, 0, sizeof(MediaStream));

	ms->pq = createPacketQueue();
	if (ms->pq == NULL) {
		destroyMediaStream(ms);
		return NULL;
	}

	ms->streamIndex = idx;
	ms->pStreamCodec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (ms->pStreamCodec == NULL) {
		destroyMediaStream(ms);
		return NULL;
	}

	ms->pStreamCodecContext = avcodec_alloc_context3(ms->pStreamCodec);
	if (ms->pStreamCodecContext == NULL) {
		destroyMediaStream(ms);
		return NULL;
	}

	if (avcodec_parameters_to_context(ms->pStreamCodecContext, stream->codecpar) < 0) {
		destroyMediaStream(ms);
		return NULL;
	}

	if (avcodec_open2(ms->pStreamCodecContext, ms->pStreamCodec, NULL) < 0) {
		destroyMediaStream(ms);
		return NULL;
	}

	ms->baseTime = av_q2d(stream->time_base);

	return ms;
}
