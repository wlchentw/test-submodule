#include <stdbool.h>
#include "ut_v4l2.h"

#define IMG_DEF_PITCH 16
#define MAX_PATH_SIZE 128
#define MAX_PIC_SIZE	(4 * 1024 * 1024)
#define MAX_PLANES_NUM 3

enum mtk_img_type {
	MTK_IMG_JPEG = 1,
	MTK_IMG_PNG = 2,
	MTK_IMG_UNCOMPRESS = 3,
};

struct mtk_img_dec_option {
	int dev_fd;
	enum v4l2_memory out_mem_type;
	enum v4l2_memory cap_mem_type;
	unsigned int out_buf_num;	/* default 1 */
	unsigned int cap_buf_num;	/* default 1 */

	struct v4l2_control ctrl;
	struct v4l2_ext_controls ext_controls;
	struct v4l2_capability cap;

	struct ut_buf_ctx out_ctx;
	struct ut_buf_ctx cap_ctx;
};

struct mtk_imghal_buf {
	char	file_path[MAX_PATH_SIZE];
	struct v4l2_plane planes[MAX_PLANES_NUM];
};

struct mtk_imghal_priv {
	struct ut_ctx *ctx;
	struct ut_ctx *ctx_2nd;
	bool need_imgrz;
};
/**
 * struct mtk_img_dec_param - image decode parameter
 * @dev_path:	optional. user can specific decoder device path
 * @in_buf:		input buffer address
 * @in_buf_size:input buffer size
 * @in_file:	input file path. if in_buf is NULL, in_file will be used.
 * @src_fmt:	will be filled while dec finish parsing image format.
 * @dst_fmt:	user should fill it in @set_dst_fmt according to @src_fmt.
 * @set_dst_fmt: user should set it, and will be called after dec finish parsing image format.
 *
 * This struct should be inited by mtk_img_dec_init_param()
 */
struct mtk_imghal_param {
	enum mtk_img_type img_type;

	struct mtk_img_dec_option option;
	struct v4l2_pix_format_mplane src_fmt;
	struct v4l2_pix_format_mplane dst_fmt;
	struct v4l2_pix_format_mplane dec_fmt;
	struct mtk_imghal_buf src_buf;
	struct mtk_imghal_buf dst_buf;
	struct mtk_imghal_buf dec_buf;
	int (*set_dst_fmt)(struct mtk_imghal_param *param);

	struct mtk_imghal_priv priv;
};

int mtk_imghal_init_param(struct mtk_imghal_param *param);
int mtk_imghal_dec(struct mtk_imghal_param *param);
