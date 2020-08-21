#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

struct queue_node {
	void *data;
	struct queue_node *next, *prev;
};

static struct queue_node *head = NULL;
static int queue_initialized = 0;

int queue_initialize(char *queue_name)
{
	if (queue_initialized == 0) {
		head = malloc(sizeof(struct queue_node));
		memset(head, 0x0, sizeof(struct queue_node));
		head->data = (void *)queue_name;
		head->next = head;
		head->prev = head;
		queue_initialized = 1;
	}
	return 0;
}

int queue_uninitialize(void)
{
	if (queue_initialized) {
		free(head);
		queue_initialized = 0;
	}
	return 0;
}

int queue_is_empty()
{
	if (head == head->next)
		return 1;

	return 0;
}

int queue_push(void *data)
{
	struct queue_node *new_node = malloc(sizeof(struct queue_node));
	memset(new_node, 0x0, sizeof(struct queue_node));
	struct queue_node *ptr = head;

	if (queue_is_empty()) {
		new_node->next = head;
		new_node->prev = head;
	} else {
		new_node->next = ptr->next;
		new_node->prev = head;
	}
	new_node->data = data;
	ptr->next = new_node;
	return 0;
}

int queue_pop(void)
{
	struct queue_node *ptr, *prev_ptr;

	if (queue_is_empty())
		return 0;

	for (prev_ptr = head, ptr = head->next; ptr->next != head;
					prev_ptr = ptr, ptr = ptr->next)
		;;

	free(prev_ptr->next);
	prev_ptr->next = head;
	return 0;
}

void queue_free_all()
{
	struct queue_node *ptr;
	for (ptr = head->next; ptr != head; ptr = ptr->next) {
		printf("%s: free addr: %p\n", __func__, ptr);
		free(ptr);
	}

}

void queue_dump()
{
	struct queue_node *ptr;
	char *charp;
	for (ptr = head->next; ptr != head; ptr = ptr->next) {
		charp = (char *)ptr->data;
		printf("node addr: %p, next: %p, data: %s\n",
			ptr, ptr->next, charp);
	};
}

int main(int argc, char *argv[])
{
	queue_initialize("HEAD");
	queue_push("11111111111");
	queue_push("22222222222");
	queue_push("33333333333");
	queue_pop();
	queue_push("44444444444");
	queue_push("55555555555");
	queue_pop();
	queue_push("66666666666");
	queue_push("77777777777");
	queue_push("88888888888");

	queue_dump();
	queue_free_all();
	queue_uninitialize();

	return 0;
}
