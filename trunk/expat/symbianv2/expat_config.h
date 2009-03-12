#define HAVE_MEMMOVE 1
#ifdef BUILDING_DLL
#define XMLPARSEAPI(t) EXPORT_C t 
#else
#define XMLPARSEAPI(t) IMPORT_C t 
#endif
#include <string.h>
