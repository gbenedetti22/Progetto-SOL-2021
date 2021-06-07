typedef struct elem{
    int value;
    struct elem *next;
} elem;

typedef struct {
    elem *head;
    elem *tail;
    int size;
} sorted_list;

sorted_list *sortedlist_create();
bool sortedlist_remove(sorted_list **l, int value);
bool sortedlist_isEmpty(sorted_list* l);
void sortedlist_insert(sorted_list **l, int value);
void sortedlist_destroy(sorted_list** l);
int sortedlist_getMax(sorted_list* l);
int sortedlist_getNext(sorted_list* l);
void sortedlist_iterate();

