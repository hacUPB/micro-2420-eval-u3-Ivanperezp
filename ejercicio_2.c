#include <stdio.h>

#define TOGGLE (regt, bit) ((regt) ^= (1<<bit))  
#define is_present (PCC) (PCC>>31)