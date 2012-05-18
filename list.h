#ifndef _LIST_H_
#define _LIST_H_

/*
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
*/

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define container_of_first_member(ptr, type) ({ \
		(type *) ( (char *) ptr) ;})

struct list_head {
	struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}
static inline int list_is_last(const struct list_head *list,
				const struct list_head *head)
{
	return list->next == head;
}
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}


#define list_entry(ptr, type) \
	container_of_first_member(ptr, type)

#define list_for_each_entry_safe(pos, n, type, head, member)		\
	for (pos = list_entry((head)->next, type),		\
		n = list_entry(pos->member.next, type);		\
		&pos->member != (head);					\
		pos = n, n = list_entry(n->member.next, type))

/*
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, _typeof_(*pos), member),	\
		n = list_entry(pos->member.next, _typeof_(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, _typeof_(*n), member))
*/
#endif
