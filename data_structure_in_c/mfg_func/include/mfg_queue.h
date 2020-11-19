#ifndef __MFG_FUNC_INCLUDE_MFG_QUEUE_H__
#define __MFG_FUNC_INCLUDE_MFG_QUEUE_H__

#include	<mfg_func.h>

struct mfg_queue_node {
	struct mfg_func_struct *data;
	struct mfg_queue_node *next;
};

int mfg_funcs_apply(struct mfg_funcs_struct *);
int mfg_func_queue_initialize(void);
void mfg_func_queue_uninitialize(void);

#endif /* __MFG_FUNC_INCLUDE_MFG_QUEUE_H__ */
