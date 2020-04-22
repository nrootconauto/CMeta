#pragma once
#include "ext/sds/sds.h"
#include "ext/c-vector/cvector.h"
typedef cvector_vector_type(sds) vec_sds;
sds CMetaMakeFileNameAboslute(sds filePath);
sds CMetaCombineFormat(const vec_sds items,const char *format);
