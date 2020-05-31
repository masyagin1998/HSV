/*
 * Copyright (c) 2020 Mikhail Masyagin
 *
 * This file is NOT part of FFmpeg.
 */
#include "libavutil/opt.h"
#include "avfilter.h"
#include "audio.h"

#include "hsv_interface.h"

struct HSV_DEN
{
	const AVClass*class;

	char*mode;

	struct HSV_CONFIG conf;
	hsvc_t hsvc;
};

typedef struct HSV_DEN HSV_DEN;

#define OFFSET(x) offsetof(HSV_DEN, x)
#define A AV_OPT_FLAG_AUDIO_PARAM|AV_OPT_FLAG_FILTERING_PARAM

static const AVOption hsvden_options[] =
{
	{
	 .name = "mode",
	 .help = "Noise suppression mode: \"specsub\"|\"wiener\"|\"tsnr\"|\"tsnrg\"|\"rtsnr\"|\"rtsnrg\".",
	 .offset = OFFSET(mode),
	 .type = AV_OPT_TYPE_STRING,
	 .default_val = { .str = "tsnr" },
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},
	{
	 .name = "m",
	 .help = "Noise suppression mode: \"specsub\"|\"wiener\"|\"tsnr\"|\"tsnrg\"|\"rtsnr\"|\"rtsnrg\".",
	 .offset = OFFSET(mode),
	 .type = AV_OPT_TYPE_STRING,
	 .default_val = { .str = "tsnr" },
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},

	{
	 .name = "frame",
	 .help = "Frame size in samples.",
	 .offset = OFFSET(conf.frame_size_smpls),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},
	{
	 .name = "f",
	 .help = "Frame size in samples.",
	 .offset = OFFSET(conf.frame_size_smpls),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},

	{
	 .name = "overlap",
	 .help = "Overlap in percents.",
	 .offset = OFFSET(conf.overlap_perc),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 100,
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},
	{
	 .name = "o",
	 .help = "Overlap in percents.",
	 .offset = OFFSET(conf.overlap_perc),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 100,
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},

	{
	 .name = "dft",
	 .help = "DFT size in samples.",
	 .offset = OFFSET(conf.dft_size_smpls),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},
	{
	 .name = "d",
	 .help = "DFT size in samples.",
	 .offset = OFFSET(conf.dft_size_smpls),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},

	{
	 .name = "cap",
	 .help = "Internal buffer capacity in bytes",
	 .offset = OFFSET(conf.cap),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},
	{
	 .name = "c",
	 .help = "Internal buffer capacity in bytes",
	 .offset = OFFSET(conf.cap),
	 .type = AV_OPT_TYPE_INT,
	 .default_val = { .i64 = 0 },
	 .min = 0,
	 .max = 1048576, /* 2^20 */
	 .flags = (AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM),
	},

	{ NULL },
};

AVFILTER_DEFINE_CLASS(hsvden);

static av_cold int init(AVFilterContext*ctx)
{
	HSV_DEN*s = ctx->priv;

	memset(&s->conf, 0, sizeof(s->conf));

	return 0;
}

static av_cold void uninit(AVFilterContext*ctx)
{
	HSV_DEN*s = ctx->priv;
	if (s->hsvc != NULL) {
		hsvc_deconfig(s->hsvc);
		hsvc_free(s->hsvc);
	}
}

static const enum AVSampleFormat sample_fmts[] =
{
	AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE,
};

static int query_formats(AVFilterContext*ctx)
{
	int r;
	
	AVFilterChannelLayouts*layouts;
	AVFilterFormats*formats;

	layouts = ff_all_channel_counts();
	if (layouts == NULL) {
		return AVERROR(ENOMEM);
	}

	r = ff_set_common_channel_layouts(ctx, layouts);
	if (r < 0) {
		return r;
	}

	formats = ff_make_format_list(sample_fmts);
	if (formats == NULL) {
		return AVERROR(ENOMEM);
	}

	r = ff_set_common_formats(ctx, formats);
	if (r < 0) {
		return r;
	}

	formats = ff_all_samplerates();
	if (formats == NULL) {
		return AVERROR(ENOMEM);
	}

	return ff_set_common_samplerates(ctx, formats);
}

