/* SPDX-License-Identifier: MIT */
/*
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#ifndef _GS_DCS_HELPER_H_
#define _GS_DCS_HELPER_H_

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0))
#include <drm/display/drm_dsc.h>
#else
#include <drm/drm_dsc.h>
#endif
#include <drm/drm_mipi_dsi.h>

/** Private DSI msg flags **/

/* Stack all commands until lastcommand bit and trigger all in one go */
#define GS_DSI_MSG_QUEUE BIT(15)

/* packetgo feature to batch msgs can wait for vblank, use this flag to ignore */
#define GS_DSI_MSG_IGNORE_VBLANK BIT(14)
/* Mark the start of mipi commands transaction. Following commands should not be
 * sent to panel until see a GS_DSI_MSG_FORCE_FLUSH flag
 */
#define GS_DSI_MSG_FORCE_BATCH BIT(13)
/** Mark the end of mipi commands transaction */
#define GS_DSI_MSG_FORCE_FLUSH BIT(12)

/** Panel Command Flags **/

/* indicates that all commands in this cmd set should be batched together */
#define GS_PANEL_CMD_SET_BATCH BIT(0)
/*
 * indicates that all commands in this cmd set should be queued, a follow up
 * command should take care of triggering transfer of batch
 */
#define GS_PANEL_CMD_SET_QUEUE BIT(1)

/* packetgo feature to batch msgs can wait for vblank, use this flag to ignore explicitly */
#define GS_PANEL_CMD_SET_IGNORE_VBLANK BIT(2)

/* Panel Rev bits */
#define PANEL_REV_PROTO1        BIT(0)
#define PANEL_REV_PROTO1_1      BIT(1)
#define PANEL_REV_PROTO1_2      BIT(2)
#define PANEL_REV_PROTO2        BIT(3)
#define PANEL_REV_EVT1          BIT(4)
#define PANEL_REV_EVT1_0_2      BIT(5)
#define PANEL_REV_EVT1_1        BIT(6)
#define PANEL_REV_EVT1_2        BIT(7)
#define PANEL_REV_EVT2          BIT(8)
#define PANEL_REV_DVT1          BIT(9)
#define PANEL_REV_DVT1_1        BIT(10)
#define PANEL_REV_PVT           BIT(11)
#define PANEL_REV_MP            BIT(12)
#define PANEL_REV_LATEST        BIT(31)
#define PANEL_REV_ALL           (~0)
#define PANEL_REV_GE(rev)       (~((rev) - 1))
#define PANEL_REV_LT(rev)       ((rev) - 1)
#define PANEL_REV_ALL_BUT(rev)  (PANEL_REV_ALL & ~(rev))

/** Command Set data structures **/

/**
 * struct gs_dsi_cmd - information for a dsi command.
 * @cmd_len:  Length of a dsi command.
 * @cmd:      Pointer to a dsi command.
 * @delay_ms: Delay time after executing this dsi command.
 * @panel_rev:Send the command only when the panel revision is matched.
 */
struct gs_dsi_cmd {
	u32 cmd_len;
	const u8 *cmd;
	u32 delay_ms;
	u32 panel_rev;
	u8 type;
};

/**
 * struct gs_dsi_cmdset - a dsi command sequence.
 * @num_cmd:  Number of dsi commands in this sequence.
 * @cmds:     Pointer to a dsi command sequence.
 */
struct gs_dsi_cmdset {
	const u32 num_cmd;
	const struct gs_dsi_cmd *cmds;
};

/* Arrays */

/**
 * GS_DSI_DELAY_REV_CMDLIST - construct a struct gs_dsi_cmd from inline data
 * @delay: The delay to attach to sending the command
 * @rev: The panel revision this applies to, if any
 * @cmdlist: The binary array of data to be sent to the device
 *
 * Return: struct gs_dsi_cmd holding data necessary to send the command to the
 * panel.
 */
#define GS_DSI_DELAY_REV_CMDLIST(delay, rev, cmdlist) \
{                                                     \
	sizeof(cmdlist),                              \
	cmdlist,                                      \
	delay,                                        \
	(u32)rev,                                     \
}
#define GS_DSI_DELAY_CMDLIST(delay, cmdlist) GS_DSI_DELAY_REV_CMDLIST(delay, PANEL_REV_ALL, cmdlist)
#define GS_DSI_REV_CMDLIST(rev, cmdlist) GS_DSI_DELAY_REV_CMDLIST(0, rev, cmdlist)
#define GS_DSI_CMDLIST(cmdlist) GS_DSI_DELAY_REV_CMDLIST(0, PANEL_REV_ALL, cmdlist)

/* Variadic */

/**
 * GS_DSI_DELAY_REV_CMD - construct a struct gs_dsi_cmd from inline data
 * @delay: The delay to attach to sending the command
 * @rev: The panel revision this applies to, if any
 * @seq: Sequence of binary data to be sent to the device
 *
 * This is functionally the same as the CMDLIST invocation, except that it takes
 * a variadic list of bytes to pack into the struct gs_dsi_cmd.
 *
 * Return: struct gs_dsi_cmd holding data necessary to send the command to the
 * panel.
 */
