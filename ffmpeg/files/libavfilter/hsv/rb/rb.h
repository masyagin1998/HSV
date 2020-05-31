/**
 * \file rb.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief API буферизации.
 */
/**
 * \defgroup rb Модуль буферизации.
 * \{
 */
#ifndef AVFILTER_HSV_RB_RB_H
#define AVFILTER_HSV_RB_RB_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * Коды, возвращаемые методами rb_...
 */
enum RB_CODE
{
	RB_CODE_OK = 0,            /**< Метод успешно отработал.    */
	RB_CODE_ALLOC_ERR    = -1, /**< Ошибка выделения памяти.    */
	RB_CODE_OVERFLOW_ERR = -2, /**< Ошибка переполнения буфера. */
};

/**
 * Структура кольцевого буфера.
 */
struct RING_BUFFER
{
	char*data; /**< Массив бинарных данных. */

	unsigned len; /**< Размер данных в буфере.          */
	unsigned cap; /**< Максимальная вместимость буфера. */

	unsigned idx_in;  /**< Указатель на текущее место записи. */
	unsigned idx_out; /**< Указатель на текущее место чтения. */
};

typedef struct RING_BUFFER* rb_t;

/**
 * Создание структуры кольцевого буфера.
 * \return указатель на структуру кольцевого буфера (при ошибке - NULL).
 */
rb_t create_rb();

/**
 * Конфигурация кольцевого буфера.
 * \param cap вместимость кольцевого буфера.
 * \return результат конфигурирования.
 */
enum RB_CODE rb_config(rb_t rb, unsigned cap);

/**
 * \return размер данных в буфере.
 */
unsigned rb_len(const rb_t rb);

/**
 * \return вместимость данных в буфере.
 */
unsigned rb_cap(const rb_t rb);

/**
 * \return индекс массива буфера, в который будут добавляться элементы.
 */
unsigned rb_idx_in(const rb_t rb);

/**
 * \return индекс массива буфера, из которого будут браться элементы.
 */
unsigned rb_idx_out(const rb_t rb);

/**
 * Запись данных в кольцевой буфер.
 * \param data массив бинарных данных для чтения.
 * \param data_len размер массива.
 * \return результат записи.
 */
enum RB_CODE rb_push(rb_t rb, const char*data, unsigned data_len);

/**
 * Чтение данных из кольцевого буфера.
 * \param data массив бинарных данных для записи.
 * \param data_cap вместимость массива.
 * \return число считанных байт.
 */
unsigned rb_get(rb_t rb, char*data, unsigned data_cap);

/**
 * Удаление всех внутренних динамических структур.
 */
void rb_deconfig(rb_t rb);

/**
 * Зануление структуры кольцевого буфера.
 */
void rb_clean(rb_t rb);

/**
 * Удаление структуры кольцевого буфера.
 */
void rb_free(rb_t rb);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* AVFILTER_HSV_RB_RB_H */
/**
 * /}
 */
