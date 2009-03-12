#include <E32BASE.H>
#include <e32def.h>
#include <e32std.h>

#ifndef __S60V3__
GLDEF_C TInt E32Dll(TDllReason aReason)
{
  return(KErrNone);
}
#endif

extern "C" {
void UserCheck() {
	User::Check();
}
}
