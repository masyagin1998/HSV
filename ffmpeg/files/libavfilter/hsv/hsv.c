/**
 * \file hsv.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API алгоритма шумоочистки "Hubbub Suppression for Voice" ("HSV")
 */
/**
 * \ingroup hsv
 * \{
 */

/* Относительные импорты, т.к. FFmpeg не поддерживает абсолютные. */
#include "../hsv_interface.h"
#include "hsv_priv.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <inttypes.h>

hsvc_t create_hsvc()
{
	hsvc_t hsvc;

	hsvc = (hsvc_t) calloc(1, sizeof(struct HSV_CONTEXT));
	return hsvc;
}

int hsvc_validate_config(const struct HSV_CONFIG*conf)
{
	struct HSV_CONFIG tmp = *conf;
	
	if (tmp.sr == 0) {
		return 1;
	} else if (tmp.ch == 0) {
		return 2;
	} else if (tmp.bs != HSV_SUPPORTED_BS) {
		return 3;
	}

	if ((tmp.mode < HSV_SUPPRESSOR_MODE_SPECSUB) || (tmp.mode > HSV_SUPPRESSOR_MODE_RTSNR_G)) {
		return 4;
	}

	if (tmp.frame_size_smpls == HSV_DEFAULT) {
		tmp.frame_size_smpls = (unsigned) HSV_FLOOR(2.0 * tmp.sr / 100.0);
	}

	if (tmp.overlap_perc == HSV_DEFAULT) {
		tmp.overlap_perc = HSV_DEFAULT_OVERLAP_PERC;
	} else if (tmp.overlap_perc >= 100) {
		return 6;
	}

	if (tmp.dft_size_smpls == HSV_DEFAULT) {
		tmp.dft_size_smpls = 2 * tmp.frame_size_smpls;
	} else if (tmp.dft_size_smpls < tmp.frame_size_smpls) {
		return 7;
	}

	if (tmp.cap != HSV_DEFAULT) {
		if ((tmp.cap % 2 != 0) || (tmp.cap < tmp.frame_size_smpls)) {
			return 8;
		}
	}

	return HSV_CODE_OK;
}

static enum HSV_CODE switch_rb_code(enum RB_CODE r)
{
	switch (r) {
	case RB_CODE_OK:
		return HSV_CODE_OK;
	case RB_CODE_ALLOC_ERR:
		return HSV_CODE_ALLOC_ERR;
	case RB_CODE_OVERFLOW_ERR:
		return HSV_CODE_OVERFLOW_ERR;
	default:
		return HSV_CODE_UNKNOWN_ERR;
	}
}

static enum HSV_CODE switch_dft_code(enum RB_CODE r)
{
	switch (r) {
	case DFT_CODE_OK:
		return HSV_CODE_OK;
	case DFT_CODE_ALLOC_ERR:
		return HSV_CODE_ALLOC_ERR;
	default:
		return HSV_CODE_UNKNOWN_ERR;
	}
}

static enum HSV_CODE switch_estimator_code(enum ESTIMATOR_CODE r)
{
	switch (r) {
	case ESTIMATOR_CODE_OK:
		return HSV_CODE_OK;
	case ESTIMATOR_CODE_ALLOC_ERR:
		return HSV_CODE_ALLOC_ERR;
	default:
		return HSV_CODE_UNKNOWN_ERR;
	}
}

static enum HSV_CODE switch_suppressor_code(enum SUPPRESSOR_CODE r)
{
	switch (r) {
	case SUPPRESSOR_CODE_OK:
		return HSV_CODE_OK;
	case SUPPRESSOR_CODE_ALLOC_ERR:
		return HSV_CODE_ALLOC_ERR;
	default:
		return HSV_CODE_UNKNOWN_ERR;
	}
}

static enum HSV_CODE hsvc_config_chan(struct HSV_CHAN*chan, unsigned sr, unsigned dft_size_smpls, enum HSV_SUPPRESSOR_MODE mode)
{
	enum HSV_CODE r;

	enum DFT_CODE dft_r;
	enum ESTIMATOR_CODE est_r;
	enum SUPPRESSOR_CODE sup_r;

	dft_r = dft_config(&(chan->dft), dft_size_smpls);
	if (dft_r != DFT_CODE_OK) {
		r = switch_dft_code(dft_r);
		goto err0;
	}
	
