// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

/* Standard Linux headers */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include "aml_hdr10_plus.h"
#include "aml_hdr10_plus_ootf.h"

uint debug_hdr10p;
#define pr_hdr(fmt, args...)\
	do {\
		if (debug_hdr10p)\
			pr_info(fmt, ## args);\
	} while (0)

#define HDR10_PLUS_VERSION  "hdr10_plus v1_20181024"

static struct hdr_plus_bits_s sei_md_bits = {
	.len_itu_t_t35_country_code = 8,
	.len_itu_t_t35_terminal_provider_code = 16,
	.len_itu_t_t35_terminal_provider_oriented_code = 16,
	.len_application_identifier = 8,
	.len_application_version = 8,
	.len_num_windows = 2,
	.len_window_upper_left_corner_x = 16,
	.len_window_upper_left_corner_y = 16,
	.len_window_lower_right_corner_x = 16,
	.len_window_lower_right_corner_y = 16,
	.len_center_of_ellipse_x = 16,
	.len_center_of_ellipse_y = 16,
	.len_rotation_angle = 8,
	.len_semimajor_axis_internal_ellipse = 16,
	.len_semimajor_axis_external_ellipse = 16,
	.len_semiminor_axis_external_ellipse = 16,
	.len_overlap_process_option = 1,
	.len_tgt_sys_disp_max_lumi = 27,
	.len_tgt_sys_disp_act_pk_lumi_flag = 1,
	.len_num_rows_tgt_sys_disp_act_pk_lumi = 5,
	.len_num_cols_tgt_sys_disp_act_pk_lumi = 5,
	.len_tgt_sys_disp_act_pk_lumi = 4,
	.len_maxscl = 17,
	.len_average_maxrgb = 17,
	.len_num_distribution_maxrgb_percentiles = 4,
	.len_distribution_maxrgb_percentages = 7,
	.len_distribution_maxrgb_percentiles = 17,
	.len_fraction_bright_pixels = 10,
	.len_mast_disp_act_pk_lumi_flag = 1,
	.len_num_rows_mast_disp_act_pk_lumi = 5,
	.len_num_cols_mast_disp_act_pk_lumi = 5,
	.len_mast_disp_act_pk_lumi = 4,
	.len_tone_mapping_flag = 1,
	.len_knee_point_x = 12,
	.len_knee_point_y = 12,
	.len_num_bezier_curve_anchors = 4,
	.len_bezier_curve_anchors = 10,
	.len_color_saturation_mapping_flag = 1,
	.len_color_saturation_weight = 6
};

struct vframe_hdr10p_sei hdr10p_sei;

#define NAL_UNIT_SEI 39
#define NAL_UNIT_SEI_SUFFIX 40

static int getbits(char buffer[], int totbitoffset,
	int *info, int bytecount, int numbits)
{
	int inf;
	int bitoffset = (totbitoffset & 0x07);/*bit from start of byte*/
	long byteoffset = (totbitoffset >> 3);/*byte from start of buffer*/
	int bitcounter = numbits;
	static char *curbyte;

	if (byteoffset + ((numbits + bitoffset) >> 3) > bytecount)
		return -1;

	curbyte = &buffer[byteoffset];
	bitoffset = 7 - bitoffset;
	inf = 0;

	while (numbits--) {
		inf <<= 1;
		inf |= ((*curbyte) >> (bitoffset--)) & 0x01;

		if (bitoffset < 0) {
			curbyte++;
			bitoffset = 7;
		}
	/*curbyte -= (bitoffset >> 3);*/
	/*bitoffset &= 0x07;*/
	/*curbyte += (bitoffset == 7);*/
	}

	*info = inf;
	return bitcounter;
}

static void parser_hdr10p_medata(char *metadata, uint32_t size)
{
	int totbitoffset = 0;
	int value = 0;
	int i = 0;
	int j = 0;
	int num_win;
	unsigned int num_col_tsdapl = 0, num_row_tsdapl = 0;
	unsigned int tar_sys_disp_act_pk_lumi_flag = 0;
	unsigned int num_d_m_p = 0;
	unsigned int m_d_a_p_l_flag = 0;
	unsigned int num_row_m_d_a_p_l = 0, num_col_m_d_a_p_l = 0;
	unsigned int tone_mapping_flag = 0;
	unsigned int num_bezier_curve_anchors = 0;
	unsigned int color_saturation_mapping_flag = 0;

	memset(&hdr10p_sei, 0, sizeof(struct vframe_hdr10p_sei));

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_itu_t_t35_country_code);
	hdr10p_sei.itu_t_t35_country_code = (u16)value;
	totbitoffset += sei_md_bits.len_itu_t_t35_country_code;

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_itu_t_t35_terminal_provider_code);
	hdr10p_sei.itu_t_t35_terminal_provider_code = (u16)value;
	totbitoffset += sei_md_bits.len_itu_t_t35_terminal_provider_code;

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_itu_t_t35_terminal_provider_oriented_code);
	hdr10p_sei.itu_t_t35_terminal_provider_oriented_code =
		(u16)value;
	totbitoffset +=
		sei_md_bits.len_itu_t_t35_terminal_provider_oriented_code;

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_application_identifier);
	hdr10p_sei.application_identifier = (u16)value;
	totbitoffset += sei_md_bits.len_application_identifier;

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_application_version);
	hdr10p_sei.application_version = (u16)value;
	totbitoffset += sei_md_bits.len_application_version;

	getbits(metadata, totbitoffset, &value, size,
		sei_md_bits.len_num_windows);
	hdr10p_sei.num_windows = (u16)value;
	totbitoffset += sei_md_bits.len_num_windows;

	num_win = value;

	if (value > 1) {
		for (i = 1; i < num_win; i++) {
			getbits(metadata, totbitoffset, &value, size,
				sei_md_bits.len_window_upper_left_corner_x);
			hdr10p_sei.window_upper_left_corner_x[i] = (u16)value;
			totbitoffset +=
				sei_md_bits.len_window_upper_left_corner_x;

			getbits(metadata, totbitoffset, &value, size,
				sei_md_bits.len_window_upper_left_corner_y);
			hdr10p_sei.window_upper_left_corner_y[i] = (u16)value;
			totbitoffset +=
				sei_md_bits.len_window_upper_left_corner_y;

			getbits(metadata, totbitoffset, &value, size,
				sei_md_bits.len_window_lower_right_corner_x);
			hdr10p_sei.window_lower_right_corner_x[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_window_lower_right_corner_x;

			getbits(metadata, totbitoffset, &value, size,
				sei_md_bits.len_window_lower_right_corner_y);
			hdr10p_sei.window_lower_right_corner_y[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_window_lower_right_corner_y;

			getbits(metadata, totbitoffset, &value, size,
				sei_md_bits.len_center_of_ellipse_x);
			hdr10p_sei.center_of_ellipse_x[i] = (u16)value;
			totbitoffset += sei_md_bits.len_center_of_ellipse_x;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_center_of_ellipse_y);
			hdr10p_sei.center_of_ellipse_y[i] = (u16)value;
			totbitoffset += sei_md_bits.len_center_of_ellipse_y;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_rotation_angle);
			hdr10p_sei.rotation_angle[i] = (u16)value;
			totbitoffset += sei_md_bits.len_rotation_angle;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_semimajor_axis_internal_ellipse);
			hdr10p_sei.semimajor_axis_internal_ellipse[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_semimajor_axis_internal_ellipse;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_semimajor_axis_external_ellipse);
			hdr10p_sei.semimajor_axis_external_ellipse[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_semimajor_axis_external_ellipse;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_semiminor_axis_external_ellipse);
			hdr10p_sei.semiminor_axis_external_ellipse[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_semiminor_axis_external_ellipse;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_overlap_process_option);
			hdr10p_sei.overlap_process_option[i] = (u16)value;
			totbitoffset += sei_md_bits.len_overlap_process_option;
		}
	}

	getbits(metadata, totbitoffset,
		&value, size,
		sei_md_bits.len_tgt_sys_disp_max_lumi);
	hdr10p_sei.tgt_sys_disp_max_lumi = value;
	totbitoffset +=
		sei_md_bits.len_tgt_sys_disp_max_lumi;

	getbits(metadata, totbitoffset,
		&value, size,
		sei_md_bits.len_tgt_sys_disp_act_pk_lumi_flag);
	hdr10p_sei.tgt_sys_disp_act_pk_lumi_flag =
		(u16)value;
	totbitoffset +=
		sei_md_bits.len_tgt_sys_disp_act_pk_lumi_flag;

	tar_sys_disp_act_pk_lumi_flag = value;

	if (tar_sys_disp_act_pk_lumi_flag) {
		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_num_rows_tgt_sys_disp_act_pk_lumi);
		hdr10p_sei.num_rows_tgt_sys_disp_act_pk_lumi =
			(u16)value;
		totbitoffset +=
			sei_md_bits.len_num_rows_tgt_sys_disp_act_pk_lumi;

		num_row_tsdapl = value;

		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_num_cols_tgt_sys_disp_act_pk_lumi);
		hdr10p_sei.num_cols_tgt_sys_disp_act_pk_lumi =
			(u16)value;
		totbitoffset +=
			sei_md_bits.len_num_cols_tgt_sys_disp_act_pk_lumi;

		num_col_tsdapl = value;

		for (i = 0; i < num_row_tsdapl; i++) {
			for (j = 0; j < num_col_tsdapl; j++) {
				getbits(metadata, totbitoffset,
					&value, size,
					sei_md_bits.len_tgt_sys_disp_act_pk_lumi);
				hdr10p_sei.tgt_sys_disp_act_pk_lumi[i][j] =
					(u16)value;
				totbitoffset +=
					sei_md_bits.len_tgt_sys_disp_act_pk_lumi;
			}
		}
	}

	for (i = 0; i < num_win; i++) {
		for (j = 0; j < 3; j++) {
			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_maxscl);
			hdr10p_sei.maxscl[i][j] = value;
			totbitoffset += sei_md_bits.len_maxscl;
		}
		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_average_maxrgb);
		hdr10p_sei.average_maxrgb[i] = value;
		totbitoffset += sei_md_bits.len_average_maxrgb;

		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_num_distribution_maxrgb_percentiles);
		hdr10p_sei.num_distribution_maxrgb_percentiles[i] =
			(u16)value;
		totbitoffset +=
			sei_md_bits.len_num_distribution_maxrgb_percentiles;

		num_d_m_p = value;
		for (j = 0; j < num_d_m_p; j++) {
			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_distribution_maxrgb_percentages);
			hdr10p_sei.distribution_maxrgb_percentages[i][j] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_distribution_maxrgb_percentages;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_distribution_maxrgb_percentiles);
			hdr10p_sei.distribution_maxrgb_percentiles[i][j] =
				value;
			totbitoffset +=
				sei_md_bits.len_distribution_maxrgb_percentiles;
		}

		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_fraction_bright_pixels);
		hdr10p_sei.fraction_bright_pixels[i] = (u16)value;
		totbitoffset += sei_md_bits.len_fraction_bright_pixels;
	}

	getbits(metadata, totbitoffset,
		&value, size,
		sei_md_bits.len_mast_disp_act_pk_lumi_flag);
	hdr10p_sei.mast_disp_act_pk_lumi_flag =
		(u16)value;
	totbitoffset +=
		sei_md_bits.len_mast_disp_act_pk_lumi_flag;

	m_d_a_p_l_flag = value;
	if (m_d_a_p_l_flag) {
		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_num_rows_mast_disp_act_pk_lumi);
		hdr10p_sei.num_rows_mast_disp_act_pk_lumi =
			(u16)value;
		totbitoffset +=
			sei_md_bits.len_num_rows_mast_disp_act_pk_lumi;

		num_row_m_d_a_p_l = value;

		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_num_cols_mast_disp_act_pk_lumi);
		hdr10p_sei.num_cols_mast_disp_act_pk_lumi =
			(u16)value;
		totbitoffset +=
		sei_md_bits.len_num_cols_mast_disp_act_pk_lumi;

		num_col_m_d_a_p_l = value;

		for (i = 0; i < num_row_m_d_a_p_l; i++) {
			for (j = 0; j < num_col_m_d_a_p_l; j++) {
				getbits(metadata, totbitoffset,
					&value, size,
					sei_md_bits.len_mast_disp_act_pk_lumi);
				hdr10p_sei.mast_disp_act_pk_lumi[i][j] =
					(u16)value;
				totbitoffset +=
					sei_md_bits.len_mast_disp_act_pk_lumi;
			}
		}
	}

	for (i = 0; i < num_win; i++) {
		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_tone_mapping_flag);
		hdr10p_sei.tone_mapping_flag[i] = (u16)value;
		totbitoffset += sei_md_bits.len_tone_mapping_flag;

		tone_mapping_flag = value;

		if (tone_mapping_flag) {
			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_knee_point_x);
			hdr10p_sei.knee_point_x[i] = (u16)value;
			totbitoffset += sei_md_bits.len_knee_point_x;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_knee_point_y);
			hdr10p_sei.knee_point_y[i] = (u16)value;
			totbitoffset += sei_md_bits.len_knee_point_y;

			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_num_bezier_curve_anchors);
			hdr10p_sei.num_bezier_curve_anchors[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_num_bezier_curve_anchors;

			num_bezier_curve_anchors = value;

			for (j = 0; j < num_bezier_curve_anchors; j++) {
				getbits(metadata, totbitoffset,
					&value, size,
					sei_md_bits.len_bezier_curve_anchors);
				hdr10p_sei.bezier_curve_anchors[i][j] =
					(u16)value;
				totbitoffset +=
					sei_md_bits.len_bezier_curve_anchors;
			}
		}

		getbits(metadata, totbitoffset,
			&value, size,
			sei_md_bits.len_color_saturation_mapping_flag);
		hdr10p_sei.color_saturation_mapping_flag[i] =
			(u16)value;
		totbitoffset +=
			sei_md_bits.len_color_saturation_mapping_flag;

		color_saturation_mapping_flag = value;
		if (color_saturation_mapping_flag) {
			getbits(metadata, totbitoffset,
				&value, size,
				sei_md_bits.len_color_saturation_weight);
			hdr10p_sei.color_saturation_weight[i] =
				(u16)value;
			totbitoffset +=
				sei_md_bits.len_color_saturation_weight;
		}
	}
}

