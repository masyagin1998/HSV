/**
 * \file dft.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API алгоритма дискретного преобразования Фурье.
 */
/**
 * \ingroup dft
 * \{
 */
#include "dft.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>

dft_t create_dft()
{
	dft_t dft;

	dft = (dft_t) calloc(1, sizeof(struct DISCRETE_FOURIER_TRANSFORM));
	return dft;
}

static int is_pow_2(unsigned n);
static unsigned next_pow_2(unsigned n);
static void dft_inner(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n);

static enum DFT_CODE config_cooley_tukey(struct COOLEY_TUKEY*ct, unsigned dft_size)
{
	enum DFT_CODE r;

	unsigned i;

	if (is_pow_2(dft_size)) {
		ct->tab_size = dft_size / 2;
	} else {
		ct->tab_size = next_pow_2(dft_size) / 2;
	}
	ct->sin_tab = (hsv_numeric_t*) calloc(ct->tab_size, sizeof(hsv_numeric_t));
	if (ct->sin_tab == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err0;
	}
	ct->cos_tab = (hsv_numeric_t*) calloc(ct->tab_size, sizeof(hsv_numeric_t));
	if (ct->cos_tab == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err1;
	}

	ct->tab_size *= 2;
	for (i = 0; i < ct->tab_size / 2; i++) {
		ct->cos_tab[i] = HSV_COS(2 * M_PI * i / ct->tab_size);
		ct->sin_tab[i] = HSV_SIN(2 * M_PI * i / ct->tab_size);
	}
	ct->tab_size /= 2;
	
	ct->initialized = 1;

	return DFT_CODE_OK;

 err1:
	free(ct->sin_tab);
 err0:
	return r;
}

static void deconfig_cooley_tukey(struct COOLEY_TUKEY*ct)
{
	if (! ct->initialized) {
		return;
	}

	free(ct->cos_tab);
	free(ct->sin_tab);
}

static enum DFT_CODE config_bluestein(struct BLUESTEIN*bl, unsigned dft_size)
{
	enum DFT_CODE r;
	
	unsigned i;

	if (is_pow_2(dft_size)) {
		return DFT_CODE_OK;
	}

	bl->tab_size = dft_size;
	bl->sin_tab = (hsv_numeric_t*) calloc(dft_size, sizeof(hsv_numeric_t));
	if (bl->sin_tab == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err0;
	}
	bl->cos_tab = (hsv_numeric_t*) calloc(dft_size, sizeof(hsv_numeric_t));
	if (bl->cos_tab == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err1;
	}
	bl->nb = next_pow_2(dft_size);
	bl->a_real = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->a_real == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err2;
	}
	bl->a_imag = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->a_imag == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err3;
	}
	bl->b_real = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->b_real == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err4;
	}
	bl->b_imag = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->b_imag == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err5;
	}
	bl->c_real = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->c_real == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err6;
	}
	bl->c_imag = (hsv_numeric_t*) calloc(sizeof(hsv_numeric_t), bl->nb);
	if (bl->c_imag == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err7;
	}

	for (i = 0; i < bl->tab_size; i++) {
		unsigned tmp = ((unsigned)i * i) % ((unsigned) bl->tab_size * 2);
		hsv_numeric_t angle = M_PI * tmp / bl->tab_size;
		bl->cos_tab[i] = HSV_COS(angle);
		bl->sin_tab[i] = HSV_SIN(angle);
	}

	/* Вычисляется лишь один раз на старте. */
	bl->b_real[0] = bl->cos_tab[0];
	bl->b_imag[0] = bl->sin_tab[0];
	for (i = 1; i < bl->tab_size; i++) {
		bl->b_real[i] = bl->b_real[bl->nb - i] = bl->cos_tab[i];
		bl->b_imag[i] = bl->b_imag[bl->nb - i] = bl->sin_tab[i];
	}

	bl->initialized = 1;

	return DFT_CODE_OK;

 err7:
	free(bl->c_real);
 err6:
	free(bl->b_imag);
 err5:
	free(bl->b_real);
 err4:
	free(bl->a_imag);
 err3:
	free(bl->a_real);
 err2:
	free(bl->cos_tab);
 err1:
	free(bl->sin_tab);
 err0:
	return r;
}

