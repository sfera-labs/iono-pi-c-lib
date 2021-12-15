/*
 * ionoPi
 *
 *     Copyright (C) 2016-2019 Sfera Labs S.r.l.
 *
 *     For information, see the Iono Pi web site:
 *     http://www.sferalabs.cc/iono-pi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU General Lesser Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/lgpl-3.0.html>.
 *
 */

#include "ionoPi.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <maxdetect.h>
#include <pthread.h>
#include <sys/time.h>

#define MCP_SPI_CHANNEL 			0
#define MCP_SPI_SPEED				50000

#define AI1_AI2_FACTOR 				0.007319f
#define AI3_AI4_FACTOR 				0.000725f

#define ONEWIRE_DEVICES_PATH "/sys/bus/w1/devices/"

#define WIEGAND_MAX_BITS			64

struct DigitalInputConfig {
	int digitalInput;
	int currValue;
	int debouncedValue;
	void (*isrCallBack)(void);
	void (*callBack)(int, int);
	int callBackMode;
	struct timespec debounceTime;
	pthread_t debounceThread;
	int debounceThreadRunning;
	pthread_mutex_t mutex;
};

volatile struct DigitalInputConfig diConfs[10];

volatile struct Wiegand {
	int64_t data;
	int bitCount;
	struct timespec lastBitTs;
	int run;
} w1, w2;

int w1DataRegistered = 0;
int w2DataRegistered = 0;

struct timespec wiegandPulseWidthMax;
unsigned long int wiegandPulseIntervalMin_usec;
unsigned long int wiegandPulseIntervalMax_usec;

/*
 *
 */
int mcp3204Setup() {
	return (wiringPiSPISetup(MCP_SPI_CHANNEL, MCP_SPI_SPEED) != -1);
}

int isRPiBefore4() {
	FILE *fp;
	char model[20];

	fp = fopen("/proc/device-tree/model", "r");
	if (fp == NULL) {
		return TRUE;
	}

	fgets(model, 20, (FILE*) fp);
	fclose(fp);

	if (model[12] != ' ' || model[14] != ' ') {
		return FALSE;
	}

	if (model[13] == '2' || model[13] == '3') {
		return TRUE;
	}

	return FALSE;
}

/*
 * Must be called once at the start of your program execution.
 */
int ionoPiSetup() {
	putenv("WIRINGPI_CODES=1");
	if (wiringPiSetup() != 0) {
		return FALSE;
	}

	pinMode(LED, OUTPUT);

	pinMode(O1, OUTPUT);
	pinMode(O2, OUTPUT);
	pinMode(O3, OUTPUT);
	pinMode(O4, OUTPUT);

	pinMode(OC1, OUTPUT);
	pinMode(OC2, OUTPUT);
	pinMode(OC3, OUTPUT);

	pinMode(DI1, INPUT);
	pinMode(DI2, INPUT);
	pinMode(DI3, INPUT);
	pinMode(DI4, INPUT);
	pinMode(DI5, INPUT);
	pinMode(DI6, INPUT);

	if (isRPiBefore4()) {
		pullUpDnControl(DI1, PUD_OFF);
		pullUpDnControl(DI2, PUD_OFF);
		pullUpDnControl(DI3, PUD_OFF);
		pullUpDnControl(DI4, PUD_OFF);
		pullUpDnControl(DI5, PUD_OFF);
		pullUpDnControl(DI6, PUD_OFF);
	}

	if (!mcp3204Setup()) {
		return FALSE;
	}

	ionoPiSetWiegandPulse(150, 500, 2700);

	return TRUE;
}

/*
 *
 */
void ionoPiPinMode(int pin, int mode) {
	pinMode(pin, mode);
}

/*
 *
 */
void ionoPiDigitalWrite(int output, int value) {
	digitalWrite(output, value);
}

/*
 *
 */