	chan->amp_spec = (hsv_numeric_t*) calloc(dft_size_smpls, sizeof(hsv_numeric_t));
	if (chan->amp_spec == NULL) {
		r = HSV_CODE_ALLOC_ERR;
		goto err1;
	}
	chan->power_spec = (hsv_numeric_t*) calloc(dft_size_smpls, sizeof(hsv_numeric_t));
	if (chan->power_spec == NULL) {
		r = HSV_CODE_ALLOC_ERR;
		goto err2;
	}
	chan->phase_spec = (hsv_numeric_t*) calloc(dft_size_smpls, sizeof(hsv_numeric_t));
	if (chan->phase_spec == NULL) {
		r = HSV_CODE_ALLOC_ERR;
		goto err3;
	}
	
	est_r = estimator_config(&(chan->est), sr, dft_size_smpls);
	if (est_r != ESTIMATOR_CODE_OK) {
		r = switch_estimator_code(est_r);
		goto err4;
	}

	sup_r = suppressor_config(&(chan->sup), sr, dft_size_smpls, mode);
	if (sup_r != SUPPRESSOR_CODE_OK) {
		r = switch_suppressor_code(sup_r);
		goto err5;
	}
	
	chan->overlap_buf = (hsv_numeric_t*) calloc(dft_size_smpls, sizeof(hsv_numeric_t));
	if (chan->overlap_buf == NULL) {
		r = HSV_CODE_ALLOC_ERR;
		goto err6;
	}

	return HSV_CODE_OK;

 err6:
	suppressor_deconfig(&(chan->sup));
 err5:
	estimator_deconfig(&(chan->est));
 err4:
	free(chan->phase_spec);
 err3:
	free(chan->power_spec);
 err2:
	free(chan->amp_spec);
 err1:
	dft_deconfig(&(chan->dft));
 err0:
	return r;
}

static void hsvc_deconfig_chan(struct HSV_CHAN*chan)
{
	free(chan->overlap_buf);

	suppressor_deconfig(&(chan->sup));
	estimator_deconfig(&(chan->est));

	free(chan->phase_spec);
	free(chan->power_spec);
	free(chan->amp_spec);

	dft_deconfig(&(chan->dft));
}

enum HSV_CODE hsvc_config(hsvc_t hsvc, const struct HSV_CONFIG*conf)
{
	enum HSV_CODE r;

	enum RB_CODE rb_r;

	unsigned ch, k;

	hsvc->conf = *conf;

	if (hsvc->conf.cap == HSV_DEFAULT) {
		hsvc->conf.cap = HSV_DEFAULT_CAP;
	}
	rb_r = rb_config(&(hsvc->rb), hsvc->conf.cap);
	if (rb_r != RB_CODE_OK) {
		r = switch_rb_code(rb_r);
		goto err0;
	}

	if (hsvc->conf.frame_size_smpls == HSV_DEFAULT) {
		hsvc->conf.frame_size_smpls = (unsigned) HSV_FLOOR(2.0 * hsvc->conf.sr / 100.0);
	}
	hsvc->frame_size_smpls = hsvc->conf.frame_size_smpls;
	if (hsvc->frame_size_smpls % 2 == 1) {
		hsvc->frame_size_smpls++;
	}
	if (hsvc->conf.overlap_perc == HSV_DEFAULT) {
		hsvc->conf.overlap_perc = HSV_DEFAULT_OVERLAP_PERC;
	}
	hsvc->overlap_size_smpls = (unsigned) HSV_FLOOR(hsvc->frame_size_smpls * hsvc->conf.overlap_perc / 100.0);
	hsvc->step_size_smpls = hsvc->frame_size_smpls - hsvc->overlap_size_smpls;

	hsvc->frame_size_bs = hsvc->frame_size_smpls * 2 * hsvc->conf.ch;
	hsvc->overlap_size_bs = hsvc->overlap_size_smpls * 2 * hsvc->conf.ch;
	hsvc->step_size_bs = hsvc->step_size_smpls * 2 * hsvc->conf.ch;

	hsvc->norm_factor = 1.0 / ((100.0 - hsvc->conf.overlap_perc) / 100.0);

