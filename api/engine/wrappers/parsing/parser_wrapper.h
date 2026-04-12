#ifndef PARSER_WRAPPER_H
#define PARSER_WRAPPER_H

#include <cjson/cJSON.h>
#include "parser.h"
#include "ip_tree.h"


cJSON* wrap_parser_results(file_analysis_context_t *core);

#endif