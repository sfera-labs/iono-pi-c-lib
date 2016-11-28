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

#include <ionoPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void printDigitalValue(int di, int val) {
	if (val == HIGH) {
		printf("high\n");
	} else {
		printf("low\n");
	}
	fflush(stdout);
}

int printWiegandCont;

int printWiegand(int interface, int bitCount, uint64_t data) {
	printf("%d %ju\n", bitCount, data);
	return printWiegandCont;
}

int main(int argc, char *argv[]) {
	if (!ionoPiSetup()) {
		fprintf(stderr, "ionoPi setup error\n");
		exit(EXIT_FAILURE);
	}

	int ok = 0;

	if (argc >= 2) {
		char *cmd = argv[1];
		int cmdLen = strlen(cmd);

		if (argc == 2 && strcmp(cmd, "-v") == 0) {
			printf("%s\n", IONOPI_VERSION);
			ok = 1;
		} else if (argc == 3 && strcmp(cmd, "led") == 0) {
			char *prm = argv[2];
			if (strcmp(prm, "on") == 0) {
				ionoPiDigitalWrite(LED, ON);
				ok = 1;
			} else if (strcmp(prm, "off") == 0) {
				ionoPiDigitalWrite(LED, OFF);
				ok = 1;
			}
		} else if (argc >= 3 && strcmp(cmd, "1wire") == 0) {
			char *prm = argv[2];
			int ttlx = -1;
			if (strcmp(prm, "ttl1") == 0) {
				ttlx = TTL1;
			} else if (strcmp(prm, "ttl2") == 0) {
				ttlx = TTL2;
			} else if (strcmp(prm, "ttl3") == 0) {
				ttlx = TTL3;
			} else if (strcmp(prm, "ttl4") == 0) {
				ttlx = TTL4;
			} else if (strcmp(prm, "bus") == 0) {
				ttlx = -2;
			}

			if (ttlx >= 0) {
				int temp;
				int rh;
				if (ionoPi1WireMaxDetectRead(ttlx, 3, &temp, &rh)) {
					printf("%.1f %.1f\n", temp / 10.0, rh / 10.0);
				} else {
					fprintf(stderr, "1-Wire max detect error\n");
				}
				ok = 1;
			}

			if (ttlx == -2) {
				if (argc == 3) {
					char** ids = NULL;
					int count = ionoPi1WireBusGetDevices(&ids);
					if (count < 0) {
						fprintf(stderr, "1-Wire bus error\n");
					}
					int i;
					for (i = 0; i < count; ++i) {
						printf("%s\n", ids[i]);
					}
					ok = 1;
				} else if (argc == 4) {
					int temp;
					if (ionoPi1WireBusReadTemperature(argv[3], &temp)) {
						printf("%.3f\n", temp / 1000.0);
					} else {
						fprintf(stderr, "1-wire bus error\n");
					}
					ok = 1;
				}
			}

		} else if (argc >= 3 && strcmp(cmd, "wiegand") == 0) {
			char *prm = argv[2];
			int itf = -1;
			if (strcmp(prm, "1") == 0) {
				itf = 1;
			} else if (strcmp(prm, "2") == 0) {
				itf = 2;
			}

			if (itf > 0) {
				if (argc == 4 && strcmp(argv[3], "-f") == 0) {
					printWiegandCont = TRUE;
				} else {
					printWiegandCont = FALSE;
				}
				if (!ionoPiWiegandMonitor(itf, printWiegand)) {
					fprintf(stderr, "Wiegand error\n");
				}
				ok = 1;
			}

		} else if (argc == 3 && cmdLen == 2 && cmd[0] == 'o') {
			int ox = -1;
			switch (cmd[1]) {
			case '1':
				ox = O1;
				break;
			case '2':
				ox = O2;
				break;
			case '3':
				ox = O3;
				break;
			case '4':
				ox = O4;
				break;
			default:
				break;
			}

			if (ox >= 0) {
				char *prm = argv[2];
				if (strcmp(prm, "open") == 0) {
					ionoPiDigitalWrite(ox, OPEN);
					ok = 1;
				} else if (strcmp(prm, "close") == 0) {
					ionoPiDigitalWrite(ox, CLOSED);
					ok = 1;
				}
			}
		} else if (cmdLen == 3) {
			if (argc == 3 && cmd[0] == 'o' && cmd[1] == 'c') {
				int ocx = -1;
				switch (cmd[2]) {
				case '1':
					ocx = OC1;
					break;
				case '2':
					ocx = OC2;
					break;
				case '3':
					ocx = OC3;
					break;
				default:
					break;
				}

				if (ocx >= 0) {
					char *prm = argv[2];
					if (strcmp(prm, "open") == 0) {
						ionoPiDigitalWrite(ocx, OPEN);
						ok = 1;
					} else if (strcmp(prm, "close") == 0) {
						ionoPiDigitalWrite(ocx, CLOSED);
						ok = 1;
					}
				}

			} else if (cmd[0] == 'd' && cmd[1] == 'i') {
				int dix = -1;
				switch (cmd[2]) {
				case '1':
					dix = DI1;
					break;
				case '2':
					dix = DI2;
					break;
				case '3':
					dix = DI3;
					break;
				case '4':
					dix = DI4;
					break;
				case '5':
					dix = DI5;
					break;
				case '6':
					dix = DI6;
					break;
				default:
					break;
				}

				if (dix >= 0) {
					if (argc == 2) {
						printDigitalValue(dix, ionoPiDigitalRead(dix));
						ok = 1;
					} else if (argc == 3 && strcmp(argv[2], "-f") == 0) {
						printDigitalValue(dix, ionoPiDigitalRead(dix));
						ionoPiDigitalInterrupt(dix, printDigitalValue);
						ok = 1;
						for (;;) {
							sleep(1);
						}
					}
				}
			} else if (argc == 2 && cmd[0] == 'a' && cmd[1] == 'i') {
				int aix = -1;
				switch (cmd[2]) {
				case '1':
					aix = AI1;
					break;
				case '2':
					aix = AI2;
					break;
				case '3':
					aix = AI3;
					break;
				case '4':
					aix = AI4;
					break;
				default:
					break;
				}

				if (aix >= 0) {
					printf("%f\n", ionoPiAnalogRead(aix));
					ok = 1;
				}
			}
		}
	}

	if (!ok) {
		fprintf(stderr, "usage: %s <command>\n\n", argv[0]);
		fprintf(stderr,
				"Commands:\n"
						"   -v              Print Iono Pi Utility version number\n"
						"   led on          Turn on the green LED\n"
						"   led off         Turn off the green LED\n"
						"   o<n> open       Open relay output o<n> (<n>=1..4)\n"
						"   o<n> close      Close relay output o<n> (<n>=1..4)\n"
						"   oc<n> open      Open open collector oc<n> (<n>=1..3)\n"
						"   oc<n> close     Close open collector oc<n> (<n>=1..3)\n"
						"   di<n>           Print the state (\"high\" or \"low\") of digital input di<n> (<n>=1..6)\n"
						"   di<n> -f        Print the state of digital input di<n> now and on every change\n"
						"   ai<n>           Print the voltage value (V) read from analog input ai<n> (<n>=1..4)\n"
						"   1wire bus       Print the list of device IDs found on the 1-Wire bus\n"
						"   1wire bus <id>  Print the temperature value (°C) read from 1-Wire device <id>\n"
						"   1wire ttl<n>    Print temperature (°C) and humidity (%%) values read from the\n"
						"                   MaxDetect 1-wire sensor on TTL<n> (N=1..4)\n"
						"   wiegand <n>     Wait for data to be available on Wiegand interface <n> (<n>=1|2)\n"
						"                   and print number of bits and value read\n"
						"   wiegand <n> -f  Continuously print number of bits and value read from Wiegand\n"
						"                   interface <n> whenever data is available\n");

		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
