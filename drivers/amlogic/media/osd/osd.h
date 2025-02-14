/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _OSD_H_
#define _OSD_H_
#include <linux/kthread.h>
#include "osd_sync.h"
enum color_index_e {
	COLOR_INDEX_02_PAL4    = 2,  /* 0 */
	COLOR_INDEX_04_PAL16   = 4, /* 0 */
	COLOR_INDEX_08_PAL256 = 8,
	COLOR_INDEX_16_655 = 9,
	COLOR_INDEX_16_844 = 10,
	COLOR_INDEX_16_6442 = 11,
	COLOR_INDEX_16_4444_R = 12,
	COLOR_INDEX_16_4642_R = 13,
	COLOR_INDEX_16_1555_A = 14,
	COLOR_INDEX_16_4444_A = 15,
	COLOR_INDEX_16_565 = 16,

	COLOR_INDEX_24_6666_A = 19,
	COLOR_INDEX_24_6666_R = 20,
	COLOR_INDEX_24_8565 = 21,
	COLOR_INDEX_24_5658 = 22,
	COLOR_INDEX_24_888_B = 23,
	COLOR_INDEX_24_RGB = 24,

	COLOR_INDEX_32_BGRX = 25,
	COLOR_INDEX_32_XBGR = 26,
	COLOR_INDEX_32_RGBX = 27,
	COLOR_INDEX_32_XRGB = 28,

	COLOR_INDEX_32_BGRA = 29,
	COLOR_INDEX_32_ABGR = 30,
	COLOR_INDEX_32_RGBA = 31,
	COLOR_INDEX_32_ARGB = 32,

	COLOR_INDEX_YUV_422 = 33,

	COLOR_INDEX_RGBA_1010102 = 34,
};

#define VPP_OSD2_PREBLEND           BIT(17)
#define VPP_OSD1_PREBLEND           BIT(16)
#define VPP_VD2_PREBLEND            BIT(15)
#define VPP_VD1_PREBLEND            BIT(14)
#define VPP_OSD2_POSTBLEND          BIT(13)
#define VPP_OSD1_POSTBLEND          BIT(12)
#define VPP_VD2_POSTBLEND           BIT(11)
#define VPP_VD1_POSTBLEND           BIT(10)
#define VPP_POSTBLEND_EN            BIT(7)
#define VPP_PRE_FG_OSD2             BIT(5)
#define VPP_PREBLEND_EN             BIT(6)
#define VPP_POST_FG_OSD2            BIT(4)

/* OSD device ioctl definition */
#define FBIOPUT_OSD_SRCKEY_ENABLE        0x46fa
#define FBIOPUT_OSD_SRCCOLORKEY          0x46fb
#define FBIOGET_OSD_DMABUF               0x46fc
#define FBIOPUT_OSD_SET_GBL_ALPHA        0x4500
#define FBIOGET_OSD_GET_GBL_ALPHA        0x4501
#define FBIOPUT_OSD_2X_SCALE             0x4502
#define FBIOPUT_OSD_ENABLE_3D_MODE       0x4503
#define FBIOPUT_OSD_FREE_SCALE_ENABLE    0x4504
#define FBIOPUT_OSD_FREE_SCALE_WIDTH     0x4505
#define FBIOPUT_OSD_FREE_SCALE_HEIGHT    0x4506
#define FBIOPUT_OSD_ORDER                0x4507
#define FBIOGET_OSD_ORDER                0x4508
#define FBIOGET_OSD_SCALE_AXIS           0x4509
#define FBIOPUT_OSD_SCALE_AXIS           0x450a
#define FBIOGET_OSD_BLOCK_WINDOWS        0x450b
#define FBIOPUT_OSD_BLOCK_WINDOWS        0x450c
#define FBIOGET_OSD_BLOCK_MODE           0x450d
#define FBIOPUT_OSD_BLOCK_MODE           0x450e
#define FBIOGET_OSD_FREE_SCALE_AXIS      0x450f
#define FBIOPUT_OSD_FREE_SCALE_AXIS      0x4510
#define FBIOPUT_OSD_FREE_SCALE_MODE      0x4511
#define FBIOGET_OSD_WINDOW_AXIS          0x4512
#define FBIOPUT_OSD_WINDOW_AXIS          0x4513
#define FBIOGET_OSD_FLUSH_RATE           0x4514
#define FBIOPUT_OSD_REVERSE              0x4515
#define FBIOPUT_OSD_ROTATE_ON            0x4516
#define FBIOPUT_OSD_ROTATE_ANGLE         0x4517
#define FBIOPUT_OSD_SYNC_ADD             0x4518
#define FBIOPUT_OSD_SYNC_RENDER_ADD      0x4519
#define FBIOPUT_OSD_HWC_ENABLE           0x451a
#define FBIOPUT_OSD_DO_HWC               0x451b
#define FBIOPUT_OSD_BLANK                0x451c
#define FBIOGET_OSD_CAPBILITY            0x451e

#define FB_IOC_MAGIC   'O'
#define FBIOPUT_OSD_CURSOR	\
	_IOWR(FB_IOC_MAGIC, 0x0,  struct fb_cursor_user)

/* OSD color definition */
#define KEYCOLOR_FLAG_TARGET  1
#define KEYCOLOR_FLAG_ONHOLD  2
#define KEYCOLOR_FLAG_CURRENT 4

#define HW_OSD_COUNT     4
#define OSD_BLEND_LAYERS 4
#define VIU_COUNT        3
#define MAX_TRACE_NUM    16

