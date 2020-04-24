#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/** @@CMETA_INCLUDES@@ **/
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
/** @@CMETA_MARKS@@ **/
/** @@CMETA_FUNCTIONS@@ **/
int main(int argc,char **argv)
{
  /** @@CMETA_INLINE@@ **/
  /** @@CMETA_WRITE_TO_FILE@@ **/
  return EXIT_SUCCESS;
}
