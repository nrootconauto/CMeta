# CMeta, a metaprogramming preprocessor for C.
## Basics
This runs c code on c code. To use it,use any macro-like invocation that starts with  `CMETA_SEGMENT_START` and insert your C code inside. The C code should write to `OUTPUT_FILE`  to create source code.For example:
```
CMETA_SEGMENT_START({
	const char *hw="Hello World";
	fwrite(hw , 1, strlen(hw), OUTPUT_FILE);
})
```
###  Mock values
Any macro like invocation that starts with`CMETA_SEGMENT_START` will invoke the inline C code. This can be used for making "mock" macros that can represent the type or value that will be generated. This is useful for integrating with code auto-completion engines. For Example:
```
#define CMETA_SEGMENT_START_MockInt(...) 10 
int x=CMETA_SEGMENT_START_MockInt({
	const char *toInsert="255";
	fwrite(toInsert, 1, strlen(toInsert), OUTPUT_FILE);
})
//x will be 255,not 10. The 10 is a placeholder value 
```
## Usage