/* OSD block definition */
#define HW_OSD_BLOCK_COUNT 4
#define HW_OSD_BLOCK_REG_COUNT (HW_OSD_BLOCK_COUNT * 2)
#define HW_OSD_BLOCK_ENABLE_MASK        0x000F
#define HW_OSD_BLOCK_ENABLE_0           0x0001 /* osd blk0 enable */
#define HW_OSD_BLOCK_ENABLE_1           0x0002 /* osd blk1 enable */
#define HW_OSD_BLOCK_ENABLE_2           0x0004 /* osd blk2 enable */
#define HW_OSD_BLOCK_ENABLE_3           0x0008 /* osd blk3 enable */
#define HW_OSD_BLOCK_LAYOUT_MASK        0xFFFF0000
#define HW_OSD_BLOCK_LAYOUT_HORIZONTAL  0x00010000
#define HW_OSD_BLOCK_LAYOUT_VERTICAL    0x00020000
#define HW_OSD_BLOCK_LAYOUT_GRID        0x00030000
#define HW_OSD_BLOCK_LAYOUT_CUSTOMER    0xFFFF0000

#define OSD_LEFT                0
#define OSD_RIGHT               1
#define OSD_ORDER_01            1
#define OSD_ORDER_10            2
#define OSD_GLOBAL_ALPHA_DEF    0x100
#define OSD_DATA_BIG_ENDIAN     0
#define OSD_DATA_LITTLE_ENDIAN  1
#define OSD_TC_ALPHA_ENABLE_DEF 0  /* disable tc_alpha */

#define INT_VIU_VSYNC  30 /* 35 */
#define INT_VIU2_VSYNC 45
#define INT_RDMA       121

#define OSD_MAX_BUF_NUM        1  /* fence relative */
#define MALI_AFBC_16X16_PIXEL  0
#define MALI_AFBC_32X8_PIXEL   1

#define MALI_AFBC_SPLIT_OFF  0
#define MALI_AFBC_SPLIT_ON   1
#define OSD_HW_CURSOR        BIT(0)
#define OSD_UBOOT_LOGO       BIT(1)
#define OSD_ZORDER           BIT(2)
#define OSD_PRIMARY          BIT(3)
#define OSD_FREESCALE        BIT(4)
#define OSD_AFBC             BIT(5)
#define OSD_VIU3             BIT(28)
#define OSD_VIU2             BIT(29)
#define OSD_VIU1             BIT(30)
#define OSD_LAYER_ENABLE     BIT(31)
#define BYPASS_DIN           BIT(7)
#define OSD_BACKUP_COUNT     24

#define LOGO_DEV_OSD0      0x0
#define LOGO_DEV_OSD1      0x1
#define LOGO_DEV_VIU2_OSD0 0x3
#define LOGO_DEBUG         0x1001
#define LOGO_LOADED        0x1002

enum osd_index_e {
	OSD1 = 0,
	OSD2,
	OSD3,
	OSD4,
	OSD_MAX,
	OSD_ERR
};

/* osd table for viux */
#define OSD_TABLE_1   (0xffff0000 | (OSD4 << 12) | (OSD3 << 8) | (OSD2 << 4) | OSD1)
#define OSD_TABLE_2_1 (0xfffff000 | (OSD4 << 8) | (OSD2 << 4) | OSD1)
#define OSD_TABLE_2_2 (0xfffffff0 | OSD3)
#define OSD_TABLE_3_1 (0xffffff00 | (OSD2 << 4) | OSD1)
#define OSD_TABLE_3_2 (0xfffffff0 | OSD3)
#define OSD_TABLE_3_3 (0xfffffff0 | OSD4)

enum osd_enable_e {
	DISABLE = 0,
	ENABLE
};

enum scan_mode_e {
	SCAN_MODE_INTERLACE,
	SCAN_MODE_PROGRESSIVE
};

struct color_bit_define_s {
	enum color_index_e color_index;
	u8 hw_colormat;
	u8 hw_blkmode;

	u8 red_offset;
	u8 red_length;
	u8 red_msb_right;

	u8 green_offset;
	u8 green_length;
	u8 green_msb_right;

	u8 blue_offset;
	u8 blue_length;
	u8 blue_msb_right;

	u8 transp_offset;
	u8 transp_length;
	u8 transp_msb_right;

	u8 color_type;
	u8 bpp;
};

struct osd_ctl_s {
	u32 xres_virtual;
	u32 yres_virtual;
	u32 xres;
	u32 yres;
	u32 disp_start_x; /* coordinate of screen */
	u32 disp_start_y;
	u32 disp_end_x;
	u32 disp_end_y;
	u32 addr;
	u32 index;
};

struct osd_info_s {
	u32 index;
	u32 osd_reverse;
};

struct para_osd_info_s {
	char *name;
	u32 info;
	u32 prev_idx;
	u32 next_idx;
	u32 cur_group_start;
	u32 cur_group_end;
};

enum osd_dev_e {
	DEV_OSD0 = 0,
	DEV_OSD1,
	DEV_OSD2,
	DEV_OSD3,
	DEV_ALL,
	DEV_MAX
};

enum reverse_info_e {
	REVERSE_FALSE = 0,
	REVERSE_TRUE,
	REVERSE_X,
	REVERSE_Y,
	REVERSE_MAX
};

enum hw_reg_index_e {
	OSD_COLOR_MODE = 0,
	OSD_ENABLE,
	OSD_COLOR_KEY,
	OSD_COLOR_KEY_ENABLE,
	OSD_GBL_ALPHA,
	OSD_CHANGE_ORDER,
	OSD_FREESCALE_COEF,
	DISP_GEOMETRY,
	DISP_SCALE_ENABLE,
	DISP_FREESCALE_ENABLE,
	DISP_OSD_REVERSE,
	DISP_OSD_ROTATE,
	OSD_FIFO,
	HW_REG_INDEX_MAX
};

