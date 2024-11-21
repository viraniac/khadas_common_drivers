/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_queue.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/06/05	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/

#include "adlak_api.h"
#include "adlak_common.h"
#include "adlak_context.h"
#include "adlak_device.h"
#include "adlak_submit.h"

/************************** Constant Definitions *****************************/
#define ADLAK_RADY_LIST_MAX (1)
#define ADLAK_SCHEDULE_LIST_MAX (1)

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/*****************************************************************************/

#include "adlak_inference.c"

int adlak_queue_init(struct adlak_device *padlak) {
    struct adlak_workqueue *pwq = &padlak->queue;
    AML_LOG_DEBUG("%s", __func__);

    INIT_LIST_HEAD(&pwq->pending_list);
    INIT_LIST_HEAD(&pwq->scheduled_list);
    INIT_LIST_HEAD(&pwq->finished_list);
    adlak_os_mutex_init(&pwq->wq_mutex);
    adlak_os_sema_init(&pwq->wk_update, 1, 0);

    adlak_os_mutex_lock(&pwq->wq_mutex);
    pwq->padlak = padlak;
    adlak_queue_reset(padlak);

    adlak_os_mutex_unlock(&pwq->wq_mutex);
#ifdef CONFIG_ADLAK_DEBUG_RESET_ID
    adlak_os_printf("debug: reset dependency id enable\n");
#endif
#ifdef CONFIG_ADLAK_DEBUG_SOFT_RESET
    adlak_os_printf("debug: soft reset enable\n");
#endif
    return 0;
}
int adlak_queue_reset(struct adlak_device *padlak) {
    struct adlak_workqueue *pwq = &padlak->queue;
    AML_LOG_DEBUG("%s", __func__);
    // clear schedule list
    // adlak_clear_sch_list(padlak);

    pwq->sched_num     = 0;
    pwq->sched_num_max = ADLAK_SCHEDULE_LIST_MAX;
    pwq->ptask_sch_cur = NULL;

    return 0;
}

static void adlak_invoke_list_del(struct list_head *hd) {
    struct adlak_task *ptask     = NULL;
    struct adlak_task *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                AML_LOG_DEBUG("net_id=%d", ptask->context->net_id);
                list_del(&ptask->head);
                adlak_invoke_destroy(ptask);
            }
        }
    }
}

int adlak_queue_deinit(struct adlak_device *padlak) {
    struct adlak_workqueue *pwq = &padlak->queue;
    AML_LOG_DEBUG("%s", __func__);

    adlak_os_mutex_lock(&pwq->wq_mutex);
    adlak_invoke_list_del(&pwq->pending_list);
    adlak_invoke_list_del(&pwq->scheduled_list);
    adlak_invoke_list_del(&pwq->finished_list);

    adlak_os_mutex_unlock(&pwq->wq_mutex);

    adlak_os_mutex_destroy(&pwq->wq_mutex);
    adlak_os_sema_destroy(&pwq->wk_update);

    return 0;
}

#if ADLAK_DEBUG
static int print_task_info(char *buf, ssize_t buf_size, struct adlak_task *ptask) {
    int  ret = 0;
    char state_str[20];

    if ((!buf) || (!ptask)) {
        return ret;
    }

    switch (ptask->state) {
        case ADLAK_SUBMIT_STATE_IDLE:
            adlak_os_snprintf(state_str, sizeof(state_str), "idel");
            break;

        case ADLAK_SUBMIT_STATE_PENDING:
            adlak_os_snprintf(state_str, sizeof(state_str), "pending");
            break;

        case ADLAK_SUBMIT_STATE_READY:
            adlak_os_snprintf(state_str, sizeof(state_str), "ready");
            break;

        case ADLAK_SUBMIT_STATE_RUNNING:
            adlak_os_snprintf(state_str, sizeof(state_str), "running");
            break;

        case ADLAK_SUBMIT_STATE_FINISHED:
            adlak_os_snprintf(state_str, sizeof(state_str), "finished");
            break;

        case ADLAK_SUBMIT_STATE_FAIL:
            adlak_os_snprintf(state_str, sizeof(state_str), "fail");
            break;

        default:
            adlak_os_snprintf(state_str, sizeof(state_str), "not support");
            break;
    }

    return adlak_os_snprintf(&buf[0], buf_size, "%-*d%-*d%-*d%-*d%-*s%-*d\n", 12,
                             ptask->context->net_id, 12, ptask->invoke_idx, 20,
                             ptask->invoke_start_idx, 20, ptask->invoke_end_idx, 12, state_str, 12,
                             ptask->flag);
}

