#ifndef SVN_UPDATE_H
#define SVN_UPDATE_H

typedef struct notify_item {
    const char *path;
    svn_wc_notify_state_t action;
    notify_item *next;
} notify_item;

typedef struct notify_list {
    notify_item *first;
    notify_item *last;
} notify_list;

void __notify_callback (void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool);
void __notify_free_list (notify_list *list);
notify_item *__notify_alloc_item ();
notify_list *__notify_alloc_list ();

#endif
