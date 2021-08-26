#include "VideoPlayer.h"

VideoPlayer* createVideoPlayer(VideoFile* vf) {
	if (vf == NULL) {
		return NULL;
	}

	VideoPlayer* vp = (VideoPlayer*)malloc(sizeof(VideoPlayer));
	if (vp == NULL) {
		return NULL;
	}
	memset(vp, 0, sizeof(VideoPlayer));

	vp->vf = vf;

	SDL_Rect screenBounds;
	if (SDL_GetDisplayBounds(0, &screenBounds) < 0) {
		destroyVideoPlayer(vp);
		return NULL;
	}
	vp->width = screenBounds.w;
	vp->height = screenBounds.h;
	vp->renderRect = screenBounds;

	vp->videoFrameQueue = createFrameQueue((size_t)av_q2d(vf->pFormatContext->streams[vf->pVideoStream->streamIndex]->r_frame_rate), vp->renderRect.w * vp->renderRect.h * 4);
	if (vp->videoFrameQueue == NULL) {
		destroyVideoPlayer(vp);
		return NULL;
	}

	vp->videoConvertContext = sws_getContext(vf->pVideoStream->pStreamCodecContext->width, vf->pVideoStream->pStreamCodecContext->height, vf->pVideoStream->pStreamCodecContext->pix_fmt,
		vp->renderRect.w, vp->renderRect.h, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, 0);
	if (vp->videoConvertContext == NULL) {
		destroyVideoPlayer(vp);
		return NULL;
	}

	vp->audioConvertContext = swr_alloc_set_opts(NULL, vf->pAudioStream->pStreamCodecContext->channel_layout, AV_SAMPLE_FMT_FLT, vf->pAudioStream->pStreamCodecContext->sample_rate,
		vf->pAudioStream->pStreamCodecContext->channel_layout, vf->pAudioStream->pStreamCodecContext->sample_fmt, vf->pAudioStream->pStreamCodecContext->sample_rate, 0, NULL);
	if (vp->audioConvertContext == NULL) {
		destroyVideoPlayer(vp);
		return NULL;
	}
	if (swr_init(vp->audioConvertContext) < 0) {
		destroyVideoPlayer(vp);
		return NULL;
	}

	return vp;
}

void destroyVideoPlayer(VideoPlayer* vp) {
	if (vp == NULL) {
		return;
	}

	destroyFrameQueue(vp->videoFrameQueue);

	SDL_DestroyWindow(vp->window);
	SDL_DestroyRenderer(vp->renderer);
	SDL_DestroyTexture(vp->videoFrameTexture);
	SDL_CloseAudioDevice(vp->audioDeviceID);

	sws_freeContext(vp->videoConvertContext);
	swr_free(&vp->audioConvertContext);

	free((void*)vp);
}

