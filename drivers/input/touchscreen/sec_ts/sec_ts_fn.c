/* Samsung Touchscreen Controller Driver.
 * Copyright (c) 2007-2012, Samsung Electronics
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/firmware.h>
#include <linux/sec_sysfs.h>

#include "sec_ts.h"
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/trustedui.h>
#endif

#define FACTORY_MODE
#define tostring(x) (#x)

#ifdef FACTORY_MODE
#include <linux/uaccess.h>

#define FT_CMD(name, func) .cmd_name = name, .cmd_func = func

extern struct class *sec_class;

enum {
	TYPE_RAW_DATA = 0,		/*  Tota cap - offset(19) = remnant dta */
	TYPE_SIGNAL_DATA = 1,
	TYPE_AMBIENT_BASELINE = 2,	/* Cap Baseline */
	TYPE_AMBIENT_DATA = 3,		/* Cap Ambient */
	TYPE_REMV_BASELINE_DATA = 4,
	TYPE_DECODED_DATA = 5,
	TYPE_REMV_AMB_DATA = 6,
	TYPE_OFFSET_DATA_SEC = 19,	/* Cap Offset for Normal Touch */
	TYPE_OFFSET_DATA_SDC = 29,	/* Cap Offset in SDC */
	TYPE_INVALID_DATA = 0xFF, /* Invalid data type for release factory mode*/
};

enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

struct ft_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count);
static ssize_t cmd_status_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t cmd_result_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t scrub_position_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, cmd_store);
static DEVICE_ATTR(cmd_status, S_IRUGO, cmd_status_show, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, cmd_result_show, NULL);
static DEVICE_ATTR(scrub_pos, S_IRUGO, scrub_position_show, NULL);

static int execute_selftest(struct sec_ts_data *ts);

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_checksum_data(void *device_data);
static void run_reference_read(void *device_data);
static void run_reference_read_all(void *device_data);
static void get_reference(void *device_data);
static void run_rawcap_read(void *device_data);
static void run_rawcap_read_all(void *device_data);
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void run_delta_read_all(void *device_data);
static void get_delta(void *device_data);
static void run_self_reference_read(void *device_data);
static void run_self_reference_read_all(void *device_data);
static void run_self_delta_read(void *device_data);
static void run_self_delta_read_all(void *device_data);
static void get_selfcap(void *device_data);
static void run_force_calibration(void *device_data);
static void get_force_calibration(void *device_data);
static void set_tsp_test_result(void *device_data);
static void get_tsp_test_result(void *device_data);
static void run_trx_short_test(void *device_data);
static void glove_mode(void *device_data);
static void hover_enable(void *device_data);
static void clear_cover_mode(void *device_data);
static void dead_zone_enable(void *device_data);
#ifdef SMARTCOVER_COVER
static void smartcover_cmd(void *device_data);
#endif
static void set_lowpower_mode(void *device_data);
static void spay_enable(void *device_data);
static void edge_swipe_enable(void *device_data);
static void set_log_level(void *device_data);

static int get_tsp_nvm_data(struct sec_ts_data *ts);
static void not_support_cmd(void *device_data);
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern int tui_force_close(uint32_t arg);
#endif

