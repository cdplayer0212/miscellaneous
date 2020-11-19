#include	<stdio.h>
#include	<stdio.h>
#include	<string.h>

#define SUSPEND_STAGE_FLAG	0x10
#define PRE_SUSPEND_STAGE	(SUSPEND_STAGE_FLAG | (0x1 << 0))
#define SUSPEND_STAGE		(SUSPEND_STAGE_FLAG | (0x1 << 1))
#define LATE_SUSPEND_STAGE	(SUSPEND_STAGE_FLAG | (0x1 << 2))
#define RESUME_STAGE_FLAG	0x20
#define PRE_RESUME_STAGE	(RESUME_STAGE_FLAG  | (0x1 << 0))
#define RESUME_STAGE		(RESUME_STAGE_FLAG  | (0x1 << 1))
#define LATE_RESUME_STAGE	(RESUME_STAGE_FLAG  | (0x1 << 2))

#define PM_STAGE_TOTAL_COUNT	3

int pm_data_01_func(void *, unsigned char);


struct __pm_data_structure {
	char *func_name;
	unsigned char stage;
	void *data;
	int (*pm_callback)(void *, unsigned char stage);
};

struct __pm_data_structure pm_data_01 = {
	.func_name = "pm_data_01",
	.stage = PRE_SUSPEND_STAGE | LATE_RESUME_STAGE,
	.data = NULL,
	.pm_callback = pm_data_01_func,
};

int pm_data_01_func(void *data, unsigned char stage)
{
	struct __pm_data_structure *pm_data = data;
	switch (stage) {
	case PRE_SUSPEND_STAGE:
		printf("%s: func: %s, PRE_SUSPEND_STAGE\n", __func__,
					pm_data->func_name);
		break;
	case PRE_RESUME_STAGE:
		printf("%s: func: %s, PRE_RESUME_STAGE\n", __func__,
					pm_data->func_name);
		break;
	}
	return 0;
}

int perform_stage(struct __pm_data_structure *pm_data[])
{
	unsigned char suspend_stage[PM_STAGE_TOTAL_COUNT] = {
		PRE_SUSPEND_STAGE,
		SUSPEND_STAGE,
		LATE_SUSPEND_STAGE,
	};
	unsigned char resume_stage[PM_STAGE_TOTAL_COUNT] = {
		PRE_RESUME_STAGE,
		RESUME_STAGE,
		LATE_RESUME_STAGE,
	};
	int i;
	int ret = 0;
	for (i = 0; i < PM_STAGE_TOTAL_COUNT; i++) {
		if (pm_data[i] == NULL)
			continue;

		pm_data[i]->pm_callback(pm_data[i], suspend_stage[i]);
	}
	for (i = PM_STAGE_TOTAL_COUNT - 1; i >= 0; i--) {
		if (pm_data[i] == NULL)
			continue;

		pm_data[i]->pm_callback(pm_data[i], resume_stage[i]);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	struct __pm_data_structure *pm_data[8];

	memset(pm_data, 0x0, sizeof(pm_data));
	pm_data_01.data = &pm_data_01;
	pm_data[0] = &pm_data_01;
	pm_data[0]->pm_callback(&pm_data_01, PRE_SUSPEND_STAGE);
	pm_data[0]->pm_callback(&pm_data_01, PRE_RESUME_STAGE);

	perform_stage(pm_data);
	return 0;
}
