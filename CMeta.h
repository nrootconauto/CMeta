#pragma once
#include "lexer.h"
#include "ext/c-vector/cvector.h"
#include "ext/sds/sds.h"
#define CMETA_TEMPLATE_FILE "../CMetaTemplate.c"
typedef enum
  {
   CMETA_SEGMENT_MARK,
   CMETA_SEGMENT_FUNCTION,
   CMETA_SEGMENT_INCLUDE,
   CMETA_SEGMENT_INLINE,
  } CMetaSegmentType;
typedef struct
{
  bool completed;
  CMetaSegmentType type;
  sds scope;
  CMetaFilePosition positionStart;
  CMetaFilePosition positionEnd;
} CMetaSegment;
typedef cvector_vector_type(CMetaSegment) vec_CMetaSegment;
typedef struct
{
  size_t pos;
  sds name;
} CMetaMark;
typedef cvector_vector_type(CMetaMark) vec_CMetaMark; 
typedef struct
{
  int cbracketDepth;
  bool inSegment;
  int segmentDepth;
  CMetaLexerState state;
  vec_CMetaSegment segments;
} CMetaInstance;
typedef cvector_vector_type(const char *)vec_const_charP;
typedef struct
{
  const char *compiler;
  vec_const_charP options;
  bool deleteSourceAfterUse;
} CMetaCompilerOptions;

CMetaInstance CMetaInstanceInit();
void CMetaInstanceDestroy(CMetaInstance *instance);
int CMetaRun(CMetaInstance *instance,CMetaBuffer *textStart);
void CMetaWriteOut(const CMetaInstance *instance, const CMetaBuffer *buffer, const char *outputFile, const CMetaCompilerOptions *compilerOptions, const char *testSourceFile);
sds CMetaTrimSds(const sds input);
char *CMetaReadSliceFromFile(FILE *file, char *buffer, size_t bufferStart, size_t bufferSize, size_t start, size_t end);
