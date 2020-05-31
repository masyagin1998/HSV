/**
 * \file suppressor.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief API алгоритм шумоподавления.
 */
/**
 * \defgroup suppressor Модуль подавления шума.
 * \{
 */
#ifndef AVFILTER_HSV_SUPPRESSOR_SUPPRESSOR_H
#define AVFILTER_HSV_SUPPRESSOR_SUPPRESSOR_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Относительные импорты, т.к. FFmpeg не поддерживает абсолютные. */
#include "../hsv_types/hsv_types.h"

#include "../dft/dft.h"
#include "../utils/utils.h"

/**
 * Коды, возвращаемые методами suppressor_...
 */
enum SUPPRESSOR_CODE
{
	SUPPRESSOR_CODE_OK           =  0, /**< Метод успешно отработал. */
	SUPPRESSOR_CODE_INVALID_MODE = -1, /**< Неизвестный режим.       */
	SUPPRESSOR_CODE_ALLOC_ERR    = -2, /**< Ошибка выделения памяти. */
};

/**
 * Режим работы алгоритма шумоподавления.
 */
enum SUPPRESSOR_MODE
{
	SUPPRESSOR_MODE_SPECSUB, /**< Режим, основанный на спектральном вычитании Берути. */

	SUPPRESSOR_MODE_WIENER,  /**< Режим, основанный на винеровской фильтрации Скалара. */

	SUPPRESSOR_MODE_TSNR,    /**< Режим, основанный на двухшаговой фильтрации Скалара.               */
	SUPPRESSOR_MODE_TSNR_G,  /**< Режим, основанный на двухшаговой фильтрации Скалара с "усилением". */

	SUPPRESSOR_MODE_RTSNR,   /**< Режим, основанный на двухшаговой фильтрации Шифенга.               */
	SUPPRESSOR_MODE_RTSNR_G, /**< Режим, основанный на двухшаговой фильтрации Шифенга с "усилением". */
};

/**
 * Структура спектрального вычитания.
 * Вычитание базируется на алгоритмах Берути-Шварца.
 *
 * Berouti M., Schwartz M., Makhoul J. Enhancement of speech corrupted by acoustic noise,
 * Proceedings of IEEE International Conference on Acoustic Speech Signal Processing, 1979 г.
 */
struct SUPPRESSOR_SPECSUB
{
	unsigned sr;   /**< Частота дискретизации. */
	unsigned size; /**< Размер фрейма.         */
	/**
	 * Степень, в которую возводятся спектры амплитуд и шума при вычитании.
	 * 1.0 - вычитание спектров амплитуд по Боллу.
	 * 2.0 - вычитание спектров мощности по Берути.
	 */
	hsv_numeric_t power_exponent;
};

/**
 * Структура винеровской фильтрации.
 * Фильтрация базируется на алгоритме Скалара-Филхо.
 *
 * Ephraim Y., Malah D., Speech Enhancement Using a Minimum Mean-Square Error
 * Short-Time Spectral Amplitude Estimator, 1984 г.
 *
 * Scalart P., Filho J. V. Speech enhancement based on a priori signal to noise estimation,
 * Acoustics, Speech, and Signal Processing, 1996 г.
 */
struct SUPPRESSOR_WIENER
{
	unsigned sr;   /**< Частота дискретизации. */
	unsigned size; /**< Размер фрейма.         */

	hsv_numeric_t beta;  /**< Коэффициент метода принятия решений.                  */
	hsv_numeric_t floor; /**< Коэффициент сглаживания для предотвращения искажений. */

	hsv_numeric_t*noise_power_spec; /**< Спектр мощности шума. */

	hsv_numeric_t*SNR_inst;    /**< Мгновенный SNR.                                     */
	hsv_numeric_t*SNR_prio_dd; /**< Априорный SNR, полученный методом принятия решений. */
	hsv_numeric_t*G_dd;        /**< Фильтр винера.                                      */

