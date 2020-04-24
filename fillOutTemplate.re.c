#include "util.h"
#include "CMeta.h"
#include "lexer.h"
#define CMETA_FILE_BUFFER_SIZE 1024
#define YYMAXFILL 10
/*!max:re2c*/

void CMetaFillOutTemplateFile(const char *templateFile, const char* outputFile, const vec_sds includes, const vec_sds marksDeclarations, const vec_sds functionsCode, const vec_sds inlinesCode, const vec_sds writeCode)
{
  const size_t bufferSize=YYMAXFILL;
  char buffer[bufferSize];
  char buffer2[CMETA_FILE_BUFFER_SIZE+1];
  size_t buffer2Size=0;
  char *limit=buffer;
  char *cursor=buffer, *YYMARKER=NULL;
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
#define YYFILL(n) CMetaYYFILL(iFile, buffer, &limit, &cursor, n, bufferSize);
  char ch;
  YYFILL(1);
 loop:
  ch=*cursor;
  /*!re2c
    re2c:yyfill:enable=1;
    re2c:define:YYCTYPE=char;
    re2c:define:YYCURSOR=cursor;
    re2c:define:YYLIMIT=limit;

    Whitespace=[ \t\n\r]*;

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
