#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/errno.h>
#include "img_hal.h"

#define IMG_LOG(level, fmt, arg...)		\
	do {						\
		if (debug >= (level))			\
			printf(fmt , ## arg);		\
	} while (0)

#define ERR(fmt, arg...)					\
	do {								\
		printf("(%s:%d)" fmt, __func__, __LINE__, ##arg);	\
	} while (0)

#define RET(x)					\
	do {						\
		if ((x) != 0) { 			\
			ERR("\'%s\' fail.\n", #x);	\
			goto err;			\
		}					\
	} while (0)

#define MTK_PNG_MODULE_NAME	"mtk-png decoder"
#define MTK_JDEC_MODULE_NAME	"mtk-jdec decoder"
#define MTK_IMGRZ_MODULE_NAME   "mtk-imgrz"

static bool mtk_imghal_is_supported_argb(__u32 pixelformat)
{
	return (pixelformat == V4L2_PIX_FMT_ARGB32 ||
		pixelformat == V4L2_PIX_FMT_ARGB444 ||
		pixelformat == V4L2_PIX_FMT_RGB565 ||
		pixelformat == V4L2_PIX_FMT_ARGB555);

}
static int mtk_imghal_set_dev_name(struct ut_ctx *ctx, struct mtk_imghal_param *param)
{
	if (param->priv.need_imgrz) {
		ctx->dev_name = MTK_IMGRZ_MODULE_NAME;
		return 0;
	}

	switch (param->img_type) {
	case MTK_IMG_JPEG:
		ctx->dev_name = MTK_JDEC_MODULE_NAME;
		break;
	case MTK_IMG_PNG:
		ctx->dev_name = MTK_PNG_MODULE_NAME;
		break;
	case MTK_IMG_UNCOMPRESS:
		ctx->dev_name = MTK_IMGRZ_MODULE_NAME;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int mtk_imghal_set_fmt(struct ut_ctx *ctx, struct mtk_imghal_param *param, __u32 buf_type)
{
	struct v4l2_format *fmt;
	const unsigned int *bpl;
	int i;

	fmt = ut_get_fmt(ctx, buf_type);
	fmt->type = buf_type;

	if (V4L2_TYPE_IS_OUTPUT(buf_type)) {
 		if (ctx->dev_name == MTK_JDEC_MODULE_NAME) {
			fmt->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_JPEG;
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = MAX_PIC_SIZE;
			fmt->fmt.pix_mp.plane_fmt[0].bytesperline = 0;
			fmt->fmt.pix_mp.num_planes = 1;
 		} else if (ctx->dev_name == MTK_PNG_MODULE_NAME) {
			fmt->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_PNG;
			fmt->fmt.pix_mp.plane_fmt[0].sizeimage = MAX_PIC_SIZE;
			fmt->fmt.pix_mp.plane_fmt[0].bytesperline = 0;
			fmt->fmt.pix_mp.num_planes = 1;
 		} else if (ctx->dev_name = MTK_IMGRZ_MODULE_NAME) {
			if (param->priv.need_imgrz)
				fmt->fmt.pix_mp = param->dec_fmt;
			else {
				bpl = ut_v4l2_fmt_bpl(param->src_fmt.pixelformat);
				for (i = 0; i < param->src_fmt.num_planes; i++) {
					param->src_fmt.plane_fmt[i].bytesperline =
						ALIGN(DIV_ALIGN(param->src_fmt.width * bpl[i], 8), 16);
					param->src_fmt.plane_fmt[i].sizeimage = DMA_ALIGN;
				}
				fmt->fmt.pix_mp = param->src_fmt;
			}
		} else
			return -EINVAL;
		goto end;
	}

	if (param->priv.need_imgrz && ctx->dev_name != MTK_IMGRZ_MODULE_NAME)
		param->dec_fmt = ctx->cap_ctx.fmt.fmt.pix_mp;
	else
		fmt->fmt.pix_mp = param->dst_fmt;
end:
	return ut_v4l2_set_fmt(ctx, buf_type);

}

int mtk_imghal_q_buf(struct ut_ctx *ctx, struct mtk_imghal_param *param, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	if (V4L2_TYPE_IS_OUTPUT(buf_type)) {
		if (param->priv.need_imgrz && ctx->dev_name == MTK_IMGRZ_MODULE_NAME) {
			buf_ctx->planes = param->dec_buf.planes;
			buf_ctx->n_plane = param->dec_fmt.num_planes;
		} else {
			ut_set_in_file_path(ctx, param->src_buf.file_path);
			buf_ctx->planes = param->src_buf.planes;
			buf_ctx->n_plane = param->src_fmt.num_planes;
		}
		return (ut_v4l2_q_buf(ctx, OUT));
	}

	if (param->priv.need_imgrz && ctx->dev_name != MTK_IMGRZ_MODULE_NAME) {
		int i;
		for (i = 0; i < param->dec_fmt.num_planes; i++) {
			param->dec_buf.planes[i].bytesused = param->dec_fmt.plane_fmt[i].bytesperline * param->dec_fmt.height;
			param->dec_buf.planes[i].length = ALIGN(param->dec_buf.planes[i].bytesused, DMA_ALIGN);
			param->dec_buf.planes[i].m.userptr = (unsigned long)
				aligned_alloc(DMA_ALIGN, param->dec_buf.planes[i].length);
		}
		buf_ctx->planes = param->dec_buf.planes;
		buf_ctx->n_plane = param->dec_fmt.num_planes;
	} else {
		buf_ctx->planes = param->dst_buf.planes;
		buf_ctx->n_plane = param->dst_fmt.num_planes;
		if (param->dst_buf.file_path[0]) {
			ut_set_out_file_path(ctx, param->dst_buf.file_path);
		}
	}
	return (ut_v4l2_q_buf(ctx, CAP));
}

int mtk_imghal_chk_need_imgrz(struct ut_ctx *ctx, struct mtk_imghal_param *param)
{
	struct v4l2_pix_format_mplane *pix_mp;

	if (param->img_type == MTK_IMG_JPEG)
		param->priv.need_imgrz = false;
	else if (param->img_type == MTK_IMG_PNG) {
		pix_mp = &ctx->cap_ctx.fmt.fmt.pix_mp;
		if (pix_mp->width != param->dst_fmt.width ||
		    pix_mp->height != param->dst_fmt.height ||
		    !mtk_imghal_is_supported_argb(param->dst_fmt.pixelformat)) {
			param->priv.need_imgrz = true;
		}
	}
	return 0;
}

static void test_fin(struct ut_ctx *ctx, int ret)
{
	printf("---------------- Result ----------------\n");
	printf("#in data: %d\n", ut_get_in_data_num(ctx));
	printf("#out data: %d\n", ut_get_out_data_num(ctx));
	printf("----------------------------------------\n");
	printf("================= %s =================\n",
			ret ? "FAIL" : "PASS");
	ut_destroy_ctx(ctx);
}

int mtk_imghal_init_param(struct mtk_imghal_param *param)
{
	param->option.out_mem_type = V4L2_MEMORY_USERPTR;
	param->option.cap_mem_type = V4L2_MEMORY_USERPTR;
	param->option.out_buf_num = 1;
	param->option.cap_buf_num = 1;

	return 0;
}

int mtk_imghal_init_ctx(struct ut_ctx *ctx, struct mtk_imghal_param *param)
{
	ut_set_debug_level(ctx, 1);
	if (mtk_imghal_set_dev_name(ctx, param))
		return -EINVAL;
	ut_set_buf_num(ctx, param->option.out_buf_num, OUT);
	ut_set_buf_num(ctx, param->option.cap_buf_num, CAP);
	ut_set_mem_type(ctx, param->option.out_mem_type, OUT);
	ut_set_mem_type(ctx, param->option.cap_mem_type, CAP);
	return 0;
}

int mtk_imghal_start(struct mtk_imghal_param *param)
{
	struct ut_ctx *ctx, *ctx_2nd;

	ctx = ut_create_ctx();
	if (!ctx)
		return -1;
	param->priv.ctx = ctx;
	mtk_imghal_init_ctx(ctx, param);

	RET(ut_v4l2_open_dev(ctx));
	RET(mtk_imghal_set_fmt(ctx, param, OUT));
	RET(ut_v4l2_alloc_buf_queue(ctx, OUT));
	RET(ut_v4l2_stream_on(ctx, OUT));
	RET(mtk_imghal_q_buf(ctx, param, OUT));

	if (ctx->dev_name != MTK_IMGRZ_MODULE_NAME) {
		RET(ut_v4l2_get_fmt(ctx, CAP));
		param->dec_fmt = ctx->cap_ctx.fmt.fmt.pix_mp;
	}

	if (param->set_dst_fmt)
	    RET(param->set_dst_fmt(param));
	RET(mtk_imghal_chk_need_imgrz(ctx, param));

	RET(mtk_imghal_set_fmt(ctx, param, CAP));
	RET(ut_v4l2_alloc_buf_queue(ctx, CAP));
	RET(ut_v4l2_stream_on(ctx, CAP));
	RET(mtk_imghal_q_buf(ctx, param, CAP));

	if (!param->priv.need_imgrz)
		return 0;

	RET(ut_v4l2_poll(ctx, CAP));
	RET(ut_v4l2_dq_buf(ctx, CAP));
	RET(ut_v4l2_poll(ctx, OUT));
	RET(ut_v4l2_dq_buf(ctx, OUT));

	ctx_2nd = ut_create_ctx();
	if (!ctx)
		return -1;
	param->priv.ctx_2nd = ctx_2nd;
	mtk_imghal_init_ctx(ctx_2nd, param);

	RET(ut_v4l2_open_dev(ctx_2nd));
	RET(mtk_imghal_set_fmt(ctx_2nd, param, OUT));
	RET(ut_v4l2_alloc_buf_queue(ctx_2nd, OUT));
	RET(ut_v4l2_stream_on(ctx_2nd, OUT));
	RET(mtk_imghal_q_buf(ctx_2nd, param, OUT));

	RET(mtk_imghal_set_fmt(ctx_2nd, param, CAP));
	RET(ut_v4l2_alloc_buf_queue(ctx_2nd, CAP));
	RET(ut_v4l2_stream_on(ctx_2nd, CAP));
	RET(mtk_imghal_q_buf(ctx_2nd, param, CAP));

	return 0;
err:
	return -1;
}

int mtk_imghal_qbuf(struct mtk_imghal_param *param)
{
	RET(mtk_imghal_q_buf(param->priv.ctx, param, OUT));
	RET(mtk_imghal_q_buf(param->priv.ctx, param, CAP));

	if (param->priv.need_imgrz) {
		RET(ut_v4l2_poll(param->priv.ctx, CAP));
		RET(ut_v4l2_dq_buf(param->priv.ctx, CAP));
		RET(ut_v4l2_poll(param->priv.ctx, OUT));
		RET(ut_v4l2_dq_buf(param->priv.ctx, OUT));
		RET(mtk_imghal_q_buf(param->priv.ctx_2nd, param, OUT));
		RET(mtk_imghal_q_buf(param->priv.ctx_2nd, param, CAP));
	}
	return 0;
err:
	return -1;
}

int mtk_imghal_dqbuf(struct mtk_imghal_param *param)
{
	struct ut_ctx *ctx = param->priv.need_imgrz ?
			param->priv.ctx_2nd : param->priv.ctx;

	RET(ut_v4l2_poll(ctx, CAP));
	RET(ut_v4l2_dq_buf(ctx, CAP));
	RET(ut_v4l2_poll(ctx, OUT));
	RET(ut_v4l2_dq_buf(ctx, OUT));
	return 0;
err:
	return -1;
}

int mtk_imghal_stop(struct mtk_imghal_param *param)
{
	struct ut_ctx *ctx = param->priv.ctx;
	struct ut_ctx *ctx_2nd = param->priv.ctx_2nd;

	ut_v4l2_stream_off(ctx, OUT);
	ut_v4l2_free_buf_queue(ctx, OUT);
	ut_v4l2_stream_off(ctx, CAP);
	ut_v4l2_free_buf_queue(ctx, CAP);
	ut_v4l2_close_dev(ctx);
	test_fin(ctx, 0);

	if (param->priv.need_imgrz) {
		ut_v4l2_stream_off(ctx_2nd, OUT);
		ut_v4l2_free_buf_queue(ctx_2nd, OUT);
		ut_v4l2_stream_off(ctx_2nd, CAP);
		ut_v4l2_free_buf_queue(ctx_2nd, CAP);
		ut_v4l2_close_dev(ctx_2nd);
		test_fin(ctx_2nd, 0);
	}
	return 0;
}

int mtk_imghal_dec(struct mtk_imghal_param *param)
{
	RET(mtk_imghal_start(param));
	RET(mtk_imghal_dqbuf(param));
	RET(mtk_imghal_stop(param));
	return 0;
err:
	return -1;
}
