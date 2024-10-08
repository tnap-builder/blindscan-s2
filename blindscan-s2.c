/* blindscan-s2 -- blindscan tool for the Linux DVB S2 API
 *
 * chinesebob@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "blindscan-s2.h"

int open_frontend (unsigned int adapter, unsigned int frontend, int verbose);

void blindscan (int startfreq, int endfreq, int symrate,
	int step, unsigned int scan_v, unsigned int scan_h, int lof,
	unsigned int interactive, int fec, unsigned int adapter,
	unsigned int frontend, unsigned int verbose, int fefd, int delsys,
	unsigned int monitor, unsigned int polarity, int retune,
	unsigned int monitor_retune, int tone);

void tune (int fefd, int tpfreq, int symrate, int polarity, int fec,
	int delsys, int tone);
void getinfo (int fefd, int lof, unsigned int verbose);
void convert_freq (int lof, int *startfreq,
	int *endfreq, int *symrate, int *step);
int lastfreq;
int lastpol;
int lastsr;
int lastsys;
int lastfec;
int lastmod;
int lastinv;
int lastrol;
int lastpil;

int main(int argc, char *argv[])
{

	int opt, fefd;
	unsigned int verbose = VERBOSE;
	unsigned int interactive = INTERACTIVE;
	unsigned int monitor = MONITOR;
	unsigned int monitor_retune = MONITOR_RETUNE;
	unsigned int adapter = ADAPTER, frontend = FRONTEND;
	unsigned int run_blindscan = RUN_BLINDSCAN;
	unsigned int scan_v = SCAN_V, scan_h = SCAN_H, scan_n = SCAN_N;
	int startfreq = STARTFREQ;
	int endfreq = ENDFREQ;
	int symrate = SYMRATE;
	int step = STEP;
	int lof = LOF;
	int fec = FEC;
	int delsys = DELSYS;
	int retune = RETUNE;
	int tone = TONE;
	int committed = COMMITTED;
	int uncommitted = UNCOMMITTED;
	int motor_wait_time = MOTOR_WAIT_TIME;
	double site_lat = SITE_LAT;
	double site_long = SITE_LONG;
	double sat_long = SAT_LONG;

	static char *usage =
	"\nBlindscan tool for the Linux DVB S2 API\n"
	"\nModified by SatDreamGr for Enigma2\n"
	"\nusage: blindscan-s2\n"
	"	-b        : run blind scan\n"
	"	-T        : Tune a specific transponder\n"
	"	-H        : only scan horizontal polarity\n"
	"	-V        : only scan vertical polarity\n"
	"	-N        : no polarity\n"
	"	-i        : interactive mode\n"
	"	-m        : monitor signal mode\n"
	"	-M        : monitor and re-tune each time\n"
	"	-a number : adapter number (default 0)\n"
	"	-f number : frontend number (default 0)\n"
	"	-F number : fec (default auto)\n"
	"	-s number : starting transponder frequency in MHz (default 950)\n"
	"	-e number : ending transponder frequency in MHz (default 1450)\n"
	"	-d number : delivery system 4=DSS 5=DVB-S 6=DVB-S2 (default 5)\n"
	"	-t number : step value for scan in MHz (default 20)\n"
	"	-R number : amount of times to try tuning each step (default 1)\n"
	"	-r number : symbol rate in MSym/s (default 1000)\n"
	"	-l number : local oscillator frequency of LNB (default 0)\n"
	"	-2        : enable 22KHz tone (default OFF)\n"
	"	-c number : use DiSEqC COMMITTED switch position N (1-4)\n"
	"	-u number : use DiSEqC uncommitted switch position N (1-4)\n"
	"	-U N.N    : orbital position for USALS\n"
	"	-G N.N    : site longtude\n"
	"	-A N.N    : site latitude\n"
	"	-W number : seconds to wait after usals motor move (default 45)\n"
	"	-v        : verbose\n"
	"\nExample:\n"
	"	Default scans L-band range in steps of 20 on H and V polarity\n"
	"	blindscan-s2 -b\n"
	"	Scan 11700-11900 vertical in steps of 10, and calculate for lof\n"
	"	blindscan-s2 -b -s 11700 -e 11900 -V -t 10 -l 10750\n";

	while ((opt = getopt(argc, argv, "2a:A:bc:d:e:f:F:G:hHil:mMNr:R:s:t:T:u:U:VvW:")) != -1) {
		switch (opt) {
		case '?':

		default:
			printf("%s\n", usage);
			exit(1);
		case 'b':
			run_blindscan = 1;
			break;
		case 'a':
			adapter = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			frontend = strtoul(optarg, NULL, 0);
			break;
		case 'F':
			fec = strtoul(optarg, NULL, 0);
			break;
		case 's':
			startfreq = strtoul(optarg, NULL, 0);
			break;
		case 'e':
			endfreq = strtoul(optarg, NULL, 0);
			break;
		case 'T':
			startfreq = strtoul(optarg, NULL, 0);
			run_blindscan = 1;
			step = 3000;
			break;
		case 'd':
			delsys = strtoul(optarg, NULL, 0);
			break;
		case 't':
			step = strtoul(optarg, NULL, 0);
			break;
		case 'R':
			retune = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			symrate = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			lof = strtoul(optarg, NULL, 0);
			break;
		case 'H':
			scan_v = 0;
			break;
		case 'V':
			scan_h = 0;
			break;
		case 'N':
			scan_h = 0;
			scan_v = 0;
			scan_n = 1;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'm':
			monitor = 1;
			break;
		case 'M':
			monitor_retune = 1;
			monitor = 1;
			break;
		case '2':
			tone = SEC_TONE_ON;
			break;
		case 'c':
			committed = strtoul(optarg, NULL, 0);
			break;
		case 'u':
			uncommitted = strtoul(optarg, NULL, 0);
			break;
		case 'U':
			sat_long = strtod(optarg, 0);
			break;
		case 'G':
			site_long = strtod(optarg, 0);
			break;
		case 'A':
			site_lat = strtod(optarg, 0);
			break;
		case 'W':
			motor_wait_time = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}

	if (run_blindscan) {
		FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
		time_t mytime = time(NULL);
		char * time_str = ctime(&mytime);
		time_str[strlen(time_str)-1] = '\0';
		fprintf(fptr,"Time at Start-of-Scan : %s\n", time_str);
		fclose(fptr);
		convert_freq (lof, &startfreq, &endfreq, &symrate, &step);
		fefd = open_frontend (adapter, frontend, verbose);

		if (sat_long)
			motor_usals(fefd, site_lat, site_long, sat_long, verbose, motor_wait_time);

		if (scan_h) {
			if (committed || uncommitted)
				setup_switch (fefd, HORIZONTAL, tone, committed, uncommitted, verbose);

			blindscan(startfreq, endfreq, symrate,
				step, scan_v, scan_h, lof, interactive, fec,
				adapter, frontend, verbose, fefd, delsys,
				monitor, HORIZONTAL, retune, monitor_retune, tone);
		}
		if (scan_v) {
			if (committed || uncommitted)
				setup_switch (fefd, VERTICAL, tone, committed, uncommitted, verbose);

			blindscan(startfreq, endfreq, symrate,
				step, scan_v, scan_h, lof, interactive, fec,
				adapter, frontend, verbose, fefd, delsys,
				monitor, VERTICAL, retune, monitor_retune, tone);
		}
		if (scan_n) {
			if (committed || uncommitted)
				setup_switch (fefd, NOPOLARITY, tone, committed, uncommitted, verbose);

			blindscan(startfreq, endfreq, symrate,
				step, scan_v, scan_h, lof, interactive, fec,
				adapter, frontend, verbose, fefd, delsys,
				monitor, NOPOLARITY, retune, monitor_retune, tone);
		}
	}
	else {
		printf("%s\n", usage);
		exit(1);
	}

	close(fefd);

	return (0);
}

int open_frontend (unsigned int adapter, unsigned int frontend, int verbose) {
	int fefd;
	char fedev[128];
	FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
	snprintf(fedev, sizeof(fedev), FEDEV, adapter, frontend);
	fprintf(fptr,"USB Device Location = ");
	fprintf(fptr, fedev, sizeof(fedev), FEDEV, adapter, frontend);
	fprintf(fptr,"\n");
	fefd = open(fedev, O_RDWR | O_NONBLOCK);


        struct dvb_frontend_info info;
        if ((ioctl(fefd, FE_GET_INFO, &info)) == -1) {
                if (verbose) perror("FE_GET_INFO failed\n");
                return -1;
        }
        fprintf(fptr,"frontend:(%s)  fmin=%d MHz fmax=%d MHz min_sr=%d Ksps max_sr=%d Ksps \n", info.name,
        info.type == 0 ? info.frequency_min / 1000: info.frequency_min / 1000000,
        info.type == 0 ? info.frequency_max / 1000: info.frequency_max / 1000000,
        info.type == 0 ? info.symbol_rate_min / 1000: info.symbol_rate_min /100000,
        info.type == 0 ? info.symbol_rate_max / 1000: info.symbol_rate_max /1000000);

	fclose(fptr);
	return fefd;
}

void convert_freq (int lof, int *startfreq,
	int *endfreq, int *symrate, int *step) {
	int lof_khz, lband_max_khz, cband_lof_khz;
	lof_khz = lof * FREQ_MULT;
	lband_max_khz = LBAND_MAX * FREQ_MULT;
	cband_lof_khz = CBAND_LOF * FREQ_MULT;
	*step = *step * FREQ_MULT;
	*symrate = 100000;
	*startfreq = *startfreq * FREQ_MULT;
	*endfreq = *endfreq * FREQ_MULT;
	if (lof_khz > 0) {
		if ((*startfreq > lband_max_khz) && (lof_khz > cband_lof_khz))
			*startfreq = *startfreq - lof_khz;
		else if (*startfreq > lband_max_khz)
			*startfreq = lof_khz - *startfreq;
		if ((*endfreq > lband_max_khz) && (lof_khz > cband_lof_khz))
			*endfreq = *endfreq - lof_khz;
		else if (*endfreq > lband_max_khz)
			*endfreq = lof_khz - *endfreq;
	}
}

void blindscan (int startfreq, int endfreq, int symrate,
	int step, unsigned int scan_v, unsigned int scan_h, int lof,
	unsigned int interactive, int fec, unsigned int adapter,
	unsigned int frontend, unsigned int verbose, int fefd, int delsys,
	unsigned int monitor, unsigned int polarity, int retune,
	unsigned int monitor_retune, int tone) {
	int r;
	int f;
	int userstep = step;
	fe_status_t status;

	int dtv_symbol_rate_prop = 0;
	struct dtv_property p[] = {
                { .cmd = DTV_SYMBOL_RATE }
        };
        struct dtv_properties qp = {
                .num = 1,
                .props = p
        };

        if (ioctl(fefd, FE_GET_PROPERTY, &qp) == -1) {
                perror("blindscan () FE_GET_PROPERTY failed_1");
                return;
	}

	if (startfreq > endfreq) {
		for (f = startfreq; f >= endfreq; f -= step) {
			for (r = retune; r > 0; r -= 1) {
				printf("Tuning LBAND-1: %d \n", f / FREQ_MULT);
				tune(fefd, f, symrate, polarity, fec, delsys, tone);
				usleep(500000);
				getinfo(fefd, lof, verbose);
				usleep(500000);
				getinfo(fefd, lof, verbose);
				usleep(500000);
				getinfo(fefd, lof, verbose);
				usleep(500000);
				getinfo(fefd, lof, verbose);
				usleep(500000);
				getinfo(fefd, lof, verbose);
				printf("Tuning LBAND-1: %d \n", f / FREQ_MULT);
				if (ioctl(fefd, FE_READ_STATUS, &status) == -1) {
					perror("FE_READ_STATUS failed");
				}
				if(status & ((FE_HAS_VITERBI || FE_HAS_SYNC))) {
					ioctl(fefd, FE_GET_PROPERTY, &qp);
					dtv_symbol_rate_prop = qp.props[0].u.data / FREQ_MULT;
					if(dtv_symbol_rate_prop < 100) {
						dtv_symbol_rate_prop = 100;
					}
					step = (dtv_symbol_rate_prop );
				} else {
					step = userstep;
				}
				if (interactive) {
					printf("Interactive: press i [enter] for info, r to retune, q to quit\n");
					verbose = 1;
					char c;
					while ((c = getchar()) != 'n') {
						switch(c) {
							case 'q':
								goto outer;
							case 'i':
								getinfo(fefd, lof, verbose);
								break;
							case 'r':
								tune(fefd, f, symrate, polarity, fec, delsys, tone);
								getinfo(fefd, lof, verbose);
								break;
						}
					}
				}
				else if (monitor) {
					printf(" \n We Have Monitor...");
					verbose = 1;
					while (1) {
						getinfo(fefd, lof, verbose);
						usleep(1000000);
						if (monitor_retune) {
							printf(" \n We Have Monitor Re-Tuning...");
							tune(fefd, f, symrate, polarity, fec, delsys, tone);
							usleep(1000000);
						}
					}
				}
			}
		}
	}
	else {
		for (f = startfreq; f <= endfreq; f += step) {
			//retune = 3
			for (r = retune; r > 0; r -= 1) {
				printf("Tuning LBAND-2: %d \n", f / FREQ_MULT);
				tune(fefd, f, symrate, polarity, fec, delsys, tone);
				//FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
				//fprintf(fptr, "\n step-Frequency = %d end-frequency = %d  step = %d \n", f, endfreq, step);
				//fclose(fptr);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);				
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				usleep(10000);
				getinfo(fefd, lof, verbose);
				printf("Done--Tuning LBAND-2: %d \n", f / FREQ_MULT);

				if (ioctl(fefd, FE_READ_STATUS, &status) == -1) {
					perror("FE_READ_STATUS failed");
				}
				if(status & ((FE_HAS_VITERBI || FE_HAS_SYNC))) {
					ioctl(fefd, FE_GET_PROPERTY, &qp);
					dtv_symbol_rate_prop = qp.props[0].u.data / FREQ_MULT;
					if(dtv_symbol_rate_prop > userstep) {
						step = dtv_symbol_rate_prop;
					}
					else
						step = userstep;
					
				}  
					
				FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
				fprintf(fptr, "\n dtv_symbol_rate_prop = %d  step = %d", dtv_symbol_rate_prop, step);
				fclose(fptr);


				if (interactive) {
					printf("Interactive: press i [enter] for info, r to retune, q to quit\n");
					verbose = 1;
					char c;
					while ((c = getchar()) != 'n') {
						switch(c) {
							case 'q':
								goto outer;
							case 'i':
								getinfo(fefd, lof, verbose);
								break;
							case 'r':
								tune(fefd, f, symrate, polarity, fec, delsys, tone);
								getinfo(fefd, lof, verbose);
								break;
						}
					}
				}
				else if (monitor) {
					verbose = 1;
					while (1) {
						printf(" \n We Have Monitor --usleep = 1000000 ...");
						getinfo(fefd, lof, verbose);
						usleep(1000000);
						if (monitor_retune) {
							printf(" \n We Have Monitor Re-Tuning --usleep = 1000000...");
							tune(fefd, f, symrate, polarity, fec, delsys, tone);
							usleep(1000000);
						}
					}
				}

			}
		}
		outer:;
	if (f >= (endfreq - step )) {
		FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
		time_t mytime = time(NULL);
		char * time_str = ctime(&mytime);
		time_str[strlen(time_str)-1] = '\0';
		fprintf(fptr,"\nCurrent Time at End-of-Scan : %s\n", time_str);
		fclose(fptr);
	}
	}
}

void tune(int fefd, int tpfreq, int symrate, int polarity, int fec, int delsys, int tone) {

        struct dtv_property p_clear[] = {
                { .cmd = DTV_CLEAR },
        };

        struct dtv_properties cmdseq_clear = {
                .num = 1,
                .props = p_clear
        };

        ioctl(fefd, FE_SET_PROPERTY, &cmdseq_clear);

//	struct dvb_frontend_event ev;

       /* discard stale QPSK events */