static int print_task_list(struct list_head *hd, char *buf, ssize_t buf_size) {
    int ret = 0;

    struct adlak_task *ptask     = NULL;
    struct adlak_task *ptask_tmp = NULL;
    int                number;

    char boundary[] =
        "-----------------------------------------------"
        "-----------------------------------------------";

    ret += adlak_os_snprintf(buf + ret, buf_size - ret, "%s\n", boundary);
    ptask  = NULL;
    number = 0;
    list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
        ret += print_task_info(buf + ret, buf_size - ret, ptask);
        number++;
    }
    if (!number) {
        ret += adlak_os_snprintf(buf + ret, buf_size - ret, "No task.\n");
    }
    ret += adlak_os_snprintf(buf + ret, buf_size - ret, "%s\n", boundary);

    return ret;
}
#endif

int adlak_debug_invoke_list_dump(struct adlak_device *padlak, uint32_t debug) {
#if ADLAK_DEBUG
    int                     ret = 0;
    struct adlak_workqueue *pwq = NULL;
    char *                  buf;
    size_t                  buf_size;
    if (!padlak) {
        return 0;
    }
    AML_LOG_DEBUG("%s", __func__);
    pwq      = &padlak->queue;
    buf_size = 4096;
    buf      = adlak_os_malloc(buf_size, ADLAK_GFP_KERNEL);
    if (!buf) {
        return 0;
    }

    adlak_os_mutex_lock(&pwq->wq_mutex);

    ret = adlak_os_snprintf(buf, buf_size, "\npending list:\n");
    ret += adlak_os_snprintf(buf + ret, buf_size - ret, "%-*s%-*s%-*s%-*s%-*s%-*s\n", 12, "net_id",
                             12, "invoke_id", 20, "invoke_start_id", 20, "invoke_end_id", 12,
                             "state", 12, "flag");
    ret += print_task_list(&pwq->pending_list, buf + ret, buf_size - ret);
    if (debug) {
        AML_LOG_DEBUG("%s\n", buf);
    } else {
        adlak_os_printf("%s\n", buf);
    }

    ret = adlak_os_snprintf(buf, buf_size, "\nscheduled list:\n");
    ret += adlak_os_snprintf(buf + ret, buf_size - ret, "%-*s%-*s%-*s%-*s%-*s%-*s\n", 12, "net_id",
                             12, "invoke_id", 20, "invoke_start_id", 20, "invoke_end_id", 12,
                             "state", 12, "flag");
    ret += print_task_list(&pwq->scheduled_list, buf + ret, buf_size - ret);
    if (debug) {
        AML_LOG_DEBUG("%s\n", buf);
    } else {
        adlak_os_printf("%s\n", buf);
    }

    ret = adlak_os_snprintf(buf, buf_size, "\nFinished list:\n");
    ret += adlak_os_snprintf(buf + ret, buf_size - ret, "%-*s%-*s%-*s%-*s%-*s%-*s\n", 12, "net_id",
                             12, "invoke_id", 20, "invoke_start_id", 20, "invoke_end_id", 12,
                             "state", 12, "flag");
    ret += print_task_list(&pwq->finished_list, buf + ret, buf_size - ret);
    if (debug) {
        AML_LOG_DEBUG("%s\n", buf);
    } else {
        adlak_os_printf("%s\n", buf);
    }
    adlak_os_mutex_unlock(&pwq->wq_mutex);
    adlak_os_free(buf);
#endif
    return 0;
}
