/**
 * \file hsv_priv.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Внутреннее API алгоритма шумоочистки "Hubbub Suppression for Voice" ("HSV")
 */
/**
 * \ingroup hsv
 * \{
 */
#ifndef HSV_PRIV_H_INCLUDED
#define HSV_PRIV_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "hsv.h"

#include "hsv_types.h"

#include "rb.h"
#include "utils.h"
#include "dft.h"
#include "estimator.h"
#include "suppressor.h"

/**
 * Структура одного канала звука.
 */
struct HSV_CHAN
{
	struct DISCRETE_FOURIER_TRANSFORM dft; /**< Структура ДПФ. */

	hsv_numeric_t*amp_spec;   /**< Текущий спектр амплитуд. */
	hsv_numeric_t*power_spec; /**< Текущий спектр мощности. */
	hsv_numeric_t*phase_spec; /**< Текущий спектр фаз.      */

	struct ESTIMATOR est; /**< Структура оценки шума. */

	struct SUPPRESSOR sup; /**< Структура подавления шума. */

	/**
	 * Буфер перекрытия для хранения данных,
	 * полученных при обработке предыдущих фреймов.
	 */
	hsv_numeric_t*overlap_buf;
};


/**
 * Структура контекста \"HSV\"
 */
struct HSV_CONTEXT
{
	struct HSV_CONFIG conf; /**< Параметры конфигурации. */

	struct RING_BUFFER rb; /**< Кольцевой буфер. */

	unsigned frame_size_smpls;   /**< Размер обрабатываемого фрейма в сэмплах. */
	unsigned overlap_size_smpls; /**< Размер перекрытия фреймов в сэмплах.     */
	unsigned step_size_smpls;    /**< Размер шага фрейма в сэмплах.            */

	unsigned frame_size_bs;   /**< Размер обрабатываемого фрейма в байтах с учетом числа каналов. */
	unsigned overlap_size_bs; /**< Размер перекрытия фреймов в байтах с учетом числа каналов.     */
	unsigned step_size_bs;    /**< Размер шага фрейма в байтах с учетом числа каналов.            */

	hsv_numeric_t norm_factor; /**< Фактор нормализации данных при перекрытии. */

	unsigned dft_size_smpls; /**< Размер ДПФ. */

	hsv_numeric_t*window; /**< Оконная функция. */

	struct HSV_CHAN chans[HSV_MAX_CHANS]; /**< Обрабатываемые каналы звука. */

	unsigned idx_frame;     /**< Индекс начала фрейма в кольцевом буфере.            */
	unsigned pending_bytes; /**< Число байт в кольцевом буфере, ожидающих обработки. */
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HSV_PRIV_H_INCLUDED */
/**
 * /}
 */
