#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlifgdfgfb.h>
typedef struct
{
  size_t capacity;
  size_t size;
  char *text;
} CMetaMark;
#define MARK_INCREMENT_SIZE 128
const char *OUTPUT_FILE_NAME="%s";
FILE *OUTPUT_FILE;
void CMetaWriteToMark(const char *text,CMetaMark *mark)
{
  size_t len=strlen(text);
  if(len+mark->size>mark->capacity)
    {
      size_t newCapacity=len+mark->size+MARK_INCREMENT_SIZE;
      mark->text=realloc(mark->text, 1+newCapacity); //+1 for string NULL charactor
      mark->capacity=newCapacity;
    }
  memcpy(mark->text+mark->size, text, len);
  mark->text[len+mark->size]='\0';
  mark->size+=len;
}
void CMetaWriteTemplate(const char *template[], size_t dim)
{
  for(int i=0;i!=dim;i++)
    {
      fwrite(template[i], 1, strlen(template[i]), OUTPUT_FILE);
      fwrite("\n", 1, 1, OUTPUT_FILE);
    }
}
void CMetaWriteMarkToFile(CMetaMark *mark)
{
  fwrite(mark->text, 1, mark->size, OUTPUT_FILE);
}
CMetaMark __IMPLICIT_SCOPE_206;


int main(int argc,char **argv)
{
  #define CURRENT_MARK __IMPLICIT_SCOPE_206
{
	    //include string.h with -istring.h
	    const char *hw="\"Hello World\"";
	    fwrite(hw , 1, strlen(hw), OUTPUT_FILE);		
	}

    const char *template0[]={
    "//CMeta test.cmeta.c \055istring.h", 
    "#include <stdio.h>", 
    "#define CMETA_INCLUDE(...) ", 
  };
  WriteTemplate(template0, sizeof(template0)/sizeof(const char *));
  const char *template1[]={
    "", 
    "#define CMETA_INLINE(...) \"placeholder\"", 
    "int main() {", 
    "    printf(", 
  };
  WriteTemplate(template1, sizeof(template1)/sizeof(const char *));
WriteMarkToFile(&__IMPLICIT_SCOPE_206);
  const char *template2[]={
    "", 
    "    );", 
    "    return 0;", 
    "}", 
  };
  WriteTemplate(template2, sizeof(template2)/sizeof(const char *));

  return EXIT_SUCCESS