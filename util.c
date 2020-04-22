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
