//CMeta test.cmeta.c -istring.h
#include <stdio.h>
#define CMETA_INCLUDE(...) 
CMETA_INCLUDE(#include <stdlib.h>)
#define CMETA_INLINE(...) "placeholder"
int main() {
    printf(
        CMETA_INLINE({
	    //include string.h with -istring.h
	    const char *hw="\"Hello World\"";
	    fwrite(hw , 1, strlen(hw), OUTPUT_FILE);		
	})
    );
    return 0;
}
