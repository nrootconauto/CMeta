#include "lexer.h"
#include <assert.h>
#include "CMeta.h"
#include "util.h"
//placeholder
#define YYMAXFILL 10
/*!max:re2c*/
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
bool CMetaIsIdentifier(const char *text)
{
  const char *ptr;
  for(ptr=text;*ptr!='\0';ptr++)
    {
      if(*ptr>='a'&&*ptr<='z')
	continue;
      if(*ptr>='A'&&*ptr<='Z')
	continue;
      if(*ptr=='_')
	continue;
      //check if passed first charactor
      if(ptr!=text)
	if(*ptr>='0'&&*ptr<='9')
	  continue;
      break;
    }
  return ptr!=text;
}
sds CMetaGetLocationString(CMetaBuffer *buffer, size_t pos)
{
  char buffer2[YYMAXFILL+1];
  const char *limit=buffer2;
  const char *cursor,*YYMARKER;
  int line=1,col=1;
#define YYFILL(n) CMetaYYFILL(file, buffer2, &limit, &cursor, n, YYMAXFILL);
  FILE *file=fopen(buffer->fileName, "r");
  if(file==NULL)
    return NULL;
 loop:
  if(pos--==0)
    goto end;
  /*!re2c
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=cursor;
    re2c:define:YYLIMIT=limit;

    * {col++;goto loop;}
    [\n] {line++; col=1; goto loop;}
   */
 end:
  fclose(file);
  return sdscatfmt(sdsnew(""), "\"%s\" %i:%i", buffer->fileName, line, col);
}
