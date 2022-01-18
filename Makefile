INCLUDE_PATH ?= /usr/local/include
C_COMPILER = ${CROSS_PREFIX}gcc
LIBRARIES != pkg-config --libs --static libavformat libavcodec libswscale libswresample sdl2

build: AudioQueue.o FrameQueue.o main.o MediaStream.o PacketQueue.o VideoFile.o VideoPlayer.o
	${C_COMPILER} -o VideoPlayer AudioQueue.o FrameQueue.o main.o MediaStream.o PacketQueue.o VideoFile.o VideoPlayer.o ${LIBRARIES}

AudioQueue.o:
	${C_COMPILER} -o AudioQueue.o -c AudioQueue.c -I/${INCLUDE_PATH}

FrameQueue.o:
	${C_COMPILER} -o FrameQueue.o -c FrameQueue.c -I/${INCLUDE_PATH}

main.o:
	${C_COMPILER} -o main.o -c main.c -I/${INCLUDE_PATH}

MediaStream.o:
	${C_COMPILER} -o MediaStream.o -c MediaStream.c -I/${INCLUDE_PATH}

PacketQueue.o:
	${C_COMPILER} -o PacketQueue.o -c PacketQueue.c -I/${INCLUDE_PATH}

VideoFile.o:
	${C_COMPILER} -o VideoFile.o -c VideoFile.c -I/${INCLUDE_PATH}

VideoPlayer.o:
	${C_COMPILER} -o VideoPlayer.o -c VideoPlayer.c -I/${INCLUDE_PATH}

clean:
	rm *.o