/*        while (1) {
                if (ioctl(fefd, FE_GET_EVENT, &ev) == -1)
                break;
        }
*/

	struct dtv_property p[DTV_IOCTL_MAX_MSGS] = {
		//{ .cmd = FE_TUNE_MODE_ONESHOT },
		{ .cmd = DTV_DELIVERY_SYSTEM,  .u.data = delsys },
		{ .cmd = DTV_FREQUENCY,   .u.data = tpfreq }, 
		{ .cmd = DTV_SYMBOL_RATE, .u.data = symrate }, 
		{ .cmd = DTV_TONE, .u.data = tone }, 
		{ .cmd = DTV_VOLTAGE,     .u.data = polarity },
		{ .cmd = DTV_INNER_FEC,     .u.data = fec },
		{ .cmd = DTV_TUNE },
	};

	struct dtv_properties cmdseq = {
		.num = 7,
		.props = p
	};

	ioctl(fefd, FE_SET_PROPERTY, &cmdseq);
}

void getinfo(int fefd, int lof, unsigned int verbose) {

#if DVB_API_VERSION > 5
 	float snr, signal;
#else
	uint16_t snr, snr_percent, signal;
#endif
	unsigned int lvl_scale, snr_scale;
	int dtv_frequency_prop = 0;
	int dtv_symbol_rate_prop = 0;
	int dtv_inner_fec_prop = 0;
#if DVB_API_VERSION > 5
	fe_status_t status;
	if (ioctl(fefd, FE_READ_STATUS, &status) == -1) {
		if (verbose) perror("FE_READ_STATUS failed");
	}

	struct dtv_property sp[3];
	sp[0].cmd = DTV_STAT_SIGNAL_STRENGTH;
	sp[1].cmd = DTV_STAT_CNR;
	sp[2].cmd = DTV_STAT_POST_ERROR_BIT_COUNT;

	struct dtv_properties sp_status;
	sp_status.num = 3;
	sp_status.props = sp;

	if (ioctl(fefd, FE_GET_PROPERTY, &sp_status) == -1) {
		if (verbose) perror("FE_GET_PROPERTY failed");
		return;
	}
	lvl_scale = sp_status.props[0].u.st.stat[0].scale;
	if (lvl_scale == FE_SCALE_DECIBEL) {
		signal = sp_status.props[0].u.st.stat[0].svalue * 0.001;
	} else {
		uint16_t signal2 = 0;
		if (ioctl(fefd, FE_READ_SIGNAL_STRENGTH, &signal2) == -1) {
			signal = 0;
		} else {
			signal = (float)signal2 * 100 / 0xffff;
			if (signal < 0) {
				signal = 0;
			}
		}
	}
	snr_scale = sp_status.props[1].u.st.stat[0].scale;
	if (snr_scale == FE_SCALE_DECIBEL) {
		snr = sp_status.props[1].u.st.stat[0].svalue * .001;
	} else {
		uint16_t snr2 = 0;
		if (ioctl(fefd, FE_READ_SNR, &snr2) == -1) {
			snr2 = 0;
		}
		snr = (float)snr2/10;
	}
	ioctl(fefd, FE_READ_STATUS, &status);
#else
	ioctl(fefd, FE_READ_SIGNAL_STRENGTH, &signal);
	ioctl(fefd, FE_READ_SNR, &snr);
	snr_percent = (snr * 100) / 0xffff;
#endif
	struct dtv_property p[] = {
		{ .cmd = DTV_DELIVERY_SYSTEM }, //[0]  0 DVB-S, 9 DVB-S2
		{ .cmd = DTV_FREQUENCY },	//[1]
		{ .cmd = DTV_VOLTAGE },		//[2]  0 - 13V V, 1 - 18V H, 2 - Voltage OFF
		{ .cmd = DTV_SYMBOL_RATE },	//[3]
		{ .cmd = DTV_TONE },		//[4]
		{ .cmd = DTV_MODULATION },	//[5]  5 - QPSK, 6 - 8PSK
		{ .cmd = DTV_INNER_FEC },	//[6]
		{ .cmd = DTV_INVERSION },	//[7]
		{ .cmd = DTV_ROLLOFF },		//[8]
		{ .cmd = DTV_BANDWIDTH_HZ },	//[9]  Not used for DVB-S
		{ .cmd = DTV_PILOT },		//[10] 0 - ON, 1 - OFF
		{ .cmd = DTV_STREAM_ID }	//[11]
	};

	struct dtv_properties cmdseq = {
		.num = 12,
		.props = p
		};
	ioctl(fefd, FE_GET_PROPERTY, &cmdseq);

	int dtv_delivery_system_prop = cmdseq.props[0].u.data; 
	dtv_frequency_prop = cmdseq.props[1].u.data; 
	int dtv_voltage_prop = cmdseq.props[2].u.data; 
	dtv_symbol_rate_prop = cmdseq.props[3].u.data;
//	int dtv_tone_prop = cmdseq.props[4].u.data;
	int dtv_modulation_prop = cmdseq.props[5].u.data;
	dtv_inner_fec_prop = cmdseq.props[6].u.data;
	int dtv_inversion_prop = cmdseq.props[7].u.data;
	int dtv_rolloff_prop = cmdseq.props[8].u.data;
//	int dtv_bandwidth_hz_prop = cmdseq.props[9].u.data;
	int dtv_pilot_prop = cmdseq.props[10].u.data;

	int currentfreq;
	int currentpol;
	int currentsr;
	int currentsys;
	int currentfec;
	int currentmod;
	int currentinv;
	int currentrol;
	int currentpil;
	extern int lastfreq;
	extern int lastpol;
	extern int lastsr;
	extern int lastsys;
	extern int lastfec;
	extern int lastmod;
	extern int lastinv;
	extern int lastrol;
	extern int lastpil;
	currentfreq = dtv_frequency_prop / FREQ_MULT;
	currentpol = dtv_voltage_prop;
	currentsr = dtv_symbol_rate_prop / FREQ_MULT;
	currentsys = dtv_delivery_system_prop;
	currentfec = dtv_inner_fec_prop;
	currentmod = dtv_modulation_prop;
	currentinv = dtv_inversion_prop;
	currentrol = dtv_rolloff_prop;
	currentpil = dtv_pilot_prop;



	struct dvb_frontend_parameters qp;
	ioctl(fefd, FE_GET_FRONTEND, &qp);
	dtv_symbol_rate_prop = qp.u.qpsk.symbol_rate;
	FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
	if (verbose || (snr > 0 && (currentpol != lastpol || currentfreq != lastfreq
		|| currentsr != lastsr || currentsys != lastsys || currentfec != lastfec
		|| currentmod != lastmod || currentinv != lastinv || currentrol != lastrol
		|| currentpil != lastpil))) {

		fprintf(fptr,"\n lastfreq-%d currentfreq-%d lastpol-%d currentpol-%d lastsr-%d  currentsr-%d lastsys-%d currentsys-%d lastfec-%d currentfec-%d \n", lastfreq, currentfreq, lastpol, currentpol, lastsr, currentsr, lastsys, currentsys, lastfec, currentfec);
		lastfreq = currentfreq;
		lastpol = currentpol;
		lastsr = currentsr;
		lastsys = currentsys;
		lastfec = currentfec;
		lastmod = currentmod;
		lastinv = currentinv;
		lastrol = currentrol;
		lastpil = currentpil;

		printf("OK ");

		FILE *fptr = fopen("/tmp/TBS5925-scan-log.txt", "a");
		switch (dtv_voltage_prop) {
			case 0: printf("VERTICAL   "), fprintf(fptr,"VERTICAL   "); break;
			case 1: printf("HORIZONTAL "), fprintf(fptr,"HORIZONTAL "); break;
			case 2: printf("NONE       "), fprintf(fptr,"HORIZONTAL "); break;
		}

		if (lof >= 1 && lof <= CBAND_LOF && dtv_frequency_prop != 0)
			printf("%-8d ", (lof - currentfreq) * 1000), fprintf(fptr,"%-8d ", (lof - currentfreq) * 1000);
		else if (dtv_frequency_prop != 0)
			printf("%-8d ", (currentfreq + lof) * 1000), fprintf(fptr,"%-8d ", (currentfreq + lof) * 1000);
		else
			printf("%-8d ", dtv_frequency_prop * 1000), fprintf(fptr,"%-8d ", dtv_frequency_prop * 1000);

		printf("%-8d ", currentsr * 1000), fprintf(fptr,"%-8d ", currentsr * 1000);
		//printf("SIG %2.1f %s ", signal, (lvl_scale == FE_SCALE_DECIBEL) ? "dBm" : "%");
		//printf("SNR %2.1f dB ", snr);

		switch (dtv_delivery_system_prop) {
			case 4:  printf("DSS    "), fprintf(fptr,"DSS    "); break;
			case 5:  printf("DVB-S  "), fprintf(fptr,"DVB-S  "); break;
			case 6:  printf("DVB-S2 "), fprintf(fptr,"DVB-S2 "); break;
			default:
				if (verbose) printf("SYS(%d) ", dtv_delivery_system_prop), fprintf(fptr,"SYS(%d) ", dtv_delivery_system_prop);
				else printf("DVB-S  "), fprintf(fptr,"DVB-S  ");
				break;
		}

		switch (dtv_inversion_prop) {
			case 0:  printf("INVERSION_OFF  "), fprintf(fptr,"INVERSION_OFF  "); break;
			case 1:  printf("INVERSION_ON   "), fprintf(fptr,"INVERSION_ON   "); break;
			case 2:  printf("INVERSION_AUTO "), fprintf(fptr,"INVERSION_AUTO "); break;
			default:
				if (verbose) printf("INVERSION (%d) ", dtv_inversion_prop), fprintf(fptr,"INVERSION (%d) ", dtv_inversion_prop);
				else printf("INVERSION_AUTO "), fprintf(fptr,"INVERSION_AUTO ");
				break;
		}

		switch (dtv_pilot_prop) {
			case 0:  printf("PILOT_ON   "), fprintf(fptr,"PILOT_ON   "); break;
			case 1:  printf("PILOT_OFF  "), fprintf(fptr,"PILOT_OFF  "); break;
			case 2:  printf("PILOT_AUTO "), fprintf(fptr,"PILOT_AUTO "); break;
			default:
				if (verbose) printf("PILOT (%d) ", dtv_pilot_prop), fprintf(fptr,"PILOT (%d) ", dtv_pilot_prop);
				else printf("PILOT_AUTO "), fprintf(fptr,"PILOT_AUTO ");
				break;
		}

		switch (dtv_inner_fec_prop) {
			case 0: printf("FEC_NONE "), fprintf(fptr,"FEC_NONE ");  break;
			case 1: printf("FEC_1_2  "), fprintf(fptr,"FEC_1_2  ");   break;
			case 2: printf("FEC_2_3  "), fprintf(fptr,"FEC_2_3  ");   break;
			case 3: printf("FEC_3_4  "), fprintf(fptr,"FEC_3_4  ");   break;
			case 4: printf("FEC_4_5  "), fprintf(fptr,"FEC_4_5  ");   break;
			case 5: printf("FEC_5_6  "), fprintf(fptr,"FEC_5_6  ");   break;
			case 6: printf("FEC_6_7  "), fprintf(fptr,"FEC_6_7  ");   break;
			case 7: printf("FEC_7_8  "), fprintf(fptr,"FEC_7_8  ");   break;
			case 8: printf("FEC_8_9  "), fprintf(fptr,"FEC_8_9  ");   break;
			case 9: printf("FEC_AUTO "), fprintf(fptr,"FEC_AUTO ");  break;
			case 10: printf("FEC_3_5  "), fprintf(fptr,"FEC_3_5  ");  break;
			case 11: printf("FEC_9_10 "), fprintf(fptr,"FEC_9_10 "); break;
			default:
				if (verbose) printf("FEC (%d)  ", dtv_inner_fec_prop), fprintf(fptr,"FEC (%d)  ", dtv_inner_fec_prop);
				else printf("FEC_AUTO "), fprintf(fptr,"FEC_AUTO ");
				break;
		}

		switch (dtv_modulation_prop) {
			case 0: printf("QPSK "), fprintf(fptr,"QPSK "); break;
			case 9: printf("8PSK "), fprintf(fptr,"8PSK "); break;
			case 10: printf("16APSK "), fprintf(fptr,"16APSK "); break;
			case 11: printf("32APSK "), fprintf(fptr,"32APSK "); break;
			default:
				if (verbose) printf("MOD(%d) ", dtv_modulation_prop), fprintf(fptr,"MOD(%d) ", dtv_modulation_prop);
				else printf("QPSK "), fprintf(fptr,"QPSK ");
				break;
		}

		switch (dtv_rolloff_prop) {
			case 0:  printf("ROLLOFF_35\n"), fprintf(fptr,"ROLLOFF_35\n"), fclose(fptr);   break;
			case 1:  printf("ROLLOFF_20\n"), fprintf(fptr,"ROLLOFF_20\n"), fclose(fptr);   break;
			case 2:  printf("ROLLOFF_25\n"), fprintf(fptr,"ROLLOFF_25\n"), fclose(fptr);  break;
			case 3:  printf("ROLLOFF_AUTO\n"), fprintf(fptr,"ROLLOFF_AUTO\n"), fclose(fptr); break;
			default:
				if (verbose) printf("ROL (%d)\n", dtv_rolloff_prop), fprintf(fptr,"ROL (%d)\n", dtv_rolloff_prop), fclose(fptr);
				else printf("ROLLOFF_AUTO\n"), fprintf(fptr,"ROLLOFF_AUTO\n"), fclose(fptr);
				break;

		}
	fclose(fptr);
	}
}





