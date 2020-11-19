#include	<stdio.h>

#include	<mfg_func.h>
#include	<mfg_framework.h>
#include	<mfg_queue.h>


int main(int argc, char *argv[])
{
	int ret;

	ret = mfg_framework_initialize();
	return ret;
}
