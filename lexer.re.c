#include "lexer.h"
#include <assert.h>
#include "CMeta.h"
CMetaBuffer CMetaBufferInit(const char *fileName, int *errorCode)
{
  CMetaBuffer retVal;
  *errorCode=0;
  //
  FILE *file=fopen(fileName, "r");
  if(file==NULL)
    {
      *errorCode=1;
      return retVal;
    }
  fseek(file, 0, SEEK_END);
  size_t end=ftell(file);
  fseek(file, 0, SEEK_SET);
  size_t start=ftell(file);
  //
  char buffer[end-start+1];
  fread(buffer ,end-start, 1, file);
  buffer[end-start]='\0';
  sds source=sdsnew(buffer);
  fclose(file);
  //
  
  retVal.bufferPos=source;
  retVal.fileStart=source;
  retVal.fileEnd=source+sdslen(source);
  retVal.fileName=fileName;
  return retVal;
}
void CMetaBufferDestroy(CMetaBuffer *buffer)
{
  sdsfree((sds)buffer->fileStart);
}
void CMetaProccessString(CMetaBuffer *stringStart)
{
  const char *start=NULL,*end=NULL,*YYMARKER=NULL;
  
  /*!stags:re2c format='const char *@@;';*/
  /*!re2c
    re2c:yyfill:enable=0;    
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=stringStart->bufferPos;
    re2c:define:YYLIMIT=stringStart->fileEnd;

    String1=["]([\\].|[^"])*["];
    String2=[']([\\].|[^'])*['];

    @start String1 @end {goto end;}
    @start String2 @end {goto end;}
  */
    end:
  if(end==NULL)
    {
      printf("Incomplete string.");
      assert(end!=NULL);
    }
}