struct ft_cmd ft_cmds[] = {
	{FT_CMD("fw_update", fw_update),},
	{FT_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{FT_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{FT_CMD("get_config_ver", get_config_ver),},
	{FT_CMD("get_threshold", get_threshold),},
	{FT_CMD("module_off_master", module_off_master),},
	{FT_CMD("module_on_master", module_on_master),},
	/* {FT_CMD("module_off_slave", not_support_cmd),}, */
	/* {FT_CMD("module_on_slave", not_support_cmd),}, */
	{FT_CMD("get_chip_vendor", get_chip_vendor),},
	{FT_CMD("get_chip_name", get_chip_name),},
	{FT_CMD("get_x_num", get_x_num),},
	{FT_CMD("get_y_num", get_y_num),},
	{FT_CMD("get_checksum_data", get_checksum_data),},
	{FT_CMD("run_reference_read", run_reference_read),},
	{FT_CMD("run_reference_read_all", run_reference_read_all),},
	{FT_CMD("get_reference", get_reference),},
	{FT_CMD("run_rawcap_read", run_rawcap_read),},
	{FT_CMD("run_rawcap_read_all", run_rawcap_read_all),},
	{FT_CMD("get_rawcap", get_rawcap),},
	{FT_CMD("run_delta_read", run_delta_read),},
	{FT_CMD("run_delta_read_all", run_delta_read_all),},
	{FT_CMD("get_delta", get_delta),},
	{FT_CMD("run_self_reference_read", run_self_reference_read),},
	{FT_CMD("run_self_reference_read_all", run_self_reference_read_all),},
	{FT_CMD("run_self_delta_read", run_self_delta_read),},
	{FT_CMD("run_self_delta_read_all", run_self_delta_read_all),},
	{FT_CMD("get_selfcap", get_selfcap),},
	{FT_CMD("run_force_calibration", run_force_calibration),},
	{FT_CMD("get_force_calibration", get_force_calibration),},
	{FT_CMD("run_trx_short_test", run_trx_short_test),},
	{FT_CMD("set_tsp_test_result", set_tsp_test_result),},
	{FT_CMD("get_tsp_test_result", get_tsp_test_result),},
	{FT_CMD("glove_mode", glove_mode),},
	{FT_CMD("hover_enable", hover_enable),},
	{FT_CMD("clear_cover_mode", clear_cover_mode),},
	{FT_CMD("dead_zone_enable", dead_zone_enable),},
#ifdef SMARTCOVER_COVER
	{FT_CMD("smartcover_cmd", smartcover_cmd),},
#endif
	{FT_CMD("set_lowpower_mode", set_lowpower_mode),},
	{FT_CMD("spay_enable", spay_enable),},
	{FT_CMD("edge_swipe_enable", edge_swipe_enable),},
	{FT_CMD("set_log_level", set_log_level),},
	{FT_CMD("not_support_cmd", not_support_cmd),},
};

static struct attribute *cmd_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_scrub_pos.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

static int sec_ts_check_index(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	int node;

	if (ts->cmd_param[0] < 0
		|| ts->cmd_param[0] > ts->rx_count
		|| ts->cmd_param[1] < 0
		|| ts->cmd_param[1] > ts->tx_count) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		strncat(ts->cmd_result, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_FAIL;
		tsp_debug_info(true, &ts->client->dev, "%s: parameter error: %u, %u\n",
				__func__, ts->cmd_param[0], ts->cmd_param[0]);
		node = -1;
		return node;
	}
	node = ts->cmd_param[1] * ts->rx_count + ts->cmd_param[0];
	tsp_debug_info(true, &ts->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static int sec_ts_check_self_index(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	int node;

	if (ts->cmd_param[0] < 0
		|| ts->cmd_param[0] > (ts->tx_count + ts->rx_count)) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		strncat(ts->cmd_result, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_FAIL;
		tsp_debug_info(true, &ts->client->dev, "%s: parameter error: %u, %u\n",
				__func__, ts->cmd_param[0], ts->cmd_param[0]);
		node = -1;
		return node;
	}
	node = ts->cmd_param[0];
	tsp_debug_info(true, &ts->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static void set_default_result(struct sec_ts_data *data)
{
	char delim = ':';

	memset(data->cmd_result, 0x00, CMD_RESULT_STR_LEN);
	memcpy(data->cmd_result, data->cmd, strnlen(data->cmd, CMD_STR_LEN));
	strncat(data->cmd_result, &delim, 1);

	return;
}

static void set_cmd_result(struct sec_ts_data *data, char *buf, int length)
{
	strncat(data->cmd_result, buf, length);

	return;
}

static void clear_cover_cmd_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
													cover_cmd_work.work);
	if (ts->cmd_is_running) {
		schedule_delayed_work(&ts->cover_cmd_work, msecs_to_jiffies(5));
	} else {
		/* check lock   */
		mutex_lock(&ts->cmd_lock);
		ts->cmd_is_running = true;
		mutex_unlock(&ts->cmd_lock);

		ts->cmd_state = CMD_STATUS_RUNNING;
		tsp_debug_info(true, &ts->client->dev, "%s param = %d, %d\n", __func__,\
		ts->delayed_cmd_param[0], ts->delayed_cmd_param[1]);

		ts->cmd_param[0] = ts->delayed_cmd_param[0];
		if (ts->delayed_cmd_param[0] > 1)
			ts->cmd_param[1] = ts->delayed_cmd_param[1];
		strcpy(ts->cmd, "clear_cover_mode");
		clear_cover_mode(ts);
	}
}
EXPORT_SYMBOL(clear_cover_cmd_work);

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned char param_cnt = 0;
	char *start;
	char *end;
	char *pos;
	char delim = ',';
	char buffer[CMD_STR_LEN];
	bool cmd_found = false;
	int *param;
	int length;
	struct ft_cmd *ft_cmd_ptr = NULL;
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int i;

	if (!ts) {
		printk(KERN_ERR "%s: No platform data found\n", __func__);
		return -EINVAL;
	}

	if (strlen(buf) >= CMD_STR_LEN) {		
		tsp_debug_err(true, &ts->client->dev, "%s: cmd length is over (%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (count >= (unsigned int)CMD_STR_LEN) {
		tsp_debug_err(true, &ts->client->dev, "%s:  cmd length(count) is over (%d,%s)!!\n",
				 __func__, (unsigned int)count, buf);
		return -EINVAL;
	}
#if 1
	if (ts->cmd_is_running == true) {
		tsp_debug_err(true, &ts->client->dev, "%s: other cmd is running.\n", __func__);
		if (strncmp("clear_cover_mode", buf, 16) == 0) {
			cancel_delayed_work(&ts->cover_cmd_work);
			tsp_debug_err(true, &ts->client->dev,
						"[cmd is delayed] %d, param = %d, %d\n", __LINE__, buf[17] - '0', buf[19] - '0');
			ts->delayed_cmd_param[0] = buf[17] - '0';
			if (ts->delayed_cmd_param[0] > 1)
				ts->delayed_cmd_param[1] = buf[19]-'0';
				schedule_delayed_work(&ts->cover_cmd_work, msecs_to_jiffies(10));
		}
		return -EBUSY;
	}
	else if (ts->reinit_done == false) {
		tsp_debug_err(true, &ts->client->dev, "ft_cmd: reinit is working\n");
		printk(KERN_ERR "fts_cmd: reinit is working\n");
		if (strncmp("clear_cover_mode", buf, 16) == 0) {
			cancel_delayed_work(&ts->cover_cmd_work);
			tsp_debug_err(true, &ts->client->dev,
						"[cmd is delayed] %d, param = %d, %d\n", __LINE__, buf[17]-'0', buf[19]-'0');
			ts->delayed_cmd_param[0] = buf[17] - '0';
			if (ts->delayed_cmd_param[0] > 1)
				ts->delayed_cmd_param[1] = buf[19]-'0';
			if(ts->delayed_cmd_param[0] == 0)
				schedule_delayed_work(&ts->cover_cmd_work, msecs_to_jiffies(300));
		}
	}
#endif
	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = true;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = CMD_STATUS_RUNNING;

	length = (int)count;
	if (*(buf + length - 1) == '\n')
		length--;

	memset(ts->cmd, 0x00, sizeof(ts->cmd));
	memcpy(ts->cmd, buf, length);
	memset(ts->cmd_param, 0, sizeof(ts->cmd_param));
	memset(buffer, 0x00, sizeof(buffer));

	pos = strchr(buf, (int)delim);
	if (pos)
		memcpy(buffer, buf, pos - buf);
	else
		memcpy(buffer, buf, length);

	/* find command */
	list_for_each_entry(ft_cmd_ptr, &ts->cmd_list_head, list) {
		if (!strcmp(buffer, ft_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(ft_cmd_ptr,
			&ts->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", ft_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cmd_found && pos) {
		pos++;
		start = pos;
		memset(buffer, 0x00, sizeof(buffer));
		do {
			if ((*pos == delim) || (pos - buf == length)) {
				end = pos;
				memcpy(buffer, start, end - start);
				*(buffer + strlen(buffer)) = '\0';
				param = ts->cmd_param + param_cnt;
				if (kstrtoint(buffer, 10, param) < 0)
					goto err_out;
				param_cnt++;
				memset(buffer, 0x00, sizeof(buffer));
				start = pos + 1;
			}
			pos++;
		} while ((pos - buf <= length) && (param_cnt < CMD_PARAM_NUM));
	}

	tsp_debug_err(true, &ts->client->dev, "%s: Command = %s\n", __func__, buf);
	for (i = 0; i < param_cnt; i++)
		tsp_debug_info(true, &ts->client->dev, "cmd param %d= %d\n", i,
			  ts->cmd_param[i]);

	ft_cmd_ptr->cmd_func(ts);

err_out:
	return count;
}

static ssize_t cmd_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	char buffer[16];

	tsp_debug_err(true, &ts->client->dev, "%s: Command status = %d\n", __func__, ts->cmd_state);

	switch (ts->cmd_state) {
	case CMD_STATUS_WAITING:
		snprintf(buffer, sizeof(buffer), "%s", tostring(WAITING));
		break;
	case CMD_STATUS_RUNNING:
		snprintf(buffer, sizeof(buffer), "%s", tostring(RUNNING));
		break;
	case CMD_STATUS_OK:
		snprintf(buffer, sizeof(buffer), "%s", tostring(OK));
		break;
	case CMD_STATUS_FAIL:
		snprintf(buffer, sizeof(buffer), "%s", tostring(FAIL));
		break;
	case CMD_STATUS_NOT_APPLICABLE:
		snprintf(buffer, sizeof(buffer), "%s", tostring(NOT_APPLICABLE));
		break;
	default:
		snprintf(buffer, sizeof(buffer), "%s", tostring(NOT_APPLICABLE));
		break;
	}

	return snprintf(buf, CMD_RESULT_STR_LEN, "%s\n", buffer);
}

static ssize_t cmd_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	tsp_debug_info(true, &ts->client->dev, "%s: Command result = %s\n", __func__, ts->cmd_result);

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = CMD_STATUS_WAITING;

	return snprintf(buf, CMD_RESULT_STR_LEN, "%s\n", ts->cmd_result);
}

static ssize_t scrub_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	char buff[256] = { 0 };

	tsp_debug_info(true, &ts->client->dev,
				"%s: scrub_id: %d, X:%d, Y:%d\n", __func__,
				ts->scrub_id, ts->scrub_x, ts->scrub_y);

	snprintf(buff, sizeof(buff), "%d %d %d", ts->scrub_id, ts->scrub_x, ts->scrub_y);

	ts->scrub_id = 0;
	ts->scrub_x = 0;
	ts->scrub_y = 0;

	return snprintf(buf, PAGE_SIZE, "%s", buff);
}

static void fw_update(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[64] = { 0 };
	int retval = 0;

	set_default_result(ts);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
			 __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = sec_ts_firmware_update_on_hidden_menu(ts, ts->cmd_param[0]);
	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_FAIL;
		tsp_debug_info(true, &ts->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_OK;
		tsp_debug_info(true, &ts->client->dev, "%s: success [%d]\n", __func__, retval);
	}
	return;
}

static int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_OFF};
	u8 tBuff[2] = { mode, state };

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE , tBuff, sizeof(tBuff));
	sec_ts_delay(20);
	return ret;
}

static int sec_ts_release_tmode(struct sec_ts_data *ts)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_ON};
	u8 tBuff[2] = {TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH};

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE , tBuff, sizeof(tBuff));
	sec_ts_delay(20);

	return ret;
}

