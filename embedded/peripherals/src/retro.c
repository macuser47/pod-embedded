#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <bbgpio.h>
#include <retro.h>
#include <poll.h>
#include <time.h>
#include <data.h>

#define TIMEOUT 1000	/* 1 second */
#define BUF_LEN 256

static pthread_t thread;

static int onTapeStrip (int retroNum);
static int waitForStrip (int retroNum);
static int getPin(int retroNum);

extern data_t *data;
static time_t *retroTimers[3];
int initRetro() {
	retroTimers[RETRO_1] = &(data->timers->lastRetro1);
	retroTimers[RETRO_2] = &(data->timers->lastRetro2);
	retroTimers[RETRO_3] = &(data->timers->lastRetro3);
	bbGpioExport(RETRO_1_PIN);
	bbGpioSetDir(RETRO_1_PIN, IN_DIR);
	bbGpioSetEdge(RETRO_1_PIN, FALLING_EDGE);
	waitForStrip(RETRO_1);
	//int ret = pthread_create( &thread, NULL, waitForStrip, RETRO_1);
	return 0;
}

/* Not voting yet! */
static int onTapeStrip(int retroNum) {
	unsigned long currTime = (unsigned long) time(NULL);
	printf("TIME: %lu\n", (unsigned long) *retroTimers[retroNum]);
	if ((currTime - (unsigned long) *retroTimers[retroNum]) > 3) {
		*retroTimers[retroNum] = currTime;
		data->motion->retroCount++;
		printf("VALID STRIP\n");
	}
	printf("\nTAPE STRIP DETECTED\n");
	return 0;
}

/* TODO Debatting having this poll on 3 FDs or just having three FDs poll seperately
   Not sure which would be faster, currently leaning towards three threads becasue
   of the sensitivity of what it will be measuring
   */
int waitForStrip(int retroNum) {
	int gpioFd = bbGpioFdOpen(getPin(retroNum));
	struct pollfd fds[2];
	int nfds = 2;
	int ret, len;
	char buf[BUF_LEN];
	printf("Starting loop\n");
	while (1) {
		memset((void *)fds, 0, sizeof(fds));

		fds[0].fd = STDIN_FILENO;
		fds[0].events = POLLIN;

		fds[1].fd = gpioFd;
		fds[1].events = POLLPRI;

		ret = poll(fds, nfds, TIMEOUT);

		if (ret < 0) {
			printf("\npoll() failed!\n");
		}

		if (ret == 0) {
			/* If nothing is detected */
			printf("Retro Count = %d", data->motion->retroCount);
			//			printf(".");
		}

		if (fds[1].revents & POLLPRI) {
			lseek(fds[1].fd, 0, SEEK_SET);
			len = read(fds[1].fd, buf, BUF_LEN);
			onTapeStrip(retroNum);
		}

		if (fds[0].revents & POLLIN) {
			(void) read(fds[0].fd, buf, 1);
		}
		fflush(stdout);
	}
	printf("ending\n");
	return 0;
}

static int getPin(int retroNum) {
	switch(retroNum) {
		case RETRO_1:
			return RETRO_1_PIN;
		case RETRO_2:
			return RETRO_2_PIN;
		case RETRO_3:
			return RETRO_3_PIN;
		default:
			fprintf(stderr, "Invalid retro number, error...\n");
			return -1;
	}
	return -1;	/* Will never get here */
}
