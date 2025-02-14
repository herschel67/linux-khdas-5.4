/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __TVIN_H
#define __TVIN_H

#include <linux/types.h>
#include <linux/amlogic/media/amvecm/cm.h>
#include <linux/dvb/frontend.h>

enum {
	MEMP_VDIN_WITHOUT_3D = 0,
	MEMP_VDIN_WITH_3D,
	MEMP_DCDR_WITHOUT_3D,
	MEMP_DCDR_WITH_3D,
	MEMP_ATV_WITHOUT_3D,
	MEMP_ATV_WITH_3D,
};

enum adc_sel {
	ADC_ATV_DEMOD = 1,
	ADC_TVAFE = 2,
	ADC_DTV_DEMOD = 4,
	ADC_DTV_DEMODPLL = 8,
	ADC_MAX,
};

enum filter_sel {
	FILTER_ATV_DEMOD = 1,
	FILTER_TVAFE = 2,
	FILTER_DTV_DEMOD = 4,
	FILTER_DTV_DEMODT2 = 8,
	FILTER_MAX,
};

/* *********************************************************************** */

/* * TVIN general definition/enum/struct *********************************** */

/* ************************************************************************ */

/* tvin input port select */
enum tvin_port_e {
	TVIN_PORT_NULL = 0x00000000,
	TVIN_PORT_MPEG0 = 0x00000100,
	TVIN_PORT_BT656 = 0x00000200,
	TVIN_PORT_BT601,
	TVIN_PORT_CAMERA,
	TVIN_PORT_BT656_HDMI,
	TVIN_PORT_BT601_HDMI,
	TVIN_PORT_CVBS0 = 0x00001000,
	TVIN_PORT_CVBS1,
	TVIN_PORT_CVBS2,
	TVIN_PORT_CVBS3,
	TVIN_PORT_HDMI0 = 0x00004000,
	TVIN_PORT_HDMI1,
	TVIN_PORT_HDMI2,
	TVIN_PORT_HDMI3,
	TVIN_PORT_HDMI4,
	TVIN_PORT_HDMI5,
	TVIN_PORT_HDMI6,
	TVIN_PORT_HDMI7,
	TVIN_PORT_DVIN0 = 0x00008000,
	TVIN_PORT_VIU1 = 0x0000a000,
	TVIN_PORT_VIU1_VIDEO, /* vpp0 preblend vd1 */
	TVIN_PORT_VIU1_WB0_VD1, /* vpp0 vadj1 output */
	TVIN_PORT_VIU1_WB0_VD2, /* vpp0 vd2 postblend input */
	TVIN_PORT_VIU1_WB0_OSD1, /* vpp0 osd1 postblend input */
	TVIN_PORT_VIU1_WB0_OSD2, /* vpp0 osd2 postblend input */
	TVIN_PORT_VIU1_WB0_VPP, /* vpp0 output */
	TVIN_PORT_VIU1_WB0_POST_BLEND, /* vpp0 postblend output */
	TVIN_PORT_VIU1_WB1_VDIN_BIST,
	TVIN_PORT_VIU1_WB1_VIDEO,
	TVIN_PORT_VIU1_WB1_VD1,
	TVIN_PORT_VIU1_WB1_VD2,
	TVIN_PORT_VIU1_WB1_OSD1,
	TVIN_PORT_VIU1_WB1_OSD2,
	TVIN_PORT_VIU1_WB1_VPP,
	TVIN_PORT_VIU1_WB1_POST_BLEND,
	TVIN_PORT_VIU2 = 0x0000C000,
	TVIN_PORT_VIU2_ENCL,
	TVIN_PORT_VIU2_ENCI,
	TVIN_PORT_VIU2_ENCP,
	TVIN_PORT_VIU2_VD1, /* vpp1 vd1 output */
	TVIN_PORT_VIU2_OSD1, /* vpp1 osd1 output */
	TVIN_PORT_VIU2_VPP, /* vpp1 output */
	TVIN_PORT_VIU3 = 0x0000D000,
	TVIN_PORT_VIU3_VD1, /* vpp2 vd1 output */
	TVIN_PORT_VIU3_OSD1, /* vpp2 osd1 output */
	TVIN_PORT_VIU3_VPP, /* vpp2 output */
	TVIN_PORT_VENC = 0x0000E000,
	TVIN_PORT_VENC0,
	TVIN_PORT_VENC1,
	TVIN_PORT_VENC2,
	TVIN_PORT_MIPI = 0x00010000,
	TVIN_PORT_ISP = 0x00020000,
	TVIN_PORT_MAX = 0x80000000,
};

