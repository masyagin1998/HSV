/**
 * \file suppressor.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API алгоритм шумоподавления.
 */
/**
 * \ingroup suppressor
 * \{
 */
#include "suppressor.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>

suppressor_t create_suppressor()
{
	suppressor_t sup;

	sup = (suppressor_t) calloc(1, sizeof(struct SUPPRESSOR));
	return sup;
}

static enum SUPPRESSOR_CODE specsub_config(struct SUPPRESSOR_SPECSUB*specsub, unsigned sr, unsigned size)
{
	const hsv_numeric_t power_exponent = 2.0;

	specsub->sr = sr;
	specsub->size = size;

	specsub->power_exponent = power_exponent;

	return SUPPRESSOR_CODE_OK;
}

static void specsub_deconfig(struct SUPPRESSOR_SPECSUB*specsub)
{
	PREFIX_UNUSED(specsub);
}

static enum SUPPRESSOR_CODE wiener_config(struct SUPPRESSOR_WIENER*wiener, unsigned sr, unsigned size)
{
	static const hsv_numeric_t beta = 0.98;
	static const hsv_numeric_t floor = 0.01;

	enum SUPPRESSOR_CODE r;

	wiener->sr = sr;
	wiener->size = size;

	wiener->beta = beta;
	wiener->floor = floor;

	wiener->noise_power_spec = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->noise_power_spec == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err0;
	}

	wiener->SNR_inst = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->SNR_inst == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err1;
	}
	wiener->SNR_prio_dd = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->SNR_prio_dd == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err2;
	}
	wiener->G_dd = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->G_dd == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err3;
	}
	
	wiener->speech_amp_spec = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->speech_amp_spec == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err4;
	}
	wiener->speech_amp_spec_prev = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (wiener->speech_amp_spec_prev == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err5;
	}

	return SUPPRESSOR_CODE_OK;

 err5:
	free(wiener->speech_amp_spec);
 err4:
	free(wiener->G_dd);
 err3:
	free(wiener->SNR_prio_dd);
 err2:
	free(wiener->SNR_inst);
 err1:
	free(wiener->noise_power_spec);
 err0:
	return r;
}

static void wiener_deconfig(struct SUPPRESSOR_WIENER*wiener)
{
	free(wiener->speech_amp_spec_prev);
	free(wiener->speech_amp_spec);

	free(wiener->G_dd);
	free(wiener->SNR_prio_dd);
	free(wiener->SNR_inst);

	free(wiener->noise_power_spec);
}

static enum SUPPRESSOR_CODE gain_config(struct SUPPRESSOR_GAIN*gain, unsigned sr, unsigned size)
{
	enum DFT_CODE dft_r;
	
	enum SUPPRESSOR_CODE r;

	PREFIX_UNUSED(sr);

	gain->L1 = size;
	gain->L2 = gain->L1 / 2;
	
	dft_r = dft_config(&(gain->dft), size);
	if (dft_r != DFT_CODE_OK) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err0;
	}
	gain->window = (hsv_numeric_t*) calloc(gain->L2, sizeof(hsv_numeric_t));
	if (gain->window == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err1;
	}
	init_window(gain->window, gain->L2, WINDOW_TYPE_HAMMING);
	gain->impulse_response_before = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (gain->impulse_response_before == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err2;
	}
	gain->impulse_response_after = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (gain->impulse_response_after == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err3;
	}

	return SUPPRESSOR_CODE_OK;

 err3:
	free(gain->impulse_response_before);
 err2:
	free(gain->window);
 err1:
	dft_deconfig(&(gain->dft));
 err0:
	return r;
}

static void gain_deconfig(struct SUPPRESSOR_GAIN*gain)
{
	free(gain->impulse_response_after);
	free(gain->impulse_response_before);
	free(gain->window);
	dft_deconfig(&(gain->dft));
}

static enum SUPPRESSOR_CODE tsnr_config(struct SUPPRESSOR_TSNR*tsnr, unsigned sr, unsigned size, enum SUPPRESSOR_MODE mode)
{
	enum SUPPRESSOR_CODE r;

	tsnr->mode = mode;

