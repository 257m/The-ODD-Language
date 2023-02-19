#ifndef VEC_H
#define VEC_H

typedef struct {
	char* data;
	size_t size;
} vector;

void writeData(char* dest, char* data, size_t data_size);
void vector_append(vector* vec, void* data, size_t append_size);

#define vector_new_alloc(vec, size)                                            \
	{                                                                          \
		vec.data = malloc(size);                                               \
		vec.size = size;                                                       \
	}

#endif /* VEC_H */