enum cpuid_type_e {
	__MESON_CPU_MAJOR_ID_M8B = 0x1B,
	__MESON_CPU_MAJOR_ID_GXBB = 0x1F,
	__MESON_CPU_MAJOR_ID_GXTVBB,
	__MESON_CPU_MAJOR_ID_GXL,
	__MESON_CPU_MAJOR_ID_GXM,
	__MESON_CPU_MAJOR_ID_TXL,
	__MESON_CPU_MAJOR_ID_TXLX,
	__MESON_CPU_MAJOR_ID_AXG,
	__MESON_CPU_MAJOR_ID_GXLX,
	__MESON_CPU_MAJOR_ID_TXHD,
	__MESON_CPU_MAJOR_ID_G12A,
	__MESON_CPU_MAJOR_ID_G12B,
	__MESON_CPU_MAJOR_ID_TL1,
	__MESON_CPU_MAJOR_ID_SM1,
	__MESON_CPU_MAJOR_ID_TM2,
	__MESON_CPU_MAJOR_ID_A1,
	__MESON_CPU_MAJOR_ID_SC2,
	__MESON_CPU_MAJOR_ID_T5,
	__MESON_CPU_MAJOR_ID_T5D,
	__MESON_CPU_MAJOR_ID_S4,
	__MESON_CPU_MAJOR_ID_T7,
	__MESON_CPU_MAJOR_ID_T3,
	__MESON_CPU_MAJOR_ID_T5W,
	__MESON_CPU_MAJOR_ID_S5,
	__MESON_CPU_MAJOR_ID_UNKNOWN,
};

enum osd_afbc_e {
	NO_AFBC = 0,
	MESON_AFBC,
	MALI_AFBC
};

enum osd_ver_e {
	OSD_SIMPLE = 0,
	OSD_NORMAL,
	OSD_HIGH_ONE,
	OSD_NONE,
	OSD_HIGH_OTHER
};

enum osd_blend_din_index_e {
	BLEND_DIN1 = 0,
	BLEND_DIN2,
	BLEND_DIN3,
	BLEND_DIN4,
	BLEND0_DIN,
	BLEND1_DIN,
	BLEND2_DIN,
	BLEND_NO_DIN,
};

enum vpp_blend_input_e {
	POSTBLD_CLOSE = 0,
	POSTBLD_VD1,
	POSTBLD_VD2,
	POSTBLD_OSD1,
	POSTBLD_OSD2,
};

enum vpp_blend_input_t7_e {
	POSTBLD_CLOSE_T7 = 0,
	POSTBLD_VD1_T7,
	POSTBLD_VD2_T7,
	POSTBLD_VD3_T7,
	POSTBLD_OSD1_T7,
	POSTBLD_OSD2_T7,
};

enum osd_zorder_e {
	LAYER_1 = 1,
	LAYER_2,
	LAYER_3,
	LAYER_4,
	LAYER_UNSUPPORT
};

/*
 * OSD_BLEND_ABC: (OSD1 & (OSD2+SC & OSD3+SC)) +SC
 * OSD_BLEND_AB_C: (OSD1 & OSD2 + SC) + SC, OSD3+SC
 * OSD_BLEND_A_BC: OSD1+SC, (OSD2 +SC & OSD3 +SC)
 * OSD_BLEND_AB_C: ((OSD+ (OSD2+SC)) + SC) & (OSD3+SC)
 */
enum osd_blend_mode_e {
	OSD_BLEND_NONE,
	OSD_BLEND_A,
	OSD_BLEND_AC,
	OSD_BLEND_A_C,
	OSD_BLEND_ABC,
	OSD_BLEND_A_BC,
	OSD_BLEND_AB_C,
	OSD_BLEND_ABCD,
	OSD_BLEND_AB_CD,
	OSD_BLEND_A_2SLICE
};

enum afbc_pix_format_e {
	RGB565 = 0,
	RGBA5551,
	RGBA1010102,
	YUV420_10B,
	RGB888,
	RGBA8888,
	RGBA4444,
	R8,
	RG88,
	YUV420_8B,
	YUV422_8B = 11,
	YUV422_10B = 14,
};

enum viu2_rotate_format {
	YUV422 = 4,
	RGB = 6,
	RGBA = 8,
};

enum viu_type {
	VIU1,
	VIU2,
	VIU3,
};

enum vpp_mux_input_type {
	VPP_DIN_NONE = 0,
	VPP_OSD1,
	VPP_OSD2,
	VPP_OSD3,
	VPP_OSD4,
};

enum vpp_vsync_type {
	VPU_VPP0,
	VPU_VPP1,
	VPU_VPP2,
	VPU_VPP_MAX
};

enum render_cmd_type {
	LAYER_SYNC,
	BLANK_CMD,
	PAGE_FLIP,
};

struct pandata_s {
	s32 x_start;
	s32 x_end;
	s32 y_start;
	s32 y_end;
};

struct dispdata_s {
	s32 x;
	s32 y;
	s32 w;
	s32 h;
};

struct fb_geometry_s {
	u32 width;  /* in byte unit */
	u32 height;
	u32 canvas_idx;
	ulong addr;
	u32 xres;
	u32 yres;
};

struct osd_scale_s {
	u16 h_enable;
	u16 v_enable;
};

struct osd_3d_mode_s {
	struct osd_scale_s origin_scale;
	u16 enable;
	u16 left_right;
	u16 l_start;
	u16 l_end;
	u16 r_start;
	u16 r_end;
};

struct osd_rotate_s {
	u32 on_off;
	u32 angle;
};

