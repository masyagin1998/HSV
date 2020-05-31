/**
 * \file rb.c
 * \author Masyagin M.M., masyagin1998@yandex.ru
 * \brief Реализация API буферизации.
 */
/**
 * \ingroup rb
 * \{
 */
#include "rb.h"

#include <stdlib.h>
#include <string.h>

rb_t create_rb()
{
	rb_t rb;

	rb = (rb_t) calloc(1, sizeof(struct RING_BUFFER));
	return rb;
}

enum RB_CODE rb_config(rb_t rb, unsigned cap)
{
	enum RB_CODE r;

	rb->data = (char*) calloc(cap, sizeof(char));
	if (rb->data == NULL) {
		r = RB_CODE_ALLOC_ERR;
		goto err0;
	}

	rb->cap = cap;

	return RB_CODE_OK;

 err0:
	return r;
}

unsigned rb_len(const rb_t rb)
{
	return rb->len;
}

unsigned rb_cap(const rb_t rb)
{
	return rb->cap;
}

unsigned rb_idx_in(const rb_t rb)
{
	return rb->idx_in;
}

unsigned rb_idx_out(const rb_t rb)
{
	return rb->idx_out;
}

enum RB_CODE rb_push(rb_t rb, const char*data, unsigned data_len)
{
    unsigned i;
    
    if ((data_len + rb_len(rb)) > rb_cap(rb)) {
        return RB_CODE_OVERFLOW_ERR;
    }

    for (i = 0; i < data_len; i++) {
        rb->len++;
        rb->data[rb->idx_in] = data[i];
		rb->idx_in++;
        if (rb->idx_in >= rb->cap) {
            rb->idx_in = 0;
        }
    }

    return RB_CODE_OK;
}

unsigned rb_get(rb_t rb, char*data, unsigned data_cap)
{
    unsigned i;
    
    unsigned data_len;

	data_len = rb_len(rb);
    if (data_cap < data_len) {
        data_len = data_cap;
    }

    for (i = 0; i < data_len; i++) {
        rb->len--;
        data[i] = rb->data[rb->idx_out];
		rb->idx_out++;
        if (rb->idx_out >= rb->cap) {
            rb->idx_out = 0;
        }
    }

    return data_len;
}

void rb_deconfig(rb_t rb)
{
	free(rb->data);
}

void rb_clean(rb_t rb)
{
	memset(rb, '\0', sizeof(*rb));
}

void rb_free(rb_t rb)
{
	free(rb);
}
/**
 * /}
 */
