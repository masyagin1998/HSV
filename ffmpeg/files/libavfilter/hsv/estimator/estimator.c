/**
 * \file estimator.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API алгоритма оценивания шума.
 */
/**
 * \ingroup estimator
 * \{
 */
#include "estimator.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>

estimator_t create_estimator()
{
	estimator_t est;

	est = (estimator_t) calloc(1, sizeof(struct ESTIMATOR));
	return est;
}

static void init_delta_k(hsv_numeric_t*delta_k, unsigned sr, unsigned size)
{
	/* Так как частоты человеческого голоса лежат в диапазоне 0..3000 Hz,
	 причем основная часть диапазоне 0..1000 Hz, то для данных частот устанавливаются
	 меньшие пороговые значения наличия речи. Исследования Лойзю и Рангачари показали,
	 что универсальными порогами могут быть 2.0 для 0..1000HZ, 2.0 для 1000..3000Hz и
	 5.0 для 3000..SR/2 HZ. В связи с частотой дискретизации, берущейся минимум x 2 (т. Котельникова)
	 данные значения дельт отражаются симметрично относительно половина sr. */

	static const hsv_numeric_t delta_lf = 2.0;
	static const hsv_numeric_t delta_mf = 2.0;
	static const hsv_numeric_t delta_hf = 5.0;

	unsigned k;

	hsv_numeric_t freq_res = ((hsv_numeric_t) sr) / ((hsv_numeric_t) size);
	unsigned LF = HSV_FLOOR(1000.0 / freq_res);
	unsigned MF = HSV_FLOOR(3000.0 / freq_res);

	for (k = 0; k < LF; k++) {
		delta_k[k] = delta_lf;
	}
	for (k = 0; k < MF - LF; k++) {
		delta_k[k + LF] = delta_mf;
	}
	for (k = 0; k < (size / 2) - MF; k++) {
		delta_k[k + MF] = delta_hf;
	}
	delta_k[size / 2] = 5.0;
	for (k = 1; k < size / 2; k++) {
		delta_k[size - k] = delta_k[k];
	}
}

enum ESTIMATOR_CODE estimator_config(estimator_t est, unsigned sr, unsigned size)
{
	static const hsv_numeric_t alpha_smooth = 0.7;

	static const hsv_numeric_t beta = 0.8;
	static const hsv_numeric_t gamma = 0.998;

	static const hsv_numeric_t alpha_spp = 0.2;
	
	static const hsv_numeric_t alpha = 0.95;
	
	
	enum ESTIMATOR_CODE r;

	est->size = size;

	est->delta_k = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->delta_k == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err0;
	}
	init_delta_k(est->delta_k, sr, size);

	est->alpha_smooth = alpha_smooth;
	est->P = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->P == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err1;
	}
	est->P_prev = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->P_prev == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err2;
	}

	est->beta = beta;
	est->gamma = gamma;
	est->P_min = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->P_min == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err3;
	}
	est->P_min_prev = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->P_min_prev == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err4;
	}

	est->alpha_spp = alpha_spp;
	est->spp_k = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->spp_k == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err5;
	}

	est->alpha = alpha;

	est->noise_power_spec = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->noise_power_spec == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err6;
	}
	est->noise_amp_spec = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (est->noise_amp_spec == NULL) {
		r = ESTIMATOR_CODE_ALLOC_ERR;
		goto err7;
	}

	return ESTIMATOR_CODE_OK;

 err7:
	free(est->noise_power_spec);
 err6:
	free(est->spp_k);
 err5:
	free(est->P_min_prev);
 err4:
	free(est->P_min);
 err3:
	free(est->P_prev);
 err2:
	free(est->P);
 err1:
	free(est->delta_k);
 err0:
	return r;
}

static void estimator_calculate_noise_amp_spec(estimator_t est)
{
	unsigned k;

	for (k = 0; k < est->size; k++) {
		est->noise_amp_spec[k] = HSV_SQRT(est->noise_power_spec[k]);
	}
}

static void estimator_get_first(estimator_t est, hsv_numeric_t*P)
{
	memcpy(est->P, P, est->size * sizeof(hsv_numeric_t));
	memcpy(est->P_prev, P, est->size * sizeof(hsv_numeric_t));

	memcpy(est->P_min, P, est->size * sizeof(hsv_numeric_t));
	memcpy(est->P_min_prev, P, est->size * sizeof(hsv_numeric_t));

	memcpy(est->noise_power_spec, P, est->size * sizeof(hsv_numeric_t));
	estimator_calculate_noise_amp_spec(est);
	
	est->got_first = 1;
}

static void estimator_process(estimator_t est, hsv_numeric_t*P)
{
	unsigned k;

	/* Сглаживание спектра входного зашумленного сигнала. */
	for (k = 0; k < est->size; k++) {
		est->P[k] = est->alpha_smooth * est->P_prev[k] + (1.0 - est->alpha_smooth) * P[k];
	}

	memcpy(est->P_prev, est->P, est->size * sizeof(hsv_numeric_t));

	/* Непрерывное отслеживание спектральных минимумов по Доблингеру. */
	for (k = 0; k < est->size; k++) {
		if (est->P_min_prev[k] < est->P[k]) {
			est->P_min[k] = est->gamma * est->P_min_prev[k] + ((1.0 - est->gamma) / (1.0 - est->beta)) * (est->P[k] - est->beta * est->P_prev[k]);
		} else {
			est->P_min[k] = est->P[k];
		}
	}

	memcpy(est->P_min_prev, est->P_min, est->size * sizeof(hsv_numeric_t));

	/* Оценка спектра шума методом MCRA-2. */
	for (k = 0; k < est->size; k++) {
		hsv_numeric_t ak;
		/* Апостериорный SNR сглаженного зашумленного голоса. */
		hsv_numeric_t Sr_k = est->P[k] / est->P_min[k];
		/* Бинарная оценка наличия голоса. */
		hsv_numeric_t spp_k = 0.0;
		if (Sr_k > est->delta_k[k]) {
			spp_k = 1.0;
		}
		/* Сглаживание вероятности наличия голоса во времени. */
		est->spp_k[k] = est->alpha_spp * est->spp_k[k] + (1.0 - est->alpha_spp) * spp_k;
		/* Расчет коэффициента сглаживания шума в частотно-временной области. */
		ak = est->alpha + (1.0 - est->alpha) * est->spp_k[k];
		/* Итоговая оценка спектра шума. */
		est->noise_power_spec[k] = ak * est->noise_power_spec[k] + (1.0 - ak) * est->P[k];
	}

	estimator_calculate_noise_amp_spec(est);
}

void estimator_run(estimator_t est, hsv_numeric_t*P)
{
	if (! est->got_first) {
		estimator_get_first(est, P);
	} else {
		estimator_process(est, P);
	}
}

void estimator_deconfig(estimator_t est)
{
	free(est->noise_amp_spec);
	free(est->noise_power_spec);
	free(est->spp_k);
	free(est->P_min_prev);
	free(est->P_min);
	free(est->P_prev);
	free(est->P);
	free(est->delta_k);
}

void estimator_clean(estimator_t est)
{
	memset(est, '\0', sizeof(*est));
}

void estimator_free(estimator_t est)
{
	free(est);
}
/**
 * /}
 */
