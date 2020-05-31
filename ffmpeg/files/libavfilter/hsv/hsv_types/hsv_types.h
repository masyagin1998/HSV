/**
 * \file hsv_types.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Alias'ы типов данных и функций с плавающей точкой, используемых для анализа звука.
 */
/**
 * \defgroup hsv_types Модуль типов "HSV".
 * \{
 */
#ifndef AVFILTER_HSV_HSV_TYPES_HSV_TYPES_H
#define AVFILTER_HSV_HSV_TYPES_HSV_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PREFIX_UNUSED(VAR) ((void) VAR) /**< Макрос подавления неиспользуемых переменных. */

/**
 * Выбор режима точности вычислений с плавающей точкой.
 * LOW_ACC  - низкая точность  (float);
 * MED_ACC  - средняя точность (double);
 * HIGH_ACC - высокая точность (long double);
 */
#define LOW_ACC 

#ifdef LOW_ACC

typedef float hsv_numeric_t;

#define HSV_SIN(VAR) sinf(VAR)
#define HSV_COS(VAR) cosf(VAR)
#define HSV_ATAN2(VAR_A, VAR_B) atan2f(VAR_A, VAR_B)
#define HSV_SQRT(VAR) sqrtf(VAR)
#define HSV_POW(VAR_A, VAR_B) powf(VAR_A, VAR_B)
#define HSV_ROUND(VAR) roundf(VAR)
#define HSV_LOG10(VAR) log10f(VAR)
#define HSV_FLOOR(VAR) floorf(VAR)
	
#endif  /* LOW_ACC */

#ifdef MED_ACC

typedef double hsv_numeric_t;

#define HSV_SIN(VAR) sin(VAR)
#define HSV_COS(VAR) cos(VAR)
#define HSV_ATAN2(VAR_A, VAR_B) atan2(VAR_A, VAR_B)
#define HSV_SQRT(VAR) sqrt(VAR)
#define HSV_POW(VAR_A, VAR_B) pow(VAR_A, VAR_B)
#define HSV_ROUND(VAR) round(VAR)
#define HSV_LOG10(VAR) log10(VAR)
#define HSV_FLOOR(VAR) floor(VAR)
	
#endif  /* MED_ACC */

#ifdef HIGH_ACC

typedef long double hsv_numeric_t;

#define HSV_SIN(VAR) sinl(VAR)
#define HSV_COS(VAR) cosl(VAR)
#define HSV_ATAN2(VAR_A, VAR_B) atan2l(VAR_A, VAR_B)
#define HSV_SQRT(VAR) sqrtl(VAR)
#define HSV_POW(VAR_A, VAR_B) powl(VAR_A, VAR_B)
#define HSV_ROUND(VAR) roundl(VAR)
#define HSV_LOG10(VAR) log10l(VAR)
#define HSV_FLOOR(VAR) floorl(VAR)

#endif  /* HIGH_ACC */

#ifndef M_PI
#define M_PI ((hsv_numeric_t) 3.14159265358979323846)
#endif	/* M_PI */

#define HSV_MAX(VAR_A, VAR_B) ((VAR_A) > (VAR_B) ? (VAR_A) : (VAR_B))
#define HSV_MIN(VAR_A, VAR_B) ((VAR_A) < (VAR_B) ? (VAR_A) : (VAR_B))
#define HSV_ABS(VAR) (((VAR) >= 0.0) ? (VAR) : - (VAR))

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* AVFILTER_HSV_HSV_TYPES_HSV_TYPES_H */
/**
 * /}
 */