void *waitAndCallDigitalInterruptCB(void* arg) {
	volatile struct DigitalInputConfig* diConf =
			(volatile struct DigitalInputConfig*) arg;
	int currValue = diConf->currValue;
	nanosleep((struct timespec *) &(diConf->debounceTime), NULL);
	if (diConf->debouncedValue != currValue) {
		diConf->debouncedValue = currValue;
		if (diConf->callBack != NULL) {
			if ((diConf->callBackMode == INT_EDGE_RISING && currValue == HIGH)
					|| (diConf->callBackMode == INT_EDGE_FALLING
							&& currValue == LOW)
					|| diConf->callBackMode == INT_EDGE_BOTH) {
				diConf->callBack(diConf->digitalInput, diConf->debouncedValue);
			}
		}
	}
	pthread_mutex_lock((pthread_mutex_t *) &(diConf->mutex));
	diConf->debounceThreadRunning = FALSE;
	pthread_mutex_unlock((pthread_mutex_t *) &(diConf->mutex));
	return NULL;
}

/*
 *
 */
void digitalInterruptCB(int idx) {
	volatile struct DigitalInputConfig* diConf = &diConfs[idx];
	if (diConf->debounceTime.tv_sec == 0 && diConf->debounceTime.tv_nsec == 0) {
		if (diConf->callBack != NULL) {
			if (diConf->callBackMode == INT_EDGE_RISING) {
				diConf->callBack(diConf->digitalInput, HIGH);
			} else if (diConf->callBackMode == INT_EDGE_FALLING) {
				diConf->callBack(diConf->digitalInput, LOW);
			} else {
				diConf->callBack(diConf->digitalInput,
						digitalRead(diConf->digitalInput));
			}
		}
	} else {
		pthread_mutex_lock((pthread_mutex_t *) &(diConf->mutex));
		diConf->currValue = digitalRead(diConf->digitalInput);
		if (diConf->debounceThreadRunning) {
			pthread_cancel(diConf->debounceThread);
		}
		int err = pthread_create((pthread_t *) &(diConf->debounceThread), NULL,
				waitAndCallDigitalInterruptCB, (void *) diConf);
		if (err == 0) {
			diConf->debounceThreadRunning = TRUE;
			pthread_detach(diConf->debounceThread);
		} else {
			fprintf(stderr, "error creating new thread [%d]\n", err);
		}
		pthread_mutex_unlock((pthread_mutex_t *) &(diConf->mutex));
	}
}

void di1InterruptCB() {
	digitalInterruptCB(0);
}

void di2InterruptCB() {
	digitalInterruptCB(1);
}

void di3InterruptCB() {
	digitalInterruptCB(2);
}

void di4InterruptCB() {
	digitalInterruptCB(3);
}

void di5InterruptCB() {
	digitalInterruptCB(4);
}

void di6InterruptCB() {
	digitalInterruptCB(5);
}

void ttl1InterruptCB() {
	digitalInterruptCB(6);
}

void ttl2InterruptCB() {
	digitalInterruptCB(7);
}

void ttl3InterruptCB() {
	digitalInterruptCB(8);
}

void ttl4InterruptCB() {
	digitalInterruptCB(9);
}

/*
 *
 */
volatile struct DigitalInputConfig* getDigitalInputConfig(int di) {
	int idx;
	void (*isrCallBack)(void);
	switch (di) {
	case DI1:
		idx = 0;
		isrCallBack = di1InterruptCB;
		break;
	case DI2:
		idx = 1;
		isrCallBack = di2InterruptCB;
		break;
	case DI3:
		idx = 2;
		isrCallBack = di3InterruptCB;
		break;
	case DI4:
		idx = 3;
		isrCallBack = di4InterruptCB;
		break;
	case DI5:
		idx = 4;
		isrCallBack = di5InterruptCB;
		break;
	case DI6:
		idx = 5;
		isrCallBack = di6InterruptCB;
		break;
	case TTL1:
		idx = 6;
		isrCallBack = ttl1InterruptCB;
		break;
	case TTL2:
		idx = 7;
		isrCallBack = ttl2InterruptCB;
		break;
	case TTL3:
		idx = 8;
		isrCallBack = ttl3InterruptCB;
		break;
	case TTL4:
		idx = 9;
		isrCallBack = ttl4InterruptCB;
		break;
	default:
		return NULL;
	}

	diConfs[idx].digitalInput = di;
	diConfs[idx].isrCallBack = isrCallBack;

	return &diConfs[idx];
}