	if (hsvc->conf.dft_size_smpls == HSV_DEFAULT) {
		hsvc->conf.dft_size_smpls = hsvc->frame_size_smpls * 2;
	}
	hsvc->dft_size_smpls = hsvc->conf.dft_size_smpls;

	hsvc->window = (hsv_numeric_t*) calloc(hsvc->frame_size_smpls, sizeof(hsv_numeric_t));
	if (hsvc->window == NULL) {
		r = HSV_CODE_ALLOC_ERR;
		goto err1;
	}
	init_window(hsvc->window, hsvc->frame_size_smpls, WINDOW_TYPE_HANNING);

	for (ch = 0; ch < hsvc->conf.ch; ch++) {
		r = hsvc_config_chan(hsvc->chans + ch, hsvc->conf.sr, hsvc->dft_size_smpls, hsvc->conf.mode);
		if (r != HSV_CODE_OK) {
			goto err2;
		}
	}

	return HSV_CODE_OK;

 err2:
	for (k = 0; k < ch; k++) {
		hsvc_deconfig_chan(hsvc->chans + ch);
	}
 err1:
	rb_deconfig(&(hsvc->rb));
 err0:
	return r;
}

static hsv_numeric_t int16_to_hsv_numeric_t(int16_t v)
{
	static const hsv_numeric_t one = 1.0;
    static const hsv_numeric_t max_int16_inv = one / INT16_MAX;
    static const hsv_numeric_t min_int16_inv = one / INT16_MIN;

    return v * ((v > 0) ? max_int16_inv : -min_int16_inv);
}

static int16_t hsv_numeric_t_to_int16(hsv_numeric_t v)
{
	static const hsv_numeric_t half = 0.5;
	
    if (v > 0) {
        return (v >= 1) ? INT16_MAX : ((int16_t) (v * INT16_MAX + half));
    } else {
        return (v <= -1) ? INT16_MIN : ((int16_t) (-v * INT16_MIN - half));
    }
}

