#ifndef __JAIKUDEVHELPER_PAN__
#define __JAIKUDEVHELPER_PAN__

/** JaikuDevHelper application panic codes */
enum TJaikuDevHelperPanics 
  {
    EJaikuDevHelperUnknownCommand = 1
    // add further panics here
  };

inline void Panic(TJaikuDevHelperPanics aReason)
{	
  _LIT(KApplicationName,"JaikuDevHelper"); // FIXME, this shouldn't be inlined! 
  User::Panic(KApplicationName, aReason);
}

#endif // __JAIKUDEVHELPER_PAN__