#define GS_DSI_DELAY_REV_CMD(delay, rev, seq...) \
	GS_DSI_DELAY_REV_CMDLIST(delay, rev, ((const u8[]){ seq }))
#define GS_DSI_DELAY_CMD(delay, seq...) GS_DSI_DELAY_REV_CMD(delay, PANEL_REV_ALL, seq)
#define GS_DSI_REV_CMD(rev, seq...) GS_DSI_DELAY_REV_CMD(0, rev, seq)
#define GS_DSI_CMD(seq...) GS_DSI_DELAY_REV_CMD(0, PANEL_REV_ALL, seq)

/**
 * DEFINE_GS_CMDSET - Construct a struct gs_dsi_cmdset from array of commands
 * @name: The name of the array of `struct gs_dsi_cmd` members
 *
 * This function does some preprocessor expansion to attach the length of a
 * static array of `struct gs_dsi_cmd`s to that array inside a `gs_dsi_cmdset`
 * data structure. It does this using a particular naming convention, where the
 * input must be named ending in `_cmds` and the output has `_cmdset` appended
 * to it.
 *
 * Usage example:
 * static const struct gs_dsi_cmd my_panel_turn_on_cmds[] = {
 *   GS_DSI_CMD_SEQ(0x01, 0x02, 0x03, 0x04),
 *   GS_DSI_CMD0(0xB9),
 * };
 * static DEFINE_GS_CMDSET(my_panel_turn_on);
 *
 * This expands to:
 * static const struct gs_dsi_cmdset my_panel_turn_on_cmdset = {...};
 *
 * Return: expansion of array of commands into a `struct gs_dsi_cmdset`;
 */
#define DEFINE_GS_CMDSET(name)                        \
	const struct gs_dsi_cmdset name##_cmdset = {   \
	  .num_cmd = ARRAY_SIZE(name##_cmds),          \
	  .cmds = name##_cmds                          \
	}

/** TE2 Timing **/

/**
 * struct gs_panel_te2_timing - details regarding te2 timing
 */
struct gs_panel_te2_timing {
	/** @rising_edge: vertical start point. */
	u16 rising_edge;
	/** @falling_edge: vertical end point. */
	u16 falling_edge;
};

/** Binned LP Modes **/

/**
 * struct gs_binned_lp - information for binned lp mode.
 * @name: Name of this binned lp mode
 * @bl_threshold: Max brightnes supported by this mode
 * @cmdset: A dsi command sequence to enter this mode
 * @te2_timing: TE2 signal timing
 */
struct gs_binned_lp {
	const char *name;
	u32 bl_threshold;
	struct gs_dsi_cmdset cmdset;
	struct gs_panel_te2_timing te2_timing;
};

/**
 * BINNED_LP_MODE_TIMING - Constructor for struct gs_binned_lp
 * @mode_name: Name to attach to this binned LP mode
 * @bl_thr: Max brightness supported by this mode
 * @cmdset: Array of gs_dsi_cmds used to enter this mode
 * @rising: TE2 rising time
 * @falling: TE2 falling time
 *
 * Return: A `struct gs_binned_lp` containing this data
 */
#define BINNED_LP_MODE_TIMING(mode_name, bl_thr, cmdset, rising, falling) \
	{                                                                 \
		.name = mode_name, .bl_threshold = bl_thr,                \
		{ .num_cmd = ARRAY_SIZE(cmdset), .cmds = cmdset },        \
		{.rising_edge = rising, .falling_edge = falling }         \
	}
#define BINNED_LP_MODE(mode_name, bl_thr, cmdset) \
	BINNED_LP_MODE_TIMING(mode_name, bl_thr, cmdset, 0, 0)

/** Write Functions **/

/* Command Sets */

/**
 * gs_dsi_send_cmdset_flags() - Sends a series of dsi commands to the panel
 * @dsi: pointer to mipi_dsi_device by which to write to panel
 * @cmdset: Set of commands to send
 * @panel_rev: revision identifier for panel to be matched against commands
 * @flags: Any of the Private DSI msg flags to affect command behavior
 */
void gs_dsi_send_cmdset_flags(struct mipi_dsi_device *dsi, const struct gs_dsi_cmdset *cmdset,
			      u32 panel_rev, u32 flags);

/**
 * gs_dsi_send_cmdset() - Sends a series of dsi commands to the panel
 * @dsi: pointer to mipi_dsi_device by which to write to panel
 * @cmdset: Set of commands to send
 * @panel_rev: revision identifier for panel to be matched against commands
 */
void gs_dsi_send_cmdset(struct mipi_dsi_device *dsi, const struct gs_dsi_cmdset *cmdset,
			u32 panel_rev);

/* Raw DCS Writes */

ssize_t gs_dsi_dcs_write_buffer(struct mipi_dsi_device *dsi, const void *data,
				size_t len, u16 flags);