void sec_ts_print_frame(struct sec_ts_data *ts, short *min, short *max)
{
	int i = 0;
	int j = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };

	pStr = kzalloc(6 * (ts->tx_count + 1), GFP_KERNEL);
	if (pStr == NULL) {
		tsp_debug_info(true, &ts->client->dev, "SEC_TS pStr kzalloc failed\n");
		return;
	}

	memset(pStr, 0x0, 6 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), "    ");
	strncat(pStr, pTmp, 6 * ts->tx_count);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(pTmp, sizeof(pTmp), "Tx%02d  ", i);
		strncat(pStr, pTmp, 6 * ts->tx_count);
	}

	tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);
	memset(pStr, 0x0, 6 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strncat(pStr, pTmp, 6 * ts->tx_count);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strncat(pStr, pTmp, 6 * ts->rx_count);
	}

	tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);

	for (i = 0; i < ts->rx_count; i++) {
		memset(pStr, 0x0, 6 * (ts->tx_count + 1));
		snprintf(pTmp, sizeof(pTmp), "Rx%02d | ", i);
		strncat(pStr, pTmp, 6 * ts->tx_count);

		for (j = 0; j < ts->tx_count; j++) {
			snprintf(pTmp, sizeof(pTmp), "%5d ", ts->pFrame[(j * ts->rx_count) + i]);

			if (i > 0) {
				if (ts->pFrame[(j * ts->rx_count) + i] < *min)
					*min = ts->pFrame[(j * ts->rx_count) + i];

				if (ts->pFrame[(j * ts->rx_count) + i] > *max)
					*max = ts->pFrame[(j * ts->rx_count) + i];
			}
			strncat(pStr, pTmp, 6 * ts->rx_count);
		}
	tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);
	}
	kfree(pStr);
}

int sec_ts_read_frame(struct sec_ts_data *ts, u8 type, short *min,
                 short *max)
{
	unsigned int readbytes = 0xFF;
	unsigned char *pRead = NULL;
	u8 mode = TYPE_INVALID_DATA;
	int rc = 0;
	int ret = 0;
	int i = 0;
	int j = 0;
	short *temp = NULL;

	tsp_debug_info(true, &ts->client->dev, "%s\n", __func__);

	/* set data length, allocation buffer memory */
	readbytes = ts->rx_count * ts->tx_count * 2;

	pRead = kzalloc(readbytes, GFP_KERNEL);
	if (pRead == NULL) {
		tsp_debug_info(true, &ts->client->dev, "Read frame kzalloc failed\n");
		rc = 1;
		return rc;
	}

	/* set OPCODE and data type */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &type, 1);
	if (ret < 0) {
		tsp_debug_info(true, &ts->client->dev, "Set rawdata type failed\n");
		rc = 2;
		goto ErrorExit;
	}

	sec_ts_delay(50);

	if (type == TYPE_OFFSET_DATA_SDC) {
		/* excute selftest for real cap offset data, because real cap data is not memory data in normal touch. */
		char para = TO_TOUCH_MODE;
		disable_irq(ts->client->irq);
		execute_selftest(ts);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		if (ret < 0) {
			tsp_debug_err(true, &ts->client->dev, "%s: set rawdata type failed!\n", __func__);
			goto ErrorRelease;
		}
		enable_irq(ts->client->irq);
		/* end */
	}

	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_RAWDATA, pRead, readbytes);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		rc = 3;
		goto ErrorRelease;
	}

	memset(ts->pFrame, 0x00, readbytes);

	for (i = 0; i < readbytes; i += 2)
		ts->pFrame[i/2] = pRead[i+1] + (pRead[i] << 8);

	*min = *max = ts->pFrame[0];

#ifdef DEBUG_MSG
	tsp_debug_info(true, &ts->client->dev, "02X%02X%02X readbytes=%d\n",
			pRead[0], pRead[1], pRead[2], readbytes);
#endif
	sec_ts_print_frame(ts, min, max);

	temp = kzalloc(readbytes, GFP_KERNEL);
	if (temp == NULL) {
		tsp_debug_info(true, &ts->client->dev, "Read frame kzalloc failed\n");
		goto ErrorRelease;
	}

	memcpy(temp, ts->pFrame, readbytes);
	memset(ts->pFrame, 0x00, readbytes);

	for (i = 0; i < ts->tx_count; i++) {
		for (j = 0; j < ts->rx_count; j++)
			ts->pFrame[(j * ts->tx_count) + i] = temp[(i * ts->rx_count) + j];
	}

	kfree(temp);

ErrorRelease:
	/* release data monitory (unprepare AFE data memory) */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &mode, 1);
	if (ret < 0)
		tsp_debug_err(true, &ts->client->dev, "%s: set rawdata failed!\n", __func__);

ErrorExit:
	kfree(pRead);

	return rc;
}

void sec_ts_print_self_frame(struct sec_ts_data *ts, short *min, short *max,
			unsigned int num_long_ch, unsigned int num_short_ch)
{
	int i = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };

	pStr = kzalloc(6 * (num_short_ch + 1), GFP_KERNEL);
	if (pStr == NULL) {
		tsp_debug_info(true, &ts->client->dev, "SEC_TS pStr kzalloc failed\n");
		return;
	}

	memset(pStr, 0x0, 6 * (num_short_ch + 1));
	snprintf(pTmp, sizeof(pTmp), "          ");
	strncat(pStr, pTmp, 6 * num_short_ch);

	for (i = 0; i < num_short_ch; i++) {
		snprintf(pTmp, sizeof(pTmp), "Sc%02d  ", i);
		strncat(pStr, pTmp, 6 * num_short_ch);
	}

	tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);
	memset(pStr, 0x0, 6 * (num_short_ch + 1));
	snprintf(pTmp, sizeof(pTmp), "      +");
	strncat(pStr, pTmp, 6 * num_short_ch);

	for (i = 0; i < num_short_ch; i++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strncat(pStr, pTmp, 6 * num_short_ch);
	}

	tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);

	memset(pStr, 0x0, 6 * (num_short_ch + 1));
	for (i = 0; i < num_short_ch; i++) {
		snprintf(pTmp, sizeof(pTmp), "%5d ", ts->sFrame[i]);
		strncat(pStr, pTmp, 6 * num_short_ch);
		if (ts->sFrame[i] < *min)
			*min = ts->sFrame[i];
		if (ts->sFrame[i] > *max)
			*max = ts->sFrame[i];
	}

	tsp_debug_info(true, &ts->client->dev, "SEC_TS        %s\n", pStr);

	for (i = 0; i < num_long_ch; i++) {
		memset(pStr, 0x0, 6 * (num_short_ch + 1));
		snprintf(pTmp, sizeof(pTmp), "Lc%02d | ", i);
		strncat(pStr, pTmp, 6 * num_short_ch);
		snprintf(pTmp, sizeof(pTmp), "%5d ", ts->sFrame[num_short_ch + i]);
		strncat(pStr, pTmp, 6 * num_short_ch);

		if (ts->sFrame[num_short_ch + i] < *min)
			*min = ts->sFrame[num_short_ch + i];
		if (ts->sFrame[num_short_ch + i] > *max)
			*max = ts->sFrame[num_short_ch + i];

		tsp_debug_info(true, &ts->client->dev, "SEC_TS %s\n", pStr);
	}
	kfree(pStr);
}

#define PRE_DEFINED_DATA_LENGTH		208
static int sec_ts_read_channel(struct sec_ts_data *ts, u8 type, short *min, short *max)
{
	unsigned char *pRead = NULL;
	int ret = 0;
	int ii = 0;
	int jj = 0;
	u8 w_data[2];
	u8 mode = TYPE_INVALID_DATA;

	tsp_debug_info(true, &ts->client->dev, "%s\n", __func__);

	pRead = kzalloc(PRE_DEFINED_DATA_LENGTH, GFP_KERNEL);
	if (IS_ERR_OR_NULL(pRead))
		return -ENOMEM;

	/* set OPCODE and data type */
	w_data[0] = type;

	ret = ts->sec_ts_i2c_write(ts,SEC_TS_CMD_SELF_RAW_TYPE, w_data, 1);
	if (ret < 0) {
		tsp_debug_info(true, &ts->client->dev, "Set rawdata type failed\n");
		goto out_read_channel;
	}

	sec_ts_delay(50);

	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_SELF_RAWDATA, pRead, PRE_DEFINED_DATA_LENGTH);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto err_read_data;
	}

	memset(ts->pFrame, 0x00, ts->tx_count * ts->rx_count * 2);

	/* d[00] ~ d[14] : TX channel
	 * d[15] ~ d[51] : none
	 * d[52] ~ d[77] : RX channel
	 * d[78] ~ d[103] : none */
	for (ii = 0; ii < PRE_DEFINED_DATA_LENGTH; ii += 2) {
		if ((ii >= (ts->tx_count * 2)) && (ii < (PRE_DEFINED_DATA_LENGTH / 2)))
			continue;
		if (ii >= ((PRE_DEFINED_DATA_LENGTH / 2) + (ts->rx_count * 2)))
			break;

		ts->pFrame[jj] = ((pRead[ii] << 8) | pRead[ii + 1]);

		if (ii == 0)
			*min = *max = ts->pFrame[jj];

		*min = min(*min, ts->pFrame[jj]);
		*max = max(*max, ts->pFrame[jj]);

		tsp_debug_info(true, &ts->client->dev, "%s: [%s][%d] %d\n", __func__,
				(jj < ts->tx_count) ? "TX" : "RX", jj, ts->pFrame[jj]);
		jj++;
	}