	r = wiener_config(&(tsnr->wiener), sr, size);
	if (r != SUPPRESSOR_CODE_OK) {
		goto err0;
	}

	tsnr->SNR_prio_2_step = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (tsnr->SNR_prio_2_step == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err1;
	}
	tsnr->G_2_step = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (tsnr->G_2_step == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err2;
	}

	if ((mode == SUPPRESSOR_MODE_TSNR_G) || (mode == SUPPRESSOR_MODE_RTSNR_G)) {
		r = gain_config(&(tsnr->gain), sr, size);
		if (r != SUPPRESSOR_CODE_OK) {
			goto err3;
		}
	}

	return SUPPRESSOR_CODE_OK;

 err3:
	free(tsnr->G_2_step);
 err2:
	free(tsnr->SNR_prio_2_step);
 err1:
	wiener_deconfig(&(tsnr->wiener));
 err0:
	return r;
}

static void tsnr_deconfig(struct SUPPRESSOR_TSNR*tsnr)
{
	if ((tsnr->mode == SUPPRESSOR_MODE_TSNR_G) || (tsnr->mode == SUPPRESSOR_MODE_RTSNR_G)) {
		gain_deconfig(&(tsnr->gain));
	}

	free(tsnr->G_2_step);
	free(tsnr->SNR_prio_2_step);

	wiener_deconfig(&(tsnr->wiener));
}

enum SUPPRESSOR_CODE suppressor_config(suppressor_t sup, unsigned sr, unsigned size, enum SUPPRESSOR_MODE mode)
{
	enum SUPPRESSOR_CODE r;

	sup->sr = sr;
	sup->size = size;

	sup->mode = mode;

	switch (mode) {
	case SUPPRESSOR_MODE_SPECSUB:
		r = specsub_config(&(sup->specsub), sr, size);
		break;
	case SUPPRESSOR_MODE_WIENER:
		r = wiener_config(&(sup->wiener), sr, size);
		break;
	case SUPPRESSOR_MODE_TSNR:
	case SUPPRESSOR_MODE_TSNR_G:
	case SUPPRESSOR_MODE_RTSNR:
	case SUPPRESSOR_MODE_RTSNR_G:
		r = tsnr_config(&(sup->tsnr), sr, size, mode);
		break;
	default:
		return SUPPRESSOR_CODE_INVALID_MODE;
	}

	if (r != SUPPRESSOR_CODE_OK) {
		goto err0;
	}

	sup->speech_amp_spec = (hsv_numeric_t*) calloc(size, sizeof(hsv_numeric_t));
	if (sup->speech_amp_spec == NULL) {
		r = SUPPRESSOR_CODE_ALLOC_ERR;
		goto err1;
	}

	return SUPPRESSOR_CODE_OK;

 err1:
	switch (sup->mode) {
	case SUPPRESSOR_MODE_SPECSUB:
		specsub_deconfig(&(sup->specsub));
		break;
	case SUPPRESSOR_MODE_WIENER:
		wiener_deconfig(&(sup->wiener));
		break;
	case SUPPRESSOR_MODE_TSNR:
	case SUPPRESSOR_MODE_TSNR_G:
	case SUPPRESSOR_MODE_RTSNR:
	case SUPPRESSOR_MODE_RTSNR_G:
		tsnr_deconfig(&(sup->tsnr));
		break;
	}
 err0:
	return r;
}

static hsv_numeric_t specsub_calculate_SNR_post(const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec, unsigned size)
{
	unsigned k;

	hsv_numeric_t noisy_speech_power = 0.0;
	hsv_numeric_t noise_power = 0.0;

	for (k = 0; k < size; k++) {
		noisy_speech_power += noisy_speech_amp_spec[k] * noisy_speech_amp_spec[k];
		noise_power += noise_amp_spec[k] * noise_amp_spec[k];
	}

	return 10.0 * HSV_LOG10(noisy_speech_power / noise_power);
}