static inline ssize_t gs_dsi_dcs_write_buffer_force_batch_begin(struct mipi_dsi_device *dsi)
{
	return gs_dsi_dcs_write_buffer(dsi, NULL, 0, GS_DSI_MSG_FORCE_BATCH);
}

static inline ssize_t gs_dsi_dcs_write_buffer_force_batch_end(struct mipi_dsi_device *dsi)
{
	return gs_dsi_dcs_write_buffer(dsi, NULL, 0,
				       GS_DSI_MSG_FORCE_FLUSH | GS_DSI_MSG_IGNORE_VBLANK);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)) || IS_ENABLED(CONFIG_DRM_DISPLAY_DP_HELPER)
/**
 * gs_dcs_write_dsc_config() - function to write dsc configuration to panel
 * @dev: struct device corresponding to dsi panel
 * @dsc_cfg: dsc configuration to write
 *
 * This function wraps the packing and sending of the pps payload from the
 * more user-readable drm_dsc_config structure. Makes use of the
 * mipi_dsi_picture_parameter_set function for the actual transfer.
 *
 * Return: result of the underlying transfer function
 */
int gs_dcs_write_dsc_config(struct device *dev, const struct drm_dsc_config *dsc_cfg);
#else
static inline int gs_dcs_write_dsc_config(struct device *dev, const struct drm_dsc_config *dsc_cfg)
{
	return -ENOTSUPP;
}
#endif

/*
 * Arrays
 *
 * These macros execute dcs writes on an array of data.
 * Optionally, flags or a delay-after time may be specified.
 */

#define GS_DCS_WRITE_DELAY_FLAGS_CMDLIST(dev, delay_ms, flags, cmdlist)            \
	do {                                                                       \
		struct mipi_dsi_device *dsi = to_mipi_dsi_device(dev);             \
		gs_dsi_dcs_write_buffer(dsi, cmdlist, ARRAY_SIZE(cmdlist), flags); \
		if (delay_ms > 0)                                                  \
			usleep_range(delay_ms * 1000, delay_ms * 1000 + 10);       \
	} while (0)
#define GS_DCS_WRITE_DELAY_CMDLIST(dev, delay_ms, cmdlist) \
	GS_DCS_WRITE_DELAY_FLAGS_CMDLIST(dev, delay_ms, 0, cmdlist)
#define GS_DCS_WRITE_FLAGS_CMDLIST(dev, flags, cmdlist) \
	GS_DCS_WRITE_DELAY_FLAGS_CMDLIST(dev, 0, flags, cmdlist)
#define GS_DCS_WRITE_CMDLIST(dev, cmdlist) \
	GS_DCS_WRITE_DELAY_FLAGS_CMDLIST(dev, 0, 0, cmdlist)

/*
 * Variadic
 *
 * These macros execute dcs writes on data arranged as variadic arguments
 * (that is, providing the data as a series of arguments to the function)
 * Optionally, flags or a delay-after time may be specified
 */

#define GS_DCS_WRITE_DELAY_FLAGS_CMD(dev, delay_ms, flags, seq...)         \
	do {                                                               \
		u8 d[] = { seq };                                          \
		GS_DCS_WRITE_DELAY_FLAGS_CMDLIST(dev, delay_ms, flags, d); \
	} while (0)
#define GS_DCS_WRITE_DELAY_CMD(dev, delay_ms, seq...) \
	GS_DCS_WRITE_DELAY_FLAGS_CMD(dev, delay_ms, 0, seq)
#define GS_DCS_WRITE_FLAGS_CMD(dev, flags, seq...) \
	GS_DCS_WRITE_DELAY_FLAGS_CMD(dev, 0, flags, seq)
#define GS_DCS_WRITE_CMD(dev, seq...) \
	GS_DCS_WRITE_DELAY_FLAGS_CMD(dev, 0, 0, seq)

/*
 * Buffered Writes (Arrays)
 *
 * These macros add arrays of data to a write buffer to be output to the panel
 * Optionally, that buffer may be flushed immediately after.
 */

#define GS_DCS_BUF_ADD_CMDLIST(dev, cmdlist) \
	GS_DCS_WRITE_FLAGS_CMDLIST(dev, GS_DSI_MSG_QUEUE, cmdlist)
#define GS_DCS_BUF_ADD_CMDLIST_AND_FLUSH(dev, cmdlist) \
	GS_DCS_WRITE_FLAGS_CMDLIST(dev, GS_DSI_MSG_IGNORE_VBLANK, cmdlist)

/*
 * Buffered Writes (Variadic)
 *
 * These macros add data to a write buffer to be output to the panel from
 * variadic input (that is, added as a list of arguments to the function)
 * Optionally, that buffer may be flushed immediately after.
 */

#define GS_DCS_BUF_ADD_CMD(dev, seq...) \
	GS_DCS_WRITE_FLAGS_CMD(dev, GS_DSI_MSG_QUEUE, seq)
#define GS_DCS_BUF_ADD_CMD_AND_FLUSH(dev, seq...) \
	GS_DCS_WRITE_FLAGS_CMD(dev, GS_DSI_MSG_IGNORE_VBLANK, seq)


#endif // _GS_DCS_HELPER_H_