err_read_data:
	/* release data monitory (unprepare AFE data memory) */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELF_RAW_TYPE, &mode, 1);
	if (ret < 0)
		tsp_debug_err(true, &ts->client->dev, "%s: set rawdata failed!\n", __func__);
out_read_channel:
	kfree(pRead);
	return ret;
}


static int sec_ts_read_raw_data(struct sec_ts_data *ts, struct sec_ts_test_mode *mode)
{
	int ii;
	int ret = 0;
	char temp[CMD_STR_LEN] = { 0 };
	char *buff;

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff) {
		tsp_debug_info(true, &ts->client->dev, "%s: failed to alloc mem\n",
				__func__);
		goto error_alloc_mem;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	tsp_debug_info(true, &ts->client->dev, "%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->frame_channel)
		ret = sec_ts_read_channel(ts, mode->type, &mode->min, &mode->max);
	else
		ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->allnode) {
		if (mode->frame_channel) {
			for (ii = 0; ii < (ts->rx_count + ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, CMD_STR_LEN);
			}
		} else {
			for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, CMD_STR_LEN);
			}
		}
	} else {
		snprintf(buff, CMD_STR_LEN, "%d,%d", mode->min, mode->max);
	}

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
		goto error_test_fail;
	}

	set_cmd_result(ts, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));
	ts->cmd_state = CMD_STATUS_OK;

	kfree(buff);

	return ret;

error_test_fail:
error_power_state:
	kfree(buff);
error_alloc_mem:
	snprintf(temp, CMD_STR_LEN, "FAIL");
	set_cmd_result(ts, temp, CMD_STR_LEN);
	ts->cmd_state = CMD_STATUS_FAIL;

	return ret;
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };

	set_default_result(ts);

	sprintf(buff, "SE%02X%02X%02X",
		ts->plat_data->panel_revision, ts->plat_data->img_version_of_bin[2],
		ts->plat_data->img_version_of_bin[3]);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };
	u8 img_ver[4];
	int ret;

	set_default_result(ts);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, img_ver, 4);
	if(ret < 0)	{
		tsp_debug_info(true, &ts->client->dev, "%s: Image version read error\n ", __func__);
		ts->cmd_state = CMD_STATUS_FAIL;
	}

	sprintf(buff, "SE%02X%02X%02X",
		ts->plat_data->panel_revision, img_ver[2], img_ver[3]);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_config_ver(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[20] = { 0 };

	set_default_result(ts);

	sprintf(buff, "%s_SE_%02X%02X",
		ts->plat_data->project_name ?: ts->plat_data->model_name ?: SEC_TS_DEVICE_NAME,
		ts->plat_data->para_version_of_ic[2], ts->plat_data->para_version_of_ic[3]);

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_threshold(void *device_data)
{
    struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
    char buff[20] = { 0 };

    char w_param[1];
    char r_param[2];
    int threshold = 0;
    int ret;

    set_default_result(ts);

    if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
        char buff[CMD_STR_LEN] = { 0 };
        tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
        __func__);
        snprintf(buff, sizeof(buff), "%s", "TSP turned off");
        set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
        ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
        return;
    }

    w_param[0] = 0;
    ret = ts->sec_ts_i2c_write(ts, SEC_TS_READ_THRESHOLD, w_param, 1 );
    if(ret < 0)
            tsp_debug_err(true, &ts->client->dev, "%s: threshold write type failed. ret: %d\n", __func__, ret);

    ret = ts->sec_ts_i2c_read_bulk(ts, r_param, 2);
    if (ret < 0)
            tsp_debug_err(true, &ts->client->dev, "%s threshold read failed. ret: %d\n", __func__, ret);

    threshold = (r_param[0]<<8 | r_param[1]);
    snprintf(buff, sizeof(buff), "%d", threshold);

    set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
    ts->cmd_state = CMD_STATUS_OK;
    tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void module_off_master(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[3] = { 0 };
	int ret = 0;

	mutex_lock(&ts->lock);
	if (ts->power_status) {
		disable_irq(ts->client->irq);
		ts->power_status = SEC_TS_STATE_POWER_OFF;
	}
	mutex_unlock(&ts->lock);

	if (ts->plat_data->power)
		ts->plat_data->power(ts, false);
	else
		ret = 1;

	if (ret == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

    set_default_result(ts);
    set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
    if (strncmp(buff, "OK", 2) == 0)
    	ts->cmd_state = CMD_STATUS_OK;
    else
    	ts->cmd_state = CMD_STATUS_FAIL;
    tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
    char buff[3] = { 0 };
    int ret = 0;

    mutex_lock(&ts->lock);
    if (!ts->power_status) {
		enable_irq(ts->client->irq);
		ts->power_status = SEC_TS_STATE_POWER_ON;
    }
    mutex_unlock(&ts->lock);

    if (ts->plat_data->power) {
		ts->plat_data->power(ts, true);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
		if (ret < 0)
			tsp_debug_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
    } else
		ret = 1;

    if (ret == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
    else
		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(ts);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		ts->cmd_state = CMD_STATUS_OK;
	else
		ts->cmd_state = CMD_STATUS_FAIL;

	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };
	strncpy(buff, "SEC", sizeof(buff));
	set_default_result(ts);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };

	if (ts->plat_data->img_version_of_ic[0] == 2)
		strncpy(buff, "MC44", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 5)
		strncpy(buff, "A552", sizeof(buff));

	set_default_result(ts);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };

	set_default_result(ts);
	snprintf(buff, sizeof(buff), "%d", ts->tx_count);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = 2;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };

	set_default_result(ts);
	snprintf(buff, sizeof(buff), "%d", ts->rx_count);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[16] = { 0 };
	char csum_result[4] = { 0 };
	u8 nv_result;
	u8 cal_result;
	u8 temp = 0;
	u8 csum = 0;
	int ret, i;

	set_default_result(ts);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		goto err;
	}

	temp = DO_FW_CHECKSUM | DO_PARA_CHECKSUM;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_GET_CHECKSUM, &temp, 1);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: send get_checksum_cmd fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "SendCMDfail");
		goto err;
	}

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read_bulk(ts, csum_result, 4);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: read get_checksum result fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "ReadCSUMfail");
		goto err;
	}

	nv_result = get_tsp_nvm_data(ts);
	cal_result = sec_ts_read_calibration_report(ts);

	for (i = 0; i < 4; i++)
		csum += csum_result[i];

	csum += temp;
	csum += cal_result;
	csum = ~csum;

	tsp_debug_info(true, &ts->client->dev, "%s: checksum = %02X\n", __func__, csum);
	snprintf(buff, sizeof(buff), "%02X", csum);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	return;

err:
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
	return;
}

static void run_reference_read(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);
	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;

	sec_ts_read_raw_data(ts, &mode);
}

static void run_reference_read_all(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
}

static void get_reference(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	set_default_result(ts);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;

	sec_ts_read_raw_data(ts, &mode);
}

static void run_rawcap_read_all(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
}

