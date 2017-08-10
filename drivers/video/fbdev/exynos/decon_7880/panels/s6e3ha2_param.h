#ifndef __S6E3HA2_PARAM_H__
#define __S6E3HA2_PARAM_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include "dynamic_aid_s6e3ha2.h"

#define EXTEND_BRIGHTNESS	355
#define UI_MAX_BRIGHTNESS	255
#define UI_MIN_BRIGHTNESS	0
#define UI_DEFAULT_BRIGHTNESS	128
#define NORMAL_TEMPERATURE	25	/* 25 degrees Celsius */

#define GAMMA_CMD_CNT		ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET)
#define ACL_CMD_CNT		ARRAY_SIZE(SEQ_ACL_OFF)
#define OPR_CMD_CNT		ARRAY_SIZE(SEQ_OPR_ACL_OFF)
#define ELVSS_CMD_CNT		ARRAY_SIZE(SEQ_ELVSS_SET)
#define AID_CMD_CNT		ARRAY_SIZE(SEQ_AID_SETTING)
#define HBM_CMD_CNT		ARRAY_SIZE(SEQ_HBM_OFF)
#define TSET_CMD_CNT		ARRAY_SIZE(SEQ_TSET_SETTING)
#define VINT_CMD_CNT		ARRAY_SIZE(SEQ_VINT_SETTING)

#define LDI_REG_ELVSS		0xB6
#define LDI_REG_COORDINATE	0xA1
#define LDI_REG_DATE		LDI_REG_MTP
#define LDI_REG_ID		0x04
#define LDI_REG_CHIP_ID		0xD6
#define LDI_REG_MTP		0xC8
#define LDI_REG_HBM		0xB4
#define LDI_REG_RDDPM		0x0A
#define LDI_REG_RDDSM		0x0E
#define LDI_REG_ESDERR		0xEE

/* len is read length */
#define LDI_LEN_ELVSS		(ELVSS_CMD_CNT - 1)
#define LDI_LEN_COORDINATE	4
#define LDI_LEN_DATE		7
#define LDI_LEN_ID		3
#define LDI_LEN_CHIP_ID		5
#define LDI_LEN_MTP		35
#define LDI_LEN_HBM		31
#define LDI_LEN_RDDPM		1
#define LDI_LEN_RDDSM		1
#define LDI_LEN_ESDERR		1

/* offset is position including addr, not only para */
#define LDI_OFFSET_AOR_1	1
#define LDI_OFFSET_AOR_2	2

#define LDI_OFFSET_VINT_1	1
#define LDI_OFFSET_VINT_2	2

#define LDI_OFFSET_ELVSS_1	1	/* B6h 1st Para: MPS_CON */
#define LDI_OFFSET_ELVSS_2	2	/* B6h 2nd Para: ELVSS_Dim_offset */
#define LDI_OFFSET_ELVSS_3	22	/* B6h 22nd Para: ELVSS Temp Compensation */

#define LDI_OFFSET_HBM		1
#define LDI_OFFSET_ACL		1
#define LDI_OFFSET_TSET		1

#define LDI_GPARA_DATE		40	/* 0xC8 41st Para */
#define LDI_GPARA_HBM_ELVSS	22	/* 0xB6 23th para */

struct lcd_seq_info {
	unsigned char	*cmd;
	unsigned int	len;
	unsigned int	sleep;
};


static const unsigned char SEQ_TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char SEQ_TEST_KEY_ON_F1[] = {
	0xF1,
	0x5A, 0x5A,
};

static const unsigned char SEQ_TEST_KEY_OFF_F1[] = {
	0xF1,
	0xA5, 0xA5,
};

static const unsigned char SEQ_TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char SEQ_TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5
};

static const unsigned char SEQ_SLEEP_OUT[] = {
	0x11
};

static const unsigned char SEQ_SLEEP_IN[] = {
	0x10
};

static const unsigned char SEQ_DISPLAY_ON[] = {
	0x29
};

static const unsigned char SEQ_DISPLAY_OFF[] = {
	0x28
};

static const unsigned char SEQ_DUAL_DSI_1[] = {
	0xF2,
	0x63
};

static const unsigned char SEQ_GAMMA_CONDITION_SET[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x00, 0x00, 0x00,
	0x00, 0x00
};