static void deconfig_bluestein(struct BLUESTEIN*bl)
{
	if (! bl->initialized) {
		return;
	}

	free(bl->c_imag);
	free(bl->c_real);
	free(bl->b_imag);
	free(bl->b_real);
	free(bl->a_imag);
	free(bl->a_real);
	free(bl->cos_tab);
	free(bl->sin_tab);
}

enum DFT_CODE dft_config(dft_t dft, unsigned dft_size)
{
	enum DFT_CODE r;

	dft->dft_size = dft_size;
	dft->real = (hsv_numeric_t*) calloc(dft_size, sizeof(hsv_numeric_t));
	if (dft->real == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err0;
	}
	dft->imag = (hsv_numeric_t*) calloc(dft_size, sizeof(hsv_numeric_t));
	if (dft->imag == NULL) {
		r = DFT_CODE_ALLOC_ERR;
		goto err1;
	}

	r = config_cooley_tukey(&(dft->ct), dft_size);
	if (r != DFT_CODE_OK) {
		goto err2;
	}

	r = config_bluestein(&(dft->bl), dft_size);
	if (r != DFT_CODE_OK) {
		goto err3;
	}
	/* Вычисляется лишь один раз на старте. */
	dft_inner(dft, dft->bl.b_real, dft->bl.b_imag, dft->bl.nb);

	return DFT_CODE_OK;

 err3:
	deconfig_cooley_tukey(&(dft->ct));
 err2:
	free(dft->imag);
 err1:
	free(dft->real);
 err0:
	return r;
}

static void cooley_tukey(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n);
static void bluestein(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n);

static void dft_inner(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n)
{
	if (n == 0) {
		return;
	} else if (is_pow_2(n)) {
		cooley_tukey(dft, real, imag, n);
	} else {
		bluestein(dft, real, imag, n);
	}
}

static unsigned inverse(unsigned val, int w)
{
	int i;

	unsigned res = 0;

	for (i = 0; i < w; i++, val >>= 1) {
		res = (res << 1) | (val & 1U);
	}
	return res;
}

static void swap_complex(hsv_numeric_t*real_a, hsv_numeric_t*imag_a, hsv_numeric_t*real_b, hsv_numeric_t*imag_b)
{
	hsv_numeric_t tmp;

	tmp = *real_a;
	*real_a = *real_b;
	*real_b = tmp;

	tmp = *imag_a;
	*imag_a = *imag_b;
	*imag_b = tmp;
}

static void cooley_tukey(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n)
{
	unsigned i, j, k;

	unsigned n2;
	
	int lvls = 0;
	for (i = n; i > ((unsigned) 1); i >>= 1) {
		lvls++;
	}

	for (i = 0; i < n; i++) {
		j = inverse(i, lvls);
		if (j > i) {
			swap_complex(real + i, imag + i, real + j, imag + j);
		}
	}

	for (n2 = 2; n2 <= n; n2 *= 2) {
		unsigned half = n2 / 2;
		unsigned tab_step = n / n2;

		for (i = 0; i < n; i += n2) {
			for (j = i, k = 0; j < i + half; j++, k += tab_step) {
				unsigned ind = j + half;
				hsv_numeric_t tmp_real = real[ind] * dft->ct.cos_tab[k] +
					imag[ind] * dft->ct.sin_tab[k];
				hsv_numeric_t tmp_imag = -real[ind] * dft->ct.sin_tab[k] +
					imag[ind] * dft->ct.cos_tab[k];

				real[ind] = real[j] - tmp_real;
				imag[ind] = imag[j] - tmp_imag;

				real[j] += tmp_real;
				imag[j] += tmp_imag;
			}
		}

		if (n2 == n) {
			return;
		}
	}
}

static void convolve(
		dft_t dft,
		hsv_numeric_t*a_real, hsv_numeric_t*a_imag,
		hsv_numeric_t*b_real, hsv_numeric_t*b_imag,
		hsv_numeric_t*c_real, hsv_numeric_t*c_imag, unsigned n);