	hsv_numeric_t*speech_amp_spec;      /**< Текущий спектр амплитуд голоса. */
	hsv_numeric_t*speech_amp_spec_prev; /**< Прошлый спектр амплитуд голоса. */
};

/**
 * Структура вычисления фильтра "усиления".
 */
struct SUPPRESSOR_GAIN
{
	unsigned L1; /**< Размер фильтра. */
	unsigned L2; /**< Размер окна.    */

	struct DISCRETE_FOURIER_TRANSFORM dft; /**< Структура ДПФ. */

	hsv_numeric_t*window; /**< Оконная функция. */

	hsv_numeric_t*impulse_response_before; /**< Импульсный отклик до усиления.    */
	hsv_numeric_t*impulse_response_after;  /**< Импульсный отклик после усиления. */
};

/**
 * Структура двухшаговой фильтрации.
 * Фильтрация базируется на алгоритмах Скалара и Шифенга.
 *
 * Scalart P., Plapous C. A two-step noise reduction technique,
 * Acoustics, Speech, and Signal Processing, 2004 г.
 *
 * Shifeng Ou., Chao G., Ying G. Improved a Priori SNR Estimation for Speech Enhancement Incorporating Speech Distortion Component,
 * Institute of Science and Technology for Opto-electronic Information, 2013 г.
 */
struct SUPPRESSOR_TSNR
{
	enum SUPPRESSOR_MODE mode; /**< Режим работа алгоритма шумоподавления. */
	
	struct SUPPRESSOR_WIENER wiener; /**< Структура винеровской фильтрации Скалара. */

	hsv_numeric_t*SNR_prio_2_step; /**< Априорный SNR, полученный двухшаговым методом. */
	hsv_numeric_t*G_2_step;        /**< Фильтр винера.                                 */

	struct SUPPRESSOR_GAIN gain; /**< Улучшенный фильтр усилениея. */
};

/**
 * Структура шумоподавления.
 */
struct SUPPRESSOR
{
	enum SUPPRESSOR_MODE mode; /**< Режим работа алгоритма шумоподавления. */

	unsigned sr;   /**< Частота дискретизации. */
	unsigned size; /**< Размер фрейма.         */

	union {
		struct SUPPRESSOR_SPECSUB specsub; /**< Структура алгоритма спектрального вычитания Берути-Шварца.           */
		struct SUPPRESSOR_WIENER  wiener;  /**< Структура алгоритма винеровской фильтрации Скалара.                  */
		struct SUPPRESSOR_TSNR    tsnr;    /**< Общая структура алгоритмов двухшаговой фильтрации Скалара и Шифенга. */
	};

	hsv_numeric_t*speech_amp_spec; /**< Спектр амплитуд очищенного голоса. */
};

typedef struct SUPPRESSOR* suppressor_t;

/**
 * Создание структуры подавления шума.
 * \return указатель на структуру подавления шума (при ошибке - NULL).
 */
suppressor_t create_suppressor();

/**
 * Конфигурация подавления шума.
 * \param sr частота дискретизации.
 * \param size размер анализируемого фрейма.
 * \return результат конфигурирования.
 */
enum SUPPRESSOR_CODE suppressor_config(suppressor_t sup, unsigned sr, unsigned size, enum SUPPRESSOR_MODE mode);

/**
 * Выполнение подавления шума.
 */
void suppressor_run(suppressor_t sup, const hsv_numeric_t*noisy_speech_amp_spec, const hsv_numeric_t*noise_amp_spec);

/**
 * Удаление всех внутренних динамических структур.
 */
void suppressor_deconfig(suppressor_t sup);

/**
 * Зануление структуры оценки шума.
 */
void suppressor_clean(suppressor_t sup);

/**
 * Удаление структуры оценки шума.
 */
void suppressor_free(suppressor_t sup);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* AVFILTER_HSV_SUPPRESSOR_SUPPRESSOR_H */
/**
 * /}
 */
