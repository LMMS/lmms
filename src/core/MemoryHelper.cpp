#include <stdlib.h>

#include "lmms_basics.h"
#include "MemoryHelper.h"

/**
 * Allocate a number of bytes and return them. 
 * @param _byteNum is the number of bytes 
 */
void* MemoryHelper::alignedMalloc(int _byteNum) {
	char *ptr,*ptr2,*aligned_ptr;
	int align_mask = ALIGN_SIZE- 1;

	ptr = (char *) malloc(_byteNum + ALIGN_SIZE + sizeof(int));

	if(ptr==NULL) return(NULL);

	ptr2 = ptr + sizeof(int);
	aligned_ptr = ptr2 + (ALIGN_SIZE- ((size_t)ptr2 & align_mask));

	ptr2 = aligned_ptr - sizeof(int);
	*((int *)ptr2)=(int)(aligned_ptr - ptr);

	return(aligned_ptr);
}




/**
 * Free an aligned buffer
 * @param _buffer is the buffer to free
 */
void MemoryHelper::alignedFree(void* _buffer) {
	if( _buffer != NULL )
	{
		int *ptr2=(int *)_buffer - 1;
		_buffer = (char *)_buffer - *ptr2;
		free(_buffer);
	}
}




