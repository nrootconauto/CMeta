#include "CMeta.h"
#include "string.h"
#include "util.h"
#include "ext/C_Unescaper/escaper.h"
CMetaInstance CMetaInstanceInit()
{
  CMetaInstance retVal;
  retVal.segments=NULL;
  retVal.inSegment=false;
  retVal.cbracketDepth=0;
  retVal.state=NULL;
  return retVal;
}
void vec_sdsDestroy(vec_sds toDestroy)
{
  size_t size=cvector_size(toDestroy);
  sds *basePtr=cvector_begin(toDestroy);
  for(size_t i=0;i!=size;i++)
    {
      sdsfree(basePtr[i]);
    }
  cvector_free(toDestroy);
}
void CMetaRun(CMetaInstance *instance,CMetaBuffer *textStart)
{
  const char *start, *cutout,*YYMARKER=NULL;
  CMetaSegmentType segType;
 loop:
  ;
  /*!stags:re2c format='const char *@@;';*/
  /*!re2c
    re2c:yyfill:enable=0;
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=textStart->bufferPos;
    re2c:define:YYLIMIT=textStart->fileEnd;
    
    StringStart=["'];
    Comment1="//"[^\n]*;
    Comment2="/"[*]([^*]|[*][^/])[*]"/";    
    Whitespace=([ \t\n\r]*|Comment1|Comment2)*;
    InlineSegmentStart="CMETA_INLINE"[0-9a-zA-Z_]*?Whitespace;
    FunctionSegmentStart="CMETA_FUNCTION"[0-9a-zA-Z_]*?Whitespace;
    IncludeSegmentStart="CMETA_INCLUDE"[0-9a-zA-Z_]*?Whitespace;
    DefineCMetaMacro=[#]Whitespace"define" Whitespace "CMETA_"[0-9a-zA-Z_]*;

    * {goto loop;}
    DefineCMetaMacro {goto loop;}

    @cutout InlineSegmentStart "(" @start  {segType=CMETA_SEGMENT_INLINE; goto foundPossibleSeg;} 
    "(" {goto depthInc;}
    @cutout IncludeSegmentStart "(" @start  {segType=CMETA_SEGMENT_INCLUDE; goto foundPossibleSeg;} 
    @cutout FunctionSegmentStart "(" @start  {segType=CMETA_SEGMENT_FUNCTION; goto foundPossibleSeg;}

    
    @start ")" @cutout {goto depthDec;}
    Comment1 {goto loop;}
    Comment2 {goto loop;}    
    [\x00] {goto end;}
    @start StringStart {goto stringStart;}
  */
 foundPossibleSeg:
  //no segments in segment
  if(!instance->inSegment)
    {
      instance->inSegment=true;
      instance->segmentDepth=instance->cbracketDepth++;
      CMetaSegment temp;
      temp.type=segType;
      temp.completed=false;
      temp.positionStart.pos=start-textStart->fileStart;
      temp.positionStart.cutoutPos=cutout-textStart->fileStart;
      cvector_push_back(instance->segments, temp); 
    }
  goto loop;
 depthInc:
  instance->cbracketDepth++;
  goto loop;
 depthDec:
  instance->cbracketDepth--;
  if(instance->cbracketDepth==instance->segmentDepth&&instance->inSegment)
    {
      //found the ending ")" of the segement
      size_t last=cvector_size(instance->segments)-1;
      //
      CMetaSegment *seg=&cvector_begin(instance->segments)[last];
      seg->completed=true;
      seg->positionEnd.pos=start-textStart->fileStart;
      seg->positionEnd.cutoutPos=cutout-textStart->fileStart;
      //
      size_t startPos=seg->positionStart.pos;
      size_t endPos=start-textStart->fileStart;
      seg->code=sdsnewlen(textStart->fileStart+startPos,endPos-startPos);
      //
      instance->inSegment=false;
    }
  goto loop;
 stringStart:
  textStart->bufferPos=start;
  CMetaProccessString(textStart);  
  goto loop;
 end:
  return;
}
sds CMetaTextToTemplate(const sds text, int templateNumber)
{
  const char *oldPtr=text;
  const char *ptr=oldPtr;
  sds retVal=sdscatfmt(sdsnew(""), "  const char *template%i[]={\n", templateNumber);
  while((ptr=strchr(oldPtr, '\n')))
    {
      size_t size=ptr-oldPtr;
      char buffer[size*4+1];
      sds temp=sdsnewlen(oldPtr, size); //FREED
      *unescapeString(temp, buffer)='\0';
      retVal=sdscatfmt(retVal, "    \"%s\", \n", buffer);
      sdsfree(temp);
      oldPtr=ptr+1;
    }
  retVal=sdscat(retVal, "  };\n");
  retVal=sdscatfmt(retVal, "  WriteTemplate(template%i, sizeof(template%i)/sizeof(const char *));\n", templateNumber, templateNumber);
  return retVal;
}
void CMetaWriteOut(const CMetaInstance *instance, const CMetaBuffer *buffer, const char *outputFile, const CMetaCompilerOptions *compilerOptions, const char *testSourceFile)
{
  char inputFileNameBuffer[4*strlen(buffer->fileName)+1];
  *unescapeString((uint8_t*)buffer->fileName, (uint8_t*)inputFileNameBuffer)='\0';
  char outputFileNameBuffer[4*strlen(outputFile)+1];
  *unescapeString((uint8_t*)outputFile, (uint8_t*)outputFileNameBuffer)='\0';
  //
  const char *template[]=
    {
     "#include <stdio.h>",
     "#include <string.h>",
     "%s",
     "const char *INPUT_FILE_NAME=\"%s\";",
     "const char *OUTPUT_FILE_NAME=\"%s\";",
     "#define WRITE_OUT(text) fwrite(text ,1, strlen(text), OUTPUT_FILE_NAME)",
     "FILE *OUTPUT_FILE;",
     "void WriteTemplate(const char *template[], size_t count) {",
     "  for(int i=0;i!=count;i++) {",
     "    fwrite(template[i], 1, strlen(template[i]), OUTPUT_FILE);",
     "    fwrite(\"\\n\", 1, 1, OUTPUT_FILE);",
     "  }",
     "}",
     "%s",
     "int main(int argc, const  char** argv) {",
     "  OUTPUT_FILE=fopen(OUTPUT_FILE_NAME, \"w\");",
     "%s",
     "  fclose(OUTPUT_FILE);",
     "  return 0;",
     "}"
    };
  //
  vec_sds functions=NULL;
  vec_sds includes=NULL;
  //
  size_t startPos=0;
  const char *inlineTextPtr=buffer->fileStart;
  size_t segmentCount=cvector_size(instance->segments);
  sds extractedSource=sdsnew("");
  CMetaSegment *basePtr=cvector_begin(instance->segments);
  for(size_t currentSegment=0;currentSegment!=segmentCount;currentSegment++)
    {
      //
      const CMetaSegment *segPtr=&basePtr[currentSegment];
      //
      size_t outsideCodeStart=startPos;
      size_t outsideCodeEnd=segPtr->positionStart.cutoutPos;
      sds outsideCodeSegment=sdsnewlen(buffer->fileStart+outsideCodeStart, outsideCodeEnd-outsideCodeStart); //FREED
      sds codeSegment=CMetaTextToTemplate(outsideCodeSegment,currentSegment); //FREED
      sdsfree(outsideCodeSegment);
      //
      //
      const char *codeSliceStart=segPtr->positionStart.pos+buffer->fileStart;
      size_t codeSliceSize=segPtr->positionEnd.pos-segPtr->positionStart.pos;
      //
      sds inlineCode=sdsnewlen(codeSliceStart, codeSliceSize); //FREED
      codeSegment=sdscatfmt(codeSegment, "\n%s\n", inlineCode); //FREED
      //
      if(segPtr->type==CMETA_SEGMENT_INLINE)
	{
	  extractedSource=sdscatsds(extractedSource, codeSegment);
	  sdsfree(codeSegment);
	  sdsfree(inlineCode);
	}
      else if(segPtr->type==CMETA_SEGMENT_FUNCTION)
	{
	  cvector_push_back(functions, inlineCode);
	}
      else if(segPtr->type==CMETA_SEGMENT_INCLUDE)
	{
	  cvector_push_back(includes, inlineCode);
	}
      else
	{
	  printf("Unkown segment type.");
	  assert(false);
	}
      //
      startPos=segPtr->positionEnd.cutoutPos;
    }
  sds lastOutsideCodeSegment=sdsnew(buffer->fileStart+startPos); //FREED
  sds templated=CMetaTextToTemplate(lastOutsideCodeSegment, segmentCount); //FREED
  extractedSource=sdscatsds(extractedSource, templated);
  sdsfree(lastOutsideCodeSegment);
  sdsfree(templated);
  
  //COMPILE
  sds compileCommands=sdsnew("cd /tmp/ &&"); //FREED
  sds options=sdsnew("");
  if(cvector_size(compilerOptions->options)!=0)
    {
      size_t count=cvector_size(compilerOptions->options);
      const char** basePtr=cvector_begin(compilerOptions->options);
      for(size_t index=0;index!=count;index++)
	{
	  options=sdscatfmt(options, "%s ", basePtr[index]);
	}
    }
  char *outName=tempnam(NULL, "c");
  sds absolutePath=CMetaMakeFileNameAboslute(sdsnew(testSourceFile));
  compileCommands=sdscatfmt(compileCommands, "%s -o %s %s %s && %s ", compilerOptions->compiler, outName, options, absolutePath, outName);
  free(outName);
  //
  sds filledOutTemplate=sdsnew(""); //FREED
  size_t templateLines=sizeof(template)/sizeof(const char *);
  vec_sds items=NULL;
  cvector_push_back(items, extractedSource); //extractedSource freed in loop
  sds functionsStr=CMetaCombineFormat(functions, "%s\n"); //FREED in loop
  cvector_push_back(items, functionsStr);
  cvector_push_back(items, sdsnew(outputFileNameBuffer)); //FREED in loop
  cvector_push_back(items, CMetaMakeFileNameAboslute(sdsnew(inputFileNameBuffer))); //FREED in loop
  sds includesStr=CMetaCombineFormat(includes, "%s\n"); //FREED in loop
  cvector_push_back(items, includesStr);
  for(int i=0;i!=templateLines;i++)
    {
      const char* line=template[i];
      if(strstr(line, "%s")!=NULL)
	{
	  sds item=*(cvector_end(items)-1); //FREED
	  filledOutTemplate=sdscatfmt(filledOutTemplate, line, item);
	  sdsfree(item); //FREED
	  cvector_pop_back(items);
	}
      else
	{
	  filledOutTemplate=sdscat(filledOutTemplate, line);
	}
      filledOutTemplate=sdscat(filledOutTemplate, "\n");
    }
  //
  //write to file
  
  FILE *outputSource=fopen(absolutePath, "w");
  if(outputSource!=NULL)
    {
      fwrite(filledOutTemplate, 1, sdslen(filledOutTemplate), outputSource);
      fclose(outputSource);
    }
  else
    {
      printf("Coudlnt open output file \"%f\" for writing.", absolutePath);
    }
  //compile
  system(compileCommands);
  //
  if(compilerOptions->deleteSourceAfterUse)
    remove(outputFile);
  sdsfree(absolutePath);
  sdsfree(filledOutTemplate);
  sdsfree(options);
  sdsfree(compileCommands);
  
  vec_sdsDestroy(includes);
  vec_sdsDestroy(functions);
}
void CMetaInstanceDestroy(CMetaInstance *instance)
{
  size_t size=cvector_size(instance->segments);
  CMetaSegment *basePtr=cvector_begin(instance->segments);
  while(--size)
    {
      sdsfree(basePtr[size].code);
    }
  cvector_free(instance->segments);
}