static hsv_numeric_t specsub_calculate_alpha(hsv_numeric_t snr_post)
{
	static const hsv_numeric_t min = -5.0;
	static const hsv_numeric_t max = 20.0;
	static const hsv_numeric_t alpha0 = 4.0;

	hsv_numeric_t alpha;

	if ((snr_post >= min) && (snr_post <= max)) {
		alpha = alpha0 - snr_post * 3.0 / ((hsv_numeric_t) max);
	} else if (snr_post < min) {
		alpha = 5.0;
	} else /* if (snr_post > max) */ { /* Чтобы GCC не кидал Warning'и при компиляции. */
		alpha = 1.0;
	}

	return alpha;
}

static hsv_numeric_t specsub_calculate_beta(hsv_numeric_t snr_post)
{
	hsv_numeric_t beta;

	if (snr_post > 0.0) {
		beta = 0.01;
	} else if (snr_post < -5.0) {
		beta = 0.04;
	} else {
		beta = 0.02;
	}

	return beta;
}

static void specsub_run(struct SUPPRESSOR_SPECSUB*specsub, const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec, hsv_numeric_t*out)
{
	unsigned k;

	/* Апостериорный SNR. */
	unsigned SNR_post = specsub_calculate_SNR_post(noisy_speech_amp_spec, noise_amp_spec, specsub->size);
	/* alpha является основным параметром вычитания. */
	hsv_numeric_t alpha = specsub_calculate_alpha(SNR_post);
	/* beta маскиррует "музыкальный шум" с помощью остаточного шума. */
	hsv_numeric_t beta = specsub_calculate_beta(SNR_post);

	for (k = 0; k < specsub->size; k++) {
		hsv_numeric_t tmp;
		if (HSV_POW(noisy_speech_amp_spec[k], specsub->power_exponent) > (alpha + beta) * HSV_POW(noise_amp_spec[k], specsub->power_exponent)) {
			/* Вычитание. */
			tmp = HSV_POW(noisy_speech_amp_spec[k], specsub->power_exponent) - alpha * HSV_POW(noise_amp_spec[k], specsub->power_exponent);
		} else {
			/* Маскировка "музыкального" шума остаточным шумом. */
			tmp = beta * HSV_POW(noise_amp_spec[k], specsub->power_exponent);
		}

		out[k] = HSV_POW(tmp, 1.0 / specsub->power_exponent);
	}
}

static void wiener_run(struct SUPPRESSOR_WIENER*wiener, const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec, hsv_numeric_t*out)
{
	unsigned i;

	/* Вычисление спектра мощности шума. */
	for (i = 0; i < wiener->size; i++) {
		wiener->noise_power_spec[i] = noise_amp_spec[i] * noise_amp_spec[i];
	}
	/* Вычисление мгновенного SNR по Скалару-Филхо: мгновенный SNR_inst = апостериорный SNR - 1.
	   Минимальное значение используется для уменьшения искажения сигнала. */
	for (i = 0; i < wiener->size; i++) {
		hsv_numeric_t noisy_speech_power_spec = noisy_speech_amp_spec[i] * noisy_speech_amp_spec[i];
		hsv_numeric_t SNR_post = noisy_speech_power_spec / wiener->noise_power_spec[i];
		wiener->SNR_inst[i] = HSV_MAX(SNR_post - 1.0, wiener->floor);
	}
	/* Вычисление априорного SNR по методу принятия решений Эфраима-Малаха. */
	for (i = 0; i < wiener->size; i++) {
		wiener->SNR_prio_dd[i] = wiener->beta * ((wiener->speech_amp_spec_prev[i] * wiener->speech_amp_spec_prev[i]) / wiener->noise_power_spec[i]) +
			(1.0 - wiener->beta) * wiener->SNR_inst[i];
	}
	/* Вычисление коэффициентов фильтра Винера. */
	for (i = 0; i < wiener->size; i++) {
		wiener->G_dd[i] = wiener->SNR_prio_dd[i] / (wiener->SNR_prio_dd[i] + 1.0);
	}
	/* Винеровская фильтрация. */
	for (i = 0; i < wiener->size; i++) {
		wiener->speech_amp_spec[i] = wiener->G_dd[i] * noisy_speech_amp_spec[i];
	}

	memcpy(wiener->speech_amp_spec_prev, wiener->speech_amp_spec, wiener->size * sizeof(hsv_numeric_t));
	memcpy(out, wiener->speech_amp_spec, wiener->size * sizeof(hsv_numeric_t));
}

