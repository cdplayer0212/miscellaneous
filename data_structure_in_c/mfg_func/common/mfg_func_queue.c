#include	<stdio.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<string.h>

#include	<mfg_queue.h>

static struct mfg_queue_node *head = NULL;
static int mfg_func_queue_initialized = 0;

static int queue_initialize(void)
{
	if (mfg_func_queue_initialized == 0) {
		head = malloc(sizeof(struct mfg_queue_node));
		if (head == NULL) {
			fprintf(stderr, "%s: memory allocate failed\n",
								__func__);
			return -EIO;
		}
		memset(head, 0x0, sizeof(struct mfg_queue_node));
		head->next = head;
		mfg_func_queue_initialized = 1;
	}
	return 0;
}

static void queue_free_all(void)
{
	struct mfg_queue_node *ptr;
	for (ptr = head->next; ptr != head; ptr = ptr->next) {
		printf("%s: free addr: %p\n", __func__, ptr);
		free(ptr);
	}
}

static void queue_uninitialize(void)
{
	if (mfg_func_queue_initialized != 0) {
		free(head);
		head = NULL;
		mfg_func_queue_initialized = 0;
	}
}

static int queue_is_empty(void)
{
	if (head == head->next)
		return 1;

	return 0;
}

static int mfg_func_queue_push(struct mfg_func_struct *data)
{
	struct mfg_queue_node *new_node = malloc(sizeof(struct mfg_queue_node));
	memset(new_node, 0x0, sizeof(struct mfg_queue_node));
	struct mfg_queue_node *ptr = head;

	if (new_node == NULL) {
		fprintf(stderr, "%s: alloc new node failed\n", __func__);
		return -EIO;
	}
	if (queue_is_empty())
		new_node->next = head;
	else
		new_node->next = ptr->next;

	new_node->data = data;
	ptr->next = new_node;
	return 0;
}

static int mfg_func_queue_pop(struct mfg_func_struct *data)
{
	struct mfg_queue_node *ptr, *prev_ptr;

	if (queue_is_empty())
		return 0;

	for (prev_ptr = head, ptr = head->next; ptr->next != head;
						prev_ptr = ptr, ptr = ptr->next)
		;;

	free(prev_ptr->next);
	prev_ptr->next = head;
	return 0;
}

int mfg_funcs_apply(struct mfg_funcs_struct *data)
{
	int i;
	for (i = 0; i < data->mfg_func_struct_count; i++) {
		if (mfg_func_queue_push(data->mfg_func_sets[i]) != 0) {
			fprintf(stderr, "%s: apply mfg_funcs failed\n",
								__func__);
			return -EINVAL;
		}
	}
	return 0;
}

int mfg_func_queue_initialize(void)
{
	return queue_initialize();
}

void mfg_func_queue_uninitialize(void)
{
	queue_free_all();
	queue_uninitialize();
}