enum viu_mux_port {
	VIU_MUX_SEL_ENCI = 1,
	VIU_MUX_SEL_ENCP = 2,
	VIU_MUX_SEL_ENCL = 4,
	VIU_MUX_SEL_WB0 = 8,
	VIU_MUX_SEL_WB1 = 16,
	VIU_MUX_SEL_WB2 = 32,/*t7*/
	VIU_MUX_SEL_WB3 = 64,/*t7*/
};

enum wb_chan_sel {
	WB_CHAN_DISABLE = 0,
	WB_CHAN_SEL_POST_BLD_VD1 = 1,
	WB_CHAN_SEL_POST_BLD_VD2 = 2,
	WB_CHAN_SEL_POST_OSD1 = 3,
	WB_CHAN_SEL_POST_OSD2 = 4,
	WB_CHAN_SEL_POST_DOUT = 5,
	WB_CHAN_SEL_VPP_DOUT = 6,
	WB_CHAN_SEL_PRE_BLD_VD1 = 7,/*t7*/
	WB_CHAN_SEL_POST_VD3 = 8,/*t7*/
};

const char *tvin_port_str(enum tvin_port_e port);

/* tvin signal format table */
enum tvin_sig_fmt_e {
	TVIN_SIG_FMT_NULL = 0,
	/* HDMI Formats */
	TVIN_SIG_FMT_HDMI_640X480P_60HZ = 0x401,
	TVIN_SIG_FMT_HDMI_720X480P_60HZ = 0x402,
	TVIN_SIG_FMT_HDMI_1280X720P_60HZ = 0x403,
	TVIN_SIG_FMT_HDMI_1920X1080I_60HZ = 0x404,
	TVIN_SIG_FMT_HDMI_1440X480I_60HZ = 0x405,
	TVIN_SIG_FMT_HDMI_1440X240P_60HZ = 0x406,
	TVIN_SIG_FMT_HDMI_2880X480I_60HZ = 0x407,
	TVIN_SIG_FMT_HDMI_2880X240P_60HZ = 0x408,
	TVIN_SIG_FMT_HDMI_1440X480P_60HZ = 0x409,
	TVIN_SIG_FMT_HDMI_1920X1080P_60HZ = 0x40a,
	TVIN_SIG_FMT_HDMI_720X576P_50HZ = 0x40b,
	TVIN_SIG_FMT_HDMI_1280X720P_50HZ = 0x40c,
	TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A = 0x40d,
	TVIN_SIG_FMT_HDMI_1440X576I_50HZ = 0x40e,
	TVIN_SIG_FMT_HDMI_1440X288P_50HZ = 0x40f,
	TVIN_SIG_FMT_HDMI_2880X576I_50HZ = 0x410,
	TVIN_SIG_FMT_HDMI_2880X288P_50HZ = 0x411,
	TVIN_SIG_FMT_HDMI_1440X576P_50HZ = 0x412,
	TVIN_SIG_FMT_HDMI_1920X1080P_50HZ = 0x413,
	TVIN_SIG_FMT_HDMI_1920X1080P_24HZ = 0x414,
	TVIN_SIG_FMT_HDMI_1920X1080P_25HZ = 0x415,
	TVIN_SIG_FMT_HDMI_1920X1080P_30HZ = 0x416,
	TVIN_SIG_FMT_HDMI_2880X480P_60HZ = 0x417,
	TVIN_SIG_FMT_HDMI_2880X576P_50HZ = 0x418,
	TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B = 0x419,
	TVIN_SIG_FMT_HDMI_1920X1080I_100HZ = 0x41a,
	TVIN_SIG_FMT_HDMI_1280X720P_100HZ = 0x41b,
	TVIN_SIG_FMT_HDMI_720X576P_100HZ = 0x41c,
	TVIN_SIG_FMT_HDMI_1440X576I_100HZ = 0x41d,
	TVIN_SIG_FMT_HDMI_1920X1080I_120HZ = 0x41e,
	TVIN_SIG_FMT_HDMI_1280X720P_120HZ = 0x41f,
	TVIN_SIG_FMT_HDMI_720X480P_120HZ = 0x420,
	TVIN_SIG_FMT_HDMI_1440X480I_120HZ = 0x421,
	TVIN_SIG_FMT_HDMI_720X576P_200HZ = 0x422,
	TVIN_SIG_FMT_HDMI_1440X576I_200HZ = 0x423,
	TVIN_SIG_FMT_HDMI_720X480P_240HZ = 0x424,
	TVIN_SIG_FMT_HDMI_1440X480I_240HZ = 0x425,
	TVIN_SIG_FMT_HDMI_1280X720P_24HZ = 0x426,
	TVIN_SIG_FMT_HDMI_1280X720P_25HZ = 0x427,
	TVIN_SIG_FMT_HDMI_1280X720P_30HZ = 0x428,
	TVIN_SIG_FMT_HDMI_1920X1080P_120HZ = 0x429,
	TVIN_SIG_FMT_HDMI_1920X1080P_100HZ = 0x42a,
	TVIN_SIG_FMT_HDMI_1280X720P_60HZ_FRAME_PACKING = 0x42b,
	TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING = 0x42c,
	TVIN_SIG_FMT_HDMI_1280X720P_24HZ_FRAME_PACKING = 0x42d,
	TVIN_SIG_FMT_HDMI_1280X720P_30HZ_FRAME_PACKING = 0x42e,
	TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING = 0x42f,
	TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING = 0x430,
	TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING = 0x431,
	TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING = 0x432,
	TVIN_SIG_FMT_HDMI_800X600_00HZ = 0x433,
	TVIN_SIG_FMT_HDMI_1024X768_00HZ = 0x434,
	TVIN_SIG_FMT_HDMI_720X400_00HZ = 0x435,
	TVIN_SIG_FMT_HDMI_1280X768_00HZ = 0x436,
	TVIN_SIG_FMT_HDMI_1280X800_00HZ = 0x437,
	TVIN_SIG_FMT_HDMI_1280X960_00HZ = 0x438,
	TVIN_SIG_FMT_HDMI_1280X1024_00HZ = 0x439,
	TVIN_SIG_FMT_HDMI_1360X768_00HZ = 0x43a,
	TVIN_SIG_FMT_HDMI_1366X768_00HZ = 0x43b,
	TVIN_SIG_FMT_HDMI_1600X1200_00HZ = 0x43c,
	TVIN_SIG_FMT_HDMI_1920X1200_00HZ = 0x43d,
	TVIN_SIG_FMT_HDMI_1440X900_00HZ = 0x43e,
	TVIN_SIG_FMT_HDMI_1400X1050_00HZ = 0x43f,
	TVIN_SIG_FMT_HDMI_1680X1050_00HZ = 0x440,
	/* for alternative and 4k2k */
	TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE = 0x441,
	TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_ALTERNATIVE = 0x442,
	TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE = 0x443,
	TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_ALTERNATIVE = 0x444,
	TVIN_SIG_FMT_HDMI_3840_2160_00HZ = 0x445,
	TVIN_SIG_FMT_HDMI_4096_2160_00HZ = 0x446,
	TVIN_SIG_FMT_HDMI_1600X900_60HZ = 0x447,
	TVIN_SIG_FMT_HDMI_RESERVE8 = 0x448,
	TVIN_SIG_FMT_HDMI_RESERVE9 = 0x449,
	TVIN_SIG_FMT_HDMI_RESERVE10 = 0x44a,
	TVIN_SIG_FMT_HDMI_RESERVE11 = 0x44b,
	TVIN_SIG_FMT_HDMI_720X480P_60HZ_FRAME_PACKING = 0x44c,
	TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING = 0x44d,
	TVIN_SIG_FMT_HDMI_640X480P_72HZ = 0x44e,
	TVIN_SIG_FMT_HDMI_640X480P_75HZ = 0x44f,
	TVIN_SIG_FMT_HDMI_1152X864_00HZ = 0x450,
	TVIN_SIG_FMT_HDMI_3840X600_00HZ = 0x451,
	TVIN_SIG_FMT_HDMI_720X350_00HZ = 0x452,
	TVIN_SIG_FMT_HDMI_2688X1520_00HZ = 0x453,
	TVIN_SIG_FMT_HDMI_1920X2160_60HZ = 0x454,
	TVIN_SIG_FMT_HDMI_960X540_60HZ = 0x455,
	TVIN_SIG_FMT_HDMI_2560X1440_00HZ = 0x456,
	TVIN_SIG_FMT_HDMI_640X350_85HZ = 0x457,
	TVIN_SIG_FMT_HDMI_640X400_85HZ = 0x458,
	TVIN_SIG_FMT_HDMI_848X480_60HZ = 0x459,
	TVIN_SIG_FMT_HDMI_1792X1344_85HZ = 0x45a,
	TVIN_SIG_FMT_HDMI_1856X1392_00HZ = 0x45b,
	TVIN_SIG_FMT_HDMI_1920X1440_00HZ = 0x45c,
	TVIN_SIG_FMT_HDMI_2048X1152_60HZ = 0x45d,
	TVIN_SIG_FMT_HDMI_2560X1600_00HZ = 0x45e,
	TVIN_SIG_FMT_HDMI_MAX = 0x45f,
	TVIN_SIG_FMT_HDMI_THRESHOLD = 0x600,
	/* Video Formats */
	TVIN_SIG_FMT_CVBS_NTSC_M = 0x601,
	TVIN_SIG_FMT_CVBS_NTSC_443 = 0x602,
	TVIN_SIG_FMT_CVBS_PAL_I = 0x603,
	TVIN_SIG_FMT_CVBS_PAL_M = 0x604,
	TVIN_SIG_FMT_CVBS_PAL_60 = 0x605,
	TVIN_SIG_FMT_CVBS_PAL_CN = 0x606,
	TVIN_SIG_FMT_CVBS_SECAM = 0x607,
	TVIN_SIG_FMT_CVBS_NTSC_50 = 0x608,
	TVIN_SIG_FMT_CVBS_MAX = 0x609,
	TVIN_SIG_FMT_CVBS_THRESHOLD = 0x800,
	/* 656 Formats */
	TVIN_SIG_FMT_BT656IN_576I_50HZ = 0x801,
	TVIN_SIG_FMT_BT656IN_480I_60HZ = 0x802,
	/* 601 Formats */
	TVIN_SIG_FMT_BT601IN_576I_50HZ = 0x803,
	TVIN_SIG_FMT_BT601IN_480I_60HZ = 0x804,
	/* Camera Formats */
	TVIN_SIG_FMT_CAMERA_640X480P_30HZ = 0x805,
	TVIN_SIG_FMT_CAMERA_800X600P_30HZ = 0x806,
	TVIN_SIG_FMT_CAMERA_1024X768P_30HZ = 0x807,
	TVIN_SIG_FMT_CAMERA_1920X1080P_30HZ = 0x808,
	TVIN_SIG_FMT_CAMERA_1280X720P_30HZ = 0x809,
	TVIN_SIG_FMT_BT601_MAX = 0x80a,
	TVIN_SIG_FMT_BT601_THRESHOLD = 0xa00,
	TVIN_SIG_FMT_MAX,
};

