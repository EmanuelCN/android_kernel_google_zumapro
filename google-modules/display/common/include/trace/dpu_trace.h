/* SPDX-License-Identifier: GPL-2.0 */
/*
 * DPU trace support
 *
 * Copyright (C) 2020 Google, Inc.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM dpu

#if !defined(_DPU_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _DPU_TRACE_H_

#include <linux/tracepoint.h>

TRACE_EVENT_CONDITION(dsi_tx,
	TP_PROTO(u8 type, const u8 *tx_buf, size_t length, bool last, u32 delay_ms),
	TP_ARGS(type, tx_buf, length, last, delay_ms),
	TP_CONDITION(length > 0),
	TP_STRUCT__entry(
			__field(u8, type)
			__dynamic_array(u8, tx_buf, length)
			__field(bool, last)
			__field(u32, delay_ms)
		),
	TP_fast_assign(
			__entry->type = type;
			memcpy(__get_dynamic_array(tx_buf), tx_buf, length);
			__entry->last = last;
			__entry->delay_ms = delay_ms;
		),
	TP_printk("type=0x%02x length=%u last=%d delay=%d tx=[%s]", __entry->type,
			  __get_dynamic_array_len(tx_buf), __entry->last, __entry->delay_ms,
			  __print_hex(__get_dynamic_array(tx_buf),
				      __get_dynamic_array_len(tx_buf)))
);

TRACE_EVENT_CONDITION(dsi_rx,
	TP_PROTO(u8 cmd, const u8 *rx_buf, size_t length),
	TP_ARGS(cmd, rx_buf, length),
	TP_CONDITION(length > 0),
	TP_STRUCT__entry(
			__field(u8, cmd)
			__dynamic_array(u8, rx_buf, length)
		),
	TP_fast_assign(
			__entry->cmd = cmd;
			memcpy(__get_dynamic_array(rx_buf), rx_buf, length);
		),
	TP_printk("cmd=0x%02x length=%u rx=[%s]", __entry->cmd,
			  __get_dynamic_array_len(rx_buf),
			  __print_hex(__get_dynamic_array(rx_buf),
				      __get_dynamic_array_len(rx_buf)))
);

TRACE_EVENT(dsi_cmd_fifo_status,
	TP_PROTO(u8 header, u16 payload),
	TP_ARGS(header, payload),
	TP_STRUCT__entry(
			__field(u8, header)
			__field(u16, payload)
		),
	TP_fast_assign(
			__entry->header   = header;
			__entry->payload  = payload;
		),
	TP_printk("header=%d payload=%d", __entry->header, __entry->payload)
);

TRACE_EVENT(msleep,
	TP_PROTO(u32 delay_ms),
	TP_ARGS(delay_ms),
	TP_STRUCT__entry(__field(u32, delay_ms)),
	TP_fast_assign(__entry->delay_ms = delay_ms;),
	TP_printk("delay=%d", __entry->delay_ms)
);

TRACE_EVENT(dsi_label_scope,
	TP_PROTO(const char *name, bool begin),
	TP_ARGS(name, begin),
	TP_STRUCT__entry(
			__string(name, name)
			__field(bool, begin)
		),
	TP_fast_assign(
			__assign_str(name, name);
			__entry->begin = begin;
		),
	TP_printk("%s %s", __get_str(name), __entry->begin ? "begin" : "end")
);
#define PANEL_SEQ_LABEL_BEGIN(name) trace_dsi_label_scope(name, true)
#define PANEL_SEQ_LABEL_END(name) trace_dsi_label_scope(name, false)

TRACE_EVENT(tracing_mark_write,
	TP_PROTO(char type, int pid, const char *name, int value),
	TP_ARGS(type, pid, name, value),
	TP_STRUCT__entry(
		__field(char, type)
		__field(int, pid)
		__string(name, name)
		__field(int, value)
	),
	TP_fast_assign(
		__entry->type = type;
		__entry->pid = pid;
		__assign_str(name, name);
		__entry->value = value;
	),
	TP_printk("%c|%d|%s|%d",
		__entry->type, __entry->pid, __get_str(name), __entry->value)
);

#define DPU_ATRACE_INT_PID(name, value, pid) trace_tracing_mark_write('C', pid, name, value)
#define DPU_ATRACE_INT(name, value) DPU_ATRACE_INT_PID(name, value, current->tgid)
#define DPU_ATRACE_BEGIN(name) trace_tracing_mark_write('B', current->tgid, name, 0)
#define DPU_ATRACE_END(name) trace_tracing_mark_write('E', current->tgid, "", 0)

#endif /* _DPU_TRACE_H_ */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/.

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE dpu_trace

/* This part must be outside protection */
#include <trace/define_trace.h>