static void bluestein(dft_t dft, hsv_numeric_t*real, hsv_numeric_t*imag, unsigned n)
{
	unsigned i;
	unsigned m = next_pow_2(n);

	memset(dft->bl.a_real, '\0', dft->bl.nb * sizeof(hsv_numeric_t));
	memset(dft->bl.a_imag, '\0', dft->bl.nb * sizeof(hsv_numeric_t));

	for (i = 0; i < n; i++) {
		dft->bl.a_real[i] =	real[i] * dft->bl.cos_tab[i] +
			imag[i] * dft->bl.sin_tab[i];
		dft->bl.a_imag[i] = -real[i] * dft->bl.sin_tab[i] +
			imag[i] * dft->bl.cos_tab[i];
	}

	/*
	// Вычисляется лишь один раз на старте в строках 138-144.
	memset(dft->bl.b_real, '\0', dft->bl.nb * sizeof(hsv_numeric_t));
	memset(dft->bl.b_imag, '\0', dft->bl.nb * sizeof(hsv_numeric_t));
	
	dft->bl.b_real[0] = dft->bl.cos_tab[0];
	dft->bl.b_imag[0] = dft->bl.sin_tab[0];
	for (i = 1; i < n; i++) {
	  	dft->bl.b_real[i] = dft->bl.b_real[m - i] = dft->bl.cos_tab[i];
		dft->bl.b_imag[i] = dft->bl.b_imag[m - i] = dft->bl.sin_tab[i];
	}
	*/
	
	convolve(dft,
			 dft->bl.a_real, dft->bl.a_imag,
			 dft->bl.b_real, dft->bl.b_imag,
			 dft->bl.c_real, dft->bl.c_imag, m);
	
	for (i = 0; i < n; i++) {
		real[i] =  dft->bl.c_real[i] * dft->bl.cos_tab[i] +
			dft->bl.c_imag[i] * dft->bl.sin_tab[i];
		imag[i] = -dft->bl.c_real[i] * dft->bl.sin_tab[i] +
			dft->bl.c_imag[i] * dft->bl.cos_tab[i];
	}
}

static void convolve(
		dft_t dft,
		hsv_numeric_t*a_real, hsv_numeric_t*a_imag,
		hsv_numeric_t*b_real, hsv_numeric_t*b_imag,
		hsv_numeric_t*c_real, hsv_numeric_t*c_imag, unsigned n)
{
	unsigned i;

	dft_inner(dft, a_real, a_imag, n);
	/*
	// Вычисляется лишь один раз на старте в строках 209-210
	dft_inner(dft, b_real, b_imag, n);
	*/
	
	for (i = 0; i < n; i++) {
		hsv_numeric_t tmp = a_real[i] * b_real[i] -
			a_imag[i] * b_imag[i];
		a_imag[i] = a_imag[i] * b_real[i] +
			a_real[i] * b_imag[i];
		a_real[i] = tmp;
	}

	/* Обратное быстрое преобразование Фурье. */
	dft_inner(dft, a_imag, a_real, n);
	
	for (i = 0; i < n; i++) {
		c_real[i] = a_real[i] / n;
		c_imag[i] = a_imag[i] / n;
	}
}

static int is_pow_2(unsigned n)
{
	return ((n & (n - 1)) == 0);
}

static unsigned next_pow_2(unsigned n)
{
	unsigned i = 1;

	while ((i / 2) <= n) {
		i *= 2;
	}
	return i;
}

void dft_run_dft(dft_t dft)
{
	dft_inner(dft, dft->real, dft->imag, dft->dft_size);
}

void dft_run_i_dft(dft_t dft)
{
	unsigned i;

	dft_inner(dft, dft->imag, dft->real, dft->dft_size);

	for (i = 0; i < dft->dft_size; i++) {
		dft->real[i] = dft->real[i] / ((hsv_numeric_t) dft->dft_size);
		dft->imag[i] = dft->imag[i] / ((hsv_numeric_t) dft->dft_size);
	}
}

void dft_deconfig(dft_t dft)
{
	deconfig_bluestein(&(dft->bl));

	deconfig_cooley_tukey(&(dft->ct));

	free(dft->imag);
	free(dft->real);
}

void dft_clean(dft_t dft)
{
	memset(dft, '\0', sizeof(*dft));
}

void dft_free(dft_t dft)
{
	free(dft);
}
/**
 * /}
 */
