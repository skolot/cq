#ifndef __FQ_H__
# define __FQ_H__

/**** ELEMENT ********/

typedef long cq_element_head_t;

# define F_EMPTY   ((cq_element_head_t)0x01) //00000001 
# define F_READ    ((cq_element_head_t)0x02) //00000010 
# define F_RECV    ((cq_element_head_t)0x04) //00000100 
# define F_EXPECT  ((cq_element_head_t)0x08) //00001000 
# define F_CORRUPT ((cq_element_head_t)0x10) //00010000 
# define F_ALLMARK ((cq_element_head_t)0x1F) //00011111 

typedef long counter_t;

struct cq;

struct cq_element {
    cq_element_head_t *head;
    counter_t counter;
    void * data;
    size_t data_size;
    struct cq_element *prev;
    struct cq_element *next;
    struct cq *parent_q;

#ifdef DEBUG
    size_t num;
#endif
};

typedef struct cq_element cq_element_t;

# define cq_get_data(cq_element) (cq_element->data)
# define cq_get_data_size(cq_element) (cq_element->data_size)

/**** FQ ***********/

struct cq {
    cq_element_t *q;
    cq_element_t *first;
    cq_element_t *last;
};

typedef struct cq cq_t;
typedef struct cq cqwo_t;
typedef struct cq cqro_t;

cq_t *
_cq_alloc(size_t count);

# define cqwo_alloc(count)                      \
    ((cqwo_t *)_cq_alloc(count))

# define fqro_alloc(count)                      \
    ((fqro_t *)_cq_alloc(count))

void
_cq_free(cq_t *cq);

# define cqwo_free(cqwo)                        \
    _fq_free((cq_t *)cqwo)

# define cqro_free(cqro)                        \
    _fq_free((cq_t *)cqro)

int
cqwo_init(cqwo_t *cqwo, void *mem, size_t max_data_size);

int
cqro_init(cqro_t *cqwo, void *mem, size_t max_data_size);

ssize_t
_cq_get_data(cq_t *cq, cq_element_t **elements, size_t count);

# define cqwo_get_empty_elements(cqwo, elements, count)                 \
    _cq_get_empty_elements((cq_t *)cqwo, elements, count, F_EMPTY, F_EXPECT)

# define cqro_get_recv_elements(cqwo, elements, count)          \
    _cq_get_data((cq_t *)cqro, elements, count, F_RECV, F_READ)

ssize_t
_cq_return_element(cq_element_t *element, cq_element_head_t unset_mark, cq_element_head_t set_mark);

# define cqwo_return_recv_element(element)          \
    _cq_return_element(element, F_EXPECT, F_RECV)

# define cqro_return_read_element(element)          \
    _cq_return_element(element, F_READ, F_EMPTY)

void
cq_dump(cq_t *cq);

#endif /* __CQ_H__ */ 
