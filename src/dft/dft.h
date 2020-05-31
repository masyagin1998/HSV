/**
 * \file dft.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief API алгоритма дискретного преобразования Фурье.
 */
/**
 * \defgroup dft Модуль дискретного преобразования Фурье.
 * \{
 */
#ifndef DFT_H_INCLUDED
#define DFT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "hsv_types.h"

/**
 * Коды, возвращаемые методами dft_...
 */
enum DFT_CODE
{
	DFT_CODE_OK = 0,         /**< Метод успешно отработал. */
	DFT_CODE_ALLOC_ERR = -1, /**< Ошибка выделения памяти. */
};

/**
 * Таблицы sin и cos алгоритма Кули-Тьюки.
 *
 * Cooley J. W., Tukey J. W. An algorithm for the machine calculation of complex Fourier series, 1965 г.
 */
struct COOLEY_TUKEY
{
	hsv_numeric_t*sin_tab;
	hsv_numeric_t*cos_tab;
	unsigned tab_size;

	int initialized;
};

/**
 * Таблицы sin, cos и предрасчитанная часть свертки алгоритма Блюштейна.
 *
 * Bluestein L. A linear filtering approach to the computation of discrete Fourier transform, 1970 г.
 */
struct BLUESTEIN
{
	hsv_numeric_t*sin_tab;
	hsv_numeric_t*cos_tab;
	unsigned tab_size;	

	hsv_numeric_t*a_real;
	hsv_numeric_t*a_imag;

	hsv_numeric_t*b_real;
	hsv_numeric_t*b_imag;

	hsv_numeric_t*c_real;
	hsv_numeric_t*c_imag;

	unsigned nb;

	int initialized;
};

/**
 * Структура ДПФ.
 * При dft_size - степени двойки используется только алгоритм Кули-Тьюки, иначе оба алгоритма.
 */
struct DISCRETE_FOURIER_TRANSFORM
{
	hsv_numeric_t*real; /**< Действительная часть. */
	hsv_numeric_t*imag; /**< Мнимая часть.         */
	unsigned dft_size;  /**< Размер ДПФ.           */

	struct COOLEY_TUKEY ct; /**< Вспомогательная структура алгоритма Кули-Тьюки, ускоряющая его работу. */
	struct BLUESTEIN bl;    /**< Вспомогательная структура алгоритма Блюштейна, ускоряющая его работу.  */
};

typedef struct DISCRETE_FOURIER_TRANSFORM* dft_t;

/**
 * Создание структуры ДПФ.
 * \return указатель на структуру ДПФ (при ошибке - NULL).
 */
dft_t create_dft();

/**
 * Конфигурация ДПФ.
 * \param dft_size размер ДПФ.
 * \return результат конфигурирования.
 */
enum DFT_CODE dft_config(dft_t dft, unsigned dft_size);

/**
 * Выполнение прямого ДПФ над массивами real и imag.
 */
void dft_run_dft(dft_t dft);

/**
 * Выполнение обратного ДПФ над массивами real и imag.
 */
void dft_run_i_dft(dft_t dft);

/**
 * Удаление всех внутренних динамических структур.
 */
void dft_deconfig(dft_t dft);

/**
 * Зануление структуры ДПФ.
 */
void dft_clean(dft_t dft);

/**
 * Удаление структуры ДПФ.
 */
void dft_free(dft_t dft);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* DFT_H_INCLUDED */
/**
 * /}
 */