static void get_rawcap(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	set_default_result(ts);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_delta_read(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;

	sec_ts_read_raw_data(ts, &mode);
}

static void run_delta_read_all(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
}

static void get_delta(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_self_reference_read(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.frame_channel= TEST_MODE_READ_CHANNEL;

	sec_ts_read_raw_data(ts, &mode);
}

static void run_self_reference_read_all(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.frame_channel= TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
}

static void run_self_delta_read(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.frame_channel= TEST_MODE_READ_CHANNEL;

	sec_ts_read_raw_data(ts, &mode);
}
static void run_self_delta_read_all(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_mode mode;

	set_default_result(ts);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.frame_channel= TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, &mode);
}

static void get_selfcap(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_self_index(ts);
	if (node < 0)
		return;

	val = ts->sFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

#if defined(CONFIG_SEC_DEBUG_TSP_LOG)
void sec_ts_run_rawdata_all(struct sec_ts_data *ts)
{
	short min, max;
	int ret;

	sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	ret = sec_ts_read_frame(ts, TYPE_OFFSET_DATA_SEC, &min, &max);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s, Offset error ## ret:%d\n", __func__, ret);
	} else {
		tsp_debug_err(true, &ts->client->dev, "%s, Offset Max/Min %d,%d ##\n", __func__, max, min);
		sec_ts_release_tmode(ts);
	}

	sec_ts_delay(20);

	sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	ret = sec_ts_read_frame(ts, TYPE_RAW_DATA, &min, &max);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s, raw error ## ret:%d\n", __func__, ret);
	} else {
		tsp_debug_err(true, &ts->client->dev, "%s, raw Max/Min %d,%d ##\n", __func__, max, min);
		sec_ts_release_tmode(ts);
	}
}
#endif

static int get_tsp_nvm_data(struct sec_ts_data *ts)
{
	char buff[2] = { 0 };
	int ret;

	tsp_debug_info(true, &ts->client->dev, "%s\n", __func__);

	/* send NV data using command
	 * Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 */
	memset(buff, 0x00, 2);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s nvm send command failed. ret: %d\n", __func__, ret);
		return ret;
	}

	sec_ts_delay(10);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */
	ret = ts->sec_ts_i2c_read_bulk(ts, buff, 1);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s nvm send command failed. ret: %d\n", __func__, ret);
		return ret;
	}

	tsp_debug_info(true, &ts->client->dev, "%s: %X\n", __func__, buff[0]);

	return buff[0];
}

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA modue(2) / OCTA Assy(1)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */

#define TEST_OCTA_ASSAY		1
#define TEST_OCTA_MODULE	2

#define TEST_OCTA_NONE		0
#define TEST_OCTA_FAIL		1
#define TEST_OCTA_PASS		2

static void set_tsp_test_result(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	struct sec_ts_test_result *result;
	char buff[CMD_STR_LEN] = { 0 };
	char r_data[1] = { 0 };
	int ret = 0;

	tsp_debug_info(true, &ts->client->dev, "%s\n", __func__);

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}
/*
	if ((ts->cmd_param[0] < 0) || (ts->cmd_param[0] > 1)
		|| (ts->cmd_param[1] < 0) || (ts->cmd_param[1]> 1)) {
		snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
		set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));

		ts->cmd_state = CMD_STATUS_FAIL;

		tsp_debug_info(true, &ts->client->dev, "%s: parameter error: %u, %u\n",
				__func__, ts->cmd_param[0], ts->cmd_param[1]);

		goto err_set_result;
	}
*/

	r_data[0] = get_tsp_nvm_data(ts);
	result = (struct sec_ts_test_result *)r_data;

	if (ts->cmd_param[0] == TEST_OCTA_ASSAY) {
		result->assy_result = ts->cmd_param[1];
		if (result->assy_count < 3)
			result->assy_count++;
	}

	if (ts->cmd_param[0] == TEST_OCTA_MODULE) {
		result->module_result = ts->cmd_param[1];
		if (result->module_count < 3)
			result->module_count++;
	}

	/* temporary code */
	if (ts->cmd_param[1] == 9)
		*result->data = ts->cmd_param[0];

	tsp_debug_err(true, &ts->client->dev, "%s: %d, %d, %d, %d, 0x%X\n", __func__,
			result->module_result, result->module_count,
			result->assy_result, result->assy_count, result->data[0]);

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	memset(buff, 0x00, CMD_STR_LEN);
	buff[2] = *result->data;

	tsp_debug_err(true, &ts->client->dev, "%s command (1)%X, (2)%X: %X\n",
				__func__, ts->cmd_param[0], ts->cmd_param[1], buff[2]);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		tsp_debug_err(true, &ts->client->dev, "%s nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(10);

	snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "OK");
	ts->cmd_state = CMD_STATUS_OK;

	set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
}

static void get_tsp_test_result(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	struct sec_ts_test_result *result;

	tsp_debug_info(true, &ts->client->dev, "%s\n", __func__);

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
	    tsp_debug_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
			    __func__);
	    snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
	    set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	    ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
	    return;
	}

	memset(buff, 0x00, CMD_STR_LEN);
	buff[0] = get_tsp_nvm_data(ts);

	result = (struct sec_ts_test_result *)buff;

	tsp_debug_info(true, &ts->client->dev, "%s: [0x%X][0x%X] M:%d, M:%d, A:%d, A:%d\n",
			__func__, *result->data, buff[0],
			result->module_result, result->module_count,
			result->assy_result, result->assy_count);

	snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "M:%s, M:%d, A:%s, A:%d\n",
			result->module_result == 0 ? "NONE" :
			result->module_result == 1 ? "FAIL" : "PASS", result->module_count,
			result->assy_result == 0 ? "NONE" :
			result->assy_result == 1 ? "FAIL" : "PASS", result->assy_count);

	ts->cmd_state = CMD_STATUS_OK;

	set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
}

#define GLOVE_MODE_EN (1 << 0)
#define CLEAR_COVER_EN (1 << 1)
#define FAST_GLOVE_MODE_EN (1 << 2)

static void glove_mode(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	int glove_mode_enables = 0;
	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1) {
		snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		if (ts->cmd_param[0])
			glove_mode_enables |= GLOVE_MODE_EN;
		else
			glove_mode_enables &= ~(GLOVE_MODE_EN);

		retval = sec_ts_glove_mode_enables(ts, glove_mode_enables);

		if (retval < 0) {
			tsp_debug_err(true, &ts->client->dev, "%s failed, retval = %d\n", __func__, retval);
			snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
			ts->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "OK");
			ts->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void hover_enable(void *device_data)
{
	int enables;
	int retval;
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;

	tsp_debug_info(true, &ts->client->dev, "%s: enter hover enable, param = %d\n", __func__, ts->cmd_param[0]);

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1) {
		snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
	} else {
		enables = ts->cmd_param[0];
		retval = sec_ts_hover_enables(ts, enables);

		if (retval < 0) {
			tsp_debug_err(true, &ts->client->dev, "%s failed, retval = %d\n", __func__, retval);
			snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
			ts->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "OK");
			ts->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));
	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	ts->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void clear_cover_mode(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };

	tsp_debug_info(true, &ts->client->dev, "%s: start clear_cover_mode %s\n", __func__, buff);
	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
	} else {
		if (ts->cmd_param[0] > 1) {
			ts->flip_enable = true;
			ts->cover_type = ts->cmd_param[1];
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){
				sec_ts_delay(100);
				tui_force_close(1);
				sec_ts_delay(200);
				if(TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()){
					trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
					trustedui_set_mode(TRUSTEDUI_MODE_OFF);
				}
			}
#endif // CONFIG_TRUSTONIC_TRUSTED_UI			
		} else {
			ts->flip_enable = false;
		}

		if (!(ts->power_status == SEC_TS_STATE_POWER_OFF) && ts->reinit_done) {
			if (ts->flip_enable)
				sec_ts_set_cover_type(ts, true);
			else
				sec_ts_set_cover_type(ts, false);
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		ts->cmd_state = CMD_STATUS_OK;
	}
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_WAITING;

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
};
#endif