static const unsigned char SEQ_AOR_CONTROL[] = {
	0xB2,
	0x03, 0x10
};

static const unsigned char SEQ_VINT_SETTING[] = {
	0xF4,
	0x8B, 0x21
};

static const unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03
};

static const unsigned char SEQ_GAMMA_UPDATE_L[] = {
	0xF7,
	0x00,
};

static const unsigned char SEQ_TSET_GLOBAL[] = {
	0xB0,
	0x07
};

static const unsigned char SEQ_HBM_GLOBAL[] = {
	0xB0,
	0x01
};

static const unsigned char SEQ_TE_ON[] = {
	0x35,
	0x00
};
static const unsigned char SEQ_TE_OFF[] = {
	0x34,
};

static const unsigned char SEQ_TSP_TE[] = {
	0xBD,
	0x11, 0x11, 0x02, 0x16, 0x02, 0x16
};

static const unsigned char SEQ_PENTILE_SETTING[] = {
	0xC0,
	0x00, 0x00, 0xD8, 0xD8
};

static const unsigned char SEQ_POC_SETTING1[] = {
	0xB0,
	0x20
};

static const unsigned char SEQ_POC_SETTING2[] = {
	0xFE,
	0x08
};

static const unsigned char SEQ_OSC_SETTING1[] = {
	0xFD,
	0x1C
};

static const unsigned char SEQ_OSC_SETTING2[] = {
	0xFE,
	0xA0
};

static const unsigned char SEQ_OSC_SETTING3[] = {
	0xFE,
	0x20
};


static const unsigned char SEQ_PCD_SETTING[] = {
	0xCC,
	0x4C
};

static const unsigned char SEQ_ERR_FG_SETTING[] = {
	0xED,
	0x44
};

static unsigned char SEQ_HBM_OFF[] = {
	0x53,
	0x00
};

static unsigned char SEQ_HBM_ON[] = {
	0x53,
	0xC0
};

static const unsigned char SEQ_ACL_OFF_OPR[] = {
	0xB5,
	0x40
};

static const unsigned char SEQ_ACL_ON_OPR[] = {
	0xB5,
	0x50
};


static const unsigned char SEQ_TSET[] = {
	0xB8,
	0x19	/* Global para(8th) + 25 degrees  : 0x19 */
};

static const unsigned int VINT_DIM_TABLE[] = {
	5,	6,	7,	8,	9,
	10,	11,	12,	13,	14
};

static unsigned char SEQ_AID_SETTING[] = {
	0xB2,
	0x00, 0x10
};

static unsigned char SEQ_ELVSS_SET[] = {
	0xB6,
	0xBC,	/* B6h 1st Para: MPS_CON */
	0x04,	/* B6h 2nd Para: ELVSS_Dim_offset */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	0x00,	/* B6h 22nd Para: ELVSS Temp Compensation */
	0x00	/* B6h 23rd Para: OTP for B6h 22nd Para of HBM Interpolation */
};

static unsigned char SEQ_OPR_ACL_OFF[] = {
	0xB5,
	0x40	/* 16 Frame Avg. at ACL Off */
};

static unsigned char SEQ_OPR_ACL_ON[] = {
	0xB5,
	0x50	/* 32 Frame Avg. at ACL On */
};

static unsigned char SEQ_ACL_OFF[] = {
	0x55,
	0x00
};

static unsigned char SEQ_ACL_ON[] = {
	0x55,
	0x02	/* 0x02 : ACL 8% (Default) */
};

static unsigned char SEQ_TSET_SETTING[] = {
	0xB8,
	0x19	/* (ex) 25 degree : 0x19 */
};

enum {
	ACL_STATUS_0P,
	ACL_STATUS_8P,
	ACL_STATUS_MAX
};

enum {
	OPR_STATUS_ACL_OFF,
	OPR_STATUS_ACL_ON,
	OPR_STATUS_MAX
};

enum {
	CAPS_OFF,
	CAPS_ON,
	CAPS_MAX
};

enum {
	TEMP_ABOVE_MINUS_00_DEGREE,	/* T > 0 */
	TEMP_ABOVE_MINUS_15_DEGREE,	/* -15 < T <= 0 */
	TEMP_BELOW_MINUS_15_DEGREE,	/* T <= -15 */
	TEMP_MAX
};

enum {
	HBM_STATUS_OFF,
	HBM_STATUS_ON,
	HBM_STATUS_MAX
};

