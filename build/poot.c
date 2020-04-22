#include <stdio.h>
#include <string.h>
const char *INPUT_FILE_NAME="/home/clayton/projects/cMeta/build/../test.cmeta.c";
const char *OUTPUT_FILE_NAME="/home/clayton/projects/cMeta/build/../test.out.cmeta.c";
FILE *OUTPUT_FILE;
void WriteTemplate(const char *template[], size_t count) {
  for(int i=0;i!=count;i++) {
    fwrite(template[i], 1, strlen(template[i]), OUTPUT_FILE);
    fwrite("\n", 1, 1, OUTPUT_FILE);
  }
}
int main(int argc, const  char** argv) {
  OUTPUT_FILE=fopen(OUTPUT_FILE_NAME, "w");
  const char *template0[]={
    "//CMeta test.cmeta.c \055istring.h", 
    "#include <stdio.h>", 
    "#define CMETA_SEGMENT_START(...) \"placeholder\"", 
    "int main() {", 
    "    printf(", 
  };
  WriteTemplate(template0, sizeof(template0)/sizeof(const char *));

{
	    //include string.h with -istring.h
	    const char *hw="Hello World";
	    fwrite(hw , 1, strlen(hw), OUTPUT_FILE);		
	}
  const char *template1[]={
    "", 
    "    );", 
    "    return 0;", 
    "}", 
  };
  WriteTemplate(template1, sizeof(template1)/sizeof(const char *));

  fclose(OUTPUT_FILE);
  return 0;
}