/* tvin signal status */
enum tvin_sig_status_e {
	TVIN_SIG_STATUS_NULL = 0,
	/* processing status from init to */
	/*the finding of the 1st confirmed status */

	TVIN_SIG_STATUS_NOSIG,	/* no signal - physically no signal */
	TVIN_SIG_STATUS_UNSTABLE,	/* unstable - physically bad signal */
	TVIN_SIG_STATUS_NOTSUP,
	/* not supported - physically good signal & not supported */

	TVIN_SIG_STATUS_STABLE,
	/* stable - physically good signal & supported */
};

const char *tvin_sig_status_str(enum tvin_sig_status_e status);

/* tvin parameters */
#define TVIN_PARM_FLAG_CAP      0x00000001

/* tvin_parm_t.flag[ 0]: 1/enable or 0/disable frame capture function */

#define TVIN_PARM_FLAG_CAL      0x00000002

/* tvin_parm_t.flag[ 1]: 1/enable or 0/disable adc calibration */

/*used for processing 3d in ppmgr set this flag*/
/*to drop one field and send real height in vframe*/
#define TVIN_PARM_FLAG_2D_TO_3D 0x00000004

/* tvin_parm_t.flag[ 2]: 1/enable or 0/disable 2D->3D mode */

enum tvin_trans_fmt {
	TVIN_TFMT_2D = 0,
	TVIN_TFMT_3D_LRH_OLOR,
	/* 1 Primary: Side-by-Side(Half) Odd/Left picture, Odd/Right p */