static unsigned char *HBM_TABLE[HBM_STATUS_MAX] = {SEQ_HBM_OFF, SEQ_HBM_ON};
static unsigned char *ACL_TABLE[ACL_STATUS_MAX] = {SEQ_ACL_OFF, SEQ_ACL_ON};
static unsigned char *OPR_TABLE[OPR_STATUS_MAX] = {SEQ_OPR_ACL_OFF, SEQ_OPR_ACL_ON};

static unsigned char elvss_mpscon_offset_data[IBRIGHTNESS_HBM_MAX][3] = {
	[IBRIGHTNESS_002NIT] = {0xB6, 0x8C, 0x12},
	[IBRIGHTNESS_003NIT] = {0xB6, 0x8C, 0x13},
	[IBRIGHTNESS_004NIT] = {0xB6, 0x8C, 0x14},
	[IBRIGHTNESS_005NIT] = {0xB6, 0x8C, 0x15},
	[IBRIGHTNESS_006NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_007NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_008NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_009NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_010NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_011NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_012NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_013NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_014NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_015NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_016NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_017NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_019NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_020NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_021NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_022NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_024NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_025NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_027NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_029NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_030NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_032NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_034NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_037NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_039NIT] = {0xB6, 0x8C, 0x16},
	[IBRIGHTNESS_041NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_044NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_047NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_050NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_053NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_056NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_060NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_064NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_068NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_072NIT] = {0xB6, 0x9C, 0x16},
	[IBRIGHTNESS_077NIT] = {0xB6, 0x9C, 0x15},
	[IBRIGHTNESS_082NIT] = {0xB6, 0x9C, 0x15},
	[IBRIGHTNESS_087NIT] = {0xB6, 0x9C, 0x15},
	[IBRIGHTNESS_093NIT] = {0xB6, 0x9C, 0x14},
	[IBRIGHTNESS_098NIT] = {0xB6, 0x9C, 0x14},
	[IBRIGHTNESS_105NIT] = {0xB6, 0x9C, 0x14},
	[IBRIGHTNESS_111NIT] = {0xB6, 0x9C, 0x13},
	[IBRIGHTNESS_119NIT] = {0xB6, 0x9C, 0x13},
	[IBRIGHTNESS_126NIT] = {0xB6, 0x9C, 0x12},
	[IBRIGHTNESS_134NIT] = {0xB6, 0x9C, 0x12},
	[IBRIGHTNESS_143NIT] = {0xB6, 0x9C, 0x11},
	[IBRIGHTNESS_152NIT] = {0xB6, 0x9C, 0x10},
	[IBRIGHTNESS_162NIT] = {0xB6, 0x9C, 0x10},
	[IBRIGHTNESS_172NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_183NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_195NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_207NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_220NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_234NIT] = {0xB6, 0x9C, 0x0F},
	[IBRIGHTNESS_249NIT] = {0xB6, 0x9C, 0x0E},
	[IBRIGHTNESS_265NIT] = {0xB6, 0x9C, 0x0D},
	[IBRIGHTNESS_282NIT] = {0xB6, 0x9C, 0x0D},
	[IBRIGHTNESS_300NIT] = {0xB6, 0x9C, 0x0C},
	[IBRIGHTNESS_316NIT] = {0xB6, 0x9C, 0x0B},
	[IBRIGHTNESS_333NIT] = {0xB6, 0x9C, 0x0B},
	[IBRIGHTNESS_360NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_382NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_407NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_433NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_461NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_490NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_516NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_544NIT] = {0xB6, 0x9C, 0x0A},
	[IBRIGHTNESS_600NIT] = {0xB6, 0x9C, 0x0A}
};

struct elvss_otp_info {
	unsigned int	nit;
	int not_otp[TEMP_MAX];
};

struct elvss_otp_info elvss_otp_data[IBRIGHTNESS_MAX] = {
	[IBRIGHTNESS_002NIT] = {2,	{0, 1, -3}}, // target = OTP VALUE + [array value]
	[IBRIGHTNESS_003NIT] = {3,	{0, 2, -2}},
	[IBRIGHTNESS_004NIT] = {4,	{0, 3, -1}},
	[IBRIGHTNESS_005NIT] = {5,	{0, 4, 0}},
};