struct osd_fence_map_s {
	struct list_head list;
	u32 fb_index;
	u32 buf_num;
	u32 xoffset;
	u32 yoffset;
	u32 yres;
	s32 in_fd;
	s32 out_fd;
	ulong ext_addr;
	u32 format;
	u32 width;
	u32 height;
	u32 op;
	u32 compose_type;
	u32 dst_x;
	u32 dst_y;
	u32 dst_w;
	u32 dst_h;
	int byte_stride;
	int pxiel_stride;
	u32 zorder;
	u32 blend_mode;
	u32 plane_alpha;
	u32 afbc_inter_format;
	u32 background_w;
	u32 background_h;
	size_t afbc_len;
	struct dma_fence *in_fence;
};

struct layer_fence_map_s {
	u32 fb_index;
	u32 enable;
	s32 in_fd;
	s32 out_fd;
	ulong ext_addr;
	u32 format;
	u32 compose_type;
	u32 fb_width;
	u32 fb_height;
	u32 src_x;
	u32 src_y;
	u32 src_w;
	u32 src_h;
	u32 dst_x;
	u32 dst_y;
	u32 dst_w;
	u32 dst_h;
	int byte_stride;
	int pxiel_stride;
	u32 afbc_inter_format;
	u32 zorder;
	u32 blend_mode;
	u32 plane_alpha;
	u32 dim_layer;
	u32 dim_color;
	u32 secure_enable;
	size_t afbc_len;
	struct file *buf_file;
	struct dma_fence *in_fence;
};

struct osd_layers_fence_map_s {
	struct list_head list;
	int out_fd;
	unsigned char hdr_mode;
	unsigned char cmd;
	struct display_flip_info_s disp_info;
	struct layer_fence_map_s layer_map[HW_OSD_COUNT];
	u32 inc_cnt; /* increase timeline count */
};

struct afbcd_data_s {
	u32 enable;
	ulong phy_addr;
	ulong addr[OSD_MAX_BUF_NUM];
	u32 frame_width;
	u32 frame_height;
	u32 conv_lbuf_len;
	u32 out_addr_id;
	u32 format;
	u32 inter_format;
	u32 afbc_start;
};

struct osd_device_data_s {
	enum cpuid_type_e cpu_id;
	enum osd_ver_e osd_ver; /* axg: simple, others: normal; g12: high */
	enum osd_afbc_e afbc_type;/* 0:no afbc; 1: meson-afbc 2:mali-afbc */
	u8 osd_count;
	u8 has_deband;
	u8 has_lut;
	u8 has_rdma;
	u8 has_dolby_vision;
	u8 osd_fifo_len;
	u32 vpp_fifo_len;
	u32 dummy_data;
	u32 has_viu2;
	u32 osd0_sc_independ;
	u32 osd_rgb2yuv;
	u32 viu1_osd_count;
	u32 viu2_index;
	u32 mif_linear;
	u32 has_vpp1;
	u32 has_vpp2;
	u32 has_pi;
	u32 has_slice2ppc;
	struct clk *vpu_clkc;
};

struct osd_device_hw_s {
	u32 t7_display;
	u32 has_8G_addr;
	u32 multi_afbc_core;
	u32 has_multi_vpp;
	u32 new_blend_bypass;
	u32 path_ctrl_independ;
	u32 remove_afbc;
	u32 remove_pps;
	u32 prevsync_support;
	u32 s5_display;
};

struct hw_osd_reg_s {
	u32 osd_ctrl_stat; /* VIU_OSD1_CTRL_STAT */
	u32 osd_ctrl_stat2;/* VIU_OSD1_CTRL_STAT2 */
	u32 osd_color_addr;/* VIU_OSD1_COLOR_ADDR */
	u32 osd_color;/* VIU_OSD1_COLOR */
	u32 osd_tcolor_ag0; /* VIU_OSD1_TCOLOR_AG0 */
	u32 osd_tcolor_ag1; /* VIU_OSD1_TCOLOR_AG1 */
	u32 osd_tcolor_ag2; /* VIU_OSD1_TCOLOR_AG2 */
	u32 osd_tcolor_ag3; /* VIU_OSD1_TCOLOR_AG3 */
	u32 osd_blk0_cfg_w0;/* VIU_OSD1_BLK0_CFG_W0 */
	u32 osd_blk0_cfg_w1;/* VIU_OSD1_BLK0_CFG_W1 */
	u32 osd_blk0_cfg_w2;/* VIU_OSD1_BLK0_CFG_W2 */
	u32 osd_blk0_cfg_w3;/* VIU_OSD1_BLK0_CFG_W3 */
	u32 osd_blk0_cfg_w4;/* VIU_OSD1_BLK0_CFG_W4 */
	u32 osd_blk1_cfg_w4;/* VIU_OSD1_BLK1_CFG_W4 */
	u32 osd_blk2_cfg_w4;/* VIU_OSD1_BLK2_CFG_W4 */
	u32 osd_fifo_ctrl_stat;/* VIU_OSD1_FIFO_CTRL_STAT */
	u32 osd_test_rddata;/* VIU_OSD1_TEST_RDDATA */
	u32 osd_prot_ctrl;/* VIU_OSD1_PROT_CTRL */
	u32 osd_mali_unpack_ctrl;/* VIU_OSD1_MALI_UNPACK_CTRL */
	u32 osd_dimm_ctrl;/* VIU_OSD1_DIMM_CTRL */
	//u32 osd_blend_din_scope_h; /* VIU_OSD_BLEND_DIN0_SCOPE_H */
	//u32 osd_blend_din_scope_v; /* VIU_OSD_BLEND_DIN0_SCOPE_V */
	u32 osd_matrix_en_ctrl;