static void dead_zone_enable(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	int ret;
	u8 data[2] = { 0 };

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	data[0] = ts->cmd_param[0];

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_DEADZONE, data, 1);
	if (ret < 0){
		tsp_debug_err(true, &ts->client->dev, "%s: write edge deadzone failed!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	ts->cmd_state = CMD_STATUS_OK;
out:
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;
};

#ifdef SMARTCOVER_COVER
void change_smartcover_table(struct sec_ts_data *ts)
{
	//struct factory_data *info = ts->f_data;
	u8 i, j, k, h, temp, temp_sum;

	for (i = 0; i < MAX_BYTE ; i++)
		for (j = 0; j < MAX_TX; j++)
			ts->changed_table[j][i] = ts->smart_cover[i][j];

#if 1 // debug
	tsp_debug_info(true, &ts->client->dev, "%s smart_cover value\n", __func__);
	for (i = 0; i < MAX_BYTE; i++){
		pr_cont("[sec_ts] ");
		for (j = 0; j < MAX_TX; j++)
			pr_cont("%d ", ts->smart_cover[i][j]);
		pr_cont("\n");
	}

	tsp_debug_info(true, &ts->client->dev, "%s changed_table value\n", __func__);
	for (j = 0; j < MAX_TX; j++){
		pr_cont("[sec_ts] ");
		for (i = 0; i < MAX_BYTE; i++)
			pr_cont("%d ", ts->changed_table[j][i]);
		pr_cont("\n");
	}
#endif

	tsp_debug_info(true, &ts->client->dev, "%s %d\n", __func__, __LINE__);

	for (i = 0; i < MAX_TX; i++)
		for (j = 0; j < 4; j++)
			ts->send_table[i][j] = 0;
	tsp_debug_info(true, &ts->client->dev, "%s %d\n", __func__, __LINE__);

	for (i = 0; i < MAX_TX; i++){
		temp = 0;
		for (j = 0; j < MAX_BYTE; j++)
			temp += ts->changed_table[i][j];
		if (temp == 0) continue;

		for(k = 0; k < 4; k++) {
			temp_sum = 0;
			for(h = 0; h < 8; h++)
				temp_sum += ((u8)(ts->changed_table[i][h+8*k])) << (7-h);

			ts->send_table[i][k] = temp_sum;
		}

		tsp_debug_info(true, &ts->client->dev, "i:%2d, %2X %2X %2X %2X \n",
			i,ts->send_table[i][0],ts->send_table[i][1],ts->send_table[i][2],ts->send_table[i][3]);
	}
	tsp_debug_info(true, &ts->client->dev, "%s %d\n", __func__, __LINE__);
}

void set_smartcover_mode(struct sec_ts_data *ts, bool on)
{
	//struct factory_data *info = ts->f_data;
	int ret;
	unsigned char regMon[2] = {0xC1, 0x0A};
	unsigned char regMoff[2] = {0xC2, 0x0A};

	if (on == 1){
		ret = ts->sec_ts_i2c_write(ts, regMon[0], regMon+1, 1);
		if (ret < 0)
			tsp_debug_err(true, &ts->client->dev, "%s mode on failed. ret: %d\n", __func__, ret);
	} else {
		ret = ts->sec_ts_i2c_write(ts, regMoff[0], regMoff+1, 1);
		if (ret < 0)
			tsp_debug_err(true, &ts->client->dev, "%s mode off failed. ret: %d\n", __func__, ret);
	}
}

void set_smartcover_clear(struct sec_ts_data *ts)
{
	//struct factory_ts *info = ts->f_ts;
	int ret;
	unsigned char regClr[6] = {0xC5, 0xFF, 0x00, 0x00, 0x00, 0x00};

	ret = ts->sec_ts_i2c_write(ts, regClr[0], regClr+1, 5);
	if (ret < 0)
		tsp_debug_err(true, &ts->client->dev, "%s ts clear failed. ret: %d\n", __func__, ret);
}

void set_smartcover_data(struct sec_ts_data *ts)
{
	int ret;
	u8 i, j;
	u8 temp=0;
	unsigned char regData[6] = {0xC5, 0x00, 0x00, 0x00, 0x00, 0x00};


	for (i = 0; i < MAX_TX; i++) {
		temp = 0;
		for (j = 0; j < 4; j++)
			temp += ts->send_table[i][j];
		if (temp == 0) continue;

		regData[1] = i;

		for (j = 0; j < 4; j++)
			regData[2+j] = ts->send_table[i][j];

		tsp_debug_info(true, &ts->client->dev, "i:%2d, %2X %2X %2X %2X \n",
			regData[1],regData[2],regData[3],regData[4], regData[5]);

		// data write
		ret = ts->sec_ts_i2c_write(ts, regData[0], regData+1, 5);
		if (ret < 0)
			tsp_debug_err(true, &ts->client->dev, "%s data write[%d] failed. ret: %d\n", __func__,i, ret);
	}
}

/* ####################################################
        func : smartcover_cmd [0] [1] [2] [3]
        index 0
                vlaue 0 : off (normal)
                vlaue 1 : off (globe mode)
                vlaue 2 :  X
                vlaue 3 : on
                                clear -> data send(send_table value) ->  mode on
                vlaue 4 : clear smart_cover value
                vlaue 5 : data save to smart_cover value
                        index 1 : tx channel num
                        index 2 : data 0xFF
                        index 3 : data 0xFF
                value 6 : table value change, smart_cover -> changed_table -> send_table

        ex)
        // clear
        echo smartcover_cmd,4 > cmd
        // data write (hart)
        echo smartcover_cmd,5,3,16,16 > cmd
        echo smartcover_cmd,5,4,56,56 > cmd
        echo smartcover_cmd,5,5,124,124 > cmd
        echo smartcover_cmd,5,6,126,252 > cmd
        echo smartcover_cmd,5,7,127,252 > cmd
        echo smartcover_cmd,5,8,63,248 > cmd
        echo smartcover_cmd,5,9,31,240 > cmd
        echo smartcover_cmd,5,10,15,224 > cmd
        echo smartcover_cmd,5,11,7,192 > cmd
        echo smartcover_cmd,5,12,3,128 > cmd
        // data change
        echo smartcover_cmd,6 > cmd
        // mode on
        echo smartcover_cmd,3 > cmd

###################################################### */
void smartcover_cmd(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	//struct factory_data *info = ts->f_data;
	int retval;
	char buff[CMD_STR_LEN] = { 0 };
	u8 i, j, t;

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 6) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		ts->cmd_state = CMD_STATUS_FAIL;
	} else {
		if (ts->cmd_param[0] == 0) {                      // off
			set_smartcover_mode(ts, 0);
			tsp_debug_info(true, &ts->client->dev, "%s mode off, normal\n", __func__);
		} else if(ts->cmd_param[0] == 1) {       // off, globe mode
			set_smartcover_mode(ts, 0);
			tsp_debug_info(true, &ts->client->dev, "%s mode off, globe mode\n", __func__);

			if(ts->glove_enables & (0x1 << 3)) {//SEC_TS_BIT_SETFUNC_GLOVE )
				retval = sec_ts_glove_mode_enables(ts, 1);
				if (retval < 0) {
					tsp_debug_err(true, &ts->client->dev,
						"%s: glove mode enable failed, retval = %d\n", __func__, retval);
					snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
					ts->cmd_state = CMD_STATUS_FAIL;
				} else {
					snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "OK");
					ts->cmd_state = CMD_STATUS_OK;
				}
			} else if (ts->glove_enables & (0x1 << 1)) {//SEC_TS_BIT_SETFUNC_HOVER )
				retval = sec_ts_hover_enables(ts, 1);
				if (retval < 0)	{
					tsp_debug_err(true, &ts->client->dev,
						"%s: hover enable failed, retval = %d\n", __func__, retval);
					snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "NG");
					ts->cmd_state = CMD_STATUS_FAIL;
				} else {
					snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "OK");
					ts->cmd_state = CMD_STATUS_OK;
				}
			}
			/*
			if (info->fast_mshover_enabled)
			fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
			else if (info->mshover_enabled)
			fts_command(info, FTS_CMD_MSHOVER_ON);
			*/
		} else if (ts->cmd_param[0] == 3) {       // on
			set_smartcover_clear(ts);
			set_smartcover_data(ts);
			tsp_debug_info(true, &ts->client->dev, "%s data send\n", __func__);
			set_smartcover_mode(ts, 1);
			tsp_debug_info(true, &ts->client->dev, "%s mode on\n", __func__);

		} else if (ts->cmd_param[0] == 4) {       // clear
			for (i = 0; i < MAX_BYTE; i++)
				for (j = 0; j < MAX_TX; j++)
					ts->smart_cover[i][j] = 0;
			tsp_debug_info(true, &ts->client->dev, "%s data clear\n", __func__);
		} else if (ts->cmd_param[0] == 5) {       // data write
			if (ts->cmd_param[1] < 0 ||  ts->cmd_param[1] >= 32) {
				tsp_debug_info(true, &ts->client->dev, "%s data tx size is over[%d]\n",
					__func__,ts->cmd_param[1]);
				snprintf(buff, sizeof(buff), "%s", "NG");
				ts->cmd_state = CMD_STATUS_FAIL;
				goto fail;
			}
			tsp_debug_info(true, &ts->client->dev, "%s data %2X, %2X, %2X\n", __func__, 
				ts->cmd_param[1],ts->cmd_param[2],ts->cmd_param[3] );

			t = ts->cmd_param[1];
			ts->smart_cover[t][0] = (ts->cmd_param[2]&0x80)>>7;
			ts->smart_cover[t][1] = (ts->cmd_param[2]&0x40)>>6;
			ts->smart_cover[t][2] = (ts->cmd_param[2]&0x20)>>5;
			ts->smart_cover[t][3] = (ts->cmd_param[2]&0x10)>>4;
			ts->smart_cover[t][4] = (ts->cmd_param[2]&0x08)>>3;
			ts->smart_cover[t][5] = (ts->cmd_param[2]&0x04)>>2;
			ts->smart_cover[t][6] = (ts->cmd_param[2]&0x02)>>1;
			ts->smart_cover[t][7] = (ts->cmd_param[2]&0x01);
			ts->smart_cover[t][8] = (ts->cmd_param[3]&0x80)>>7;
			ts->smart_cover[t][9] = (ts->cmd_param[3]&0x40)>>6;
			ts->smart_cover[t][10] = (ts->cmd_param[3]&0x20)>>5;
			ts->smart_cover[t][11] = (ts->cmd_param[3]&0x10)>>4;
			ts->smart_cover[t][12] = (ts->cmd_param[3]&0x08)>>3;
			ts->smart_cover[t][13] = (ts->cmd_param[3]&0x04)>>2;
			ts->smart_cover[t][14] = (ts->cmd_param[3]&0x02)>>1;
			ts->smart_cover[t][15] = (ts->cmd_param[3]&0x01);

		} else if (ts->cmd_param[0] == 6) {         // data change
			change_smartcover_table(ts);
			tsp_debug_info(true, &ts->client->dev, "%s data change\n", __func__);
		} else {
			tsp_debug_info(true, &ts->client->dev, "%s cmd[%d] not use\n", __func__, ts->cmd_param[0] );
			snprintf(buff, sizeof(buff), "%s", "NG");
			ts->cmd_state = CMD_STATUS_FAIL;
			goto fail;
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		ts->cmd_state = CMD_STATUS_OK;
	}