static unsigned char VINT_TABLE[IBRIGHTNESS_HBM_MAX][AID_CMD_CNT] = {
	/* T > -20 */
	{0xF4, 0x8B, 0x18}, /* IBRIGHTNESS_002NIT */
	{0xF4, 0x8B, 0x18}, /* IBRIGHTNESS_003NIT */
	{0xF4, 0x8B, 0x18}, /* IBRIGHTNESS_004NIT */
	{0xF4, 0x8B, 0x18}, /* IBRIGHTNESS_005NIT */
	{0xF4, 0x8B, 0x19}, /* IBRIGHTNESS_006NIT */
	{0xF4, 0x8B, 0x1A}, /* IBRIGHTNESS_007NIT */
	{0xF4, 0x8B, 0x1B}, /* IBRIGHTNESS_008NIT */
	{0xF4, 0x8B, 0x1C}, /* IBRIGHTNESS_009NIT */
	{0xF4, 0x8B, 0x1D}, /* IBRIGHTNESS_010NIT */
	{0xF4, 0x8B, 0x1E}, /* IBRIGHTNESS_011NIT */
	{0xF4, 0x8B, 0x1F}, /* IBRIGHTNESS_012NIT */
	{0xF4, 0x8B, 0x20}, /* IBRIGHTNESS_013NIT */
	/* T <= -20 */
	[IBRIGHTNESS_014NIT ... IBRIGHTNESS_600NIT] = {0xF4, 0x8B, 0x21},
};

static unsigned char AOR_TABLE[IBRIGHTNESS_HBM_MAX][AID_CMD_CNT] = {
	{0xB2, 0xE3, 0x90},
	{0xB2, 0xC6, 0x90},
	{0xB2, 0xB2, 0x90},
	{0xB2, 0x95, 0x90},
	{0xB2, 0x81, 0x90},
	{0xB2, 0x67, 0x90},
	{0xB2, 0x54, 0x90},
	{0xB2, 0x42, 0x90},
	{0xB2, 0x30, 0x90},
	{0xB2, 0x16, 0x90},
	{0xB2, 0x05, 0x90},
	{0xB2, 0xF3, 0x80},
	{0xB2, 0xE3, 0x80},
	{0xB2, 0xC7, 0x80},
	{0xB2, 0xB5, 0x80},
	{0xB2, 0xA1, 0x80},
	{0xB2, 0x72, 0x80},
	{0xB2, 0x56, 0x80},
	{0xB2, 0x45, 0x80},
	{0xB2, 0x33, 0x80},
	{0xB2, 0x06, 0x80},
	{0xB2, 0xF2, 0x70},
	{0xB2, 0xC4, 0x70},
	{0xB2, 0x95, 0x70},
	{0xB2, 0x84, 0x70},
	{0xB2, 0x56, 0x70},
	{0xB2, 0x26, 0x70},
	{0xB2, 0xE4, 0x60},
	{0xB2, 0xB6, 0x60},
	{0xB2, 0x88, 0x60},
	{0xB2, 0x45, 0x60},
	{0xB2, 0xFC, 0x50},
	{0xB2, 0xB6, 0x50},
	{0xB2, 0x74, 0x50},
	{0xB2, 0x2C, 0x50},
	{0xB2, 0xC8, 0x40},
	{0xB2, 0x69, 0x40},
	{0xB2, 0x08, 0x40},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0xA5, 0x30},
	{0xB2, 0x27, 0x30},
	{0xB2, 0xA3, 0x20},
	{0xB2, 0x2C, 0x20},
	{0xB2, 0x9F, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	{0xB2, 0x03, 0x10},
	[IBRIGHTNESS_382NIT ... IBRIGHTNESS_600NIT] = {0xB2, 0x03, 0x10}
};

