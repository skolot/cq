#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <log.h>
#include <cq.h>

/**** ELEMENT ********/

cq_element_head_t
element_set_mark(cq_element_t *element, cq_element_head_t mark)
{
    cq_element_head_t head;

    if (NULL == element) {
        l_err("element is NULL");
        return -1;
    }

    if (NULL == element->head) {
        l_err("elements head is NULL");
        return -1;
    }

    l_debug("head %p", element->head);

    memcpy(&head, element->head, sizeof(cq_element_head_t));
    l_debug("mark 0x%lx", head);
    head |= mark;
    memcpy(element->head, &head, sizeof(cq_element_head_t));
    l_debug("mark 0x%lx", head);

    return head;
}

cq_element_head_t
element_unset_mark(cq_element_t *element, cq_element_head_t mark)
{
    cq_element_head_t head;

    if (NULL == element) {
        l_err("element is NULL");
        return -1;
    }

    if (NULL == element->head) {
        l_err("elements head is NULL");
        return -1;
    }

    l_debug("head %p", element->head);

    memcpy(&head, element->head, sizeof(cq_element_head_t));
    l_debug("mark 0x%lx", head);
    head ^= mark;
    memcpy(element->head, &head, sizeof(cq_element_head_t));
    l_debug("mark 0x%lx", head);

    return head;
}

void
element_clear_mark(cq_element_t *element)
{
    if (NULL == element) {
        l_err("element is NULL");
        return;
    }

    l_debug("head %p", element->head);

    memset(element->head, 0, sizeof(cq_element_head_t));
}

int
element_is_marked(const cq_element_t *element, cq_element_head_t mark)
{
    if (NULL == element) {
        l_err("element is NULL");
        return -1;
    }

    if (NULL == element->head) {
        l_err("elements head is NULL");
        return -1;
    }

    l_debug("head %p", element->head);
    l_debug("mark 0x%lx", element->head);
    
    return (*element->head & mark);
}

/**** CQ ***********/

// internal func
void
_cq_init(cq_t *cq, void *mem, size_t max_data_size);

// external func
cq_t *
_cq_alloc(size_t count)
{
    cq_t *cq = NULL;
    cq_element_t *element;
    int i;

    if (NULL == (cq = calloc(1, sizeof(struct cq)))) {
        l_crit("can't alloc memory for queue");
        return NULL;
    }

    for (i = 0; i < count; i++) {
        l_debug("iter %d", i);
        
        if (NULL == (element = calloc(1, sizeof(cq_element_t)))) {
            l_crit("can't alloc memory for element node: %s", strerror(errno));
            return NULL;
        }

        l_debug("alloc element");

        if (cq->q) {
            element->next = cq->q;
            element->prev = cq->q->prev;
            element->prev->next = element;
            cq->q->prev = element;
        } else {
            cq->q = element;
            element->next = element->prev = element;
        }

        l_debug("add node");

        element->parent_q = cq;

#ifdef DEBUG
        element->num = i;
#endif

        l_debug("next iter");
    }

    cq->first = cq->q;
    cq->last = cq->q->next; // XXX: zaglushka, see get elements

    return cq;
}

void
_cq_free(cq_t *cq)
{
    cq_element_t *element, *nelement;

    cq->q->prev->next = NULL;
    element = cq->q;

    while (element) {
        nelement = element->next;
        free(element);
        element = nelement;
    }
    
}

void
_cq_init(cq_t *cq, void *mem, size_t data_size)
{
    cq_element_t *element;
    char *p;

    /* aligment ? */

    element = cq->q;
    p = (char *)mem;

    do {
        // init head
        element->head = (cq_element_head_t *)p;
        p += sizeof(cq_element_head_t);

        // init data
        element->data = p;
        element->data_size = data_size;
        p += data_size;

        element = element->next;
    } while (element != cq->q);
}

int
cqwo_init(cqwo_t *cqwo, void *mem, size_t data_size)
{
    cq_element_t *element;

    if (NULL == cqwo) {
        l_err("cqwo is NULL");
        return -1;
    }

    if (NULL == mem) {
        l_err("shm is NULL");
        return -1;
    }
    
    _cq_init((cq_t *)cqwo, mem, data_size);
    
    element = cqwo->q;

    do {
        // set mark
        element_clear_mark(element);
        element_set_mark(element, F_EMPTY);

        element = element->next;
    } while (element != cqwo->q);

    return 0;
}

int
cqro_init(cqwo_t *cqro, void *mem, size_t data_size)
{
    if (NULL == cqro) {
        l_err("cqro is NULL");
        return -1;
    }

    if (NULL == mem) {
        l_err("shm is NULL");
        return -1;
    }
    
    _cq_init((cq_t *)cqro, mem, data_size);

    return 0;
}

ssize_t
_cq_get_elements(cq_t *cq, cq_element_t **elements, size_t element_count, cq_element_head_t unset_mark, cq_element_head_t set_mark)
{
    ssize_t count = 0;

    if (NULL == cq) {
        l_err("cqwo is NULL");
        return -1;
    }

    if (NULL == cq->q) {
        l_err("cqwo queue is empty or not initialised");
        return -1;
    }

    if (NULL == elements) {
        l_err("elements is NULL");
        return -1;
    }
    
    if (0 == element_count) {
        l_warn("getting 0 elements?");
        return 0;
    }

    while (element_is_marked(cq->last->prev, unset_mark) &&
           count != element_count) {
        
        l_debug("element_count == %ld, count == %ld", element_count, count);
        
        cq->last = cq->last->prev;
        
        element_unset_mark(cq->last, unset_mark);
        element_set_mark(cq->last, set_mark);
        
        elements[count++] = cq->last;
    }

    if (0 == count) {
        l_debug("no elements marked %ld", unset_mark);
    }

#ifdef DEBUG
    cq_dump(cq);
#endif

    return count;
}

ssize_t
_cq_return_element(cq_element_t *element, cq_element_head_t unset_mark, cq_element_head_t set_mark)
{

    if (NULL == element) {
        l_err("element is NULL");
        return -1;
    }

    if (NULL == element->parent_q) {
        l_err("element is corrupted");
        return -1;
    }

    element_unset_mark(element, unset_mark);
    element_set_mark(element, set_mark);

    // flush 

    while (element->parent_q->last != element->parent_q->first) {
        if (element_is_marked(element->parent_q->first, set_mark))
            element->parent_q->first = element->parent_q->first->prev;
        else 
            break;
    }

    return 0;
}

void
cq_dump(cq_t *cq)
{
    cq_element_t *e;
    
    l_debug("cq->q %p", cq->q);
    l_debug("cq->first %p", cq->first);
    l_debug("cq->last %p", cq->last);

    e = cq->q;

    do {
#ifdef DEBUG        
        l_debug("cq node %ld", e->num);
#endif
        l_debug("node %p prev %p next %p", e, e->prev, e->next);
        l_debug("head %p video %p audio %p", e->head, e->video, e->audio);
        l_debug("head 0x%lx", e->head[0]);

        e = e->next;
    } while (e != cq->q);

}