fail:
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
	ts->cmd_state = CMD_STATUS_WAITING;

	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
};
#endif


static void sec_ts_swap(u8 *a, u8 *b)
{
	u8 temp = *a;
	*a = *b;
	*b = temp;
}

static void rearrange_sft_result(u8 *data, int length)
{
	int i;

	for(i = 0; i < length; i+=4)
	{
		sec_ts_swap(&data[i], &data[i+3]);
		sec_ts_swap(&data[i+1], &data[i+2]);
	}
}

static int execute_selftest(struct sec_ts_data *ts)
{
	int rc;
	u8 tpara = 0x3;
	u8 *rBuff;
	int i;
	int result = 0;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;

	tsp_debug_info(true, &ts->client->dev, "%s: Self test start!\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, &tpara, 1);
	if (rc < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_exit;
	}
	sec_ts_delay(350);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	tsp_debug_info(true, &ts->client->dev, "%s: Self test done!\n", __func__);

	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff) {
		tsp_debug_err(true, &ts->client->dev, "%s: allocation failed!\n", __func__);
		goto err_exit;
	}

	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (rc < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}
	rearrange_sft_result(rBuff, result_size);

	for(i = 0; i < 80; i+=4){
		if(i%8==0) pr_cont("\n");
		if(i%4==0) pr_cont("sec_ts : ");

		if(i/4==0) pr_cont("SIG");
		else if(i/4==1) pr_cont("VER");
		else if(i/4==2) pr_cont("SIZ");
		else if(i/4==3) pr_cont("CRC");
		else if(i/4==4) pr_cont("RES");
		else if(i/4==5) pr_cont("COU");
		else if(i/4==6) pr_cont("PAS");
		else if(i/4==7) pr_cont("FAI");
		else if(i/4==8) pr_cont("CHA");
		else if(i/4==9) pr_cont("AMB");
		else if(i/4==10) pr_cont("RXS");
		else if(i/4==11) pr_cont("TXS");
		else if(i/4==12) pr_cont("RXO");
		else if(i/4==13) pr_cont("TXO");
		else if(i/4==14) pr_cont("RXG");
		else if(i/4==15) pr_cont("TXG");
		else if(i/4==16) pr_cont("RXR");
		else if(i/4==17) pr_cont("TXT");
		else if(i/4==18) pr_cont("RXT");
		else if(i/4==19) pr_cont("TXR");

		pr_cont(" %2X, %2X, %2X, %2X  ",rBuff[i],rBuff[i+1],rBuff[i+2],rBuff[i+3]);


		if (i/4 == 4) {
			if ((rBuff[i+3]&0x30) != 0)	// RX, RX open check.
				result = 0;
			else
				result = 1;
		}
	}

	return result;
err_exit:

	return 0;

}

static void run_trx_short_test(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = {0};
	int rc;
	char para = TO_TOUCH_MODE;

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(ts->client->irq);

	rc = execute_selftest(ts);
	if (rc) {

		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		enable_irq(ts->client->irq);
		snprintf(buff, sizeof(buff), "%s", "OK");
		ts->cmd_state = CMD_STATUS_OK;
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

		mutex_lock(&ts->cmd_lock);
		ts->cmd_is_running = false;
		mutex_unlock(&ts->cmd_lock);

		tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
		return;
	}


	ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	enable_irq(ts->client->irq);

	snprintf(buff, sizeof(buff), "%s", "NG");
	ts->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

}


int sec_ts_execute_force_calibration(struct sec_ts_data *ts, int cal_mode)
{
	int rc = -1;
	u8 cmd;

	if (cal_mode == OFFSET_CAL_SEC)
		cmd = SEC_TS_CMD_CALIBRATION_OFFSET_SEC;
	else if (cal_mode == AMBIENT_CAL)
		cmd = SEC_TS_CMD_CALIBRATION_AMBIENT;
	else
		return rc;

	if (ts->sec_ts_i2c_write(ts, cmd, NULL, 0) < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Write Cal commend failed!\n", __func__);
		return rc;
	}

	sec_ts_delay(1000);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_ACK_OFFSET_CAL_DONE);

	ts->cal_status = sec_ts_read_calibration_report(ts);
	return rc;
}

static void get_force_calibration(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = {0};
	char cal_result[4] = { 0 };

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	cal_result[0] = sec_ts_read_calibration_report(ts);

	if (cal_result[0] == SEC_TS_STATUS_CALIBRATION_SEC) {
		snprintf(buff, sizeof(buff), "%s", "OK");
		ts->cmd_state = CMD_STATUS_OK;
	} else {
		snprintf(buff, sizeof(buff), "%s", "NG");
	}

	tsp_debug_info(true, &ts->client->dev, "%s: %d, %d\n", __func__, cal_result[0],(cal_result[0] == SEC_TS_STATUS_CALIBRATION_SEC) );

	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_force_calibration(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = {0};
	int rc;

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_info(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	sec_ts_read_calibration_report(ts);

	if (ts->touch_count > 0) {
		snprintf(buff, sizeof(buff), "%s", "NG_FINGER_ON");
		ts->cmd_state = CMD_STATUS_FAIL;
		goto out_force_cal;
	}

	disable_irq(ts->client->irq);

	rc = sec_ts_execute_force_calibration(ts, OFFSET_CAL_SEC);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "FAIL");
		ts->cmd_state = CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		ts->cmd_state = CMD_STATUS_OK;
	}

	enable_irq(ts->client->irq);

out_force_cal:
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);

	tsp_debug_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
