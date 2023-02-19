#include <stddef.h>
#include <stdlib.h>
#include "../include/vec.h"

void writeData(char* dest, char* data, size_t data_size)
{
	for (int i = 0; i < data_size; i++) {
		dest[i] = data[i];
	}
}

void vector_append(vector* vec, void* data, size_t append_size)
{
	if (vec->size)
		vec->data = realloc(vec->data, vec->size + append_size);
	else
		vec->data = malloc(append_size);

	writeData(vec->data + vec->size, data, append_size);
	vec->size += append_size;
}