	TVIN_TFMT_3D_LRH_OLER,
	/* 2 Primary: Side-by-Side(Half) Odd/Left picture, Even/Right picture */

	TVIN_TFMT_3D_LRH_ELOR,
	/* 3 Primary: Side-by-Side(Half) Even/Left picture, Odd/Right picture */

	TVIN_TFMT_3D_LRH_ELER,
	/* 4 Primary: Side-by-Side(Half) Even/Left picture, Even/Right picture*/

	TVIN_TFMT_3D_TB,	/* 5 Primary: Top-and-Bottom */
	TVIN_TFMT_3D_FP,	/* 6 Primary: Frame Packing */
	TVIN_TFMT_3D_FA,	/* 7 Secondary: Field Alternative */
	TVIN_TFMT_3D_LA,	/* 8 Secondary: Line Alternative */
	TVIN_TFMT_3D_LRF,	/* 9 Secondary: Side-by-Side(Full) */
	TVIN_TFMT_3D_LD,	/* 10 Secondary: L+depth */
	TVIN_TFMT_3D_LDGD, /* 11 Secondary: L+depth+Graphics+Graphics-depth */
	/* normal 3D format */
	TVIN_TFMT_3D_DET_TB,	/* 12 */
	TVIN_TFMT_3D_DET_LR,	/* 13 */
	TVIN_TFMT_3D_DET_INTERLACE,	/* 14 */
	TVIN_TFMT_3D_DET_CHESSBOARD,	/* 15 */
};