static int parse_sei(char *sei_buf, uint32_t size)
{
	char *p = sei_buf;
	char *p_sei;
	u16 header;
	u16 nal_unit_type;
	u16 payload_type, payload_size;

	if (size < 2)
		return 0;

	header = *p++;
	header <<= 8;
	header += *p++;
	nal_unit_type = header >> 9;
	if (nal_unit_type != NAL_UNIT_SEI &&
		nal_unit_type != NAL_UNIT_SEI_SUFFIX)
		return 0;

	while (p + 4 <= sei_buf + size) {
		payload_type = *p++;
		/*unsupport type for the current*/
		if (payload_type == 0xff)
			payload_type += *p++;

		payload_size = *p++;
		if (payload_size == 0xff)
			payload_size += *p++;

		if (p + payload_size <= sei_buf + size) {
			switch (payload_type) {
			case SEI_SYNTAX:
				p_sei = p;
				parser_hdr10p_medata(p_sei, payload_size);
				break;
			default:
				break;
			}
		}
		p += payload_size;
	}

	return 0;
}

/*av1 hdr10p not contain nal_unit_type + payload type +  payload size  these 4bytes*/
static int parse_sei_av1(char *sei_buf, uint32_t size)
{
	if (size < 6)
		return 0;

	if (check_av1_hdr10p(sei_buf))
		parser_hdr10p_medata(sei_buf, size);

	return 0;
}

