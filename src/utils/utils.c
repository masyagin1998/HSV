/**
 * \file utils.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API вспомогательных алгоритмов
 */
/**
 * \ingroup utils
 * \{
 */
#include "utils.h"

#include <math.h>

static hsv_numeric_t hamming(unsigned i, unsigned n)
{
	static const hsv_numeric_t A0 = 0.538360;
	static const hsv_numeric_t A1 = 0.461640;
	
	return A0 - A1 * HSV_COS(((hsv_numeric_t) 2.0) * ((hsv_numeric_t) M_PI) * (i / ((hsv_numeric_t) n)));
}

static void init_hamming_window(hsv_numeric_t*window, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		window[i] = hamming(i, n);
	}
}

static hsv_numeric_t hanning(unsigned i, unsigned n)
{
	static const hsv_numeric_t A0 = 0.5;
	static const hsv_numeric_t A1 = 0.5;
	
	return A0 - A1 * HSV_COS(((hsv_numeric_t) 2.0) * ((hsv_numeric_t) M_PI) * (i / ((hsv_numeric_t) n)));
}

static void init_hanning_window(hsv_numeric_t*window, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		window[i] = hanning(i, n);
	}
}

void init_window(hsv_numeric_t*window, unsigned n, enum WINDOW_TYPE wt)
{
	switch (wt) {
	case WINDOW_TYPE_HAMMING:
		init_hamming_window(window, n);
		break;
	case WINDOW_TYPE_HANNING:
		init_hanning_window(window, n);
		break;
	default:
		init_hamming_window(window, n);
		break;
	}
}

void calculate_windowing(const hsv_numeric_t*window, const hsv_numeric_t*in, hsv_numeric_t*out, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		out[i] = in[i] * window[i];
	}
}

void calculate_amp_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*amp_spec, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		amp_spec[i] = HSV_SQRT(real[i] * real[i] + imag[i] * imag[i]);
	}
}

void calculate_power_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*power_spec, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		power_spec[i] = real[i] * real[i] + imag[i] * imag[i];
	}
}

void calculate_phase_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*phase_spec, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		phase_spec[i] = HSV_ATAN2(imag[i], real[i]);
	}
}
/**
 * /}
 */