	u32 osd_scale_coef_idx;/* VPP_OSD_SCALE_COEF_IDX */
	u32 osd_scale_coef;/* VPP_OSD_SCALE_COEF */
	u32 osd_vsc_phase_step;/* VPP_OSD_VSC_PHASE_STEP */
	u32 osd_vsc_init_phase;/* VPP_OSD_VSC_INI_PHASE */
	u32 osd_vsc_ctrl0;/* VPP_OSD_VSC_CTRL0 */
	u32 osd_hsc_phase_step;/* VPP_OSD_HSC_PHASE_STEP */
	u32 osd_hsc_init_phase;/* VPP_OSD_HSC_INI_PHASE */
	u32 osd_hsc_ctrl0;/* VPP_OSD_HSC_CTRL0 */
	u32 osd_hsc_ini_pat_ctrl; /* OSDx_PROC_HSC_CTRL0 */
	u32 osd_sc_dummy_data;/* VPP_OSD_SC_DUMMY_DATA */
	u32 osd_sc_ctrl0;/* VPP_OSD_SC_CTRL0 */
	u32 osd_sci_wh_m1;/* VPP_OSD_SCI_WH_M1 */
	u32 osd_sco_h_start_end;/* VPP_OSD_SCO_H_START_END */
	u32 osd_sco_v_start_end;/* VPP_OSD_SCO_V_START_END */
	u32 osd_db_flt_ctrl;
	u32 afbc_header_buf_addr_low_s;/* VPU_MAFBC_HEADER_BUF_ADDR_LOW_S0 */
	u32 afbc_header_buf_addr_high_s;/* VPU_MAFBC_HEADER_BUF_ADDR_HIGH_S0 */
	u32 afbc_format_specifier_s;/* VPU_MAFBC_FORMAT_SPECIFIER_S0 */
	u32 afbc_buffer_width_s;/* VPU_MAFBC_BUFFER_WIDTH_S0 */
	u32 afbc_buffer_hight_s;/* VPU_MAFBC_BUFFER_HEIGHT_S0 */
	u32 afbc_boundings_box_x_start_s;/* VPU_MAFBC_BOUNDING_BOX_X_START_S0 */
	u32 afbc_boundings_box_x_end_s;/* VPU_MAFBC_BOUNDING_BOX_X_END_S0 */
	u32 afbc_boundings_box_y_start_s;/* VPU_MAFBC_BOUNDING_BOX_Y_START_S0 */
	u32 afbc_boundings_box_y_end_s;/* VPU_MAFBC_BOUNDING_BOX_Y_END_S0 */
	u32 afbc_output_buf_addr_low_s;/* VPU_MAFBC_OUTPUT_BUF_ADDR_LOW_S0 */
	u32 afbc_output_buf_addr_high_s;/* VPU_MAFBC_OUTPUT_BUF_ADDR_HIGH_S0 */
	u32 afbc_output_buf_stride_s;/* VPU_MAFBC_OUTPUT_BUF_STRIDE_S0 */
	u32 afbc_prefetch_cfg_s;/* VPU_MAFBC_PREFETCH_CFG_S0 */

	u32 mali_afbcd_top_ctrl; /* MALI_AFBCD_TOP_CTRL */
	u32 vpu_mafbc_irq_mask; /* VPU_MAFBC_IRQ_MASK */
	u32 vpu_mafbc_surface_cfg; /* VPU_MAFBC_SURFACE_CFG */
	u32 vpu_mafbc_command; /* VPU_MAFBC_COMMAND */
	u32 vpu_mafbc_irq_raw_status; /* VPU_MAFBC_IRQ_RAW_STATUS */
	u32 vpu_mafbc_irq_clear; /* VPU_MAFBC_IRQ_CLEAR */

	u32 vpp_osd1_scale_ctrl; /* VPP_OSD1_SCALE_CTRL */
	u32 osd_proc_in_size;    /* OSD1_PROC_IN_SIZE */
	u32 osd_proc_out_size;   /* OSD1_PROC_OUT_SIZE */
};

struct hw_osd_blend_reg_s {
	/* osd_blend */
	u32 osd_blend_ctrl;         /* VIU_OSD_BLEND_CTRL */
	u32 osd_blend_ctrl1;        /* VIU_OSD_BLEND_CTRL1 */
	u32 osd_blend_din0_scope_h; /* VIU_OSD_BLEND_DIN0_SCOPE_H */
	u32 osd_blend_din0_scope_v; /* VIU_OSD_BLEND_DIN0_SCOPE_V */
	u32 osd_blend_din1_scope_h; /* VIU_OSD_BLEND_DIN1_SCOPE_H */
	u32 osd_blend_din1_scope_v; /* VIU_OSD_BLEND_DIN1_SCOPE_V */
	u32 osd_blend_din2_scope_h; /* VIU_OSD_BLEND_DIN2_SCOPE_H */
	u32 osd_blend_din2_scope_v; /* VIU_OSD_BLEND_DIN2_SCOPE_V */
	u32 osd_blend_din3_scope_h; /* VIU_OSD_BLEND_DIN3_SCOPE_H */
	u32 osd_blend_din3_scope_v; /* VIU_OSD_BLEND_DIN3_SCOPE_V */
	u32 osd_blend_dummy_data0;  /* VIU_OSD_BLEND_DUMMY_DATA0 */
	u32 osd_blend_dummy_alpha;  /* VIU_OSD_BLEND_DUMMY_ALPHA */
	u32 osd_blend_blend0_size;  /* VIU_OSD_BLEND_BLEND0_SIZE */
	u32 osd_blend_blend1_size;  /* VIU_OSD_BLEND_BLEND1_SIZE */
	u32 osd_blend_dout0_size;   /* OSD_BLEND_DOUT0_SIZE */
	u32 osd_blend_dout1_size;   /* OSD_BLEND_DOUT1_SIZE */

