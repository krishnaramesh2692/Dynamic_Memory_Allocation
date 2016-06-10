/*
 * malloc.c
 *
 *  Created on: May 24, 2016
 *      Author: krishna
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

/* Align the size of the data block requested.*/
# define align(d) ((((d-1)>>2)<<2)+4)

/* Define the block size since the sizeof will be wrong */
# define BLOCK_SIZE 20

int main(char arg, char **str) {

	typedef struct s_block *t_block;

	struct s_block {									/* Block struct to hold the meta-data of each Block  */
		int free;										/*To indiacate whether the block is free :1 or not 0*/
		t_block next;
		t_block prev;									/*DoublyLinked list to facilitate fusion of more blocks during free*/
		size_t size;									/*Size of the block*/
		void *ptr;										/*A ponter to validate address in free by holding the address of data variable*/
		char data[1];									/*A pointer to the allocated block */

	};
	t_block base = NULL;
	t_block last;


	void * find_block(t_block *last, size_t size) {   /*Finding a chunk: the First Fit Algorithm*/
		t_block b = base;
		if (b && !(b->free && b->size > size)) {
			*last = b;									/* Save last visited chunk, so -*/
			b = b->next;								/*- easily extends the end heap if  no fitting chunk spotted.*/
		}
		return (b);
	}

	void split_block(t_block b, size_t size) {			/* when a chunk is wide enough to hold the asked size plus
	 	 	 	 	 	 	 	 	 	 	 	 	 	 a new chunk , we insert a new chunk in the list.*/
		t_block new;
		new =(t_block )(b->data + size);
		new->free = 1;
		new->next = b->next;
		new->prev = b;
		new->size = b->size - size - BLOCK_SIZE;
		new ->ptr = new ->data;
		b->size = size;
		b->next = new;
		if (new ->next)
		 new ->next ->prev = new;
	}

	void* extend_heap(t_block last, size_t s) {
														/* Add a new block at the of heap */
														/* return NULL if things go wrong */
		t_block new;
		int sb;
		new = sbrk(0);
		sb= (int)(sbrk(s + BLOCK_SIZE));
		if (sb <0) {
			return NULL;
		}
		new->size = s;
		new->next = NULL;
		new->prev = NULL;
		new ->ptr = new ->data;
		if (last) {
			last->next = new;
			new->prev = last;
		}
		new->free = 0;

		return new;
	}

	void * newmalloc(size_t size) {

		t_block b;
		size_t s = align(size);

		if (base) {

			last = base;
			b = find_block(&last, s);						/* Step1 : First find a block */

			if (b) {
				if ((b->size - s) > BLOCK_SIZE + 4) {		/* Step2 :Check if can we split */
					split_block(last, s);
					b->free = 0;
				}

			} else {
				b = extend_heap(last, s);					/* Step3 :No fitting block , extend the heap */
				if (!b) {
					return (NULL);
				}

			}

		} else {											/* first time: Extend heap */
			b = extend_heap(base, s);
			if (!b)
				return NULL;
			base = b;
		}
		return (b->data);

	}

	size_t s;


	void * newcalloc(size_t number, size_t s) {

		int *new;
		int s4, i;
		new = newmalloc(number * s);						/* First do the malloc with right size (product of the two operands);*/
		s4 = align(number*s) >> 2;							/*Put 0 on any bytes in the block	*/
		for (i = 0; i < s4; i++) {
			new[i] = 5;
		}
		return new;

	}

	t_block * get_block(void *p) {
		t_block tmp;
		tmp = (t_block)p;
		return (p = tmp -= BLOCK_SIZE);						/*Return the starting of the block adress */

	}
	int valid_address(void *p) {


		if (base) {

			if ( p > base && p < sbrk(0)) {
				t_block b = (t_block)get_block(p);
				return (p == b->ptr );			/*A ponter to validate address in free by holding the address of data variable*/

			}
		}
		return (0);

	}

	t_block fusion(t_block b) {
		if (b->next && b->next->free) {
			b->size += BLOCK_SIZE + b->next->size;
			b->next = b->next->next;
			if (b->next)
				b->next->prev = b;
		}
		return (b);
	}

	void newfree(void *p) {
		t_block b;
		if (valid_address(p)) {								/* get the Valid block address*/
			b = (t_block )get_block(p);
			b->free = 1;								    /* mark it free*/
			if (b->prev && b->prev->free) {                 /*PREVIOUS EXISTS AND IS FREE,Merge*/
				b = fusion(b->prev);
			}
			if (b->next) {
				b = fusion(b);								/*Next EXISTS AND IS FREE,Merge*/
			} else {

				if (b->prev) {

					b->prev->next = NULL;					/*if we’re the last block we release memory*/
				}

				base = NULL;								/*if there’s no more block, we go back the original state, NULL*/
				brk(b);
			}

		}

	}

	void copy_data(t_block src, t_block dst) {

		size_t *srcdata, *dstdata;
		size_t i;
		srcdata = src->ptr;
		dstdata = dst->ptr;

		for (i = 0; i * 8 < src->size && i * 8 < dst->size; i++) {

			dstdata[i] = srcdata[i];
		}

	}

	void * newrealloc(void *p, size_t size) {
		t_block b, new;
		void *newp;
		if (!p) {
			return newmalloc(size);								/* Allocate a new bloc of the given size using malloc if the address is not available;*/
		}
		if (valid_address(p)) {
			s = align(size);
			b = (t_block)get_block(p);
			if (b->size > s) {									/*If the size doesn’t change, or the extra-available size	*/
				if (b->size - s > (BLOCK_SIZE + 4)) {
					split_block(b, s);							/*If we shrink the block, we try a split;*/
				}

			} else {
				if (b->next && b->next->free) {					/*If the next block is free and provide enough space, we fusion and try to split if necessary*/
					if (b->size + BLOCK_SIZE + b->next->size > s) {
						fusion(b);
						if (b->size - s > (BLOCK_SIZE + 4)) {
							split_block(b, s);
						}

					}
				} else {

					newp = newmalloc(s);
					if (!newp) {
						return NULL;
					}
					new = (t_block)get_block(newp);				/*Allocate a new bloc of the given size using malloc;*/
					copy_data(b, new);					/*Copy data from the old one to the new one;*/
					newfree(p);							/*Free the old block;*/
					return (newp);						/* Return the new pointer*/

				}

			}
			return p;
		}

		return NULL;

	}

}