static void suppressor_gain(struct SUPPRESSOR_GAIN*gain, hsv_numeric_t*G_2_step)
{
	unsigned i;

	hsv_numeric_t mean_gain_before = 0.0;
	hsv_numeric_t mean_gain_after = 0.0;

	/* Считаем среднее значение фильтра до его "усиления". */
	for (i = 0; i < gain->L1; i++) {
		mean_gain_before += G_2_step[i] * G_2_step[i];
	}
	mean_gain_before /= gain->L1;
	/* Переходим из частотной области во временную. */
	for (i = 0; i < gain->L1; i++) {
		gain->dft.real[i] = G_2_step[i]; gain->dft.imag[i] = 0.0;
	}
	dft_run_i_dft(&(gain->dft));
	/* Получаем неограниченный импульсной характеристики фильтра во времени. */
	memcpy(gain->impulse_response_before, gain->dft.real, gain->L1 * sizeof(hsv_numeric_t));
	/* На её базе строим новый ограниченный фильтр по Скалару. */
	for (i = 0; i < gain->L2 / 2; i++) {
		gain->impulse_response_after[i] = gain->impulse_response_before[i] * gain->window[i + gain->L2 / 2];
	}
	for (i = 0; i < gain->L2; i++) {
		gain->impulse_response_after[i + gain->L2 / 2] = 0.0;
	}
	for (i = 0; i < gain->L2 / 2; i++) {
		gain->impulse_response_after[i + gain->L2 / 2 + gain->L2] = gain->impulse_response_before[gain->L2 + gain->L2 / 2 + i] * gain->window[i];
	}
	/* Переходим обратно во временную область. */
	for (i = 0; i < gain->L1; i++) {
		gain->dft.real[i] = gain->impulse_response_after[i];
		gain->dft.imag[i] = 0.0;
	}
	dft_run_dft(&(gain->dft));
	/* Получаем "усиленный" фильтр. */
	calculate_amp_spec(gain->dft.real, gain->dft.imag, G_2_step, gain->L1);
	/* Считаем среднее значение фильтра после его "усиления". */
	for (i = 0; i < gain->L1; i++) {
		mean_gain_after += G_2_step[i] * G_2_step[i];
	}
	mean_gain_after /= gain->L1;
	/* Нормализуем полученный фильтр. */
	for (i = 0; i < gain->L1; i++) {
		G_2_step[i] = G_2_step[i] * HSV_SQRT(mean_gain_before / mean_gain_after);
	}
}