	/* vpp_post_blend related */
	u32 vpp_osd1_bld_h_scope; /* VPP_OSD1_BLD_H_SCOPE */
	u32 vpp_osd1_bld_v_scope; /* VPP_OSD1_BLD_V_SCOPE */
	u32 vpp_osd2_bld_h_scope; /* VPP_OSD2_BLD_H_SCOPE */
	u32 vpp_osd2_bld_v_scope; /* VPP_OSD2_BLD_V_SCOPE */
	u32 vd1_blend_src_ctrl;   /* VD1_BLEND_SRC_CTRL */
	u32 vd2_blend_src_ctrl;   /* VD2_BLEND_SRC_CTRL */
	u32 vd3_blend_src_ctrl;   /* VD3_BLEND_SRC_CTRL */
	u32 osd1_blend_src_ctrl;  /* OSD1_BLEND_SRC_CTRL */
	u32 osd2_blend_src_ctrl;  /* OSD2_BLEND_SRC_CTRL */
};

struct hw_osd_slice2ppc_reg_s {
	u32 osd_2slice2ppc_in_size;  /* OSD_2SLICE2PPC_IN_SIZE */
	u32 osd_2slice2ppc_mode;     /* OSD_2SLICE2PPC_MODE */
	u32 osd_sys_hwin0_cut;       /* OSD_SYS_HWIN0_CUT */
	u32 osd_sys_hwin1_cut;       /* OSD_SYS_HWIN1_CUT */
	u32 osd_sys_pad_ctrl;        /* OSD_SYS_PAD_CTRL */
	u32 osd_sys_pad_dummy_data0; /* OSD_SYS_PAD_DUMMY_DATA0 */
	u32 osd_sys_pad_dummy_data1; /* OSD_SYS_PAD_DUMMY_DATA1 */
	u32 osd_sys_pad_h_size;      /* OSD_SYS_PAD_H_SIZE */
	u32 osd_sys_pad_v_size;      /* OSD_SYS_PAD_V_SIZE */
	u32 osd_sys_2slice_hwin_cut; /* OSD_SYS_2SLICE_HWIN_CUT */
};

enum hwin_cut_e {
	HWIN_IN_0,
	HWIN_IN_1,
	HWIN_OUT,
	HWIN_NUM
};

enum slice_position_e {
	SLICE_LEFT,
	SLICE_RIGHT,
	SLICE_NUM,
	SLICE_NONE
};

struct hw_osd_slice2ppc_s {
	/* 2slice register values */
	u32 slice2ppc_hsize; /* hsize */
	u32 slice2ppc_vsize; /* vsize */
	u32 slice2ppc_mode;  /* 0: 2slice to 2ppc  1: 1slice to 2ppc */

	u32 hwincut_en[HWIN_NUM];
	u32 hwincut_bgn[HWIN_NUM];
	u32 hwincut_end[HWIN_NUM];

	u32 pad_en;
	u32 pad_h_bgn;
	u32 pad_h_end;
	u32 pad_v_bgn;
	u32 pad_v_end;

	/* values for calculation */
	u32 in_hsize_total;
	u32 in_vsize_total;
	u32 out_hsize_total;
	u32 out_vsize_total;
	u32 scaler_in_hsize[OSD_MAX];
	u32 scaler_in_vsize[OSD_MAX];
	u32 scaler_out_hsize[OSD_MAX];
	u32 scaler_out_vsize[OSD_MAX];
	u32 slice_in_x_start[OSD_MAX];
	u32 slice_in_x_end[OSD_MAX];
	u32 slice_in_y_start[OSD_MAX];
	u32 slice_in_y_end[OSD_MAX];
	enum slice_position_e slice_pos[OSD_MAX];
};

struct osd_blend_reg_s {
	u32 hold_line;
	u32 blend2_premult_en;
	u32 din0_byp_blend;
	u32 din2_osd_sel;
	u32 din3_osd_sel;
	u32 blend_din_en;
	u32 din_premult_en;
	u32 din_reoder_sel;
	u32 osd_blend_din_scope_h[OSD_BLEND_LAYERS];
	u32 osd_blend_din_scope_v[OSD_BLEND_LAYERS];
	u32 osd_blend_blend0_size;
	u32 osd_blend_blend1_size;
	u32 osd_bld_out0_premult;
	u32 osd_bld_out1_premult;
};

struct vpp0_blend_reg_s {
	/* pre blend */
	u32 prebld_src3_sel;
	u32 prebld_osd1_premult;
	u32 prebld_src4_sel;
	u32 prebld_osd2_premult;

	/* post blend */
	u32 postbld_src3_sel;
	u32 postbld_osd1_premult;
	u32 postbld_src4_sel;
	u32 postbld_osd2_premult;

	u32 osd1_index;
	u32 osd1_h_start;
	u32 osd1_h_end;
	u32 osd1_v_start;
	u32 osd1_v_end;
	u32 osd2_index;
	u32 osd2_h_start;
	u32 osd2_h_end;
	u32 osd2_v_start;
	u32 osd2_v_end;
};

struct vpp1_blend_reg_s {
	/* pre blend */
	u32 prebld_src3_sel;
	u32 prebld_osd1_premult;
	u32 prebld_src4_sel;
	u32 prebld_osd2_premult;
	/* post blend */
	u32 osd1_index;
	u32 osd1_h_start;
	u32 osd1_h_end;
	u32 osd1_v_start;
	u32 osd1_v_end;
	u32 osd2_index;
	u32 osd2_h_start;
	u32 osd2_h_end;
	u32 osd2_v_start;
	u32 osd2_v_end;
};

