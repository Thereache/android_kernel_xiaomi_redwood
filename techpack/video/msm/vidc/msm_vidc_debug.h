/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 */

#ifndef __MSM_VIDC_DEBUG__
#define __MSM_VIDC_DEBUG__
#include <linux/debugfs.h>
#include <linux/delay.h>
#include "msm_vidc_events.h"

/* Mock all the missing parts for successful compilation starts here */
#include <linux/types.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <soc/qcom/subsystem_restart.h>
#include "msm_vidc_internal.h"

#define CREATE_TRACE_POINTS

// void disable_irq_nosync(unsigned int irq);
// void enable_irq(unsigned int irq);

void do_gettimeofday(struct timeval *__ddl_tv);

#ifndef CONFIG_VIDEOBUF2_CORE
int vb2_reqbufs(struct vb2_queue *q, struct v4l2_requestbuffers *req);
int vb2_qbuf(struct vb2_queue *q, struct media_device *mdev,
			 struct v4l2_buffer *b);
int vb2_dqbuf(struct vb2_queue *q, struct v4l2_buffer *b, bool nonblocking);
int vb2_streamon(struct vb2_queue *q, enum v4l2_buf_type type);
int vb2_streamoff(struct vb2_queue *q, enum v4l2_buf_type type);
int vb2_queue_init(struct vb2_queue *q);
void vb2_buffer_done(struct vb2_buffer *vb, enum vb2_buffer_state state);
#endif

#define SMEM_IMAGE_VERSION_TABLE 469
/* Mock all the missing parts for successful compilation ends */

#ifndef VIDC_DBG_LABEL
#define VIDC_DBG_LABEL "msm_vidc"
#endif

/*
 * This enforces a rate limit: not more than 6 messages
 * in every 1s.
 */

#define VIDC_DBG_SESSION_RATELIMIT_INTERVAL (1 * HZ)
#define VIDC_DBG_SESSION_RATELIMIT_BURST 6

#define VIDC_DBG_TAG VIDC_DBG_LABEL ": %6s: %8x: "
#define DEFAULT_SID ((u32)-1)

/* To enable messages OR these values and
 * echo the result to debugfs file.
 *
 * To enable all messages set debug_level = 0x101F
 */

enum vidc_msg_prio {
	VIDC_ERR        = 0x00000001,
	VIDC_HIGH       = 0x00000002,
	VIDC_LOW        = 0x00000004,
	VIDC_PERF       = 0x00000008,
	VIDC_PKT        = 0x00000010,
	VIDC_BUS        = 0x00000020,
	VIDC_ENCODER    = 0x00000100,
	VIDC_DECODER    = 0x00000200,
	VIDC_PRINTK     = 0x00001000,
	VIDC_FTRACE     = 0x00002000,
	FW_LOW          = 0x00010000,
	FW_MEDIUM       = 0x00020000,
	FW_HIGH         = 0x00040000,
	FW_ERROR        = 0x00080000,
	FW_FATAL        = 0x00100000,
	FW_PERF         = 0x00200000,
	FW_PRINTK       = 0x10000000,
	FW_FTRACE       = 0x20000000,
};
#define FW_LOGSHIFT    16
#define FW_LOGMASK     0x0FFF0000

enum msm_vidc_debugfs_event {
	MSM_VIDC_DEBUGFS_EVENT_ETB,
	MSM_VIDC_DEBUGFS_EVENT_EBD,
	MSM_VIDC_DEBUGFS_EVENT_FTB,
	MSM_VIDC_DEBUGFS_EVENT_FBD,
};

enum vidc_err_recovery_disable {
	VIDC_DISABLE_NOC_ERR_RECOV     = 0x0001,
	VIDC_DISABLE_NON_NOC_ERR_RECOV = 0x0002
};

#ifdef CONFIG_DEBUG_FS
extern int msm_vidc_debug;
extern int msm_vidc_fw_debug_mode;
extern bool msm_vidc_fw_coverage;
extern bool msm_vidc_thermal_mitigation_disabled;
extern int msm_vidc_clock_voting;
extern bool msm_vidc_syscache_disable;
extern bool msm_vidc_lossless_encode;
extern bool msm_vidc_cvp_usage;
extern int msm_vidc_err_recovery_disable;
extern int msm_vidc_vpp_delay;

static inline char *get_debug_level_str(int level)
{
	switch (level) {
	case VIDC_ERR:
		return "err";
	case VIDC_HIGH | VIDC_PERF:
	case VIDC_HIGH:
		return "high";
	case VIDC_LOW:
		return "low";
	case VIDC_PERF:
		return "perf";
	case VIDC_PKT:
		return "pkt";
	case VIDC_BUS:
		return "bus";
	default:
		return "????";
	}
}

