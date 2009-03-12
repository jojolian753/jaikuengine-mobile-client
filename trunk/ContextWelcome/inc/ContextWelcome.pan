#ifndef __CONTEXTWELCOME_PAN__
#define __CONTEXTWELCOME_PAN__

/** ContextWelcome application panic codes */
enum TContextWelcomePanics 
  {
    EContextWelcomeUnknownCommand = 1
    // add further panics here
  };

inline void Panic(TContextWelcomePanics aReason)
{	
  _LIT(KApplicationName,"ContextWelcome"); // FIXME, this shouldn't be inlined! 
  User::Panic(KApplicationName, aReason);
}

#endif // __CONTEXTWELCOME_PAN__
