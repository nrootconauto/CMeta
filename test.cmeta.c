//CMeta test.cmeta.c -istring.h
#include <stdio.h>
#define CMETA_SEGMENT_START(...) "placeholder"
int main() {
    printf(
        CMETA_SEGMENT_START({
	    //include string.h with -istring.h
	    const char *hw="\"Hello World\"";
	    fwrite(hw , 1, strlen(hw), OUTPUT_FILE);		
	})
    );
    return 0;
}