static void tsnr_run(struct SUPPRESSOR_TSNR*tsnr, const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec, hsv_numeric_t*out)
{
	unsigned i;

	/* Вычисление спектра мощности шума. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->wiener.noise_power_spec[i] = noise_amp_spec[i] * noise_amp_spec[i];
	}
	/* Вычисление мгновенного SNR по Скалару-Филхо: мгновенный SNR_inst = апостериорный SNR - 1.
	   Минимальное значение используется для уменьшения искажения сигнала. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		hsv_numeric_t noisy_speech_power_spec = noisy_speech_amp_spec[i] * noisy_speech_amp_spec[i];
		hsv_numeric_t SNR_post = noisy_speech_power_spec / tsnr->wiener.noise_power_spec[i];
		tsnr->wiener.SNR_inst[i] = HSV_MAX(SNR_post - 1.0, tsnr->wiener.floor);
	}
	/* Вычисление априорного SNR по методу принятия решений Эфраима-Малаха. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->wiener.SNR_prio_dd[i] = tsnr->wiener.beta * ((tsnr->wiener.speech_amp_spec_prev[i] * tsnr->wiener.speech_amp_spec_prev[i]) / tsnr->wiener.noise_power_spec[i]) +
			(1.0 - tsnr->wiener.beta) * tsnr->wiener.SNR_inst[i];
	}
	/* Вычисление коэффициентов фильтра Винера. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->wiener.G_dd[i] = tsnr->wiener.SNR_prio_dd[i] / (tsnr->wiener.SNR_prio_dd[i] + 1.0);
	}
	/*  Винеровская фильтрация. Получили результат аналогичный алгоритму Скалара-Филхо 96-го. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		if ((tsnr->mode == SUPPRESSOR_MODE_TSNR) || (tsnr->mode == SUPPRESSOR_MODE_TSNR_G)) {
			tsnr->wiener.speech_amp_spec[i] = tsnr->wiener.G_dd[i] * noisy_speech_amp_spec[i];
		} else {
			tsnr->wiener.speech_amp_spec[i] = (2.0 - tsnr->wiener.G_dd[i]) * tsnr->wiener.G_dd[i] * noisy_speech_amp_spec[i];
		}
	}
	/* Скалар предложил итеративно повторять процедуру 96-го, для борьбы с запаздыванием априорного SNR на 1 фрейм.
	   Эксперименты показали, что 1 дополнительная итерация значительно влияет на качестве шумоочистки.
	   Остальные незначительно. Вычисление априорного SNR очищенного сигнала. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->SNR_prio_2_step[i] = (tsnr->wiener.speech_amp_spec[i] * tsnr->wiener.speech_amp_spec[i]) / tsnr->wiener.noise_power_spec[i];
	}
	/* Вычисление коэффициентов фильтра Винера. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->G_2_step[i] = tsnr->SNR_prio_2_step[i] / (tsnr->SNR_prio_2_step[i] + 1.0);
	}

	if ((tsnr->mode == SUPPRESSOR_MODE_TSNR) || (tsnr->mode == SUPPRESSOR_MODE_RTSNR)) {
		for (i = 0; i < tsnr->wiener.size; i++) {
			tsnr->G_2_step[i] = HSV_MAX(tsnr->G_2_step[i], tsnr->wiener.floor);
		}
	} else {
		/* Дополнительная функция "усиления", предложенная Скаларом-Плапусом. */
		suppressor_gain(&(tsnr->gain), tsnr->G_2_step);		
	}

	/* Применение Винеровского фильтра. */
	for (i = 0; i < tsnr->wiener.size; i++) {
		tsnr->wiener.speech_amp_spec[i] = tsnr->G_2_step[i] * noisy_speech_amp_spec[i];
	}

	memcpy(tsnr->wiener.speech_amp_spec_prev, tsnr->wiener.speech_amp_spec, tsnr->wiener.size * sizeof(hsv_numeric_t));
	memcpy(out, tsnr->wiener.speech_amp_spec, tsnr->wiener.size * sizeof(hsv_numeric_t));
}

void suppressor_run(suppressor_t sup, const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec)
{
	switch (sup->mode) {
	case SUPPRESSOR_MODE_SPECSUB:
		specsub_run(&(sup->specsub), noisy_speech_amp_spec, noise_amp_spec, sup->speech_amp_spec);
		break;
	case SUPPRESSOR_MODE_WIENER:
		wiener_run(&(sup->wiener), noisy_speech_amp_spec, noise_amp_spec, sup->speech_amp_spec);
		break;
	case SUPPRESSOR_MODE_TSNR:
	case SUPPRESSOR_MODE_TSNR_G:
	case SUPPRESSOR_MODE_RTSNR:
	case SUPPRESSOR_MODE_RTSNR_G:
		tsnr_run(&(sup->tsnr), noisy_speech_amp_spec, noise_amp_spec, sup->speech_amp_spec);
		break;
	}
}

void suppressor_deconfig(suppressor_t sup)
{
	free(sup->speech_amp_spec);

	switch (sup->mode) {
	case SUPPRESSOR_MODE_SPECSUB:
		specsub_deconfig(&(sup->specsub));
		break;
	case SUPPRESSOR_MODE_WIENER:
		wiener_deconfig(&(sup->wiener));
		break;
	case SUPPRESSOR_MODE_TSNR:
	case SUPPRESSOR_MODE_TSNR_G:
	case SUPPRESSOR_MODE_RTSNR:
	case SUPPRESSOR_MODE_RTSNR_G:
		tsnr_deconfig(&(sup->tsnr));
		break;
	}
}

void suppressor_clean(suppressor_t sup)
{
	memset(sup, '\0', sizeof(*sup));
}

void suppressor_free(suppressor_t sup)
{
	free(sup);
}
/**
 * /}
 */
