#ifndef DIBASSERT_H
# define DIBASSERT_H

#  ifdef SUPPRESS_ASSERT
#    define dibassert(ignored) ((void)0)
#  else /* NOT SUPPRESS_ASSERT ie. actually do assertions */
#    include <stdlib.h> // for abort()
#    include <stdio.h>
#    define dibassert(expr)                                             \
    do { if (!(expr)) { printf("ASSERTION ERROR: \"" #expr "\" is FALSE.\nFILE: %s : %d\n\n", __FILE__, __LINE__); abort(); } } while (0)
# endif

#endif /* DIBASSERT_H */
