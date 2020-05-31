/**
 * \file hsv.h
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Публичное API алгоритма шумоочистки "Hubbub Suppression for Voice" ("HSV").
 */
/**
 * \defgroup hsv Модуль "HSV".
 * \{
 */
#ifndef HSV_H_INCLUDED
#define HSV_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define HSV_MAX_CHANS 4 /**< Максимальное число одновременно обрабатываемых каналов. */

#define HSV_DEFAULT 0 /**< Значение по умолчанию для всех параметром. */

#define HSV_SUPPORTED_BS 16 /**< Поддерживаемый размер одного сэмпла в битах. */

#define HSV_DEFAULT_CAP 16384 /**< Вместимость кольцевого буфера по умолчанию в байтах. */

#define HSV_DEFAULT_OVERLAP_PERC 50 /**< Процент перекрытия фреймов по умолчанию. */

/**
 * Режим работы алгоритма шумоподавления.
 * Копия enum'а из suppressor.h
 * Нужна для того, чтобы все API модуля "HSV" было в одном файле (для FFmpeg).
 */
enum HSV_SUPPRESSOR_MODE
{
	HSV_SUPPRESSOR_MODE_SPECSUB, /**< Режим, основанный на спектральном вычитании Берути. */

	HSV_SUPPRESSOR_MODE_WIENER,  /**< Режим, основанный на винеровской фильтрации Скалара. */

	HSV_SUPPRESSOR_MODE_TSNR,    /**< Режим, основанный на двухшаговой фильтрации Скалара.               */
	HSV_SUPPRESSOR_MODE_TSNR_G,  /**< Режим, основанный на двухшаговой фильтрации Скалара с "усилением". */

	HSV_SUPPRESSOR_MODE_RTSNR,   /**< Режим, основанный на двухшаговой фильтрации Шифенга.               */
	HSV_SUPPRESSOR_MODE_RTSNR_G, /**< Режим, основанный на двухшаговой фильтрации Шифенга с "усилением". */
};

/**
 * Коды, возвращаемые методами hsv_...
 */
enum HSV_CODE
{
	HSV_CODE_OK = 0,              /**< Метод успешно отработал.    */
	HSV_CODE_ALLOC_ERR = -1,      /**< Ошибка выделения памяти.    */
	HSV_CODE_OVERFLOW_ERR = -2,   /**< Ошибка переполнения буфера. */
	HSV_CODE_UNKNOWN_ERR = -1024, /**< Неизвестная ошибка.         */
};

/**
 * Структура конфигурации \"HSV\".
 * Значения sr, ch и bs должны быть ненулевыми.
 * Остальные в случае нулевых значений принимают значения по умолчанию.
 */
struct HSV_CONFIG
{
	/**
	 * Частота дискретизации.
	 * Должна быть ненулевой.
	 */
	unsigned sr;
	/**
	 * Число каналов.
	 * Должно быть ненулевым и не превышать HSV_MAX_CHANS.
	 */
	unsigned ch;
	/**
	 * Размер одного сэмпла в битах.
	 * Должен совпадать с HSV_SUPPORTED_BS.
	 */
	unsigned bs;

	/**
	 * Режим работы алгоритма шумоподавления.
	 */
	enum HSV_SUPPRESSOR_MODE mode;

	/**
	 * Размер обрабатываемого фрейма в сэмпах.
	 * Рекомендуются значения 2.0 * sr / 100.0 или близкие к ним степени двойки.
	 */
	unsigned frame_size_smpls;
	/**
	 * Процент перекрытия фреймов.
	 * Рекомендуются значения 50-75%.
	 */
	unsigned overlap_perc;
	/**
	 * Размер ДПФ в сэмплах.
	 * Рекомендуется брать как удвоенный размер фрейма для увеличения частотного разрешения.
	 */
	unsigned dft_size_smpls;
	/**
	 * Вместимость кольцевого буфера в байтах.
	 * Должна быть четной и достаточно большой для упреждения задержек.
	 */
	unsigned cap;
};

/**
 * Структура контекста \"HSV\".
 */
struct HSV_CONTEXT;

typedef struct HSV_CONTEXT* hsvc_t;

/**
 * Создание структуры \"HSV\".
 * \return указатель на структуру \"HSV\" (при ошибке - NULL).
 */
hsvc_t create_hsvc();

/**
 * Проверка корректности структуры конфигурации.
 * \return 0 при корректной конфигуации, номер первого некорректного поля (с 1) при некорректной.
 */
int hsvc_validate_config(const struct HSV_CONFIG*conf);

/**
 * Конфигурация \"HSV\".
 * \param conf структура конфигурации \"HSV\".
 * \return результат конфигурирования.
 */
enum HSV_CODE hsvc_config(hsvc_t hsvc, const struct HSV_CONFIG*conf);

/**
 * Запись данных и их шумоочистка.
 * \param data массив бинарных данных для чтения.
 * \param data_len размер массива.
 * \return если >= 0, то число обработанных данных, иначе код ошибки.
 */
int hsvc_push(hsvc_t hsvc, const char*data, unsigned data_len);

/**
 * Чтение данных из кольцевого буфера.
 * \param data массив бинарных данных для записи.
 * \param data_cap вместимость массива.
 * \return число считанных байт.
 */
unsigned hsvc_get(hsvc_t hsvc, char*data, unsigned data_cap);

/**
 * Сброс внутреннего счетчика фреймов для получения необработанных данных.
 */
void hsvc_flush(hsvc_t hsvc);

/**
 * Удаление всех внутренних динамических структур.
 */
void hsvc_deconfig(hsvc_t hsvc);

/**
 * Зануление структуры \"HSV\".
 */
void hsvc_clean(hsvc_t hsvc);

/**
 * Удаление структуры \"HSV\".
 */
void hsvc_free(hsvc_t hsvc);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HSV_H_INCLUDED */
/**
 * /}
 */