void startVideoPlayer(VideoPlayer* vp) {
	if (vp == NULL) {
		return;
	}

	vp->window = SDL_CreateWindow("Video player", 0, 0, vp->width, vp->height,
		SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_SKIP_TASKBAR);
	if (vp->window == NULL) {
		return;
	}

	vp->renderer = SDL_CreateRenderer(vp->window, -1, SDL_RENDERER_ACCELERATED);
	if (vp->renderer == NULL) {
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
		return;
	}

	vp->videoFrameTexture = SDL_CreateTexture(vp->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, vp->renderRect.w, vp->renderRect.h);
	if (vp->videoFrameTexture == NULL) {
		SDL_DestroyRenderer(vp->renderer);
		vp->renderer = NULL;
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
		return;
	}

	vp->videoFramePixelBuffer = (uint8_t*)malloc(vp->renderRect.w * vp->renderRect.h * 4);
	if (vp->videoFramePixelBuffer == NULL) {
		SDL_DestroyTexture(vp->videoFrameTexture);
		vp->videoFrameTexture = NULL;
		SDL_DestroyRenderer(vp->renderer);
		vp->renderer = NULL;
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
		return;
	}

	SDL_AudioSpec desired, received;
	memset(&desired, 0, sizeof(SDL_AudioSpec));
	desired.callback = audioCallbackVideoPlayer;
	desired.userdata = (void*)vp;
	desired.channels = vp->vf->pAudioStream->pStreamCodecContext->channels;
	desired.format = AUDIO_F32;
	desired.freq = vp->vf->pAudioStream->pStreamCodecContext->sample_rate;
	desired.silence = 0;
	desired.samples = 1 << 11;

	vp->audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &desired, &received, 0);
	if (vp->audioDeviceID == 0) {
		free((void*)vp->videoFramePixelBuffer);
		vp->videoFramePixelBuffer = NULL;
		SDL_DestroyTexture(vp->videoFrameTexture);
		vp->videoFrameTexture = NULL;
		SDL_DestroyRenderer(vp->renderer);
		vp->renderer = NULL;
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
		return;
	}
	SDL_PauseAudioDevice(vp->audioDeviceID, 1);

	vp->audioSampleBuffer = (uint8_t*)malloc(received.size);
	if (vp->audioSampleBuffer == NULL) {
		SDL_CloseAudioDevice(vp->audioDeviceID);
		free((void*)vp->videoFramePixelBuffer);
		vp->videoFramePixelBuffer = NULL;
		SDL_DestroyTexture(vp->videoFrameTexture);
		vp->videoFrameTexture = NULL;
		SDL_DestroyRenderer(vp->renderer);
		vp->renderer = NULL;
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
	}

	vp->audioSampleQueue = createAudioQueue(received.size * 16);
	if (vp->audioSampleQueue == NULL) {
		SDL_CloseAudioDevice(vp->audioDeviceID);
		free((void*)vp->audioSampleBuffer);
		free((void*)vp->videoFramePixelBuffer);
		vp->videoFramePixelBuffer = NULL;
		SDL_DestroyTexture(vp->videoFrameTexture);
		vp->videoFrameTexture = NULL;
		SDL_DestroyRenderer(vp->renderer);
		vp->renderer = NULL;
		SDL_DestroyWindow(vp->window);
		vp->window = NULL;
	}

	vp->demuxThread = SDL_CreateThread(demuxThreadVideoPlayer, "demux thread", (void*)vp);
	vp->decodeAudioThread = SDL_CreateThread(decodeAudioThreadVideoPlayer, "audio decode thread", (void*)vp);
	vp->decodeVideoThread = SDL_CreateThread(decodeVideoThreadVideoPlayer, "video decode thread", (void*)vp);
	vp->renderVideoThread = SDL_CreateThread(renderVideoThreadVideoPlayer, "video render thread", (void*)vp);

	while (!vp->quit) {
		SDL_Event evnt;
		while (SDL_PollEvent(&evnt)) {
			if (evnt.type == SDL_QUIT) {
				break;
			}
		}

		SDL_Delay(15);
	}

	setEndedPacketQueue(vp->vf->pAudioStream->pq);
	setEndedPacketQueue(vp->vf->pVideoStream->pq);
	setEndedAudioQueue(vp->audioSampleQueue);
	setEndedFrameQueue(vp->videoFrameQueue);

	SDL_WaitThread(vp->demuxThread, NULL);
	SDL_WaitThread(vp->decodeAudioThread, NULL);
	SDL_WaitThread(vp->decodeVideoThread, NULL);
	SDL_WaitThread(vp->renderVideoThread, NULL);

	destroyAudioQueue(vp->audioSampleQueue);
	SDL_CloseAudioDevice(vp->audioDeviceID);
	free((void*)vp->audioSampleBuffer);
	free((void*)vp->videoFramePixelBuffer);
	vp->videoFramePixelBuffer = NULL;
	SDL_DestroyTexture(vp->videoFrameTexture);
	vp->videoFrameTexture = NULL;
	SDL_DestroyRenderer(vp->renderer);
	vp->renderer = NULL;
	SDL_DestroyWindow(vp->window);
	vp->window = NULL;
}

void audioCallbackVideoPlayer(void* userdata, uint8_t* dst, int len) {
	VideoPlayer* vp = (VideoPlayer*)userdata;
	pullAudioQueue(vp->audioSampleQueue, dst, len);
}

int demuxThreadVideoPlayer(void* data) {
	VideoPlayer* vp = (VideoPlayer*)data;
	double audioBaseTime = vp->vf->pAudioStream->baseTime;
	double videoBaseTime = vp->vf->pVideoStream->baseTime;
	const double queueDelta = 6.0;

	while (!checkEndedPacketQueue(vp->vf->pVideoStream->pq) && !checkEndedPacketQueue(vp->vf->pVideoStream->pq)) {
		if (getTimePacketQueue(vp->vf->pVideoStream->pq) * videoBaseTime < queueDelta ||
			getTimePacketQueue(vp->vf->pAudioStream->pq) * audioBaseTime < queueDelta) {
			AVPacket* pPacket = av_packet_alloc();
			int read = av_read_frame(vp->vf->pFormatContext, pPacket);

			if (read == 0) {
				if (pPacket->stream_index == vp->vf->pVideoStream->streamIndex) {
					pushPacketQueue(vp->vf->pVideoStream->pq, pPacket);
				} else if (pPacket->stream_index == vp->vf->pAudioStream->streamIndex) {
					pushPacketQueue(vp->vf->pAudioStream->pq, pPacket);
				} else {
					av_packet_unref(pPacket);
				}
			} else {
				fprintf(stderr, "Couldn't get packet\n");
				fflush(stderr);
				break;
			}
		} else {
			SDL_Delay(3000);
		}
	}

	setEndedPacketQueue(vp->vf->pVideoStream->pq);
	setEndedPacketQueue(vp->vf->pAudioStream->pq);

	return 0;
}

