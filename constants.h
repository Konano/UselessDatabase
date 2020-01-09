#ifndef __CONSTANTS
#define __CONSTANTS

#include <stdint.h>

#define MAX_BUF_NUM 65536
#define HASH_PRIME 131

#define MAX_FILE_NUM 128
#define MAX_FILE_NUM_INDEX 7

#define PAGE_SIZE 8192
#define PAGE_SIZE_INDEX 13

#define MAX_NAME_LEN 32
#define MAX_COMMENT_LEN 256
#define MAX_TABLE_NUM 256
#define MAX_COL_NUM 32
#define MAX_INDEX_NUM 256

#define MAX_RESULT_LINES 50

typedef uint8_t* BufType;
typedef unsigned int uint;

#endif