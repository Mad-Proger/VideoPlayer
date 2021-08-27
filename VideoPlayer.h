#pragma once
#include "include.h"
#include "VideoFile.h"
#include "AudioQueue.h"
#include "FrameQueue.h"

typedef struct VideoPlayer {
	VideoFile* vf;

	SDL_Thread* demuxThread;
	SDL_Thread* decodeAudioThread;
	SDL_Thread* decodeVideoThread;
	SDL_Thread* renderVideoThread;

	AudioQueue* audioSampleQueue;
	FrameQueue* videoFrameQueue;

	int width;
	int height;
	SDL_Rect renderRect;
	SDL_bool quit;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* videoFrameTexture;
	uint8_t* audioSampleBuffer;
	SDL_AudioDeviceID audioDeviceID;

	struct SwsContext* videoConvertContext;
	SwrContext* audioConvertContext;
} VideoPlayer;

VideoPlayer* createVideoPlayer(VideoFile* vf);

void destroyVideoPlayer(VideoPlayer* vp);

void startVideoPlayer(VideoPlayer* vp);

void audioCallbackVideoPlayer(void* userdata, uint8_t* dst, int len);

int demuxThreadVideoPlayer(void* data);

int decodeAudioThreadVideoPlayer(void* data);

int decodeVideoThreadVideoPlayer(void* data);

int renderVideoThreadVideoPlayer(void* data);