/*
 *
 */
void ionoPiSetDigitalDebounce(int di, int millis) {
	volatile struct DigitalInputConfig* diConf = getDigitalInputConfig(di);
	if (diConf == NULL) {
		return;
	}
	diConf->debounceTime.tv_sec = millis / 1000;
	diConf->debounceTime.tv_nsec = (millis % 1000) * 1000000L;
	pinMode(di, INPUT);
	if (millis != 0) {
		pthread_mutex_lock((pthread_mutex_t *) &(diConf->mutex));
		if (diConf->debounceThreadRunning) {
			pthread_cancel(diConf->debounceThread);
		}
		diConf->debounceThreadRunning = FALSE;
		diConf->debouncedValue = digitalRead(di);
		wiringPiISR(di, INT_EDGE_BOTH, diConf->isrCallBack);
		pthread_mutex_unlock((pthread_mutex_t *) &(diConf->mutex));
	}
}

/*
 *
 */
int ionoPiDigitalRead(int di) {
	volatile struct DigitalInputConfig* diConf = getDigitalInputConfig(di);
	if (diConf == NULL) {
		return digitalRead(di);
	}
	if (diConf->debounceTime.tv_sec == 0 && diConf->debounceTime.tv_nsec == 0) {
		return digitalRead(di);
	} else {
		return diConf->debouncedValue;
	}
}

/*
 *
 */
int ionoPiDigitalInterrupt(int di, int mode, void (*callBack)(int, int)) {
	volatile struct DigitalInputConfig* diConf = getDigitalInputConfig(di);
	if (diConf == NULL) {
		return FALSE;
	}
	diConf->callBack = callBack;
	diConf->callBackMode = mode;
	if (callBack != NULL) {
		if (diConf->debounceTime.tv_sec == 0
				&& diConf->debounceTime.tv_nsec == 0) {
			wiringPiISR(di, mode, diConf->isrCallBack);
		} else {
			wiringPiISR(di, INT_EDGE_BOTH, diConf->isrCallBack);
		}
	}

	return TRUE;
}

/*
 *
 */
int mcp3204Read(unsigned char channel) {
	/*
	 * See http://ww1.microchip.com/downloads/en/DeviceDoc/21298c.pdf Page 18
	 *
	 * 1st byte: 0 0 0 0 0 1 SGL/DIFF D2 = 0 0 0 0 0 1 1 0
	 *
	 * 2nd byte: D1 D0 X X X X X X
	 *
	 * 3rd byte: X X X X X X X X
	 *
	 */
	unsigned char data[3];
	data[0] = 0b110;
	data[1] = channel;

	if (wiringPiSPIDataRW(MCP_SPI_CHANNEL, data, 3) < 0) {
		return -1;
	}

	int val = ((data[1] & 0x0F) << 8) + (data[2] & 0xFF);
	return val;
}

/*
 *
 */
int ionoPiAnalogRead(int ai) {
	int v = mcp3204Read(ai);
	if (v < 0) {
		return -1;
	}
	return v;
}

/*
 *
 */
float ionoPiVoltageRead(int ai) {
	int v = mcp3204Read(ai);
	if (v < 0) {
		return -1;
	}

	float factor;
	if (ai == AI1 || ai == AI2) {
		factor = AI1_AI2_FACTOR;
	} else {
		factor = AI3_AI4_FACTOR;
	}

	return factor * v;
}

