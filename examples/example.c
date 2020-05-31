#include "hsv.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

#define WAV_HEADER_LEN 78

#define BUFCAP 16384

#define SAMPLE_RATE 16000
#define CHANNELS    1
#define BS          16

#define BUF_LEN_IN  8192
#define BUF_LEN_OUT 8192

static char wav_header_buf[WAV_HEADER_LEN];

static char buf_in[BUF_LEN_IN];
static char buf_out[BUF_LEN_IN];

static void LOG(const char*format, ...)
{
	va_list var_args;

	va_start(var_args, format);
	vfprintf(stderr, format, var_args);
	va_end(var_args);
}

static void print_usage(const char*prog_name)
{
	LOG("Usage: %s --mode input_file.wav output_file.wav\n", prog_name);
	LOG("Example: %s --wiener data/in/car_ns.wav data/out/car_ns.wav\n", prog_name);
	LOG("Modes:\n");
	LOG("      --specsub - Berouti-Schwartz spectral subtraction.\n");
	LOG("      --wiener  - Scalart's wiener filtering.\n");
	LOG("      --tsnr    - Scalart's two-steps noise reduction.\n");
	LOG("      --tsnrg   - Scalart's two-steps noise reduction with gain.\n");
	LOG("      --rtsnr   - Shifeng's two-steps noise reduction.\n");
	LOG("      --rtsnrg  - Shifeng's two-steps noise reduction with gain.\n");
}

#define SWITCH_HSVC_CODE(hsv_r, r_str)									\
	do {																\
		switch ((hsv_r)) {												\
		case HSV_CODE_OK:												\
			(r_str) = "No error!";										\
			break;														\
																		\
		case HSV_CODE_ALLOC_ERR:										\
			(r_str) = "\"hsv\" context allocation error!";				\
			break;														\
																		\
		case HSV_CODE_OVERFLOW_ERR:										\
			(r_str) = "\"hsv\" context overflow error!";				\
			break;														\
																		\
		case HSV_CODE_UNKNOWN_ERR:										\
			(r_str) = "\"hsv\" context unkown error!";					\
			break;														\
																		\
		default:														\
			(r_str) = "Non-\"hsv\" context error!";						\
			break;														\
		}																\
	} while (0)

int main(int argc, char**argv)
{
	int r;
	const char*r_str = NULL;

	enum HSV_CODE hsv_r;

	struct HSV_CONFIG conf;
	
	hsvc_t hsvc;

	const char*mode;

	const char*fname_in;
	const char*fname_out;
	
	FILE*f_in;
	FILE*f_out;

	unsigned data_len;

	int processed;

	clock_t proc_start_time;
	clock_t proc_end_time;
	clock_t proc_elapsed_time;

	struct timeval unix_start_time;
	struct timeval unix_end_time;
	unsigned long long unix_elapsed_time;
	
	if (argc < 4) {
		print_usage(argv[0]);
		return 1;
	}

	mode = argv[1];
	
	fname_in = argv[2];
	fname_out = argv[3];

	memset(&conf, '\0', sizeof(conf));
	conf.sr = SAMPLE_RATE;
	conf.ch = CHANNELS;
	conf.bs = BS;

	if (strcmp(mode, "--specsub") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_SPECSUB;
	} else if (strcmp(mode, "--wiener") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_WIENER;
	} else if (strcmp(mode, "--tsnr") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_TSNR;
	} else if (strcmp(mode, "--tsnrg") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_TSNR_G;
	} else if (strcmp(mode, "--rtsnr") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_RTSNR;
	} else if (strcmp(mode, "--rtsnrg") == 0) {
		conf.mode = HSV_SUPPRESSOR_MODE_RTSNR_G;
	} else {
		print_usage(argv[0]);
		return 2;
	}

	r = hsvc_validate_config(&conf);
	if ((enum HSV_CODE) r != HSV_CODE_OK) {
		LOG("Invalid configuration in parameter (%d)\n", r);
		return 3;
	}

	proc_start_time = clock();
	gettimeofday(&unix_start_time, NULL);

	hsvc = create_hsvc();
	if (hsvc == NULL) {
		r_str = "Unable to create \"hsv\" context!";
		r = 4;
		goto err0;
	}

	hsv_r = hsvc_config(hsvc, &conf);
	if (hsv_r != HSV_CODE_OK) {
		SWITCH_HSVC_CODE(hsv_r, r_str);
		r = 5;
		goto err1;
	}

	f_in = fopen(fname_in, "rb");
	if (f_in == NULL) {
		r_str = "Unable to open input \"wav\" file!";
		r = 6;
		goto err2;
	}
	f_out = fopen(fname_out, "wb");
	if (f_out == NULL) {
		r_str = "Unable to open input \"wav\" file!";
		r = 7;
		goto err3;
	}

	data_len = fread(wav_header_buf, sizeof(char), WAV_HEADER_LEN, f_in);
	if (data_len < WAV_HEADER_LEN) {
		r = 8;
		goto err4;
	}
	
	fwrite(wav_header_buf, sizeof(char), WAV_HEADER_LEN, f_out);

	while (! feof(f_in)) {
		data_len = fread(buf_in, sizeof(char), BUF_LEN_IN, f_in);
		processed = hsvc_push(hsvc, buf_in, data_len);
		if (processed < 0) {
			SWITCH_HSVC_CODE((enum HSV_CODE) processed, r_str);
			r = 8;
			goto err4;
		} else if (processed > 0) {
			processed = hsvc_get(hsvc, buf_out, BUF_LEN_OUT);
			while (processed != 0) {
				fwrite(buf_out, sizeof(char), processed, f_out);
				processed = hsvc_get(hsvc, buf_out, BUF_LEN_OUT);
			}
		}
	}

	hsvc_flush(hsvc);
	processed = hsvc_get(hsvc, buf_out, BUF_LEN_OUT);
	while (processed != 0) {
		fwrite(buf_out, sizeof(char), processed, f_out);
		processed = hsvc_get(hsvc, buf_out, BUF_LEN_OUT);
	}

	fclose(f_out);
	fclose(f_in);

	hsvc_deconfig(hsvc);
	hsvc_free(hsvc);

	proc_end_time = clock();
	gettimeofday(&unix_end_time, NULL);

	proc_elapsed_time = proc_end_time - proc_start_time;
	LOG("Proc time elapsed: %.2lf ms\n",
		((double) proc_elapsed_time) / CLOCKS_PER_SEC * 1000.0);

	unix_elapsed_time = ((unsigned long long)(unix_end_time.tv_sec) * 1000 * 1000 +
						 (unsigned long long)(unix_end_time.tv_usec)) -
		((unsigned long long)(unix_start_time.tv_sec) * 1000 * 1000 +
		 (unsigned long long)(unix_start_time.tv_usec));
	LOG("Real time elapsed: %.2lf ms\n",
		((double) unix_elapsed_time) / 1000.0);

	return 0;

 err4:
	fclose(f_out);
 err3:
	fclose(f_in);
 err2:
	hsvc_deconfig(hsvc);
 err1:
	hsvc_free(hsvc);
 err0:
	LOG("%s\n", r_str);
	return r;
}
