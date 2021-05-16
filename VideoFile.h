#pragma once
#include "include.h"
#include "MediaStream.h"

typedef struct VideoFile {
	AVFormatContext* pFormatContext;
	MediaStream* pVideoStream;
	MediaStream* pAudioStream;
} VideoFile;

VideoFile* allocVideoFile();

int openVideoFile(VideoFile* vf, const char* filepath);

int findMediaStreams(VideoFile* vf);

void destroyVideoFile(VideoFile* vf);