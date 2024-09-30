//---
// gintctl:config - Static configuration
//---

/* This is not intended to be configurable from the build system, rather that's
   for hacking around issues of binary size, features, etc. */

// When 0, the USB driver should not be linked in.
#define GINTCTL_ENABLE_USB 0
