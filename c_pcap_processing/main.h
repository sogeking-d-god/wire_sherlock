#ifndef MAIN_H
#define MAIN_H

#define __USE_MISC

#include <stdio.h>

#include "parser.h"
#include "common.h"

typedef enum main_return_codes_e
{
    MAIN_SUCCESS = 0,
    MAIN_FILE_NAME_INPUT_ERROR = -1,
    MAIN_FILE_NAME_LENGTH_ZERO_ERROR = -2
} main_return_codes_e;

#endif