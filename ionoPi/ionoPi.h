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

#ifndef IONOPI_H_INCLUDED
#define IONOPI_H_INCLUDED

#include <inttypes.h>

#define IONOPI_VERSION "1.6.2"

#define TTL1	7
#define TTL2	25
#define TTL3	28
#define TTL4	29
#define DI1		27
#define DI2		24
#define DI3		23
#define DI4		26
#define DI5		22
#define DI6		21
#define OC1		1
#define OC2		6
#define OC3		5
#define O1		0
#define O2		2
#define O3		3
#define O4		4
#define LED		11

#define AI1		0b01000000 // MCP_CH1
#define AI2		0b00000000 // MCP_CH0
#define AI3		0b10000000 // MCP_CH2
#define AI4		0b11000000 // MCP_CH3

#ifndef	INPUT
#define	INPUT	0
#define	OUTPUT	1
#endif

#ifndef	LOW
#define	LOW		0
#define	HIGH	1
#endif

#ifndef	ON
#define ON		HIGH
#define OFF		LOW
#endif

#ifndef	CLOSED
#define CLOSED	HIGH
#define OPEN	LOW
#endif

#ifndef	TRUE
#define	TRUE	(1==1)
#define	FALSE	(!TRUE)
#endif

#ifndef	INT_EDGE_FALLING
#define	INT_EDGE_FALLING	1
#define	INT_EDGE_RISING		2
#define	INT_EDGE_BOTH		3
#endif

extern int ionoPiSetup();
extern void ionoPiPinMode(int pin, int mode);
extern void ionoPiDigitalWrite(int output, int value);
extern void ionoPiSetDigitalDebounce(int di, int millis);
extern int ionoPiDigitalRead(int di);
extern int ionoPiAnalogRead(int ai);
extern float ionoPiVoltageRead(int ai);
extern int ionoPiDigitalInterrupt(int di, int mode, void (*callBack)(int, int));
extern int ionoPi1WireBusGetDevices(char*** ids);
extern int ionoPi1WireBusReadTemperature(const char* deviceId,
		const int attempts, int *temp);
extern int ionoPi1WireMaxDetectRead(const int ttl, const int attempts,
		int *temp, int *rh);
extern void ionoPiSetWiegandPulse(unsigned int maxWidthMicros,
		unsigned int minIntervalMicros, unsigned int maxIntervalMicros);
extern int ionoPiWiegandMonitor(int interface,
		int (*callBack)(int, int, uint64_t));
extern int ionoPiWiegandStop(int interface);

#endif /* IONOPI_H_INCLUDED */