const char *tvin_trans_fmt_str(enum tvin_trans_fmt trans_fmt);

enum tvin_color_fmt_e {
	TVIN_RGB444 = 0,
	TVIN_YUV422,		/* 1 */
	TVIN_YUV444,		/* 2 */
	TVIN_YUYV422,		/* 3 */
	TVIN_YVYU422,		/* 4 */
	TVIN_UYVY422,		/* 5 */
	TVIN_VYUY422,		/* 6 */
	TVIN_NV12,		/* 7 */
	TVIN_NV21,		/* 8 */
	TVIN_BGGR,		/* 9  raw data */
	TVIN_RGGB,		/* 10 raw data */
	TVIN_GBRG,		/* 11 raw data */
	TVIN_GRBG,		/* 12 raw data */
	TVIN_COLOR_FMT_MAX,
};

enum tvin_color_fmt_range_e {
	TVIN_FMT_RANGE_NULL = 0,	/* depend on video fromat */
	TVIN_RGB_FULL,		/* 1 */
	TVIN_RGB_LIMIT,		/* 2 */
	TVIN_YUV_FULL,		/* 3 */
	TVIN_YUV_LIMIT,		/* 4 */
	TVIN_COLOR_FMT_RANGE_MAX,
};

enum tvin_aspect_ratio_e {
	TVIN_ASPECT_NULL = 0,
	TVIN_ASPECT_1x1,
	TVIN_ASPECT_4x3_FULL,
	TVIN_ASPECT_14x9_FULL,
	TVIN_ASPECT_14x9_LB_CENTER,
	TVIN_ASPECT_14x9_LB_TOP,
	TVIN_ASPECT_16x9_FULL,
	TVIN_ASPECT_16x9_LB_CENTER,
	TVIN_ASPECT_16x9_LB_TOP,
	TVIN_ASPECT_MAX,
};

const char *tvin_trans_color_range_str(enum
						tvin_color_fmt_range_e
						color_range);

enum tvin_force_color_range_e {
	COLOR_RANGE_AUTO = 0,
	COLOR_RANGE_FULL,
	COLOR_RANGE_LIMIT,
	COLOR_RANGE_NULL,
};

const char *tvin_trans_force_range_str(enum tvin_force_color_range_e
				       force_range);
const char *tvin_color_fmt_str(enum tvin_color_fmt_e color_fmt);
enum tvin_scan_mode_e {
	TVIN_SCAN_MODE_NULL = 0,
	TVIN_SCAN_MODE_PROGRESSIVE,
	TVIN_SCAN_MODE_INTERLACED,
};

struct tvin_to_vpp_info_s {
	u32 is_dv;
	enum tvin_scan_mode_e scan_mode;
	unsigned int fps;
	unsigned int width;
	unsigned int height;
	enum tvin_color_fmt_e cfmt;
};

struct tvin_info_s {
	enum tvin_trans_fmt trans_fmt;
	enum tvin_sig_fmt_e fmt;
	enum tvin_sig_status_e status;
	enum tvin_color_fmt_e cfmt;
	unsigned int fps;
	unsigned int is_dvi;

	/*
	 * bit 30: is_dv
	 * bit 29: present_flag
	 * bit 28-26: video_format
	 *	"component", "PAL", "NTSC", "SECAM", "MAC", "unspecified"
	 * bit 25: range "limited", "full_range"
	 * bit 24: color_description_present_flag
	 * bit 23-16: color_primaries
	 *	"unknown", "bt709", "undef", "bt601", "bt470m", "bt470bg",
	 *	"smpte170m", "smpte240m", "film", "bt2020"
	 * bit 15-8: transfer_characteristic
	 *	"unknown", "bt709", "undef", "bt601", "bt470m", "bt470bg",
	 *	"smpte170m", "smpte240m", "linear", "log100", "log316",
	 *	"iec61966-2-4", "bt1361e", "iec61966-2-1", "bt2020-10",
	 *	"bt2020-12", "smpte-st-2084", "smpte-st-428"
	 * bit 7-0: matrix_coefficient
	 *	"GBR", "bt709", "undef", "bt601", "fcc", "bt470bg",
	 *	"smpte170m", "smpte240m", "YCgCo", "bt2020nc", "bt2020c"
	 */
	unsigned int signal_type;
	enum tvin_aspect_ratio_e aspect_ratio;
	/*
	 * 0:no dv 1:visf 2:emp
	 */
	u8 dolby_vision;
	/*
	 * 0:sink-led 1:source-led
	 */
	u8 low_latency;
};

