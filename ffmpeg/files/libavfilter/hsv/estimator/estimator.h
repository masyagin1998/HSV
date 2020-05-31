/**
 * \file estimator.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief API алгоритма оценивания шума.
 */
/**
 * \defgroup estimator Модуль оценивания шума.
 * \{
 */
#ifndef AVFILTER_HSV_ESTIMATOR_ESTIMATOR_H
#define AVFILTER_HSV_ESTIMATOR_ESTIMATOR_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Относительные импорты, т.к. FFmpeg не поддерживает абсолютные. */
#include "../hsv_types/hsv_types.h"

/**
 * Коды, возвращаемые методами estimator_...
 */
enum ESTIMATOR_CODE
{
	ESTIMATOR_CODE_OK = 0,         /**< Метод успешно отработал. */
	ESTIMATOR_CODE_ALLOC_ERR = -1, /**< Ошибка выделения памяти. */
};

/**
 * Структура оценки шума.
 * Оценка шума базируется на алгоритме Лойзю-Рагначари MCRA-2,
 * улучшенной версии алгоритма Кохена-Бердуго MCRA.
 *
 * Cohen I.,  Berdugo B.    А noise estimation by minima controlled recursive averaging for robust speech enhancement, 2002 г.
 * Loizou P., Rangachari S. A noise estimation algorithm for highly nonstationary environments, Speech Communications, 2006 г.
 * Doblinger G.             Computationally efficient speech enhancement by spectral minima tracking in subbands,      1995 г.
 */
struct ESTIMATOR
{
	unsigned sr;   /**< Частота дискретизации. */
	unsigned size; /**< Размер фрейма.         */

	hsv_numeric_t*delta_k; /**< Частотно-зависимые пороги присутствия голоса. */

	hsv_numeric_t alpha_smooth; /**< Коэффициент сглаживания зашумленного сигнала во времени. */
	hsv_numeric_t*P;            /**< Текущая оценка мощности спектра зашумленного сигнала.    */
	hsv_numeric_t*P_prev;       /**< Прошлая оценка мощности спектра зашумленного сигнала.    */

	hsv_numeric_t beta;       /**< Коэффициент прогнозирования.            */
	hsv_numeric_t gamma;      /**< Коэффициент сглаживания.                */
	hsv_numeric_t*P_min;      /**< Текущая оценка шума методом Доблингера. */
	hsv_numeric_t*P_min_prev; /**< Прошлая оценка шума методом Доблингера. */

	hsv_numeric_t alpha_spp; /**< Коэффициент сглаживания вероятности наличия голоса во времени. */
	hsv_numeric_t*spp_k;     /**< Вероятность наличия голоса.                                    */

	hsv_numeric_t alpha; /**< Вспомогательный коэффициент для расчет коэффициента сглажвания шума в частотно-временной области. */

	hsv_numeric_t*noise_power_spec; /**< Спектр мощности шума. */
	hsv_numeric_t*noise_amp_spec;   /**< Спектр амплитуд шума. */

	int got_first; /**< Был ли получен первый фрейм. */
};

typedef struct ESTIMATOR* estimator_t;

/**
 * Создание структуры оценки шума.
 * \return указатель на структуру оценки шума (при ошибке - NULL).
 */
estimator_t create_estimator();

/**
 * Конфигурация оценки шума.
 * \param sr частота дискретизации.
 * \param size размер анализируемого фрейма.
 * \return результат конфигурирования.
 */
enum ESTIMATOR_CODE estimator_config(estimator_t est, unsigned sr, unsigned size);

/**
 * Выполнение оценки шума.
 */
void estimator_run(estimator_t est, hsv_numeric_t*P);

/**
 * Удаление всех внутренних динамических структур.
 */
void estimator_deconfig(estimator_t est);

/**
 * Зануление структуры оценки шума.
 */
void estimator_clean(estimator_t est);

/**
 * Удаление структуры оценки шума.
 */
void estimator_free(estimator_t est);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* AVFILTER_HSV_ESTIMATOR_ESTIMATOR_H */
/**
 * /}
 */
