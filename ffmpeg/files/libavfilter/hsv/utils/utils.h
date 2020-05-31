/**
 * \file utils.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief API вспомогательных алгоритмов
 */
/**
 * \defgroup utils Модуль вспомогательных функций работы со спектром.
 * \{
 */
#ifndef AVFILTER_HSV_UTILS_UTILS_H
#define AVFILTER_HSV_UTILS_UTILS_H

#ifdef __cplusplus
// extern "C" {
#endif  /* __cplusplus */

/* Относительные импорты, т.к. FFmpeg не поддерживает абсолютные. */
#include "../hsv_types/hsv_types.h"

enum WINDOW_TYPE
{
	/**
	 * Оконная функция Хэмминга.
	 * w(n) = 0.53836 - 0.46164 * cos(2 * Pi * n / (N - 1))
	 */
	WINDOW_TYPE_HAMMING,
	/**
	 * Оконная функция Хэннинга (Ханна).
	 * w(n) = 0.5 * (1 - cos(2 * Pi * n / (N - 1)))
	 */
	WINDOW_TYPE_HANNING,
};

/**
 * Инициализация массива оконной функции.
 * \param n размер массива.
 * \param wt тип окна.
 */
void init_window(hsv_numeric_t*window, unsigned n, enum WINDOW_TYPE wt);

/**
 * Перемножение с оконной функцией.
 */
void calculate_windowing(const hsv_numeric_t*window, const hsv_numeric_t*in, hsv_numeric_t*out, unsigned n);

/**
 * Вычисление спектра амплитуд сигнала.
 */
void calculate_amp_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*amp_spec, unsigned n);

/**
 * Вычисление спектра мощности сигнала.
 */
void calculate_power_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*power_spec, unsigned n);

/**
 * Вычисление спектра фаз сигнала.
 */
void calculate_phase_spec(const hsv_numeric_t*real, const hsv_numeric_t*imag, hsv_numeric_t*phase_spec, unsigned n);

#ifdef __cplusplus
// }
#endif  /* __cplusplus */

#endif  /* AVFILTER_HSV_UTILS_UTILS_H */
/**
 * /}
 */