struct tvin_frontend_info_s {
	enum tvin_scan_mode_e scan_mode;
	enum tvin_color_fmt_e cfmt;
	unsigned int fps;
	unsigned int width;
	unsigned int height;
	unsigned int colordepth;
};

struct tvin_buf_info_s {
	unsigned int vf_size;
	unsigned int buf_count;
	unsigned int buf_width;
	unsigned int buf_height;
	unsigned int buf_size;
	unsigned int wr_list_size;
};

struct tvin_video_buf_s {
	unsigned int index;
	unsigned int reserved;
};

/* hs=he=vs=ve=0 is to disable Cut Window */
struct tvin_cutwin_s {
	unsigned short hs;
	unsigned short he;
	unsigned short vs;
	unsigned short ve;
};

struct tvin_parm_s {
	int index;		/* index of frontend for vdin */
	enum tvin_port_e port;	/* must set port in IOCTL */
	struct tvin_info_s info;
	unsigned int hist_pow;
	unsigned int luma_sum;
	unsigned int pixel_sum;
	unsigned short histgram[64];
	unsigned int flag;
	unsigned short dest_width;	/* for vdin horizontal scale down */
	unsigned short dest_height;	/* for vdin vertical scale down */
	bool h_reverse;		/* for vdin horizontal reverse */
	bool v_reverse;		/* for vdin vertical reverse */
	unsigned int reserved;
};

/* ************************************************************************* */

/* *** AFE module definition/enum/struct *********************************** */

/* ************************************************************************* */
struct tvafe_vga_parm_s {
	signed short clk_step;	/* clock < 0, tune down clock freq */
	/* clock > 0, tune up clock freq */
	unsigned short phase;	/* phase is 0~31, it is absolute value */
	signed short hpos_step;	/* hpos_step < 0, shift display to left */
	/* hpos_step > 0, shift display to right */
	signed short vpos_step;	/* vpos_step < 0, shift display to top */
	/* vpos_step > 0, shift display to bottom */
	unsigned int vga_in_clean;	/* flage for vga clean screen */
};

enum tvafe_cvbs_video_e {
	TVAFE_CVBS_VIDEO_HV_UNLOCKED = 0,
	TVAFE_CVBS_VIDEO_H_LOCKED,
	TVAFE_CVBS_VIDEO_V_LOCKED,
	TVAFE_CVBS_VIDEO_HV_LOCKED,
};

/* for pin selection */
enum tvafe_adc_pin_e {
	TVAFE_ADC_PIN_NULL = 0,
	/*(MESON_CPU_TYPE > MESON_CPU_TYPE_MESONG9TV) */
	TVAFE_CVBS_IN0 = 1,  /* avin ch0 */
	TVAFE_CVBS_IN1 = 2,  /* avin ch1 */
	TVAFE_CVBS_IN2 = 3,  /* avin ch2 */
	TVAFE_CVBS_IN3 = 4,  /*as atvdemod to tvafe */
	TVAFE_ADC_PIN_MAX,
};

enum tvafe_src_sig_e {
	/* TODO Only M8 first */

	/*#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV) */
	CVBS_IN0 = 0,
	CVBS_IN1,
	CVBS_IN2,
	CVBS_IN3,
	TVAFE_SRC_SIG_MAX_NUM,
};

struct tvafe_pin_mux_s {
	enum tvafe_adc_pin_e pin[TVAFE_SRC_SIG_MAX_NUM];
};

bool IS_TVAFE_SRC(enum tvin_port_e port);
bool IS_TVAFE_ATV_SRC(enum tvin_port_e port);
bool IS_TVAFE_AVIN_SRC(enum tvin_port_e port);
bool IS_HDMI_SRC(enum tvin_port_e port);

/* ************************************************************************* */

/* *** IOCTL command definition ******************************************* */

/* ************************************************************************* */

#define _TM_T 'T'

