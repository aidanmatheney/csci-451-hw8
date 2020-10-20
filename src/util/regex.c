#include "../../include/util/regex.h"

#include "../../include/util/guard.h"
#include "../../include/util/error.h"

#include <stdlib.h>
#include <regex.h>

/**
 * Compile the given regular expression pattern and flags into a regex object using regcomp. If the compilation fails,
 * abort the program with an error message.
 *
 * @param regexPtr The location of the regex.
 * @param pattern The regular expression pattern.
 * @param flags The regular expression flags.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 * the calling function, plus extra information if useful.
 */
void safeRegcomp(regex_t * const regexPtr, char const * const pattern, int const flags, char const * const callerDescription) {
    guardNotNull(regexPtr, "regexPtr", "safeRegcomp");
    guardNotNull(pattern, "pattern", "safeRegcomp");
    guardNotNull(callerDescription, "callerDescription", "safeRegcomp");

    int const regcompErrorCode = regcomp(regexPtr, pattern, flags);
    if (regcompErrorCode != 0) {
        char regcompErrorMessage[100];
        regerror(regcompErrorCode, regexPtr, regcompErrorMessage, 100);

        abortWithErrorFmt(
            "%s: Failed to compile regular expression /%s/%d using regcomp (error code: %d; error message: \"%s\")",
            callerDescription,
            pattern,
            flags,
            regcompErrorCode,
            regcompErrorMessage
        );
    }
}
