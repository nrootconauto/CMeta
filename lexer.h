#pragma once
#include <stdio.h>
#include <stdbool.h>
#include "ext/sds/sds.h"
typedef const char *CMetaLexerState; 
extern const CMetaLexerState
  CMETA_LEXER_STATE_STRING,
  CMETA_LEXER_STATE_MACRO,
  CMETA_LEXER_STATE_CODE
;
typedef struct
{
  size_t pos;
  size_t cutoutPos;
} CMetaFilePosition;
typedef struct
{
  size_t start;
  size_t end;
} CMetaSourceSlice;
typedef struct
{
  const char *fileStart;
  const char *bufferPos;
  const char *fileEnd;
  const char *fileName;
} CMetaBuffer;
CMetaBuffer CMetaBufferInit(const char *fileName,int *errorCode);
void CMetaBufferDestroy(CMetaBuffer *buffer);
void CMetaProccessString(CMetaBuffer *stringStart);
sds CMetaGetLocationString(CMetaBuffer *buffer, size_t pos);