int decodeAudioThreadVideoPlayer(void* data) {
	VideoPlayer* vp = (VideoPlayer*)data;

	double audioBaseTime = vp->vf->pAudioStream->baseTime;

	AVFrame* pAudioSample = av_frame_alloc();
	AVFrame* pDecodedSample = av_frame_alloc();

	while (!checkEndedAudioQueue(vp->audioSampleQueue)) {
		AVPacket* pPacket = NULL;
		pullPacketQueue(vp->vf->pAudioStream->pq, &pPacket);
		if (pPacket == NULL) {
			break;
		}

		int sent = avcodec_send_packet(vp->vf->pAudioStream->pStreamCodecContext, pPacket);
		int received = avcodec_receive_frame(vp->vf->pAudioStream->pStreamCodecContext, pAudioSample);

		if (sent == 0 && received == 0 && !vp->quit) {
			pDecodedSample->nb_samples = pAudioSample->nb_samples;
			pDecodedSample->format = AV_SAMPLE_FMT_FLT;
			pDecodedSample->sample_rate = pAudioSample->sample_rate;
			pDecodedSample->channel_layout = pAudioSample->channel_layout;

			swr_convert_frame(vp->audioConvertContext, pDecodedSample, pAudioSample);
			pushAudioQueue(vp->audioSampleQueue, pDecodedSample->data[0],
				av_samples_get_buffer_size(pDecodedSample->linesize, pDecodedSample->channels, pDecodedSample->nb_samples, AV_SAMPLE_FMT_FLT, 0), pAudioSample->pts * audioBaseTime);
			av_frame_unref(pDecodedSample);
		}
		av_packet_unref(pPacket);
	}

	setEndedAudioQueue(vp->audioSampleQueue);

	return 0;
}

int decodeVideoThreadVideoPlayer(void* data) {
	VideoPlayer* vp = (VideoPlayer*)data;

	double videoBaseTime = vp->vf->pVideoStream->baseTime;

	AVFrame* pVideoFrame = av_frame_alloc();
	AVFrame* pDecodedFrame = av_frame_alloc();
	pDecodedFrame->width = vp->width;
	pDecodedFrame->height = vp->height;
	pDecodedFrame->format = AV_PIX_FMT_RGBA;
	av_frame_get_buffer(pDecodedFrame, 1);

	while (!checkEndedFrameQueue(vp->videoFrameQueue)) {
		AVPacket* pPacket = NULL;
		pullPacketQueue(vp->vf->pVideoStream->pq, &pPacket);
		if (pPacket == NULL) {
			break;
		}

		int sent = avcodec_send_packet(vp->vf->pVideoStream->pStreamCodecContext, pPacket);
		int received = avcodec_receive_frame(vp->vf->pVideoStream->pStreamCodecContext, pVideoFrame);

		if (sent == 0 && received == 0 && !vp->quit) {
			sws_scale(vp->videoConvertContext, (const uint8_t* const*)pVideoFrame->data, pVideoFrame->linesize, 0, pVideoFrame->height, pDecodedFrame->data, pDecodedFrame->linesize);
			pushFrameQueue(vp->videoFrameQueue, pDecodedFrame->data[0], pVideoFrame->pts * videoBaseTime);
		}

		av_packet_unref(pPacket);
	}

	setEndedFrameQueue(vp->videoFrameQueue);

	return 0;
}

int renderVideoThreadVideoPlayer(void* data) {
	VideoPlayer* vp = (VideoPlayer*)data;

	double fps = av_q2d(vp->vf->pFormatContext->streams[vp->vf->pVideoStream->streamIndex]->r_frame_rate);
	uint32_t delay = (uint32_t)1000.0 / fps;

	SDL_PauseAudioDevice(vp->audioDeviceID, 0);
	while (!vp->quit) {
		if (checkEndedFrameQueue(vp->videoFrameQueue) && getSizeFrameQueue(vp->videoFrameQueue) == 0) {
			break;
		}
		if (checkEndedAudioQueue(vp->audioSampleQueue) && getSizeAudioQueue(vp->audioSampleQueue) == 0) {
			break;
		}

		double delta = getPresentationTimeFrameQueue(vp->videoFrameQueue) - getPresentationTimeAudioQueue(vp->audioSampleQueue);
		while (delta > 0.1 && !vp->quit) {
			SDL_Delay(20);
			delta = getPresentationTimeFrameQueue(vp->videoFrameQueue) - getPresentationTimeAudioQueue(vp->audioSampleQueue);
		}
		if (vp->quit) {
			break;
		}

		uint32_t beginTicks = SDL_GetTicks();

		SDL_RenderClear(vp->renderer);
		pullFrameQueue(vp->videoFrameQueue, vp->videoFramePixelBuffer);
		SDL_UpdateTexture(vp->videoFrameTexture, NULL, vp->videoFramePixelBuffer, vp->renderRect.w * 4);
		SDL_RenderCopy(vp->renderer, vp->videoFrameTexture, NULL, &vp->renderRect);
		SDL_RenderPresent(vp->renderer);

		uint32_t elapsedTime = SDL_GetTicks() - beginTicks;

		if (-delta <= 0.1 && delay >= elapsedTime) {
			SDL_Delay(delay - elapsedTime);
		}
	}

	SDL_PauseAudioDevice(vp->audioDeviceID, 1);
	vp->quit = SDL_TRUE;

	return 0;
}