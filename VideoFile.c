#include "VideoFile.h"

VideoFile* allocVideoFile() {
    VideoFile* vf = (VideoFile*)malloc(sizeof(VideoFile));
    if (vf == NULL)
        return NULL;

    vf->pFormatContext = avformat_alloc_context();
    if (vf->pFormatContext == NULL) {
        free((void*)vf);
        return NULL;
    }

    vf->pAudioStream = NULL;
    vf->pVideoStream = NULL;

    return vf;
}

int openVideoFile(VideoFile* vf, const char* filepath) {
    int res = avformat_open_input(&vf->pFormatContext, filepath, NULL, NULL);
    if (res < 0) {
        return res;
    }

    res = avformat_find_stream_info(vf->pFormatContext, NULL);
    if (res < 0) {
        return res;
    } else {
        return 0;
    }
}

int findMediaStreams(VideoFile* vf) {
    if (vf == NULL ||
        vf->pFormatContext == NULL ||
        vf->pFormatContext->streams == NULL) {
        return AVERROR_INVALIDDATA;
    }

    for (unsigned int i = 0; i < vf->pFormatContext->nb_streams; ++i) {
        if (vf->pFormatContext->streams[i] == NULL ||
            vf->pFormatContext->streams[i]->codecpar == NULL) {
            continue;
        }

        if (vf->pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vf->pVideoStream == NULL) {
            vf->pVideoStream = createMediaStream(vf->pFormatContext->streams[i], i);
        }

        if (vf->pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && vf->pAudioStream == NULL) {
            vf->pAudioStream = createMediaStream(vf->pFormatContext->streams[i], i);
        }
    }

    if (vf->pVideoStream == NULL) {
        return AVERROR_STREAM_NOT_FOUND;
    } else {
        return 0;
    }
}

void destroyVideoFile(VideoFile* vf) {
    if (vf == NULL)
        return;

    destroyMediaStream(vf->pVideoStream);
    destroyMediaStream(vf->pAudioStream);
    avformat_close_input(&vf->pFormatContext);
    avformat_free_context(vf->pFormatContext);

    free((void*)vf);
}