int readRHT03Fixed(const int pin, int *temp, int *rh) {
	int result;
	unsigned char buffer[4];

// Read ...

	result = maxDetectRead(pin, buffer);

	if (!result)
		return FALSE;

	*rh = (buffer[0] * 256 + buffer[1]);
	*temp = (buffer[2] * 256 + buffer[3]);

	if ((*temp & 0x8000) != 0)	// Negative
			{
		*temp &= 0x7FFF;
		*temp = -*temp;
	}

// Discard obviously bogus readings - the checksum can't detect a 2-bit error
//	(which does seem to happen - no realtime here)

	if ((*rh > 999) || (*temp > 800) || (*temp < -400))
		return FALSE;

	return TRUE;
}

/*
 *
 */
int ionoPi1WireMaxDetectRead(const int ttl, const int attempts, int *temp,
		int *rh) {
	int i;
	for (i = 0; i < attempts; i++) {
		if (readRHT03Fixed(ttl, temp, rh)) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *
 */
int ionoPi1WireBusGetDevices(char*** ids) {
	DIR *dirp;
	struct dirent *dp;
	int i, count = 0;

	dirp = opendir(ONEWIRE_DEVICES_PATH);
	if (dirp == NULL) {
		return -1;
	}
	while (dirp) {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (dp->d_name[0] != 'w' && dp->d_name[0] != '.') {
				++count;
			}
		} else {
			if (errno == 0) {
				break;
			} else {
				return -1;
			}
		}
	}

	if (count == 0) {
		closedir(dirp);
		return 0;
	}

	*ids = malloc(count * sizeof(char*));
	if (*ids == NULL) {
		closedir(dirp);
		return -1;
	}

	rewinddir(dirp);

	i = 0;
	while (dirp && i < count) {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (dp->d_name[0] != 'w' && dp->d_name[0] != '.') {
				(*ids)[i++] = strdup(dp->d_name);
			}
		} else {
			if (errno == 0) {
				break;
			} else {
				closedir(dirp);
				return -1;
			}
		}
	}

	closedir(dirp);
	return i;
}

int read1WireBusDevice(const char* devicePath, int *temp) {
	FILE *fp;
	char line[50];

	fp = fopen(devicePath, "r");
	if (fp == NULL) {
		return FALSE;
	}

	fgets(line, 50, (FILE*) fp);

	char *sp = strrchr(line, ' ');
	if (sp == NULL) {
		return FALSE;
	}

	if (sp[1] != 'Y' || sp[2] != 'E' || sp[3] != 'S') {
		return FALSE;
	}

	fgets(line, 50, (FILE*) fp);

	fclose(fp);

	char *eq = strrchr(line, '=');
	if (eq == NULL) {
		return FALSE;
	}

	intmax_t val = strtoimax(eq + 1, NULL, 10);
	if ((val == INTMAX_MAX || val == INTMAX_MIN) && errno == ERANGE) {
		return FALSE;
	}

	*temp = val;

	return TRUE;
}

/*
 *
 */
