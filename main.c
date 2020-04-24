#include <stdlib.h>
#include "CMeta.h"
#include "string.h"
#include "lexer.h"
#include "ext/argparse/argparse.h"
#include "ext/c-vector/cvector.h"
#include "util.h"
#include <unistd.h>
static const char *usage[]=
  {
   "CMeta inputFile [--cc compiler] [-s generatorFile] [-o outfile] [[--] compilerOptions]",
   NULL
  };
sds CMetaFormatIncludes(const char *string, const char chr)
{
  sds retVal=sdsnew("");
  const char *ptr=string, *oldPtr=string;
  while((ptr=strchr(ptr, chr)))
    {
      sds temp=sdsnewlen(oldPtr, ptr-oldPtr);
      retVal=sdscatfmt(retVal, "#include <%s>\n", temp);
      oldPtr=ptr+1;
      ptr=oldPtr;
      sdsfree(temp);
    }
  retVal=sdscatfmt(retVal, "#include <%s>\n", oldPtr);
  return retVal;
}
const char *CMetaLastOccurance(const char *input,const char whichChar)
{
  const char *pos=input;
  const char *last=NULL;
  while((pos=strchr(pos, whichChar)))
    {
      last=pos;
      pos++;
    }
  return last;
}
int main(int argc,const char **argv)
{
  FILE *file=fopen("../bufferTest.txt", "r");
  size_t size=3;
  char buffer[size];
  char *cursor=buffer+size,*limit=buffer+size;
  for(int i=1;i!=8;i++)
    {
      int size1=CMetaYYFILL(file, buffer, &limit, &cursor, 0, size);
      buffer[size]='\0';
      cursor+=2;
      printf("start:%i\n:buffer:%s\n", size1, buffer);
      if(size1==6)
	printf("slice7:9:%s",CMetaReadSliceFromFile(file, buffer, 6, size, 7, 9));
    }
  return EXIT_SUCCESS;
  struct argparse argparser;
  const char *compiler="gcc";
  const char *outputFile=NULL;
  const char *inputFile=NULL;
  const char *_outputSource=NULL;
  const char *includes=NULL;
  int help=0;
  struct argparse_option options[]=
    {
     OPT_BOOLEAN(
		'h',
		"help" ,
		&help,
		"Displays the usage of CMeta."),
     OPT_STRING(
		0,
		"cc" ,
		&compiler,
		"The compiler to use,(defaults to gcc)."),
     OPT_STRING(
		'o',
		"output" ,
		&outputFile,
		"The file to write the result to.(Defaults to input file name with \".out\" prepended to the extension )"),
     OPT_STRING(
		's',
		"source",
		&_outputSource,
		"The file to write the generator source to.Defaults to a tmp file."
		),
     OPT_STRING(
		'i',
		"includes",
		&includes,
		"A comma seperated list of includes to be put in \"#include<XXX>\"s for the generator code."
		),
     OPT_END()
    };
  argparse_init(&argparser, options, usage, 0);
  argc=argparse_parse(&argparser, argc, argv);
  if(argc<1)
    {
      inputFile=NULL;
    }
  else
    {
      inputFile=argv[0];
    }
  if(inputFile==NULL||help)
    {
      argparse_usage(&argparser);
      return help;
    }
  else
    {
      int errorCode;
      sds absoluteInputFileName=CMetaMakeFileNameAboslute(sdsnew(inputFile)); //FREED
      CMetaBuffer buffer=CMetaBufferInit(absoluteInputFileName, &errorCode);
      if(errorCode)
	{
	  printf("File \"%s\" doesnt exist.", inputFile);
	  return 0;
	}
      //
      sds outputFilePath;
      //
      if(outputFile==NULL)
	{
	  size_t inputStrLen=strlen(inputFile);
	  const char *lastSeperator=CMetaLastOccurance(inputFile, '/');
	  if(lastSeperator==NULL)
	    lastSeperator=inputFile;
	  const char *extensionStart=strchr(lastSeperator, '.');
	  sds newFileName;
	  //
	  //
	  if(extensionStart==NULL)
	    {
	      //no extension provided
	      newFileName=sdsnew(lastSeperator);
	      newFileName=sdscat(newFileName,".out");
	    }
	  else
	    {
	      newFileName=sdsnewlen(lastSeperator, (extensionStart-lastSeperator)/sizeof(const char));
	      newFileName=sdscatfmt(newFileName, ".out%s", extensionStart);
	    }
	  outputFilePath=sdsnewlen(inputFile, (lastSeperator-inputFile)/sizeof(const char));
	  outputFilePath=sdscat(outputFilePath, newFileName);
	  sdsfree(newFileName);
	}
      else
	{
	  outputFilePath=sdsnew(outputFile);
	}
      //TODO
      outputFilePath=CMetaMakeFileNameAboslute(outputFilePath);
      CMetaInstance instance=CMetaInstanceInit();
      CMetaRun(&instance, &buffer);
      //
      bool need2FreeOutputSource=false;
      sds outputSource;
      if(_outputSource==NULL)
	{
	  outputSource=tempnam(NULL, NULL);
	  outputSource=sdsnew(outputSource);
	  outputSource=sdscat(outputSource, ".c");
	  need2FreeOutputSource=true;
	}
      else
	{
	  outputSource=sdsnew(_outputSource);
	}
      //
      sds includesCode; //FREED
      if(includes==NULL)
	{
	  includesCode=sdsnew("");
	}
      else
	{
	  includesCode=CMetaFormatIncludes(includes, ',');
	}
      //
      CMetaCompilerOptions options;
      options.compiler=compiler;
      options.deleteSourceAfterUse=false;
      options.options=NULL;
      for(int i=1;i!=argc;i++)
	cvector_push_back(options.options, argv[i]);
      CMetaWriteOut(&instance, &buffer, outputFilePath, &options, outputSource);
      //
      sdsfree(outputSource);
      CMetaBufferDestroy(&buffer);
      sdsfree(outputFilePath);
      sdsfree(includesCode);
      sdsfree(absoluteInputFileName);
      CMetaInstanceDestroy(&instance);
    }
  return EXIT_SUCCESS;
}
