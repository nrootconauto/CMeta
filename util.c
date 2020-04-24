#include "util.h"
#include "assert.h"
#include <unistd.h>
#include <string.h>
sds CMetaMakeFileNameAboslute(sds filePath)
{
  //create output file path,if not absolute,make it absolute
  if(filePath[0]!='/')
    {
      char cwd[256];
      assert(NULL!=getcwd(cwd, 256));
      sds newOutputFilePath=sdscatfmt(sdsnew(cwd), "/%s", filePath);
      sdsfree(filePath);
      return newOutputFilePath;
    }
  return filePath;
}
sds CMetaCombineFormat(const vec_sds items,const char *format)
{
  sds retVal=sdsnew("");
  size_t vecSize=cvector_size(items);
  sds *basePtr=cvector_begin(items);
  for(size_t i=0;i!=vecSize;i++)
    {
      retVal=sdscatfmt(retVal, format, basePtr[i]);
    }
  return retVal;
}
sds CMetaTrimSds(const sds input)
{
  const char *whitespaceChars=" \t\n\r";
  //front
  const char *front=input;
  const char *end=input+sdslen(input);
  while(front!=end)
    {
      if(NULL==strchr(whitespaceChars, *front))
	break;
      front++;
    }
  //back
  while(end--!=front)
    {
      if(NULL==strchr(whitespaceChars, *end))
	break;
    }
  //
  sds temp=sdsnewlen(front,end-front);
  sdsfree(input);
  return temp;
}
size_t CMetaYYFILL(FILE *inputFile, char *buffer, const char **limit,char **cursor,size_t n, size_t bufferSize)
{
  //
  const char *oldCursor=*cursor;
  *cursor=buffer;
  //
  size_t oldRemaining=bufferSize-(oldCursor-buffer);
  memmove(*cursor, oldCursor, oldRemaining);
  //
  size_t bufferStartPosition=ftell(inputFile)-(oldCursor-buffer);
  //
  size_t requested=bufferSize-oldRemaining;
  size_t read=fread(buffer+oldRemaining, 1, requested, inputFile);
  *limit=read+*cursor;
  //
  if(read!=requested)
      memset(*cursor+read, 0, bufferSize-read);
  return bufferStartPosition;
}