static void hdr10p_vf_md_parse(struct vframe_s *vf)
{
	int i;
	struct hdr10plus_para hdr10p_md_param;
	struct vframe_hdr10p_sei *p;

	memset(&hdr10p_md_param, 0, sizeof(struct hdr10plus_para));
	memset(&hdr10p_sei, 0, sizeof(struct vframe_hdr10p_sei));
	p = &hdr10p_sei;

	hdr10p_md_param.application_version =
		vf->prop.hdr10p_data.pb4_st.app_ver;
	pr_hdr("vf:app_ver = 0x%x\n", vf->prop.hdr10p_data.pb4_st.app_ver);

	hdr10p_md_param.targeted_max_lum =
		vf->prop.hdr10p_data.pb4_st.max_lumin;
	pr_hdr("vf:target_max_lumin = 0x%x\n",
		vf->prop.hdr10p_data.pb4_st.max_lumin);

	hdr10p_md_param.average_maxrgb =
		vf->prop.hdr10p_data.average_maxrgb;
	pr_hdr("vf:average_maxrgb = 0x%x\n",
		vf->prop.hdr10p_data.average_maxrgb);

	/*distribution value*/
	memcpy(&hdr10p_md_param.distribution_values[0],
		&vf->prop.hdr10p_data.distrib_valus0,
		sizeof(uint8_t) * 9);
	for (i = 0; i < 9; i++)
		pr_hdr("vf:hdr10p_md_param.distribution_values[%d] = 0x%x\n",
			i, hdr10p_md_param.distribution_values[i]);

	hdr10p_md_param.num_bezier_curve_anchors =
	vf->prop.hdr10p_data.pb15_18_st.num_bezier_curve_anchors;
	pr_hdr("vf:num_bezier_curve_anchors = 0x%x\n",
		vf->prop.hdr10p_data.pb15_18_st.num_bezier_curve_anchors);

	/*if (!hdr10p_md_param.num_bezier_curve_anchors) {*/
	/*	hdr10p_md_param.num_bezier_curve_anchors = 9;*/
	/*	pr_hdr("hdr10p_md_param.num_bezier_curve_anchors = 0\n");*/
	/*}*/

	hdr10p_md_param.knee_point_x =
	(vf->prop.hdr10p_data.pb15_18_st.knee_point_x_9_6 << 6) |
		vf->prop.hdr10p_data.pb15_18_st.knee_point_x_5_0;
	pr_hdr("vf:knee_point_x_5_0 = 0x%x, knee_point_x_9_6 = 0x%x\n",
		vf->prop.hdr10p_data.pb15_18_st.knee_point_x_5_0,
		vf->prop.hdr10p_data.pb15_18_st.knee_point_x_9_6);

	hdr10p_md_param.knee_point_y =
	(vf->prop.hdr10p_data.pb15_18_st.knee_point_y_9_8 << 8) |
		vf->prop.hdr10p_data.pb15_18_st.knee_point_y_7_0;
	pr_hdr("vf:knee_point_y_7_0 = 0x%x, knee_point_y_9_8 = 0x%x\n",
		vf->prop.hdr10p_data.pb15_18_st.knee_point_y_7_0,
		vf->prop.hdr10p_data.pb15_18_st.knee_point_y_9_8);

	/*bezier curve*/
	hdr10p_md_param.bezier_curve_anchors[0] =
		vf->prop.hdr10p_data.pb15_18_st.bezier_curve_anchors0;

	memcpy(&hdr10p_md_param.bezier_curve_anchors[1],
		&vf->prop.hdr10p_data.bezier_curve_anchors1,
		sizeof(uint8_t) * 8);
	for (i = 0; i < 9; i++)
		pr_hdr("vf:hdr10p_md_param.bezier_curve_anchors[%d] = 0x%x\n",
			i, hdr10p_md_param.bezier_curve_anchors[i]);

	hdr10p_md_param.graphics_overlay_flag =
		vf->prop.hdr10p_data.pb27_st.overlay_flag;
	hdr10p_md_param.no_delay_flag =
		vf->prop.hdr10p_data.pb27_st.no_delay_flag;
	pr_hdr("vf:overlay_flag = 0x%x\n",
		vf->prop.hdr10p_data.pb27_st.overlay_flag);
	pr_hdr("vf:no_delay_flag = 0x%x\n",
		vf->prop.hdr10p_data.pb27_st.no_delay_flag);

	hdr10p_sei.application_identifier =
		hdr10p_md_param.application_version;
	pr_hdr("hdr10p_sei.application_identifier = %d\n",
		hdr10p_sei.application_identifier);

	/*hdr10 plus default one window*/
	hdr10p_sei.num_windows = 1;

	hdr10p_sei.tgt_sys_disp_max_lumi =
		hdr10p_md_param.targeted_max_lum << 5;
	pr_hdr("hdr10p_sei.tgt_sys_disp_max_lumi = %d\n",
		hdr10p_sei.tgt_sys_disp_max_lumi);

	hdr10p_sei.average_maxrgb[0] =
		(hdr10p_md_param.average_maxrgb << 4) * 10;
	pr_hdr("hdr10p_sei.average_maxrgb[0] = %d\n",
		hdr10p_sei.average_maxrgb[0]);

	for (i = 0; i < 9; i++) {
		if (i == 2) {
			hdr10p_sei.distribution_maxrgb_percentiles[0][2] =
				hdr10p_md_param.distribution_values[2];
			pr_hdr("hdr10p_sei.distribution_");
			pr_hdr("maxrgb_percentiles[0][%d] = %d\n",
				i, p->distribution_maxrgb_percentiles[0][i]);
			continue;
		}

		hdr10p_sei.distribution_maxrgb_percentiles[0][i] =
			(hdr10p_md_param.distribution_values[i] << 4) * 10;
		pr_hdr("hdr10p_sei.distribution_");
		pr_hdr("maxrgb_percentiles[0][%d] = %d\n",
			i, p->distribution_maxrgb_percentiles[0][i]);
	}

	hdr10p_sei.num_bezier_curve_anchors[0] =
		hdr10p_md_param.num_bezier_curve_anchors;
	pr_hdr("hdr10p_sei.num_bezier_curve_anchors[0] = %d\n",
		hdr10p_sei.num_bezier_curve_anchors[0]);

	hdr10p_sei.knee_point_x[0] =
		hdr10p_md_param.knee_point_x << 2;
	hdr10p_sei.knee_point_y[0] =
		hdr10p_md_param.knee_point_y << 2;
	pr_hdr("hdr10p_sei.knee_point_x[0] = %d\n",
		hdr10p_sei.knee_point_x[0]);
	pr_hdr("hdr10p_sei.knee_point_y[0] = %d\n",
		hdr10p_sei.knee_point_y[0]);

	for (i = 0; i < 9; i++) {
		hdr10p_sei.bezier_curve_anchors[0][i] =
			hdr10p_md_param.bezier_curve_anchors[i] << 2;
		pr_hdr("hdr10p_sei.bezier_curve_anchors[0][%d] = %d\n",
			i, hdr10p_sei.bezier_curve_anchors[0][i]);
	}

	for (i = 0; i < 9; i++) {
		if (hdr10p_sei.bezier_curve_anchors[0][i] != 0) {
			hdr10p_sei.tone_mapping_flag[0] = 1;
			break;
		}
	}

	hdr10p_sei.color_saturation_mapping_flag[0] = 0;
}

