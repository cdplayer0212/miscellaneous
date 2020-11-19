#include	<stdio.h>

#include	<mfg_queue.h>

int mfg_framework_initialize(void)
{
	printf("%s: %d ========\n", __func__, __LINE__);
	mfg_func_queue_initialize();
	mfg_AA_initialize();


	mfg_func_queue_uninitialize();
	return 0;
}
