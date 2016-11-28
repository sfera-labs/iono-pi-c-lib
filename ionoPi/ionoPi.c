/*
 * ionoPi
 *
 *     Copyright (C) 2016 Sfera Labs S.r.l.
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
#include <time.h>
#include <dirent.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <maxdetect.h>

#define MCP_SPI_CHANNEL 			0
#define MCP_SPI_SPEED				50000

#define AI1_AI2_FACTOR 				0.007319f
#define AI3_AI4_FACTOR 				0.000725f

#define ONEWIRE_DEVICES_PATH "/sys/bus/w1/devices/"

#define WIEGAND_INTERVAL_MILLIS		5
#define WIEGAND_INTERVALS_TIMEOUT	10
#define WIEGAND_MAX_BITS			64

void (*digitalInterruptCB[6])(int, int);

struct Wiegand {
	int64_t data;
	int bitCount;
	int done;
	unsigned int intervalsCounter;
	int run;
} w1, w2;

int w1DataRegistered = 0;
int w1Data0First = 1;
int w1Data1First = 1;

int w2DataRegistered = 0;
int w2Data0First = 1;
int w2Data1First = 1;

struct timespec wiegandInterval = { .tv_sec = 0, .tv_nsec =
WIEGAND_INTERVAL_MILLIS * 1000000 };

void printError(const char *msg) {
	fprintf(stderr, msg);
}

int mcp3204Setup() {
	return (wiringPiSPISetup(MCP_SPI_CHANNEL, MCP_SPI_SPEED) != -1);
}

/*
 * Must be called once at the start of your program execution.
 */
int ionoPiSetup() {
	if (wiringPiSetup() != 0) {
		printError("wiringPi setup error\n");
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
	pullUpDnControl(DI1, PUD_OFF);
	pinMode(DI2, INPUT);
	pullUpDnControl(DI2, PUD_OFF);
	pinMode(DI3, INPUT);
	pullUpDnControl(DI3, PUD_OFF);
	pinMode(DI4, INPUT);
	pullUpDnControl(DI4, PUD_OFF);
	pinMode(DI5, INPUT);
	pullUpDnControl(DI5, PUD_OFF);
	pinMode(DI6, INPUT);
	pullUpDnControl(DI6, PUD_OFF);

	if (!mcp3204Setup()) {
		printError("mcp setup error\n");
		return FALSE;
	}

	return TRUE;
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
int ionoPiDigitalRead(int di) {
	return digitalRead(di);
}

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
float ionoPiAnalogRead(int ai) {
	float factor;
	if (ai == AI1 || ai == AI2) {
		factor = AI1_AI2_FACTOR;
	} else {
		factor = AI3_AI4_FACTOR;
	}

	int v = mcp3204Read(ai);
	if (v < 0) {
		return -1;
	}

	return factor * v;
}

void callDigitalInterruptCB(int idx, int di) {
	if (digitalInterruptCB[idx] != NULL) {
		digitalInterruptCB[idx](di, digitalRead(di));
	}
}

void di1InterruptCB() {
	callDigitalInterruptCB(0, DI1);
}

void di2InterruptCB() {
	callDigitalInterruptCB(1, DI2);
}

void di3InterruptCB() {
	callDigitalInterruptCB(2, DI3);
}

void di4InterruptCB() {
	callDigitalInterruptCB(3, DI4);
}

void di5InterruptCB() {
	callDigitalInterruptCB(4, DI5);
}

void di6InterruptCB() {
	callDigitalInterruptCB(5, DI6);
}

/*
 *
 */
int ionoPiDigitalInterrupt(int di, void (*callBack)(int, int)) {
	int idx;
	switch (di) {
	case DI1:
		idx = 0;
		wiringPiISR(di, INT_EDGE_BOTH, di1InterruptCB);
		break;
	case DI2:
		idx = 1;
		wiringPiISR(di, INT_EDGE_BOTH, di2InterruptCB);
		break;
	case DI3:
		idx = 2;
		wiringPiISR(di, INT_EDGE_BOTH, di3InterruptCB);
		break;
	case DI4:
		idx = 3;
		wiringPiISR(di, INT_EDGE_BOTH, di4InterruptCB);
		break;
	case DI5:
		idx = 4;
		wiringPiISR(di, INT_EDGE_BOTH, di5InterruptCB);
		break;
	case DI6:
		idx = 5;
		wiringPiISR(di, INT_EDGE_BOTH, di6InterruptCB);
		break;
	default:
		return FALSE;
	}

	digitalInterruptCB[idx] = callBack;

	return TRUE;
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

/*
 *
 */
int ionoPi1WireBusReadTemperature(const char* deviceId, int *temp) {
	char path[50];
	snprintf(path, 50, "%s%s%s", ONEWIRE_DEVICES_PATH, deviceId, "/w1_slave");
	FILE *fp;
	char line[50];

	fp = fopen(path, "r");

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

void w1Data0() {
	if (w1Data0First) {
		w1Data0First = 0;
		return;
	}
	if (w1.run && w1.bitCount < WIEGAND_MAX_BITS) {
		w1.data <<= 1;
		w1.bitCount++;
		w1.intervalsCounter = WIEGAND_INTERVALS_TIMEOUT;
		w1.done = 0;
	}
}

void w1Data1() {
	if (w1Data1First) {
		w1Data1First = 0;
		return;
	}
	if (w1.run && w1.bitCount < WIEGAND_MAX_BITS) {
		w1.data <<= 1;
		w1.data |= 1;
		w1.bitCount++;
		w1.intervalsCounter = WIEGAND_INTERVALS_TIMEOUT;
		w1.done = 0;
	}
}

void w2Data0() {
	if (w2Data0First) {
		w2Data0First = 0;
		return;
	}
	if (w2.run && w2.bitCount < WIEGAND_MAX_BITS) {
		w2.data <<= 1;
		w2.bitCount++;
		w2.intervalsCounter = WIEGAND_INTERVALS_TIMEOUT;
		w2.done = 0;
	}
}

void w2Data1() {
	if (w2Data1First) {
		w2Data1First = 0;
		return;
	}
	if (w2.run && w2.bitCount < WIEGAND_MAX_BITS) {
		w2.data <<= 1;
		w2.data |= 1;
		w2.bitCount++;
		w2.intervalsCounter = WIEGAND_INTERVALS_TIMEOUT;
		w2.done = 0;
	}
}

/*
 *
 */
int ionoPiWiegandMonitor(int interface, int (*callBack)(int, int, uint64_t)) {
	struct Wiegand* w;
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

	w->data = 0;
	w->bitCount = 0;
	w->done = 1;

	w->run = 1;
	while (w->run) {
		if (!w->done) {
			if (--w->intervalsCounter == 0) {
				w->done = 1;
				if (w->bitCount > 0) {
					if (!callBack(interface, w->bitCount, w->data)) {
						w->run = 0;
						return TRUE;
					}
					w->data = 0;
					w->bitCount = 0;
				}
			}
		}
		nanosleep(&wiegandInterval, NULL);
	}

	return TRUE;
}