void hdr10p_parser_metadata(struct vframe_s *vf)
{
	struct provider_aux_req_s req;
	char *p;
	unsigned int size = 0;
	unsigned int type = 0;
	int i;
	int j;
	char *meta_buf;
	int len;
	u32 count = 0;
	u32 offest = 0;

	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
		hdr10p_vf_md_parse(vf);
	} else if (vf->source_type == VFRAME_SOURCE_TYPE_OTHERS) {
		req.vf = vf;
		req.bot_flag = 0;
		req.aux_buf = NULL;
		req.aux_size = 0;
		req.dv_enhance_exist = 0;
		req.low_latency = 0;
		if (get_vframe_src_fmt(vf) ==
			VFRAME_SIGNAL_FMT_HDR10PLUS) {
			size = 0;
			req.aux_buf = (char *)get_sei_from_src_fmt(vf, &size);
			req.aux_size = size;
		} else {
			vf_notify_provider_by_name("vdec.h265.00",
				VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
				(void *)&req);

			if (!req.aux_buf)
				vf_notify_provider_by_name("decoder",
					VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
					(void *)&req);
		}

		if (req.aux_buf && req.aux_size && debug_hdr) {
			meta_buf = req.aux_buf;
			pr_hdr("hdr10 source_type = %d src_fmt = %d metadata(%d):\n",
				get_vframe_src_fmt(vf),
				vf->source_type, req.aux_size);

			for (i = 0; i < req.aux_size + 8; i += 8) {
				len = req.aux_size - i;
				if (len < 8) {
					pr_hdr("\t i = %02d", i);
					for (j = 0; j < len; j++)
						pr_hdr("%02x ", meta_buf[i + j]);
					pr_hdr("\n");
				} else {
					pr_hdr("\t i = %02d: %02x %02x %02x",
						i,
						meta_buf[i],
						meta_buf[i + 1],
						meta_buf[i + 2]);
					pr_hdr(" %02x %02x %02x %02x %02x\n",
						meta_buf[i + 3],
						meta_buf[i + 4],
						meta_buf[i + 5],
						meta_buf[i + 6],
						meta_buf[i + 7]);
				}
			}
		}

		if (req.aux_buf && req.aux_size) {
			count = 0;
			offest = 0;

			p = req.aux_buf;
			while (p < req.aux_buf
				+ req.aux_size - 8) {
				size = *p++;
				size = (size << 8) | *p++;
				size = (size << 8) | *p++;
				size = (size << 8) | *p++;
				type = *p++;
				type = (type << 8) | *p++;
				type = (type << 8) | *p++;
				type = (type << 8) | *p++;
				offest += 8;

				if (offest + size > req.aux_size) {
					pr_err("%s exception: t:%x, s:%d,",
						__func__,
						type, size);
					pr_err(" p:%px, c:%d, offt:%d, aux:%px, s:%d\n",
						p, count, offest,
						req.aux_buf, req.aux_size);
					break;
				}

				if (type == 0x02000000)
					parse_sei(p, size);

				if ((type & 0xffff0000) == 0x14000000)/*av1 hdr10p*/
					parse_sei_av1(p, size);

				count++;
				offest += size;
				p += size;
			}
		}
	}

	if (debug_hdr >= 1)
		debug_hdr--;
}

