#include "glibconfig.h"
#ifdef NOUNIX
int pipe(int filedes[2])
{
}
#endif