static void set_log_level(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
	char tBuff[2] = { 0 };
	int ret;

	set_default_result(ts);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	if ( (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1) ||
		(ts->cmd_param[1] < 0 || ts->cmd_param[1] > 1) ||
		(ts->cmd_param[2] < 0 || ts->cmd_param[2] > 1) ||
		(ts->cmd_param[3] < 0 || ts->cmd_param[3] > 1) ) {
		tsp_debug_err(true, &ts->client->dev, "%s: para out of range\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Para out of range");
		set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
		ts->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_STATUS_EVENT_TYPE, tBuff, 2);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Read Event type enable status fail\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Read Stat Fail");
		goto err;
	}

	tsp_debug_info(true, &ts->client->dev, "%s: STATUS_EVENT enable = 0x%02X, 0x%02X\n",
		__func__, tBuff[0], tBuff[1]);

	tBuff[0] = 0x0;
	tBuff[1] = BIT_STATUS_EVENT_ACK(ts->cmd_param[0]) |
		BIT_STATUS_EVENT_ERR(ts->cmd_param[1]) |
		BIT_STATUS_EVENT_INFO(ts->cmd_param[2]) |
		BIT_STATUS_EVENT_GEST(ts->cmd_param[3]);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATUS_EVENT_TYPE, tBuff, 2);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Write Event type enable status fail\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Write Stat Fail");
		goto err;
	}
	tsp_debug_info(true, &ts->client->dev, "%s: ACK : %d, ERR : %d, INFO : %d, GEST : %d\n",
			__func__, ts->cmd_param[0], ts->cmd_param[1], ts->cmd_param[2], ts->cmd_param[3]);

	snprintf(buff, sizeof(buff), "%s", "OK");
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_OK;
	return;
err:
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
}

bool check_lowpower_flag(struct sec_ts_data *ts)
{
	bool ret = 0;
	unsigned char flag = ts->lowpower_flag & 0xFF;

	if (flag)
		ret = 1;

	tsp_debug_info(true, &ts->client->dev,
		"%s: lowpower_mode flag : %d, ret:%d\n", __func__, flag, ret);

	if (flag & SEC_TS_LOWP_FLAG_AOD)
		tsp_debug_info(true, &ts->client->dev, "%s: aod cmd on\n", __func__);
	if (flag & SEC_TS_LOWP_FLAG_SPAY)
		tsp_debug_info(true, &ts->client->dev, "%s: spay cmd on\n", __func__);
	if (flag & SEC_TS_LOWP_FLAG_TEMP_CMD)
		tsp_debug_info(true, &ts->client->dev, "%s: known cmd on\n", __func__);

	return ret;
}

static void set_lowpower_mode(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;
	char buff[CMD_STR_LEN] = { 0 };
/*
	char para[1] = { 0 };
	int ret;
	char tBuff[4] = { 0 };
*/

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1) {
		goto NG;
	} else {
		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			tsp_debug_err(true, &ts->client->dev, "%s: ERR, POWER OFF\n", __func__);
			goto NG;
		}

		ts->lowpower_mode = ts->cmd_param[0];
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	ts->cmd_state = CMD_STATUS_OK;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	return ;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	ts->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	return ;
}

static void spay_enable(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;

	char buff[CMD_STR_LEN] = { 0 };

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1)
		goto NG;

	if (ts->cmd_param[0])
		ts->lowpower_flag |= SEC_TS_LOWP_FLAG_SPAY;
	else
		ts->lowpower_flag &= ~(SEC_TS_LOWP_FLAG_SPAY);

	ts->lowpower_mode = check_lowpower_flag(ts);

	tsp_debug_info(true, &ts->client->dev, "%s: %02X\n", __func__, ts->lowpower_mode);

	snprintf(buff, sizeof(buff), "%s", "OK");
	ts->cmd_state = CMD_STATUS_OK;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
	return ;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	ts->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&ts->cmd_lock);
	ts->cmd_is_running = false;
	mutex_unlock(&ts->cmd_lock);
	return ;
}

static void edge_swipe_enable(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;

	char buff[CMD_STR_LEN] = { 0 };
	int ret;
	char mode;

	set_default_result(ts);

	if (ts->cmd_param[0] < 0 || ts->cmd_param[0] > 1)
		goto NG;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		tsp_debug_err(true, &ts->client->dev, "%s: ERR, POWER OFF\n", __func__);
		goto NG;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_GESTURE_MODE, &mode, 1);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Failed to read mode\n", __func__);
		goto NG;
	}

	if (ts->cmd_param[0]) {
		ts->lowpower_flag = ts->lowpower_flag | SEC_TS_LOWP_FLAG_AOD;
		mode |= SEC_TS_LOWP_FLAG_AOD;
	} else {
		ts->lowpower_flag = ts->lowpower_flag & ~(SEC_TS_LOWP_FLAG_AOD);
		mode &= ~SEC_TS_LOWP_FLAG_AOD;
	}

	ts->lowpower_mode = check_lowpower_flag(ts);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_GESTURE_MODE, &mode, 1);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Failed to write mode\n", __func__);
		goto NG;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_GESTURE_MODE, &mode, 1);
	if (ret < 0) {
		tsp_debug_err(true, &ts->client->dev, "%s: Failed to read mode\n", __func__);
		goto NG;
	}

	tsp_debug_info(true, &ts->client->dev, "%s: %02X\n", __func__, mode);

	snprintf(buff, sizeof(buff), "%s", "OK");
	ts->cmd_state = CMD_STATUS_OK;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	return ;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	ts->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(ts, buff, strnlen(buff, sizeof(buff)));
	return ;
}

static void not_support_cmd(void *device_data)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)device_data;

	set_default_result(ts);
	snprintf(ts->cmd_buff, sizeof(ts->cmd_buff), "%s", tostring(NA));

	set_cmd_result(ts, ts->cmd_buff, strlen(ts->cmd_buff));
	ts->cmd_state = CMD_STATUS_NOT_APPLICABLE;
}

int sec_ts_fn_init(struct sec_ts_data *ts)
{
	int retval;
	unsigned short ii;

	INIT_LIST_HEAD(&ts->cmd_list_head);

	for (ii = 0; ii < ARRAY_SIZE(ft_cmds); ii++)
		list_add_tail(&ft_cmds[ii].list, &ts->cmd_list_head);

	mutex_init(&ts->cmd_lock);
	ts->cmd_is_running = false;

	ts->fac_dev_ts = sec_device_create(ts, "tsp");
	retval = IS_ERR(ts->fac_dev_ts);
	if (retval) {
		tsp_debug_err(true, &ts->client->dev, "%s: Failed to create device for the sysfs\n", __func__);
		retval = IS_ERR(ts->fac_dev_ts);
		goto exit;
	}

	dev_set_drvdata(ts->fac_dev_ts, ts);

	retval = sysfs_create_group(&ts->fac_dev_ts->kobj,
            &cmd_attr_group);
    if (retval < 0) {
	tsp_debug_err(true, &ts->client->dev, "%s: Failed to create sysfs attributes\n", __func__);
            goto exit;
    }

	retval = sysfs_create_link(&ts->fac_dev_ts->kobj,
				&ts->input_dev->dev.kobj, "input");

	if (retval < 0)	{
		tsp_debug_err(true, &ts->client->dev, "%s: fail - sysfs_create_link\n", __func__);
		goto exit;
	}
	ts->reinit_done = true;

	INIT_DELAYED_WORK(&ts->cover_cmd_work, clear_cover_cmd_work);

	return 0;

exit:
	//kfree(factory_data);
	return retval;
}