static struct hdr10plus_para dbg_hdr10plus_pkt;

void hdr10p_hdmitx_vsif_parser(struct vframe_s *vf,
	struct hdr10plus_para *hdmitx_hdr10plus_param)
{
	int vsif_tds_max_l;
	int ave_maxrgb;
	int distribution_values[9];
	int i;
	int kpx, kpy;
	int bz_cur_anchors[9];
	u32 maxrgb_99_percentiles = 0;

	memset(hdmitx_hdr10plus_param, 0, sizeof(struct hdr10plus_para));

	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI) {
		hdmitx_hdr10plus_param->application_version =
			vf->prop.hdr10p_data.pb4_st.app_ver;
		hdmitx_hdr10plus_param->targeted_max_lum =
			vf->prop.hdr10p_data.pb4_st.max_lumin;
		hdmitx_hdr10plus_param->average_maxrgb =
			vf->prop.hdr10p_data.average_maxrgb;
		/*distribution value*/
		memcpy(&hdmitx_hdr10plus_param->distribution_values[0],
			&vf->prop.hdr10p_data.distrib_valus0,
			sizeof(uint8_t) * 9);

		hdmitx_hdr10plus_param->num_bezier_curve_anchors =
		vf->prop.hdr10p_data.pb15_18_st.num_bezier_curve_anchors;

		hdmitx_hdr10plus_param->knee_point_x =
		(vf->prop.hdr10p_data.pb15_18_st.knee_point_x_9_6 << 6) |
			vf->prop.hdr10p_data.pb15_18_st.knee_point_x_5_0;
		hdmitx_hdr10plus_param->knee_point_y =
		(vf->prop.hdr10p_data.pb15_18_st.knee_point_y_9_8 << 8) |
			vf->prop.hdr10p_data.pb15_18_st.knee_point_y_7_0;
		/*bezier curve*/
		hdmitx_hdr10plus_param->bezier_curve_anchors[0] =
			vf->prop.hdr10p_data.pb15_18_st.bezier_curve_anchors0;
		memcpy(&hdmitx_hdr10plus_param->bezier_curve_anchors[1],
			&vf->prop.hdr10p_data.bezier_curve_anchors1,
			sizeof(uint8_t) * 8);

		hdmitx_hdr10plus_param->graphics_overlay_flag =
			vf->prop.hdr10p_data.pb27_st.overlay_flag;
		hdmitx_hdr10plus_param->no_delay_flag =
			vf->prop.hdr10p_data.pb27_st.no_delay_flag;

		memcpy(&dbg_hdr10plus_pkt, hdmitx_hdr10plus_param,
			sizeof(struct hdr10plus_para));

		return;
	}

	hdmitx_hdr10plus_param->application_version =
		(u8)hdr10p_sei.application_version;

	if (hdr10p_sei.tgt_sys_disp_max_lumi < 1024) {
		vsif_tds_max_l =
			(hdr10p_sei.tgt_sys_disp_max_lumi + (1 << 4)) >> 5;

		if (vsif_tds_max_l > 31)
			vsif_tds_max_l = 31;
	} else {
		vsif_tds_max_l = 31;
	}

	hdmitx_hdr10plus_param->targeted_max_lum = (u8)vsif_tds_max_l;

	ave_maxrgb = hdr10p_sei.average_maxrgb[0] / 10;
	if (ave_maxrgb < (1 << 12)) {
		ave_maxrgb = (ave_maxrgb + (1 << 3)) >> 4;
		if (ave_maxrgb > 255)
			ave_maxrgb = 255;
	} else {
		ave_maxrgb = 255;
	}

	hdmitx_hdr10plus_param->average_maxrgb = (u8)ave_maxrgb;

	for (i = 0; i < 9; i++) {
		if (i == 2 &&
			/* V0 sei update */
			hdmitx_hdr10plus_param->application_version != 0) {
			distribution_values[i] =
				hdr10p_sei.distribution_maxrgb_percentiles[0][i];
			hdmitx_hdr10plus_param->distribution_values[i] =
				(u8)distribution_values[i];
			continue;
		}

		distribution_values[i] =
			hdr10p_sei.distribution_maxrgb_percentiles[0][i] / 10;
		/* V0 sei update */
		if (hdr10p_sei.num_distribution_maxrgb_percentiles[0] == 10 &&
			hdmitx_hdr10plus_param->application_version == 0 &&
			i == 8) {
			maxrgb_99_percentiles =
				hdr10p_sei.distribution_maxrgb_percentiles[0][i + 1];
			distribution_values[i] = maxrgb_99_percentiles / 10;
		}

		if (distribution_values[i] < (1 << 12)) {
			distribution_values[i] =
				(distribution_values[i] + (1 << 3)) >> 4;
			if (distribution_values[i] > 255)
				distribution_values[i] = 255;
		} else {
			distribution_values[i] = 255;
		}

		hdmitx_hdr10plus_param->distribution_values[i] =
			(u8)distribution_values[i];
	}

	if (hdr10p_sei.tone_mapping_flag[0] == 0) {
		hdmitx_hdr10plus_param->num_bezier_curve_anchors = 0;
		hdmitx_hdr10plus_param->knee_point_x = 0;
		hdmitx_hdr10plus_param->knee_point_y = 0;

		for (i = 0; i < 9; i++)
			hdmitx_hdr10plus_param->bezier_curve_anchors[0] = 0;
	} else {
		hdmitx_hdr10plus_param->num_bezier_curve_anchors =
			(u8)hdr10p_sei.num_bezier_curve_anchors[0];

		if (hdmitx_hdr10plus_param->num_bezier_curve_anchors > 9)
			hdmitx_hdr10plus_param->num_bezier_curve_anchors = 9;

		kpx = hdr10p_sei.knee_point_x[0];
		kpx = (kpx + (1 << 1)) >> 2;
		if (kpx > 1023)
			kpx = 1023;
		hdmitx_hdr10plus_param->knee_point_x = kpx;

		kpy = hdr10p_sei.knee_point_y[0];
		kpy = (kpy + (1 << 1)) >> 2;
		if (kpy > 1023)
			kpy = 1023;
		hdmitx_hdr10plus_param->knee_point_y = kpy;

		for (i = 0; i < 9; i++) {
			if (i ==
			hdmitx_hdr10plus_param->num_bezier_curve_anchors)
				break;

			bz_cur_anchors[i] =
				hdr10p_sei.bezier_curve_anchors[0][i];
			bz_cur_anchors[i] = (bz_cur_anchors[i] + (1 << 1)) >> 2;
			if (bz_cur_anchors[i] > 255)
				bz_cur_anchors[i] = 255;
			hdmitx_hdr10plus_param->bezier_curve_anchors[i] =
				(u8)bz_cur_anchors[i];
		}
	}

	/*only video, don't include graphic*/
	hdmitx_hdr10plus_param->graphics_overlay_flag = 0;
	/*metadata and video have no delay*/
	hdmitx_hdr10plus_param->no_delay_flag = 1;

	memcpy(&dbg_hdr10plus_pkt, hdmitx_hdr10plus_param,
		sizeof(struct hdr10plus_para));
}

