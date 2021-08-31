#include "VideoFile.h"
#include "VideoPlayer.h"

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    if (argc < 2) {
        fprintf(stderr, "No file provided\n");
        return 1;
    }

    VideoFile* pVideoFile = allocVideoFile();
    if (pVideoFile == NULL) {
        fprintf(stderr, "Couldn't allocate video file context\n");
        return 1;
    }

    if (openVideoFile(pVideoFile, argv[1]) < 0) {
        fprintf(stderr, "Couldn't open video file\n");
        destroyVideoFile(pVideoFile);
        return 1;
    }

    if (findMediaStreams(pVideoFile) < 0) {
        fprintf(stderr, "Couldn't find video stream\n");
        destroyVideoFile(pVideoFile);
        return 1;
    }

    VideoPlayer* pVideoPlayer = createVideoPlayer(pVideoFile);
    if (pVideoPlayer == NULL) {
        fprintf(stderr, "Couldn't open video player!");
        destroyVideoFile(pVideoFile);
        return 1;
    }

    startVideoPlayer(pVideoPlayer);

    destroyVideoPlayer(pVideoPlayer);
    destroyVideoFile(pVideoFile);
    SDL_Quit();

    return 0;
}