struct hw_vppx_blend_reg_s {
	u32 vpp_bld_din1_hscope;
	u32 vpp_bld_din1_vscope;
	u32 vpp_bld_out_size;
	u32 vpp_bld_ctrl;
	u32 vpp_bld_dummy_data;
	u32 vpp_bld_dummy_alpha;
};

struct layer_blend_s {
	u8 input1;
	u8 input2;
	struct dispdata_s input1_data;
	struct dispdata_s input2_data;
	struct dispdata_s output_data;
	u32 blend_core1_bypass;
};

struct hw_osd_blending_s {
	u8 osd_blend_mode;
	u8 osd_to_bdin_table[OSD_BLEND_LAYERS];
	u8 reorder[HW_OSD_COUNT];
	u8 blend_din;
	u32 din_reoder_sel;
	u32 layer_cnt;
	u32 b_exchange_din;
	u32 b_exchange_blend_in;
	u32 osd1_freescale_used;
	u32 osd1_freescale_disable;
	u32 vinfo_width;
	u32 vinfo_height;
	u32 screen_ratio_w_num;
	u32 screen_ratio_w_den;
	u32 screen_ratio_h_num;
	u32 screen_ratio_h_den;
	struct dispdata_s dst_data;
	struct osd_blend_reg_s osd_blend_reg;
	struct vpp0_blend_reg_s vpp0_blend_reg;
	struct vpp1_blend_reg_s vpp1_blend_reg;

	struct layer_blend_s layer_blend;
};

extern struct hw_osd_reg_s hw_osd_reg_array[HW_OSD_COUNT];
extern struct hw_osd_blend_reg_s hw_osd_reg_blend;
typedef void (*update_func_t)(u32);
struct hw_list_s {
	struct list_head list;
	update_func_t update_func;
};

typedef int (*sync_render_fence)(u32 index, u32 yres,
	struct sync_req_render_s *request,
	ulong phys_addr,
	size_t len);
typedef void (*osd_toggle_buffer_op)(struct kthread_work *work);
struct osd_fence_fun_s {
	sync_render_fence sync_fence_handler;
	osd_toggle_buffer_op toggle_buffer_handler;
};

typedef u32 (*osd_rdma_rd_op)(u32 reg);
typedef int (*osd_rdma_wr_op)(u32 reg, u32 val);
typedef int (*osd_rdma_wr_bits_op)(u32 reg, u32 val, u32 start, u32 len);
typedef int (*osd_rdma_set_mask_op)(u32 reg, u32 mask);
typedef int (*osd_rdma_clr_mask_op)(u32 reg, u32 mask);
typedef int (*osd_rdma_wr_irq_op)(u32 reg, u32 val);

struct osd_rdma_fun_s {
	osd_rdma_rd_op osd_rdma_rd;
	osd_rdma_wr_op osd_rdma_wr;
	osd_rdma_wr_bits_op osd_rdma_wr_bits;
	osd_rdma_set_mask_op osd_rdma_set_mask;
	osd_rdma_clr_mask_op osd_rdma_clr_mask;
	osd_rdma_wr_irq_op osd_rdma_wr_irq;
};

struct layer_info_s {
	int enable;
	ulong ext_addr;
	unsigned int    src_x;
	unsigned int    src_y;
	unsigned int	src_w;
	unsigned int	src_h;
	unsigned int    dst_x;
	unsigned int    dst_y;
	unsigned int    dst_w;
	unsigned int    dst_h;
	unsigned int  zorder;
	unsigned int  blend_mode;
	unsigned char  plane_alpha;
	unsigned char  dim_layer;
	unsigned int  dim_color;
};

struct osd_debug_backup_s {
	struct layer_info_s layer[HW_OSD_COUNT];
	struct osd_blend_reg_s osd_blend_reg;
	struct vpp0_blend_reg_s vpp0_blend_reg;
};

struct hw_debug_s {
	u32 wait_fence_release;
	u32 osd_single_step_mode;
	u32 osd_single_step;
	int backup_count;
	struct osd_debug_backup_s osd_backup[OSD_BACKUP_COUNT];
};

struct viu2_osd_reg_item {
	u32 addr;
	u32 val;
	u32 mask;
};

struct ffile_info {
	u32 fget_count;
	u32 fput_count;
};

