#pragma once
#include "ext/sds/sds.h"
#include "ext/c-vector/cvector.h"
#include <stdio.h>
typedef cvector_vector_type(sds) vec_sds;
sds CMetaMakeFileNameAboslute(sds filePath);
sds CMetaCombineFormat(const vec_sds items,const char *format);
size_t CMetaYYFILL(FILE *inputFile, char *buffer, char **limit,char **cursor,size_t n, size_t bufferSize);