/* platform brightness <-> gamma level */
static unsigned int brightness_table[EXTEND_BRIGHTNESS + 1] = {
	[0 ... 2] =		IBRIGHTNESS_002NIT,
	[3 ... 3] =		IBRIGHTNESS_003NIT,
	[4 ... 4] =		IBRIGHTNESS_004NIT,
	[5 ... 5] =		IBRIGHTNESS_005NIT,
	[6 ... 6] =		IBRIGHTNESS_006NIT,
	[7 ... 7] =		IBRIGHTNESS_007NIT,
	[8 ... 8] =		IBRIGHTNESS_008NIT,
	[9 ... 9] =		IBRIGHTNESS_009NIT,
	[10 ... 10] =		IBRIGHTNESS_010NIT,
	[11 ... 11] =		IBRIGHTNESS_011NIT,
	[12 ... 12] =		IBRIGHTNESS_012NIT,
	[13 ... 13] =		IBRIGHTNESS_013NIT,
	[14 ... 14] =		IBRIGHTNESS_014NIT,
	[15 ... 15] =		IBRIGHTNESS_015NIT,
	[16 ... 16] =		IBRIGHTNESS_016NIT,
	[17 ... 17] =		IBRIGHTNESS_017NIT,
	[18 ... 18] =		IBRIGHTNESS_019NIT,
	[19 ... 19] =		IBRIGHTNESS_020NIT,
	[20 ... 20] =		IBRIGHTNESS_021NIT,
	[21 ... 21] =		IBRIGHTNESS_022NIT,
	[22 ... 22] =		IBRIGHTNESS_024NIT,
	[23 ... 23] =		IBRIGHTNESS_025NIT,
	[24 ... 24] =		IBRIGHTNESS_027NIT,
	[25 ... 25] =		IBRIGHTNESS_029NIT,
	[26 ... 26] =		IBRIGHTNESS_030NIT,
	[27 ... 27] =		IBRIGHTNESS_032NIT,
	[28 ... 28] =		IBRIGHTNESS_034NIT,
	[29 ... 29] =		IBRIGHTNESS_037NIT,
	[30 ... 30] =		IBRIGHTNESS_039NIT,
	[31 ... 32] =		IBRIGHTNESS_041NIT,
	[33 ... 34] =		IBRIGHTNESS_044NIT,
	[35 ... 36] =		IBRIGHTNESS_047NIT,
	[37 ... 38] =		IBRIGHTNESS_050NIT,
	[39 ... 40] =		IBRIGHTNESS_053NIT,
	[41 ... 43] =		IBRIGHTNESS_056NIT,
	[44 ... 46] =		IBRIGHTNESS_060NIT,
	[47 ... 49] =		IBRIGHTNESS_064NIT,
	[50 ... 52] =		IBRIGHTNESS_068NIT,
	[53 ... 56] =		IBRIGHTNESS_072NIT,
	[57 ... 59] =		IBRIGHTNESS_077NIT,
	[60 ... 63] =		IBRIGHTNESS_082NIT,
	[64 ... 67] =		IBRIGHTNESS_087NIT,
	[68 ... 71] =		IBRIGHTNESS_093NIT,
	[72 ... 76] =		IBRIGHTNESS_098NIT,
	[77 ... 80] =		IBRIGHTNESS_105NIT,
	[81 ... 86] =		IBRIGHTNESS_111NIT,
	[87 ... 91] =		IBRIGHTNESS_119NIT,
	[92 ... 97] =		IBRIGHTNESS_126NIT,
	[98 ... 104] =		IBRIGHTNESS_134NIT,
	[105 ... 110] =		IBRIGHTNESS_143NIT,
	[111 ... 118] =		IBRIGHTNESS_152NIT,
	[119 ... 125] =		IBRIGHTNESS_162NIT,
	[126 ... 133] =		IBRIGHTNESS_172NIT,
	[134 ... 142] =		IBRIGHTNESS_183NIT,
	[143 ... 150] =		IBRIGHTNESS_195NIT,
	[151 ... 160] =		IBRIGHTNESS_207NIT,
	[161 ... 170] =		IBRIGHTNESS_220NIT,
	[171 ... 181] =		IBRIGHTNESS_234NIT,
	[182 ... 193] =		IBRIGHTNESS_249NIT,
	[194 ... 205] =		IBRIGHTNESS_265NIT,
	[206 ... 218] =		IBRIGHTNESS_282NIT,
	[219 ... 230] =		IBRIGHTNESS_300NIT,
	[231 ... 242] =		IBRIGHTNESS_316NIT,
	[243 ... 254] =		IBRIGHTNESS_333NIT,
	[255 ... 354] =		IBRIGHTNESS_360NIT,
	[EXTEND_BRIGHTNESS ... EXTEND_BRIGHTNESS] =		IBRIGHTNESS_600NIT
};

#endif /* __S6E3HA2_PARAM_H__ */
