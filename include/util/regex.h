#pragma once

#include <regex.h>

void safeRegcomp(regex_t *regexPtr, char const *pattern, int flags, char const *callerDescription);