static int hsvc_denoise(hsvc_t hsvc)
{
	unsigned ch, k;

	unsigned processed = 0;

	/* До тех пор пока кол-во байт, ожидающих обработку,
	   превышает размер одного фрейма, будем их обрабатывать. */
	while (hsvc->pending_bytes >= hsvc->frame_size_bs) {
		/* Структура многоканального WAV-файла подразумевает, что данные каналов лежат через один:
		   если есть 2 канала A и B, то данные лежат как ABABAB... . Поэтому обрабатываем каналы по очереди. */
        for (ch = 0; ch < hsvc->conf.ch; ch++) {
			/* Считываем очередной фрейм одного канала из кольцевого буфера в буфер обработки. */
			memset(hsvc->chans[ch].dft.real, '\0', sizeof(hsv_numeric_t) * hsvc->chans[ch].dft.dft_size);
			memset(hsvc->chans[ch].dft.imag, '\0', sizeof(hsv_numeric_t) * hsvc->chans[ch].dft.dft_size);
            for (k = 0; k < hsvc->frame_size_smpls; k++) {
				hsvc->chans[ch].dft.real[k] =
					int16_to_hsv_numeric_t(((int16_t*)hsvc->rb.data)[((hsvc->idx_frame / 2) + k * hsvc->conf.ch + ch) % (rb_cap(&(hsvc->rb)) / 2)]);
            }

			/* Применяем оконную функцию, для уменьшения эффекта растекания. */
			calculate_windowing(hsvc->window, hsvc->chans[ch].dft.real, hsvc->chans[ch].dft.real, hsvc->frame_size_smpls);
			
			dft_run_dft(&(hsvc->chans[ch].dft));

			calculate_amp_spec(hsvc->chans[ch].dft.real, hsvc->chans[ch].dft.imag,
							   hsvc->chans[ch].amp_spec, hsvc->chans[ch].dft.dft_size);
			calculate_power_spec(hsvc->chans[ch].dft.real, hsvc->chans[ch].dft.imag,
								 hsvc->chans[ch].power_spec, hsvc->chans[ch].dft.dft_size);
			calculate_phase_spec(hsvc->chans[ch].dft.real, hsvc->chans[ch].dft.imag,
								 hsvc->chans[ch].phase_spec, hsvc->chans[ch].dft.dft_size);

			estimator_run(&(hsvc->chans[ch].est), hsvc->chans[ch].power_spec);

			suppressor_run(&(hsvc->chans[ch].sup), hsvc->chans[ch].amp_spec, hsvc->chans[ch].est.noise_amp_spec);

			/* Будем считать, что спектры фаз голоса, шума и зашумленного голоса совпадают. */
			for (k = 0; k < hsvc->chans[ch].dft.dft_size; k++) {
				hsvc->chans[ch].dft.real[k] = hsvc->chans[ch].sup.speech_amp_spec[k] * HSV_COS(hsvc->chans[ch].phase_spec[k]);
				hsvc->chans[ch].dft.imag[k] = hsvc->chans[ch].sup.speech_amp_spec[k] * HSV_SIN(hsvc->chans[ch].phase_spec[k]);
			}

			dft_run_i_dft(&(hsvc->chans[ch].dft));

			/* Записываем результаты обработки очередного фрейма одного канала из буфера обработки обратно в кольцевой буфер с учетом перекрытия. */
			for (k = 0; k < hsvc->step_size_smpls; k++) {
				hsvc->pending_bytes -= 2; processed += 2;
                ((int16_t*)hsvc->rb.data)[((hsvc->idx_frame / 2) + k * hsvc->conf.ch + ch) % (rb_cap(&(hsvc->rb)) / 2)] =
					hsv_numeric_t_to_int16(hsvc->chans[ch].dft.real[k] / hsvc->norm_factor + hsvc->chans[ch].overlap_buf[k]);
			}

			/* Так как для уменьшения эффекта блочности используется перекрытие, сохраним данные, полученные при обработке n-го фрейма
			   для их использования при обработке n+1-го, n+2-го и т.д. фреймов. */
			for (k = 0; k < hsvc->chans[ch].dft.dft_size; k++) {
				hsvc->chans[ch].overlap_buf[k] += hsvc->chans[ch].dft.real[k] / hsvc->norm_factor;
			}
			memmove(hsvc->chans[ch].overlap_buf, hsvc->chans[ch].overlap_buf + hsvc->step_size_smpls, (hsvc->chans[ch].dft.dft_size - hsvc->step_size_smpls) * sizeof(hsv_numeric_t));
			memset(hsvc->chans[ch].overlap_buf + (hsvc->chans[ch].dft.dft_size - hsvc->step_size_smpls), '\0', hsvc->step_size_smpls * sizeof(hsv_numeric_t));
		}
		/* Учтём, что часть фрейма может лежать "в конце" кольцевого буфера, а часть "в начале". */
		hsvc->idx_frame = (hsvc->idx_frame + hsvc->step_size_bs) % rb_cap(&(hsvc->rb));
	}

	return processed;
}

int hsvc_push(hsvc_t hsvc, const char*data, unsigned data_len)
{
	enum HSV_CODE r;

	enum RB_CODE rb_r;

	rb_r = rb_push(&(hsvc->rb), data, data_len);
	if (rb_r != RB_CODE_OK) {
		r = switch_rb_code(rb_r);
		goto err0;
	}

	hsvc->pending_bytes += data_len;

	return hsvc_denoise(hsvc);
 err0:
	return r;
}

unsigned hsvc_get(hsvc_t hsvc, char*data, unsigned data_cap)
{
	unsigned data_len = rb_len(&(hsvc->rb)) - hsvc->pending_bytes;
	if (data_cap < data_len) {
		data_len = data_cap;
	}
	return rb_get(&(hsvc->rb), data, data_len);
}

void hsvc_flush(hsvc_t hsvc)
{
    hsvc->idx_frame = rb_idx_in(&(hsvc->rb));
    hsvc->pending_bytes = 0;
}

void hsvc_deconfig(hsvc_t hsvc)
{
	unsigned ch;

	for (ch = 0; ch < hsvc->conf.ch; ch++) {
		hsvc_deconfig_chan(hsvc->chans + ch);
	}

	free(hsvc->window);
	rb_deconfig(&hsvc->rb);
}

void hsvc_clean(hsvc_t hsvc)
{
	memset(hsvc, '\0', sizeof(*hsvc));
}

void hsvc_free(hsvc_t hsvc)
{
	free(hsvc);
}
/**
 * /}
 */