#define dprintk(__level, sid, __fmt, ...)	\
	pr_debug(VIDC_DBG_TAG __fmt, get_debug_level_str(__level), sid, \
					get_codec_name(sid), ##__VA_ARGS__)

#define dprintk_firmware(__level, __fmt, ...)	\
	pr_debug(FW_DBG_TAG __fmt, "fw", ##__VA_ARGS__)

#define dprintk_ratelimit(__level, __fmt, arg...) \
	do { \
		if (msm_vidc_check_ratelimit()) { \
			dprintk(__level, DEFAULT_SID, __fmt, arg); \
		} \
	} while (0)

#define MSM_VIDC_ERROR(value)					\
	do {	if (value)					\
			d_vpr_e("BugOn");		\
		BUG_ON(value);					\
	} while (0)


struct dentry *msm_vidc_debugfs_init_drv(void);
struct dentry *msm_vidc_debugfs_init_core(struct msm_vidc_core *core,
		struct dentry *parent);
struct dentry *msm_vidc_debugfs_init_inst(struct msm_vidc_inst *inst,
		struct dentry *parent);
void msm_vidc_debugfs_deinit_inst(struct msm_vidc_inst *inst);
void msm_vidc_debugfs_update(struct msm_vidc_inst *inst,
		enum msm_vidc_debugfs_event e);
int msm_vidc_check_ratelimit(void);

/**
 * 0xx -> allow prints for all sessions
 * 1xx -> allow only encoder prints
 * 2xx -> allow only decoder prints
 * 4xx -> allow only cvp prints
 */
static inline bool is_print_allowed(u32 sid, u32 level)
{
	if (!(msm_vidc_debug & level))
		return false;

	if (!((msm_vidc_debug >> 8) & 0xF))
		return true;

	if (!sid || sid > vidc_driver->num_ctxt)
		return true;

	if (vidc_driver->ctxt[sid-1].session_type & msm_vidc_debug)
		return true;

	return false;
}

#else
static int msm_vidc_debug = 0;
static int msm_vidc_fw_debug_mode = 0;
static bool msm_vidc_fw_coverage = false;
static bool msm_vidc_thermal_mitigation_disabled = false;
static int msm_vidc_clock_voting = 0;
static bool msm_vidc_syscache_disable = false;
static bool msm_vidc_lossless_encode = false;
static bool msm_vidc_cvp_usage = false;
static int msm_vidc_err_recovery_disable = 0;
static int msm_vidc_vpp_delay = 0;

#define dprintk(...) ((void)0)
#define dprintk_firmware(...) ((void)0)
#define dprintk_ratelimit(...) ((void)0)
#define MSM_VIDC_ERROR(...) ((void)0)
#define trace_msm_v4l2_vidc_open_start(...) ((void)0)
#define trace_msm_v4l2_vidc_open_end(...) ((void)0)
#define trace_msm_v4l2_vidc_close_start(...) ((void)0)
#define trace_msm_v4l2_vidc_close_end(...) ((void)0)
#define trace_msm_v4l2_vidc_buffer_event_start(...) ((void)0)
#define trace_msm_v4l2_vidc_fw_load_start(...) ((void)0)
#define trace_msm_v4l2_vidc_fw_load_end(...) ((void)0)
#define trace_msm_v4l2_vidc_buffer_event_end(...) ((void)0)
#define trace_msm_vidc_perf_clock_scale(...) ((void)0)
#define trace_msm_vidc_common_state_change(...) ((void)0)
#define trace_msm_smem_buffer_iommu_op_start(...) ((void)0)
#define trace_msm_smem_buffer_iommu_op_end(...) ((void)0)
#define trace_msm_smem_buffer_iommu_op_start(...) ((void)0)
#define trace_msm_smem_buffer_iommu_op_end(...) ((void)0)
#define trace_msm_smem_buffer_dma_op_start(...) ((void)0)
#define trace_msm_smem_buffer_dma_op_end(...) ((void)0)
#define trace_msm_v4l2_vidc_buffer_counter(...) ((void)0)
#define trace_venus_hfi_var_done(...) ((void)0)

static inline struct dentry *msm_vidc_debugfs_init_drv(void) { return NULL; }
static inline struct dentry *msm_vidc_debugfs_init_core(struct msm_vidc_core *core,
							struct dentry *parent)
{
	return NULL;
}
static inline struct dentry *msm_vidc_debugfs_init_inst(struct msm_vidc_inst *inst,
							struct dentry *parent)
{
	return NULL;
}
static inline void msm_vidc_debugfs_deinit_inst(struct msm_vidc_inst *inst) {}
static inline void msm_vidc_debugfs_update(struct msm_vidc_inst *inst,
					   enum msm_vidc_debugfs_event e) {}
#endif

#define s_vpr_e(sid, __fmt, ...) dprintk(VIDC_ERR, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_h(sid, __fmt, ...) dprintk(VIDC_HIGH, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_l(sid, __fmt, ...) dprintk(VIDC_LOW, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_p(sid, __fmt, ...) dprintk(VIDC_PERF, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_t(sid, __fmt, ...) dprintk(VIDC_PKT, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_b(sid, __fmt, ...) dprintk(VIDC_BUS, sid, __fmt, ##__VA_ARGS__)
#define s_vpr_hp(sid, __fmt, ...) dprintk(VIDC_HIGH | VIDC_PERF, sid, __fmt, ##__VA_ARGS__)
#define d_vpr_e(__fmt, ...) dprintk(VIDC_ERR, DEFAULT_SID, __fmt, ##__VA_ARGS__)
#define d_vpr_h(__fmt, ...) dprintk(VIDC_HIGH, DEFAULT_SID, __fmt, ##__VA_ARGS__)
#define d_vpr_l(__fmt, ...) dprintk(VIDC_LOW, DEFAULT_SID, __fmt, ##__VA_ARGS__)
#define d_vpr_p(__fmt, ...) dprintk(VIDC_PERF, DEFAULT_SID, __fmt, ##__VA_ARGS__)
#define d_vpr_t(__fmt, ...) dprintk(VIDC_PKT, DEFAULT_SID, __fmt, ##__VA_ARGS__)
#define d_vpr_b(__fmt, ...) dprintk(VIDC_BUS, DEFAULT_SID, __fmt, ##__VA_ARGS__)

static inline void tic(struct msm_vidc_inst *i, enum profiling_points p,
				 char *b)
{
	struct timeval __ddl_tv = { 0 };

	if (!i->debug.pdata[p].name[0])
		memcpy(i->debug.pdata[p].name, b, 64);
	if ((msm_vidc_debug & VIDC_PERF) &&
		i->debug.pdata[p].sampling) {
		do_gettimeofday(&__ddl_tv);
		i->debug.pdata[p].start =
			(__ddl_tv.tv_sec * 1000) + (__ddl_tv.tv_usec / 1000);
			i->debug.pdata[p].sampling = false;
	}
}

static inline void toc(struct msm_vidc_inst *i, enum profiling_points p)
{
	struct timeval __ddl_tv = { 0 };

	if ((msm_vidc_debug & VIDC_PERF) &&
		!i->debug.pdata[p].sampling) {
		do_gettimeofday(&__ddl_tv);
		i->debug.pdata[p].stop = (__ddl_tv.tv_sec * 1000)
			+ (__ddl_tv.tv_usec / 1000);
		i->debug.pdata[p].cumulative += i->debug.pdata[p].stop -
			i->debug.pdata[p].start;
		i->debug.pdata[p].sampling = true;
	}
}

static inline void show_stats(struct msm_vidc_inst *i)
{
	int x;

	for (x = 0; x < MAX_PROFILING_POINTS; x++) {
		if (i->debug.pdata[x].name[0] &&
				(msm_vidc_debug & VIDC_PERF)) {
			if (i->debug.samples) {
				s_vpr_p(i->sid, "%s averaged %d ms/sample\n",
						i->debug.pdata[x].name,
						i->debug.pdata[x].cumulative /
						i->debug.samples);
			}

			s_vpr_p(i->sid, "%s Samples: %d\n",
				i->debug.pdata[x].name, i->debug.samples);
		}
	}
}

static inline void msm_vidc_res_handle_fatal_hw_error(
	struct msm_vidc_platform_resources *resources,
	bool enable_fatal)
{
	enable_fatal &= resources->debug_timeout;
	MSM_VIDC_ERROR(enable_fatal);
}

static inline void msm_vidc_handle_hw_error(struct msm_vidc_core *core)
{
	bool enable_fatal = true;

	/*
	 * In current implementation user-initiated SSR triggers
	 * a fatal error from hardware. However, there is no way
	 * to know if fatal error is due to SSR or not. Handle
	 * user SSR as non-fatal.
	 */
	if (core->trigger_ssr) {
		core->trigger_ssr = false;
		enable_fatal = false;
	}

	/* Video driver can decide FATAL handling of HW errors
	 * based on multiple factors. This condition check will
	 * be enhanced later.
	 */
	msm_vidc_res_handle_fatal_hw_error(&core->resources, enable_fatal);
}

#endif