int ionoPi1WireBusReadTemperature(const char* deviceId, const int attempts,
		int *temp) {
	char path[50];
	snprintf(path, 50, "%s%s%s", ONEWIRE_DEVICES_PATH, deviceId, "/w1_slave");

	int i;
	for (i = 0; i < attempts; i++) {
		if (read1WireBusDevice(path, temp)) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *
 */
unsigned long int to_usec(time_t t_sec, long int t_nsec) {
	return (t_sec * 1000000UL) + (t_nsec / 1000UL);
}

/*
 *
 */
unsigned long int diff_usec(struct timespec* t1, struct timespec* t2) {
	time_t diff_sec = t2->tv_sec - t1->tv_sec;
	long int diff_nsec = t2->tv_nsec - t1->tv_nsec;
	if (diff_nsec < 0) {
		diff_sec -= 1;
		diff_nsec += 1000000000L;
	}
	return to_usec(diff_sec, diff_nsec);
}

/*
 *
 */
void wData(volatile struct Wiegand* w, int ttl, int bitVal) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	if (!w->run || w->bitCount >= WIEGAND_MAX_BITS) {
		return;
	}

	nanosleep(&wiegandPulseWidthMax, NULL);
	if (digitalRead(ttl) == LOW) {
		// pulse too long
		w->bitCount = 0;
		w->data = 0;
		return;
	}

	if (w->bitCount != 0) {
		unsigned long int diff = diff_usec((struct timespec *) &(w->lastBitTs),
				&now);
		if (diff < wiegandPulseIntervalMin_usec
				|| diff > wiegandPulseIntervalMax_usec) {
			// pulse too early or too late
			w->bitCount = 0;
			w->data = 0;
			return;
		}
	}

	w->lastBitTs.tv_sec = now.tv_sec;
	w->lastBitTs.tv_nsec = now.tv_nsec;

	w->data <<= 1;
	w->data |= bitVal;
	w->bitCount++;
}

void w1Data0() {
	wData(&w1, TTL1, 0);
}

void w1Data1() {
	wData(&w1, TTL2, 1);
}

void w2Data0() {
	wData(&w2, TTL3, 0);
}

void w2Data1() {
	wData(&w2, TTL4, 1);
}

/*
 *
 */
void ionoPiSetWiegandPulse(unsigned int maxWidthMicros,
		unsigned int minIntervalMicros, unsigned int maxIntervalMicros) {
	wiegandPulseWidthMax.tv_sec = maxWidthMicros / 1000000L;
	wiegandPulseWidthMax.tv_nsec = (maxWidthMicros % 1000000L) * 1000L;
	wiegandPulseIntervalMin_usec = minIntervalMicros;
	wiegandPulseIntervalMax_usec = maxIntervalMicros;
}

/*
 *
 */
int ionoPiWiegandMonitor(int interface, int (*callBack)(int, int, uint64_t)) {
	if (callBack == NULL) {
		return FALSE;
	}
	volatile struct Wiegand* w;
	if (interface == 1) {
		w = &w1;
		if (!w1DataRegistered) {
			w1DataRegistered = 1;
			wiringPiISR(TTL1, INT_EDGE_FALLING, w1Data0);
			wiringPiISR(TTL2, INT_EDGE_FALLING, w1Data1);
		}
	} else if (interface == 2) {
		w = &w2;
		if (!w2DataRegistered) {
			w2DataRegistered = 1;
			wiringPiISR(TTL3, INT_EDGE_FALLING, w2Data0);
			wiringPiISR(TTL4, INT_EDGE_FALLING, w2Data1);
		}
	} else {
		return FALSE;
	}

	struct timespec now;
	unsigned long int diff;
	unsigned long int timeout_usec = wiegandPulseIntervalMax_usec * 3;
	if (timeout_usec < 200000) {
		timeout_usec = 200000;
	}
	unsigned int delay_ms = timeout_usec / 4000;

	w->data = 0;
	w->bitCount = 0;
	w->run = 1;

	while (w->run) {
		if (w->bitCount > 0) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			diff = diff_usec((struct timespec *) &w->lastBitTs, &now);
			if (diff >= timeout_usec) {
				if (w->bitCount >= 4) {
					if (!callBack(interface, w->bitCount, w->data)) {
						w->run = 0;
						return TRUE;
					}
				}
				w->data = 0;
				w->bitCount = 0;
			}
		}
		delay(delay_ms);
	}

	return TRUE;
}

/*
 *
 */
int ionoPiWiegandStop(int interface) {
	if (interface == 1) {
		w1.run = 0;
	} else if (interface == 2) {
		w2.run = 0;
	} else {
		return FALSE;
	}
	return TRUE;
}
