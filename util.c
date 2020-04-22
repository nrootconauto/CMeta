#include "util.h"
#include "assert.h"
#include <unistd.h>
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
