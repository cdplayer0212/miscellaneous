#ifndef __MFG_FUNC_INCLUDE_MFG_FUNC_H__
#define __MFG_FUNC_INCLUDE_MFG_FUNC_H__

struct mfg_func_struct {
	char *func_name;
	char *func_description;
	int (*mfg_func)(int argc, char *argv[]);
};

struct mfg_funcs_struct {
	unsigned int mfg_func_struct_count;
	struct mfg_func_struct *mfg_func_sets[];
};

#endif /* __MFG_FUNC_INCLUDE_MFG_FUNC_H__ */
