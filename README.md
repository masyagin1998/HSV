# HSV

<img src="https://github.com/masyagin1998/HSV/blob/master/docs/pics/logo.png" height="150" width="150">

**HSV** is a **H**ubbub **S**uppression for **V**oice tool, written in pure C.

## Техническая информация

**HSV** - это библиотека шумоочистки/улучшения голоса в реальном времени, написанная без зависимостей на чистом [C99](https://en.cppreference.com/) и легко встраиваемая в [FFmpeg](https://ffmpeg.org/) и [MPV](https://mpv.io/). Она является моей выпускной квалификационной работой с кафедры "Теоретической информатики и компьютерных технологий" [МГТУ им Н. Э. Баумана.](https://bmstu.ru/)

Библиотека **HSV** предоставляет блочно-последовательную обработку звуковых данных через внутренний кольцевой буфер. Для детектирования шума используется алгоритм `MCRA-2` Лойзю-Рангачари, для непосредственной шумоочистки могут быть использованы алгоритм спектрального вычитания Берути-Шварца или различные модификации алгоритма винеровской фильтрации Скалара. Для вычисления дискретного преобразования Фурье и обратного дискретного преобразования Фурье используются алгоритмы Кули-Тьюки и Блюштейна.

## Установка и запуск HSV

Для сборки и запуска библиотеки **HSV** необходимы  `С99`-совместимый компилятор, утилита [Make](https://www.gnu.org/software/make/) и `unix`-совместимая ОС.
Библиотека **HSV** может быть легко перенесена на [C89](https://en.cppreference.com/), для этого необходимо лишь реализвовать некоторые библиотечные функции [<math.h>](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html), добавленные в стандарте `C99`. Обратная совместимость обеспечивается объявлением всех переменных строго в начале блоков и использованием только "классических" типов данных.

Для сборки **HSV** и тестового примера необходимо выполнить следующую последовательность команд:

```sh
$ git clone https://github.com/masyagin1998/HSV.git
$ cd HSV
$ make
```

Чтобы не увеличивать и без того немалый объем кода алгоритмами работы с `wav`, параметры входных и выходных `wav`-файлов строго задаются в примере `examples/example.c`.

Запуск тестового примера осуществляется следующим образом:

```sh
$ ./bin/example --wiener examples/in.wav examples/out.wav
```

где `--wiener` - один из видов алгоритмов шумоподавления;
`examples/in.wav` - входной `wav`-файл;
`examples/out.wav` - выходной `wav`-файл;

## Встраивание в FFmpeg

На сегодяшний день `FFmpeg` является одной из наиболее активно используемых библиотек для обработки аудио- и видеопотоков в реальном времени. На данный момент в ней отсутствуют методы для непосредственного подавления шума/улучшения голоса, поэтому было решено встроить в неё **HSV**.

Чтобы собрать `FFmpeg` с поддержкой **HSV** необходимо выполнить следующие шаги:

- скачать зависимости для сборки `FFmpeg`:
```sh
$ sudo apt-get update -qq && sudo apt-get -y install autoconf automake build-essential cmake git-core \
$ libass-dev libfreetype6-dev libgnutls28-dev libsdl2-dev libtool libva-dev libvdpau-dev libvorbis-dev \
$ libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo wget yasm zlib1g-dev
```

- создать папки для сборки `FFmpeg`:
```sh
$ mkdir -p ~/ffmpeg_sources ~/bin
```

- скачать архив с исходным кодом `FFmpeg` и распаковать его:
```sh
$ cd ~/ffmpeg_sources
$ wget -O ffmpeg-snapshot.tar.bz2 https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
$ tar xjvf ffmpeg-snapshot.tar.bz2
```

- применить патч `hsvden.patch` из папки `HSV/ffmpeg/patches` или добавить/заменить файлы исходного кода `FFmpeg` файлами из `HSV/ffmpeg/src`:
```sh
$ cp -r HSV/ffmpeg/* ~/ffmpeg_sources
$ ./apply_patches.sh
```

- выполнить сборку "пропатченного" `FFmpeg` (возможно, Вам придётся отключить `gnutls` и `libaom`):
```sh
$ cd ffmpeg
$ PATH="$HOME/bin:$PATH" PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" ./configure --prefix="$HOME/ffmpeg_build" \
$ --pkg-config-flags="--static" --extra-cflags="-I$HOME/ffmpeg_build/include" --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
$ --extra-libs="-lpthread -lm"  --bindir="$HOME/bin" --enable-gpl --enable-gnutls --enable-libaom --enable-libass \
$ --enable-libfdk-aac --enable-libfreetype --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-nonfree
$ PATH="$HOME/bin:$PATH" make -j8
$ make install -j8
```

- после этого в папке `~/bin` должна появиться сборка `FFmpeg` с поддержкой библиотеки `HSV`. Для запуска `FFmpeg` с одним из фильтров `HSV` необходимо выполнить следующую команду:
```sh
$ ./bin/ffmpeg -i examples/in.wav -af "[aid1] hsvden=mode=tsnrg [ao]" examples/out.wav
```

где `wiener` - один из видов алгоритмов шумоподавления;
`examples/in.wav` - входной `wav`-файл;
`examples/out.wav` - выходной `wav`-файл;

При использовании **HSV** в `FFmpeg` можно задавать следующие флаги:
- `mode` (`m`) - режим работы шумоподавления:
   - `specsub` - спектральное вычитание Берути-Шварца;
   - `wiener` - винеровская фильтрация Скалара;
   - `tsnr` - двухшаговая фильтрация Скалара;
   - `tsnrg` - двухшаговая фильтрация Скалара с "усилением";
   - `rtsnr` - двухщаговая фильтрация Шифенга;
   - `rtsnrg` - двухшаговая фильтрация Шифенга с "усилением";

- `frame` (`f`) - размер одного обрабатываемого фрейма в сэмплах. Рекомендуются значения `2.0 * sr / 100.0` или близкие к ним степени двойки;

- `overlap` (`o`) - величина перекрытия фреймов в процентах. Рекумендуются значения 50-75%;

- `dft` (`d`) - размер дискретного преобразования Фурье в сэмплах. Рекомендуется брать как удвоенный размер фрейма для увеличения частотного разрешения.;

- `cap` (`c`) - вместимость кольцевого буфера в байтах. Должна быть кратна 2 и достаточно велика для упреждения задержек;

Флаги задаются через `:`, например:
```sh
$ ./bin/ffmpeg -i examples/in.wav -af "[aid1] hsvden=mode=tsnrg:o=50:cap=32768 [ao]" examples/out.wav
```

## Встраивание в MPV

`MPV` - это компактный свободный кроссплатформенный медиаплеер, основанный на `MPlayer/mplayer2`. Он крайне активно используется и развивается с 2012 года по сей день. Встраивание **HSV** в `MPV` является наглядной демонстрацией работы библиотеки шумоочистки в реальном времени, так как при воспроизведении аудио- или видеопотока **HSV** имеет доступ лишь к текущим фреймам звука и не должна вносить существенной задержки в воспроизведение.

Чтобы собрать `MPV` с поддержкой **HSV**, необходимо выполнить следующие шаги:

- скачать официальный набор скриптов сборки `MPV` и выполнить сборку чистого `MPV`. Сборка автоматические подтянет все зависимости:
```sh
$ git clone https://github.com/mpv-player/mpv-build.git
$ cd mpv-build
$ ./rebuild -j8
```

- далее нужно очистить оригинальный `MPV` от результатов прошлой сборки и удалить скачанный `FFmpeg`:
```sh
$ cd mpv-build
$ ./clean
$ rm -rf ffmpeg
```

- затем аналогично пункту про встраивание в `FFmpeg` нужно скачать архив с кодом `FFmpeg`, распаковать его и пропатчить, но не собирать (!!!). Далее пропатченный исходный код `FFmpeg` следует поместить в папку `mpv-build`:
```sh
$ mkdir -p ~/ffmpeg_sources
$ cd ~/ffmpeg_sources
$ wget -O ffmpeg-snapshot.tar.bz2 https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
$ tar xjvf ffmpeg-snapshot.tar.bz2
$ cp -r HSV/ffmpeg/* ~/ffmpeg_sources
$ ./apply_patches.sh
cp -r ~/ffmpeg_sources/ffmpeg ~/mpv-build
```

- после этого необходимо заменить файл `rebuild` в `mpv-build` на файл, лежащий в папке `HSV/mpv`. После этого выполнить пересборку `MPV`:
```sh
$ cp HSV/mpv/rebuild mpv-build/rebuild
$ cd mpv-build
$ ./rebuild -j8
```

- после этого в папке `~/mpv-build/mpv/build` должна появиться сборка `MPV` с поддержкой библиотеки `HSV`. Для запуска `MPV` с одним из фильтров `HSV` необходимо выполнить следующую команду:
```sh
$ ./mpv-build/mpv/build/mpv examples/in.wav -af lavfi="[hsvden=m=rtsnr]"
```

где `wiener` - один из видов алгоритмов шумоподавления;
`examples/in.wav` - входной `wav`-файл;
`examples/out.wav` - выходной `wav`-файл;

В целом использование **HSV** в `MPV` совпадает с использованием **HSV** в `FFmpeg`.

## Документация

Все комментарии к библиотеке **HSV** совместимы с [Doxygen](https://www.doxygen.nl/). Чтобы сгенерировать и открыть документацию в формате [HTML](https://www.w3.org/TR/html52/) необходимо выполнить следующие шаги:

```sh
$ doxygen docs/doxygen.conf
$ firefox doxygen/html/index.html
```

## Объективная оценка качества шумоочистки

Для проведения тестов эффективности как **HSV**, так и других алгоритмов шумоочистки, в папке `scripts` имеются скрипты для вычисления [ОСШ](https://ru.wikipedia.org/wiki/%D0%9E%D1%82%D0%BD%D0%BE%D1%88%D0%B5%D0%BD%D0%B8%D0%B5_%D1%81%D0%B8%D0%B3%D0%BD%D0%B0%D0%BB/%D1%88%D1%83%D0%BC), сегментного ОСШ и [PESQ](https://www.itu.int/rec/T-REC-P.862).

Чем выше значения `SNR` и `PESQ` по сравнению с зашумленным сигналом, тем выше качество шумоочистки. Замечу, что повышение одной метрики зачастую не гарантирует повышение другой.

## Пример работы

В папке `data` можно найти примеры работы всех алгоритмов шумоочистки на мультишумовом примере. Также доступен архив с более чем 500 мегабайтами тестовых данных и сравнением разработанного алгоритма с `WebRTC` и `Speex` по [данной ссылке](https://yadi.sk/d/ttNJxNwO3E8zkg). Эффективность шумоочистки может быть оценена скриптами для вычисления `ОСШ`, сегментного `ОСШ` и `PESQ` из папки `scripts`.

- Чистый сигнал: PESQ=4.5, SegSNR = 35;
![clean](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/clean.png)

- Шум: PESQ=0.0, SegSNR = -10;
![noise](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/noise.png)

- Зашумленный сигнал: PESQ=0.887, SegSNR = -0.542;
![noised](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/noised.png)

- Спектральное вычитание: PESQ=1.440, SegSNR = 3.841;
![specsub](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/specsub.png)

- Винеровская фильтрация Скалара: PESQ=1.807, SegSNR = 4.008;
![wiener](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/wiener.png)

- TSNR: PESQ=1.826, SegSNR = 4.414;
![tsnr](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/tsnr.png)

- RTSNR: PESQ=1.778, SegSNR = 4.549;
![rtsnr](https://github.com/masyagin1998/HSV/blob/master/docs/pics/10/rtsnr.png)

## Ссылки на основную использованную литературу

- [Loizou P. C. Speech Enhancement. Theory and Practice. Second Edition // CRC Press, 2013 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/estimator/Loizou_P_C_Speech_Enhancement.pdf) - лучшая книга, вводящая в теорию шумоочистки речевых сигналов;
- [Cohen I., Berdugo B. Noise Estimation by Minima Controlled Recursive Averaging for Robust Speech Enhancement // Lamar Signal Processing Ltd., 2002 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/estimator/Cohen_I_MCRA.pdf) - оригинальный алгоритм детектирования шума MCRA;
- [Scalart P., Filho J. V. Speech enhancement based on a priori SNR estimator // Acoustics, Speech and Signal Processing, 1996 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/suppressor/Scalart_P_Wiener.pdf) - оригинальный алгоритм винеровской фильтрации, предложенный Скаларом;
- [Scalart P., Plapous C. A two-step noise reduction technique // Acoustics, Speech and Signal Processing, 2004 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/suppressor/Scalart_P_TSNR.pdf) - улучшенный вариант алгоритма винеровской фильтрации, также известный как `TSNR`;
- [Shifeng Ou, Chao G., Ying G. Improve a priori SNR estimator  for speech enhancement incorporating speech distortion component // Yantai Univercity, 2013 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/suppressor/Shifeng_Ou_RTSNR.pdf) - улучшенный вариант алгоритма винеровской фильтрации, также известный как `RTSNR`;
- [Тропченко А. Ю., Тропченко А. А. Цифровая обработка сигналов // ИТМО, 2009 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/fft/%D0%A2%D1%80%D0%BE%D0%BF%D1%87%D0%B5%D0%BD%D0%BA%D0%BE_%D0%90_%D0%A6%D0%9E%D0%A1.pdf) - книга с хорошим описанием алгоритма БПФ Кули-Тьюки;
- [Кренёв А. Н., Артёмова Т. К. Цифровой анализ сигналов // ЯГУ, 2003 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/fft/%D0%9A%D1%80%D0%B5%D0%BD%D1%91%D0%B2_%D0%90_%D0%A6%D0%A1%D0%90.pdf) - книга с хорошим описанием алгоритма БПФ Блюштейна;
- [Fukane A. R., Shashikant L. S. Noise Estimation Algorithms for Speech Enhancement in highly non-stationary environments // 2011 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/comparison/estimators.pdf) - сравнительный анализ алгоритмов детектирвоания шума;
- [Shrawankar U., Thakare V. M. Performance Analysis of Noise Filters and Speech Enhancement Techniques // 2012 г.](https://github.com/masyagin1998/HSV/blob/master/docs/articles/comparison/suppressors.pdf) - сравнительный анализ множества алгоритмов шумоочистки;

## Благодарности

- Игорю Эдуардовичу Вишнякову и Олегу Александровичу Одинцову - моим научными руководителям;