static int config_props(AVFilterLink*inlink)
{
	int r;

	enum HSV_CODE hsv_r;

	AVFilterContext*ctx = inlink->dst;
	HSV_DEN*s = ctx->priv;

	s->conf.sr = inlink->sample_rate;
	s->conf.ch = inlink->channels;
	s->conf.bs = 16;

	if (strcmp(s->mode, "specsub") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_SPECSUB;
	} else if (strcmp(s->mode, "wiener") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_WIENER;
	} else if (strcmp(s->mode, "tsnr") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_TSNR;
	} else if (strcmp(s->mode, "tsnrg") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_TSNR_G;
	} else if (strcmp(s->mode, "rtsnr") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_RTSNR;
	} else if (strcmp(s->mode, "rtsnrg") == 0) {
		s->conf.mode = HSV_SUPPRESSOR_MODE_RTSNR_G;
	} else {
		av_log(ctx, AV_LOG_ERROR, "Invalid mode (%s)\n", s->mode);
		return AVERROR(EINVAL);
	}

	r = hsvc_validate_config(&(s->conf));
	if ((enum HSV_CODE) r != HSV_CODE_OK) {
		av_log(ctx, AV_LOG_ERROR, "Invalid configuration in parameter (%d)\n", r);
		return AVERROR(EINVAL);
	}

	s->hsvc = create_hsvc();
	if (s->hsvc == NULL) {
		av_log(ctx, AV_LOG_ERROR, "Unable to create \"hsv\" context!\n");
		return AVERROR(ENOMEM);
	}

	hsv_r = hsvc_config(s->hsvc, &(s->conf));
	if (hsv_r != HSV_CODE_OK) {
		hsvc_free(s->hsvc);
		s->hsvc = NULL;
		av_log(ctx, AV_LOG_ERROR, "Unable to allocate \"hsv\" context internal buffers!\n");
		return AVERROR(ENOMEM);
	}

	return 0;
}

static int filter_frame(AVFilterLink*inlink, AVFrame*frame)
{
	int processed;
	
	AVFilterContext*ctx = inlink->dst;
	HSV_DEN*s = ctx->priv;
	AVFrame*out_frame;

	if (s->hsvc == NULL) {
		out_frame = frame;
		return ff_filter_frame(ctx->outputs[0], out_frame);
	}

	out_frame = ff_get_audio_buffer(ctx->outputs[0], frame->nb_samples * 2);
	if (out_frame == NULL) {
		av_frame_free(&frame);
		return AVERROR(ENOMEM);
	}
	av_frame_copy_props(out_frame, frame);

	processed = hsvc_push(s->hsvc, (char*) frame->extended_data[0], (frame->nb_samples * 2) * frame->channels);
	if (processed < 0) {
		av_log(ctx, AV_LOG_ERROR, "\"hsv\" context overflow error!\n");
		return AVERROR(EINVAL);
	} else if (processed == 0) {
		out_frame->nb_samples = 0;
	} else if (processed > 0) {
		out_frame->nb_samples = hsvc_get(s->hsvc, (char*) out_frame->extended_data[0], (out_frame->nb_samples * 2) * frame->channels) / 2;
		out_frame->nb_samples /= frame->channels;
	}

	av_frame_free(&frame);

	return ff_filter_frame(ctx->outputs[0], out_frame);
}

static const AVFilterPad hsvden_inputs[] =
{
	{
		.name = "default",
		.type = AVMEDIA_TYPE_AUDIO,
		.config_props = config_props,
		.filter_frame = filter_frame,
	},

	{ NULL },
};

static const AVFilterPad hsvden_outputs[] =
{
	{
		.name = "default",
		.type = AVMEDIA_TYPE_AUDIO,
	},

	{ NULL },
};

AVFilter ff_af_hsvden = {
	.name		   = "hsvden",
	.description   = NULL_IF_CONFIG_SMALL("Denoise speech using \"HSV\" library."),
	.query_formats = query_formats,
	.priv_size	   = sizeof(HSV_DEN),
	.priv_class	   = &hsvden_class,
	.init		   = init,
	.uninit		   = uninit,
	.inputs		   = hsvden_inputs,
	.outputs	   = hsvden_outputs,
};
