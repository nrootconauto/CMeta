#include "CMeta.h"
#include "string.h"
#include "util.h"
#include "ext/C_Unescaper/escaper.h"
//placeholder
#define CMETA_FILE_BUFFER_SIZE 1024
#define YYMAXFILL 10
/*!max:re2c*/
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
int CMetaIsIncomplete(CMetaBuffer *buffer,CMetaInstance *instance)
{
  size_t len=cvector_size(instance->segments);
  CMetaSegment *basePtr=cvector_begin(instance->segments);
  for(size_t i=0;i!=len;i++)
    if(!basePtr[i].completed)
      {
	sds location=CMetaGetLocationString(buffer, basePtr[i].positionStart.cutoutPos);
	printf("CMeta directive has unmatched parenethisis at %s.", location);
	sdsfree(location);
	return 1;
      }
  return 0;
}
int CMetaRun(CMetaInstance *instance,CMetaBuffer *textStart)
{
  char buffer[YYMAXFILL+1];
  const char *start, *cutout, *YYMARKER=NULL, *currentSegmentBegin;
  const char *scopeNameStart, *scopeNameEnd, *end; 
  CMetaSegmentType segType;
  bool expectingScopeName,scopeNameAsString;
 loop:
  ;
  /*!stags:re2c format='const char *@@;';*/
  /*!re2c
    re2c:yyfill:enable=0;
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=textStart->bufferPos;
    re2c:define:YYLIMIT=textStart->fileEnd;

    String1=["]([\\].|[^"])*["];
    String2=[']([\\].|[^'])*['];
    String=String1|String2;

    Comment1="//"[^\n]*;
    Comment2="/"[*]([^*]|[*][^/])[*]"/";    
    Whitespace=([ \t\n\r]*|Comment1|Comment2)*;
    Identifier=[A-Za-z_][A-Za-z_0-9]*;

    InlineSegmentStart="CMETA_INLINE"[0-9a-zA-Z_]*?Whitespace;
    FunctionSegmentStart="CMETA_FUNCTION"[0-9a-zA-Z_]*?Whitespace;
    IncludeSegmentStart="CMETA_INCLUDE"[0-9a-zA-Z_]*?Whitespace;
    MarkSegmentStart="CMETA_MARK"[0-9a-zA-Z_]?Whitespace;

    DefineCMetaMacro=[#]Whitespace"define" Whitespace "CMETA_"[0-9a-zA-Z_]*;

    * {goto loop;}
    DefineCMetaMacro {goto loop;}

    @cutout InlineSegmentStart "(" @start  {segType=CMETA_SEGMENT_INLINE; goto foundPossibleSeg;} 
    "(" {goto depthInc;}
    @cutout IncludeSegmentStart "(" @start  {segType=CMETA_SEGMENT_INCLUDE; goto foundPossibleSeg;} 
    @cutout FunctionSegmentStart "(" @start  {segType=CMETA_SEGMENT_FUNCTION; goto foundPossibleSeg;}
    @cutout MarkSegmentStart "(" @start {segType=CMETA_SEGMENT_MARK; goto foundPossibleSeg;}
    
    @start  @scopeNameStart String @scopeNameEnd Whitespace [,;]? @end {scopeNameAsString=true; goto foundPossibleScope;}
    @start  @scopeNameStart Identifier @scopeNameEnd Whitespace [,;]? @end {scopeNameAsString=false; goto foundPossibleScope;}
    @start  @scopeNameStart Identifier @scopeNameEnd  @end Whitespace / ")" {scopeNameAsString=true;goto foundPossibleScope;}    

    @start ")" @cutout {goto depthDec;}
    Whitespace {goto loop;}
    [\x00] {goto end;}
    String {goto loop;}
  */
 foundPossibleScope:
   //for an identifer that can represent the scope,cehck if it is at the start of the current segment
   if(expectingScopeName)
     {
       if(currentSegmentBegin==start)
	 {
	   size_t len=scopeNameEnd-scopeNameStart-2;//-2 ignores the " or '
	   char scopeString[len+1];
	   memcpy(scopeString-1, scopeNameStart+1, len); //-1 +1 gets rid of the " or '
	   scopeString[len]='\0';
	   char escaped[4*len+1];
	   *unescapeString(scopeString, escaped)='\0';
	   //last element
	   CMetaSegment *last=cvector_end(instance->segments)-1;
	   last->scope=sdsnew(escaped);
	   //move the start of the segement past the scope identifier
	   last->positionStart.pos=end-textStart->fileStart;
	 }
       expectingScopeName=false;
     }
   goto loop;
 foundPossibleSeg:
   expectingScopeName=true;
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
      temp.scope=NULL;
      cvector_push_back(instance->segments, temp);
      //
      currentSegmentBegin=start;
      expectingScopeName=true;
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
      CMetaSegment *seg=&cvector_begin(instance->segments)[last];
      //ensure mark directives have a scope
      if(seg->type==CMETA_SEGMENT_MARK&&seg->scope==NULL)
	{
	  sds location=CMetaGetLocationString(textStart, seg->positionStart.cutoutPos);
	  printf("MARK directive require a scope. %s", location);
	  sdsfree(location);
	  goto error;
	}
      //give directive a scope if doesnt already have one
      if(seg->scope==NULL)
	{
	  seg->scope=sdscatfmt(sdsnew("__IMPLICIT_SCOPE_"), "%i", seg->positionStart.pos);
	}
      //
      seg->completed=true;
      seg->positionEnd.pos=start-textStart->fileStart;
      seg->positionEnd.cutoutPos=cutout-textStart->fileStart;
      //
      size_t startPos=seg->positionStart.pos;
      size_t endPos=start-textStart->fileStart;
      //
      instance->inSegment=false;
    }
  goto loop;
 stringStart:
  textStart->bufferPos=start;
  CMetaProccessString(textStart);  
  goto loop;
 end:
  if(CMetaIsIncomplete(textStart, instance))
    goto error;
  return 0;
 error:
  return 1;
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
sds CMetaSegmentMarkName(const CMetaSegment *segment)
{
  return sdsnew(segment->scope);
}
sds CMetaWriteMark(const char *markName)
{
  return sdscatfmt(sdsnew(""), "WRITE_OUT(%s)", markName);
}
void CMetaFillOutTemplateFile(const char *templateFile, const char* outputFile, const vec_sds includes, const vec_sds marksDeclarations, const vec_sds functionsCode, const vec_sds inlinesCode, const vec_sds writeCode)
{
  
  char buffer[YYMAXFILL];
  char buffer2[CMETA_FILE_BUFFER_SIZE+1];
  size_t buffer2Size=0;
  const char *limit=buffer;
  char *cursor=buffer+YYMAXFILL, *YYMARKER=NULL;
  //
  FILE *iFile=fopen(templateFile, "r");
  FILE *oFile=fopen(outputFile, "w");
#define WRITEOUT(str) \
  fwrite(buffer2, 1, buffer2Size, oFile); \
  fwrite(str, 1, sdslen(str), oFile);	  \
  buffer2Size=0;
  
#define WRITEOUT_VEC(vec)			\
  {						\
    fwrite(buffer2, 1, buffer2Size, oFile);	\
    buffer2Size=0;				\
    size_t size=cvector_size(vec);		\
    sds *basePtr=cvector_begin(vec);		\
    for(size_t i=0;i!=size;i++)			\
      {						\
	sds str=basePtr[i];			\
	fwrite(str, 1, sdslen(str), oFile);	\
      }						\
  }
#define YYFILL(n) CMetaYYFILL(iFile, buffer, &limit, &cursor, n, YYMAXFILL);
  char ch;
  YYFILL(1);
 loop:
  ch=*cursor;
  /*!re2c
    re2c:yyfill:enable=1;
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=cursor;
    re2c:define:YYLIMIT=limit;

    Includes="@@CMETA_INCLUDES@@";
    Marks="@@CMETA_MARKS@@";
    Functions="@@CMETA_FUNCTIONS@@";
    Inline="@@CMETA_INLINE@@";
    WriteOut="@@CMETA_WRITE_TO_FILE@@";

    * {goto gotChar;}
    [\x00] {goto end;}
    "/"[*]{1,} Whitespace Includes Whitespace [*]{1,}"/" {goto gotIncludes;}
    "/"[*]{1,} Whitespace Marks Whitespace [*]{1,}"/" {goto gotMarks;}
    "/"[*]{1,} Whitespace Functions Whitespace [*]{1,}"/" {goto gotFunctions;}
    "/"[*]{1,} Whitespace Inline Whitespace [*]{1,}"/" {goto gotInline;}
    "/"[*]{1,} Whitespace WriteOut Whitespace [*]{1,}"/" {goto gotWriteOut;}
   */
 gotWriteOut:
  WRITEOUT_VEC(writeCode);
  goto loop;
 gotInline:
  WRITEOUT_VEC(inlinesCode);
  goto loop;
 gotFunctions:
  WRITEOUT_VEC(functionsCode);
  goto loop;
 gotMarks:
  WRITEOUT_VEC(marksDeclarations);
  goto loop;
 gotIncludes:
  WRITEOUT_VEC(includes);
  goto loop;
 gotChar:
  buffer2[buffer2Size++]=ch;
  if(buffer2Size==CMETA_FILE_BUFFER_SIZE)
    {
      fwrite(buffer2, 1, buffer2Size, oFile);
      buffer2Size=0;
    }
  goto loop;
 end:
  fwrite(buffer2, 1, buffer2Size, oFile);
  fclose(iFile);
  fclose(oFile);
}
void CMetaWriteOut(const CMetaInstance *instance, const CMetaBuffer *buffer, const char *outputFile, const CMetaCompilerOptions *compilerOptions, const char *testSourceFile)
{
  vec_sds segmentCodes=NULL; //FREED
  //
  char inputFileNameBuffer[4*strlen(buffer->fileName)+1];
  *unescapeString((uint8_t*)buffer->fileName, (uint8_t*)inputFileNameBuffer)='\0';
  char outputFileNameBuffer[4*strlen(outputFile)+1];
  *unescapeString((uint8_t*)outputFile, (uint8_t*)outputFileNameBuffer)='\0';
  //
  vec_sds markDeclarations=NULL; //FREED
  vec_sds functions=NULL; //FREED
  vec_sds includes=NULL; //FREED
  vec_sds inlines=NULL; //FREED
  //
  size_t startPos=0;
  const char *inlineTextPtr=buffer->fileStart;
  size_t segmentCount=cvector_size(instance->segments);
  CMetaSegment *basePtr=cvector_begin(instance->segments);
  for(size_t currentSegment=0;currentSegment!=segmentCount;currentSegment++)
    {
      //
      const CMetaSegment *segPtr=&basePtr[currentSegment];
      //
      size_t outsideCodeStart=startPos;
      size_t outsideCodeEnd=segPtr->positionStart.cutoutPos;
      sds outsideCodeSegment=sdsnewlen(buffer->fileStart+outsideCodeStart, outsideCodeEnd-outsideCodeStart); //FREED
      sds templateSegment=CMetaTextToTemplate(outsideCodeSegment,currentSegment); //FREED
      cvector_push_back(segmentCodes, templateSegment);
      sdsfree(outsideCodeSegment);
      //
      const char *codeSliceStart=segPtr->positionStart.pos+buffer->fileStart;
      size_t codeSliceSize=segPtr->positionEnd.pos-segPtr->positionStart.pos;
      //
      sds inlineCode=sdsnewlen(codeSliceStart, codeSliceSize); //FREED
      //
      
      if(segPtr->type==CMETA_SEGMENT_INLINE)
	{
	  sds markName=CMetaSegmentMarkName(segPtr); //FREED
	  cvector_push_back(inlines, sdscatfmt(sdsnew("#define CURRENT_MARK "), "%s\n%s\n#undef CURRENT_MARK\n", markName, inlineCode)); //inlineCode FREED in inlines
	  
	  cvector_push_back(markDeclarations, sdscatfmt(sdsnew(""), "CMetaMark %s;\n", markName)); //FREED
	  cvector_push_back(segmentCodes, sdscatfmt(sdsnew(""), "WriteMarkToFile(&%s);\n", markName)); //FREED
	  sdsfree(markName);
	}
      else if(segPtr->type==CMETA_SEGMENT_FUNCTION)
	{
	  cvector_push_back(functions, inlineCode);
	}
      else if(segPtr->type==CMETA_SEGMENT_INCLUDE)
	{
	  cvector_push_back(includes, inlineCode);
	}
      else if(segPtr->type==CMETA_SEGMENT_MARK)
	{
	  sds markName=CMetaSegmentMarkName(segPtr); //FREED
	  cvector_push_back(markDeclarations, sdscatfmt(sdsnew(""), "CMetaMark %s;\n", markName)); //FREED
	  cvector_push_back(segmentCodes, sdscatfmt(sdsnew(""), "WriteMarkToFile(&%s);\n", markName)); //FREED
	  sdsfree(markName);
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
  cvector_push_back(segmentCodes, templated); //templated is FREED in segment codes
  sdsfree(lastOutsideCodeSegment);
  //TURN marks into string
  sds marksString;
  //COMPILE COMMANDS
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
  char *outName=tempnam(NULL, "cmeta");
  sds absolutePath=CMetaMakeFileNameAboslute(sdsnew(testSourceFile));
  compileCommands=sdscatfmt(compileCommands, "%s -o %s %s %s && %s ", compilerOptions->compiler, outName, options, absolutePath, outName);
  
  //
  CMetaFillOutTemplateFile(CMETA_TEMPLATE_FILE, absolutePath, includes, markDeclarations, functions, inlines, segmentCodes);
  free(outName);
  //compile
  system(compileCommands);
  //
  if(compilerOptions->deleteSourceAfterUse)
    remove(absolutePath);
  sdsfree(absolutePath);
  sdsfree(options);
  sdsfree(compileCommands);
  vec_sdsDestroy(markDeclarations);
  vec_sdsDestroy(segmentCodes);
  vec_sdsDestroy(includes);
  vec_sdsDestroy(inlines);
  vec_sdsDestroy(functions);
}
void CMetaInstanceDestroy(CMetaInstance *instance)
{
  size_t size=cvector_size(instance->segments);
  CMetaSegment *basePtr=cvector_begin(instance->segments);
  while(--size)
    {
      if(basePtr[size].scope!=NULL)
	sdsfree(basePtr[size].scope);
    }
  cvector_free(instance->segments);
}
