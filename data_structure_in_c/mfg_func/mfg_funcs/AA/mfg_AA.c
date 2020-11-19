#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>

#include	<mfg_framework.h>
#include	<mfg_func.h>
#include	<mfg_queue.h>

#include	<mfg_AA_funcs.h>

#define		FUNC_COUNT	2

struct mfg_func_struct mfg_aa_funcs[FUNC_COUNT] = {
	{
		.func_name = "version",
		.func_description = "print version number and "
					"necessary information\n",
		//.mfg_func = mfg_aa_version,
		.mfg_func = NULL,
	},
	{
		.func_name = "version",
		.func_description = "print version number and "
					"necessary information\n",
		//.mfg_func = mfg_aa_version,
		.mfg_func = NULL,
	},
};

struct mfg_funcs_struct mfg_aa = {
	.mfg_func_struct_count = FUNC_COUNT,
	.mfg_func_sets = { mfg_aa_funcs },
};

int mfg_AA_initialize(void)
{
/*
	struct mfg_funcs_struct mfg_aa;
	mfg_aa.mfg_func_struct_count = 1;
	mfg_aa.mfg_func_sets = &mfg_aa_funcs;
*/
	printf("%s: %d ========\n", __func__, __LINE__);
	return mfg_funcs_apply(&mfg_aa);
}