void hdr10p_debug(void)
{
	int i = 0;
	int j = 0;
	struct vframe_hdr10p_sei *p;

	p = &hdr10p_sei;

	pr_info("itu_t_t35_country_code = 0x%x\n",
		hdr10p_sei.itu_t_t35_country_code);
	pr_info("itu_t_t35_terminal_provider_code = 0x%x\n",
		hdr10p_sei.itu_t_t35_terminal_provider_code);
	pr_info("itu_t_t35_terminal_provider_oriented_code = 0x%x\n",
		hdr10p_sei.itu_t_t35_terminal_provider_oriented_code);
	pr_info("application_identifier = 0x%x\n",
		hdr10p_sei.application_identifier);
	pr_info("application_version = 0x%x\n",
		hdr10p_sei.application_version);
	pr_info("num_windows = 0x%x\n",
		hdr10p_sei.num_windows);

	for (i = 1; i < hdr10p_sei.num_windows; i++) {
		pr_info("window_upper_left_corner_x[%d] = 0x%x\n",
			i, hdr10p_sei.window_upper_left_corner_x[i]);
		pr_info("window_upper_left_corner_y[%d] = 0x%x\n",
			i, hdr10p_sei.window_upper_left_corner_y[i]);
		pr_info("window_lower_right_corner_x[%d] = 0x%x\n",
			i, hdr10p_sei.window_lower_right_corner_x[i]);
		pr_info("window_lower_right_corner_y[%d] = 0x%x\n",
			i, hdr10p_sei.window_lower_right_corner_y[i]);
		pr_info("center_of_ellipse_x[%d] = 0x%x\n",
			i, hdr10p_sei.center_of_ellipse_x[i]);
		pr_info("center_of_ellipse_y[%d] = 0x%x\n",
			i, hdr10p_sei.center_of_ellipse_y[i]);
		pr_info("rotation_angle[%d] = 0x%x\n",
			i, hdr10p_sei.rotation_angle[i]);
		pr_info("semimajor_axis_internal_ellipse[%d] = 0x%x\n",
			i, hdr10p_sei.semimajor_axis_internal_ellipse[i]);
		pr_info("semimajor_axis_external_ellipse[%d] = 0x%x\n",
			i, hdr10p_sei.semimajor_axis_external_ellipse[i]);
		pr_info("semiminor_axis_external_ellipse[%d] = 0x%x\n",
			i, hdr10p_sei.semiminor_axis_external_ellipse[i]);
		pr_info("overlap_process_option[%d] = 0x%x\n",
			i, hdr10p_sei.overlap_process_option[i]);
	}

	pr_info("targeted_system_display_maximum_luminance = 0x%x\n",
		hdr10p_sei.tgt_sys_disp_max_lumi);
	pr_info("targeted_system_display_actual_peak_luminance_flag = 0x%x\n",
		hdr10p_sei.tgt_sys_disp_act_pk_lumi_flag);
	if (hdr10p_sei.tgt_sys_disp_act_pk_lumi_flag) {
		for (i = 0;
			i < hdr10p_sei.num_rows_tgt_sys_disp_act_pk_lumi;
			i++) {
			for (j = 0;
				j < hdr10p_sei.num_cols_tgt_sys_disp_act_pk_lumi;
				j++) {
				pr_info("tgt_sys_disp_act_pk_lumi");
				pr_info("[%d][%d] = 0x%x\n", i, j,
					hdr10p_sei.tgt_sys_disp_act_pk_lumi[i][j]);
			}
		}
	}

	for (i = 0; i < hdr10p_sei.num_windows; i++) {
		for (j = 0; j < 3; j++)
			pr_info("maxscl[%d][%d] = 0x%x\n",
				i, j, hdr10p_sei.maxscl[i][j]);

		pr_info("average_maxrgb[%d] = 0x%x\n",
			i, hdr10p_sei.average_maxrgb[i]);
		pr_info("num_distribution_maxrgb_percentiles[%d] = 0x%x\n",
			i, hdr10p_sei.num_distribution_maxrgb_percentiles[i]);

		for (j = 0;
			j < hdr10p_sei.num_distribution_maxrgb_percentiles[i];
			j++) {
			pr_info("distribution_maxrgb_pcntages[%d][%d] = 0x%x\n",
				i, j,
				hdr10p_sei.distribution_maxrgb_percentages[i][j]);
			pr_info("distribution_maxrgb_pcntiles[%d][%d] = 0x%x\n",
				i, j,
				hdr10p_sei.distribution_maxrgb_percentiles[i][j]);
		}

		pr_info("fraction_bright_pixels[%d] = 0x%x\n",
			i, hdr10p_sei.fraction_bright_pixels[i]);
	}

	pr_info("mast_disp_act_pk_lumi_flag = 0x%x\n",
		hdr10p_sei.mast_disp_act_pk_lumi_flag);
	if (hdr10p_sei.mast_disp_act_pk_lumi_flag) {
		pr_info("num_rows_mast_disp_act_pk_lumi = 0x%x\n",
			hdr10p_sei.num_rows_mast_disp_act_pk_lumi);
		pr_info("num_cols_mast_disp_act_pk_lumi = 0x%x\n",
			hdr10p_sei.num_cols_mast_disp_act_pk_lumi);
		for (i = 0;
			i < hdr10p_sei.num_rows_mast_disp_act_pk_lumi;
			i++) {
			for (j = 0;
				j < hdr10p_sei.num_cols_mast_disp_act_pk_lumi;
				j++) {
				pr_info("mast_disp_act_pk_lumi");
				pr_info("[%d][%d] = 0x%x\n",
					i, j,
					p->mast_disp_act_pk_lumi[i][j]);
			}
		}
	}

	for (i = 0; i < hdr10p_sei.num_windows; i++) {
		pr_info("tone_mapping_flag[%d] = 0x%x\n",
			i, hdr10p_sei.tone_mapping_flag[i]);
		pr_info("knee_point_x[%d] = 0x%x\n",
			i, hdr10p_sei.knee_point_x[i]);
		pr_info("knee_point_y[%d] = 0x%x\n",
			i, hdr10p_sei.knee_point_y[i]);
		pr_info("num_bezier_curve_anchors[%d] = 0x%x\n",
			i, hdr10p_sei.num_bezier_curve_anchors[i]);
		for (j = 0; j < hdr10p_sei.num_bezier_curve_anchors[i]; j++)
			pr_info("bezier_curve_anchors[%d][%d] = 0x%x\n",
				i, j, hdr10p_sei.bezier_curve_anchors[i][j]);

		pr_info("color_saturation_mapping_flag[%d] = 0x%x\n",
			i, hdr10p_sei.color_saturation_mapping_flag[i]);
		pr_info("color_saturation_weight[%d] = 0x%x\n",
			i, hdr10p_sei.color_saturation_weight[i]);
	}

	pr_info("\ntx vsif packet data print begin\n");
	pr_info("application_version = 0x%x\n",
		dbg_hdr10plus_pkt.application_version);
	pr_info("targeted_max_lum = 0x%x\n",
		dbg_hdr10plus_pkt.targeted_max_lum);
	pr_info("average_maxrgb = 0x%x\n",
		dbg_hdr10plus_pkt.average_maxrgb);
	for (i = 0; i < 9; i++)
		pr_info("distribution_values[%d] = 0x%x\n",
			i, dbg_hdr10plus_pkt.distribution_values[i]);
	pr_info("num_bezier_curve_anchors = 0x%x\n",
		dbg_hdr10plus_pkt.num_bezier_curve_anchors);
	pr_info("knee_point_x = 0x%x\n",
		dbg_hdr10plus_pkt.knee_point_x);
	pr_info("knee_point_y = 0x%x\n",
		dbg_hdr10plus_pkt.knee_point_y);

	for (i = 0; i < 9; i++)
		pr_info("bezier_curve_anchors[%d] = 0x%x\n",
			i, dbg_hdr10plus_pkt.bezier_curve_anchors[i]);
	pr_info("graphics_overlay_flag = 0x%x\n",
		dbg_hdr10plus_pkt.graphics_overlay_flag);
	pr_info("no_delay_flag = 0x%x\n",
		dbg_hdr10plus_pkt.no_delay_flag);
	pr_info("\ntx vsif packet data print end\n");

	pr_info(HDR10_PLUS_VERSION);
}