/* GENERAL */
#define TVIN_IOC_OPEN               _IOW(_TM_T, 0x01, struct tvin_parm_s)
#define TVIN_IOC_START_DEC          _IOW(_TM_T, 0x02, struct tvin_parm_s)
#define TVIN_IOC_STOP_DEC           _IO(_TM_T, 0x03)
#define TVIN_IOC_CLOSE              _IO(_TM_T, 0x04)
#define TVIN_IOC_G_PARM             _IOR(_TM_T, 0x05, struct tvin_parm_s)
#define TVIN_IOC_S_PARM             _IOW(_TM_T, 0x06, struct tvin_parm_s)
#define TVIN_IOC_G_SIG_INFO         _IOR(_TM_T, 0x07, struct tvin_info_s)
#define TVIN_IOC_G_BUF_INFO         _IOR(_TM_T, 0x08, struct tvin_buf_info_s)
#define TVIN_IOC_START_GET_BUF      _IO(_TM_T, 0x09)
#define TVIN_IOC_G_EVENT_INFO	_IOW(_TM_T, 0x0a, struct vdin_event_info)

#define TVIN_IOC_GET_BUF            _IOR(_TM_T, 0x10, struct tvin_video_buf_s)
#define TVIN_IOC_PAUSE_DEC          _IO(_TM_T, 0x41)
#define TVIN_IOC_RESUME_DEC         _IO(_TM_T, 0x42)
#define TVIN_IOC_VF_REG             _IO(_TM_T, 0x43)
#define TVIN_IOC_VF_UNREG           _IO(_TM_T, 0x44)
#define TVIN_IOC_FREEZE_VF          _IO(_TM_T, 0x45)
#define TVIN_IOC_UNFREEZE_VF        _IO(_TM_T, 0x46)
#define TVIN_IOC_SNOW_ON             _IO(_TM_T, 0x47)
#define TVIN_IOC_SNOW_OFF            _IO(_TM_T, 0x48)
#define TVIN_IOC_GET_COLOR_RANGE	_IOR(_TM_T, 0X49,\
	enum tvin_force_color_range_e)
#define TVIN_IOC_SET_COLOR_RANGE	_IOW(_TM_T, 0X4a,\
	enum tvin_force_color_range_e)
#define TVIN_IOC_GAME_MODE          _IOW(_TM_T, 0x4b, unsigned int)
#define TVIN_IOC_VRR_MODE           _IOW(_TM_T, 0x54, unsigned int)
#define TVIN_IOC_GET_LATENCY_MODE		_IOR(_TM_T, 0x4d,\
	struct tvin_latency_s)
#define TVIN_IOC_G_FRONTEND_INFO    _IOR(_TM_T, 0x4e,\
	struct tvin_frontend_info_s)
#define TVIN_IOC_S_CANVAS_ADDR  _IOW(_TM_T, 0x4f,\
	struct vdin_set_canvas_s)
#define TVIN_IOC_S_PC_MODE		_IOW(_TM_T, 0x50, unsigned int)
#define TVIN_IOC_S_FRAME_WR_EN		_IOW(_TM_T, 0x51, unsigned int)
#define TVIN_IOC_G_INPUT_TIMING		_IOR(_TM_T, 0x52, struct tvin_format_s)
#define TVIN_IOC_G_VRR_STATUS		_IOR(_TM_T, 0x53, struct vdin_vrr_freesync_param_s)

#define TVIN_IOC_S_CANVAS_RECOVERY  _IO(_TM_T, 0x0a)
/* TVAFE */
#define TVIN_IOC_S_AFE_VGA_PARM     _IOW(_TM_T, 0x16, struct tvafe_vga_parm_s)
#define TVIN_IOC_G_AFE_VGA_PARM     _IOR(_TM_T, 0x17, struct tvafe_vga_parm_s)
#define TVIN_IOC_S_AFE_VGA_AUTO     _IO(_TM_T, 0x18)
#define TVIN_IOC_G_AFE_CVBS_LOCK    _IOR(_TM_T, 0x1a, enum tvafe_cvbs_video_e)
#define TVIN_IOC_S_AFE_CVBS_STD     _IOW(_TM_T, 0x1b, enum tvin_sig_fmt_e)
#define TVIN_IOC_CALLMASTER_SET     _IOW(_TM_T, 0x1c, enum tvin_port_e)
#define TVIN_IOC_CALLMASTER_GET	    _IO(_TM_T, 0x1d)
#define TVIN_IOC_G_AFE_CVBS_STD     _IOW(_TM_T, 0x1e, enum tvin_sig_fmt_e)
#define TVIN_IOC_LOAD_REG          _IOW(_TM_T, 0x20, struct am_regs_s)
#define TVIN_IOC_S_AFE_SNOW_ON     _IO(_TM_T, 0x22)
#define TVIN_IOC_S_AFE_SNOW_OFF     _IO(_TM_T, 0x23)
#define TVIN_IOC_G_VDIN_HIST       _IOW(_TM_T, 0x24, struct vdin_hist_s)
#define TVIN_IOC_S_VDIN_V4L2START  _IOW(_TM_T, 0x25, struct vdin_v4l2_param_s)
#define TVIN_IOC_S_VDIN_V4L2STOP   _IO(_TM_T, 0x26)
#define TVIN_IOC_S_AFE_SNOW_CFG     _IOW(_TM_T, 0x27, unsigned int)
#define TVIN_IOC_S_DV_DESCRAMBLE	_IOW(_TM_T, 0x28, unsigned int)
#define TVIN_IOC_S_AFE_ATV_SEARCH  _IOW(_TM_T, 0x29, unsigned int)