struct hw_para_s {
	struct pandata_s pandata[HW_OSD_COUNT];
	struct pandata_s dispdata[HW_OSD_COUNT];
	struct pandata_s dispdata_backup[HW_OSD_COUNT];
	struct pandata_s scaledata[HW_OSD_COUNT];
	struct pandata_s free_src_data[HW_OSD_COUNT];
	struct pandata_s free_dst_data[HW_OSD_COUNT];
	struct pandata_s free_src_data_backup[HW_OSD_COUNT];
	struct pandata_s free_dst_data_backup[HW_OSD_COUNT];
	/* struct pandata_s rotation_pandata[HW_OSD_COUNT]; */
	struct pandata_s cursor_dispdata[HW_OSD_COUNT];
	struct dispdata_s src_data[HW_OSD_COUNT];
	struct dispdata_s dst_data[HW_OSD_COUNT];
	u32 src_crop[HW_OSD_COUNT];
	u32 buffer_alloc[HW_OSD_COUNT];
	u32 gbl_alpha[HW_OSD_COUNT];
	u32 color_key[HW_OSD_COUNT];
	u32 color_key_enable[HW_OSD_COUNT];
	u32 enable[HW_OSD_COUNT];
	u32 enable_save[HW_OSD_COUNT];
	u32 powered[HW_OSD_COUNT];
	u32 reg_status_save;
	u32 reg_status_save1;
	u32 reg_status_save2;
	u32 reg_status_save3[2];
	u32 reg_status_save4;
#ifdef FIQ_VSYNC
	bridge_item_t fiq_handle_item;
#endif
	struct osd_scale_s scale[HW_OSD_COUNT];
	struct osd_scale_s free_scale[HW_OSD_COUNT];
	struct osd_scale_s free_scale_backup[HW_OSD_COUNT];
	u32 free_scale_enable[HW_OSD_COUNT];
	u32 free_scale_enable_backup[HW_OSD_COUNT];
	struct fb_geometry_s fb_gem[HW_OSD_COUNT];
	const struct color_bit_define_s *color_info[HW_OSD_COUNT];
	const struct color_bit_define_s *color_backup[HW_OSD_COUNT];
	u32 scan_mode[HW_OSD_COUNT];
	u32 order[HW_OSD_COUNT];
	u32 premult_en[HW_OSD_COUNT];
	struct display_flip_info_s disp_info[VIU_COUNT];
	struct osd_3d_mode_s mode_3d[HW_OSD_COUNT];
	u32 updated[HW_OSD_COUNT];
	/* u32 block_windows[HW_OSD_COUNT][HW_OSD_BLOCK_REG_COUNT]; */
	u32 block_mode[HW_OSD_COUNT];
	u32 free_scale_mode[HW_OSD_COUNT];
	u32 free_scale_mode_backup[HW_OSD_COUNT];
	u32 osd_reverse[HW_OSD_COUNT];
	u32 osd_rotate[HW_OSD_COUNT];
	u32 force_dimm[HW_OSD_COUNT];
	u32 dim_layer[HW_OSD_COUNT];
	u32 dim_color[HW_OSD_COUNT];
	/* struct osd_rotate_s rotate[HW_OSD_COUNT]; */
	int use_h_filter_mode[HW_OSD_COUNT];
	int use_v_filter_mode[HW_OSD_COUNT];
	struct hw_list_s reg[HW_REG_INDEX_MAX];
	u32 field_out_en[VIU_COUNT];
	u32 scale_workaround;
	u32 fb_for_4k2k;
	u32 antiflicker_mode;
	u32 angle[HW_OSD_COUNT];
	u32 clone[HW_OSD_COUNT];
	u32 bot_type;
	u32 hw_reset_flag;
	struct afbcd_data_s osd_afbcd[HW_OSD_COUNT];
	struct osd_device_data_s osd_meson_dev;
	u32 urgent[HW_OSD_COUNT];
	u32 osd_deband_enable[HW_OSD_COUNT];
	u32 osd_fps[VIU_COUNT];
	u32 osd_fps_start[VIU_COUNT];
	u32 osd_display_debug[VIU_COUNT];
	u32 osd_display_fb[VIU_COUNT];
	ulong screen_base[HW_OSD_COUNT];
	ulong screen_size[HW_OSD_COUNT];
	ulong screen_base_backup[HW_OSD_COUNT];
	ulong screen_size_backup[HW_OSD_COUNT];
	u32 vinfo_width[VIU_COUNT];
	u32 vinfo_height[VIU_COUNT];
	u32 fb_drvier_probe;
	u32 afbc_force_reset;
	u32 afbc_regs_backup;
	u32 afbc_status_err_reset;
	u32 afbc_use_latch;
	u32 hwc_enable[VIU_COUNT];
	u32 osd_use_latch[HW_OSD_COUNT];
	u32 hw_cursor_en;
	u32 hw_rdma_en;
	u32 blend_bypass[HW_OSD_COUNT];
	u32 hdr_used;
	u32 workaround_line;
	u32 basic_urgent;
	u32 two_ports;
	u32 afbc_err_cnt[HW_OSD_COUNT];
	u32 viu_type;
	u32 line_n_rdma;
	u32 osd_preblend_en; /* only for viu1 */
	u32 fix_target_width;
	u32 fix_target_height;
	u32 preblend_pps_w_den;
	u32 preblend_pps_h_den;
	u32 preblend_pps_w_num;
	u32 preblend_pps_h_num;
	u32 adjust_position_x;
	u32 adjust_position_y;
	u32 preblend_y_offset;
	u32 rdma_trace_enable;
	u32 rdma_trace_num;
	u32 rdma_trace_reg[MAX_TRACE_NUM];
	u32 osd_v_skip[HW_OSD_COUNT];
	u32 secure_enable[HW_OSD_COUNT];
	u32 secure_src;
	u32 rdma_delayed_cnt1;
	u32 rdma_delayed_cnt2;
	u32 rdma_delayed_cnt3;
	u32 osd_reg_check;
	u32 mif_linear;
	u32 vpp_num;
	u32 display_dev_cnt;
	struct hw_debug_s osd_debug;
	int out_fence_fd[VIU_COUNT];
	int in_fd[HW_OSD_COUNT];
	struct osd_fence_fun_s osd_fence[VIU_COUNT][2];
	struct osd_rdma_fun_s osd_rdma_func[VIU_COUNT];
	u32 viu_osd_table[VIU_COUNT];
	u32 blend_mode[HW_OSD_COUNT];
	bool afbc_support[HW_OSD_COUNT];
	bool pps_support[HW_OSD_COUNT];
	u32 force_save_frame;
	u32 cur_frame_count;
	u32 save_frame_number[HW_OSD_COUNT];
	struct ffile_info file_info_debug[HW_OSD_COUNT];
	struct pandata_s pi_out;
	u32 pi_enable;
	u32 slice2ppc_enable;
};
#endif /* _OSD_H_ */