/*
 *function defined applied for other driver
 */

struct dfe_adcpll_para {
	unsigned int adcpllctl;
	unsigned int demodctl;
	unsigned int atsc;
	enum fe_delivery_system delsys;
	unsigned int adc_clk;
};

struct rx_audio_stat_s {
	/* 1: AUDS, 2: OBA, 4:DST, 8: HBR, 16: OBM, 32: MAS */
	int aud_rcv_packet;
	/*audio stable status*/
	bool aud_stb_flag;
	/*audio sample rate*/
	int aud_sr;
	/**audio channel count*/
	/*0: refer to stream header,*/
	/*1: 2ch, 2: 3ch, 3: 4ch, 4: 5ch,*/
	/*5: 6ch, 6: 7ch, 7: 8ch*/
	int aud_channel_cnt;
	/**audio coding type*/
	/*0: refer to stream header, 1: IEC60958 PCM,*/
	/*2: AC-3, 3: MPEG1 (Layers 1 and 2),*/
	/*4: MP3 (MPEG1 Layer 3), 5: MPEG2 (multichannel),*/
	/*6: AAC, 7: DTS, 8: ATRAC, 9: One Bit Audio,*/
	/*10: Dolby Digital Plus, 11: DTS-HD,*/
	/*12: MAT (MLP), 13: DST, 14: WMA Pro*/
	int aud_type;
	/* indicate if audio fifo start threshold is crossed */
	bool afifo_thres_pass;
	/*
	 * 0 [ch1 ch2]
	 * 1,2,3 [ch1 ch2 ch3 ch4]
	 * 4,8 [ch1 ch2 ch5 ch6]
	 * 5,6,7,9,10,11 [ch1 ch2 ch3 ch4 ch5 ch6]
	 * 12,16,24,28 [ch1 ch2 ch5 ch6 ch7 ch8]
	 * 20 [ch1 ch2 ch7 ch8]
	 * 21,22,23[ch1 ch2 ch3 ch4 ch7 ch8]
	 * all others [all of 8ch]
	 */
	int aud_alloc;
	u8 ch_sts[7];
};

int get_vdin_delay_num(void);
#ifdef CONFIG_AMLOGIC_MEDIA_ADC
void adc_set_pll_reset(void);
int adc_get_pll_flag(void);
/*ADC_EN_ATV_DEMOD	0x1*/
/*ADC_EN_TVAFE		0x2*/
/*ADC_EN_DTV_DEMOD	0x4*/
/*ADC_EN_DTV_DEMODPLL	0x8*/
int adc_set_pll_cntl(bool on, enum adc_sel module_sel, void *p_para_);
void adc_set_ddemod_default(enum fe_delivery_system delsys);/* add for dtv demod */
int adc_set_filter_ctrl(bool on, enum filter_sel module_sel, void *data);
#else
static inline void adc_set_pll_reset(void)
{
}

static inline int adc_get_pll_flag(void)
{
	return 0;
}

static inline int adc_set_pll_cntl(bool on, enum adc_sel module_sel, void *p_para_)
{
	return 0;
}

static inline void adc_set_ddemod_default(enum fe_delivery_system delsys)
{
}

static inline int adc_set_filter_ctrl(bool on, enum filter_sel module_sel, void *data)
{
	return 0;
}
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_VDIN
unsigned int get_vdin_buffer_num(void);
#else
static inline unsigned int get_vdin_buffer_num(void)
{
}
#endif
void rx_get_audio_status(struct rx_audio_stat_s *aud_sts);
void rx_set_atmos_flag(bool en);
bool rx_get_atmos_flag(void);
u_char rx_edid_get_aud_sad(u_char *sad_data);
bool rx_edid_set_aud_sad(u_char *sad, u_char len);
int rx_set_audio_param(uint32_t param);
void rx_earc_hpd_cntl(void);
#endif
