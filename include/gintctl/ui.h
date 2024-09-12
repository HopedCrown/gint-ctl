//---
// gintctl.ui: gintctl's UI integration system
//
// gintctl runs a user interface based on JustUI. It features a few custom
// widgets, notably gscreen, a standard kind of screen with a title bar, an
// F-key bar and contents in the middle. There are multiple such screens in the
// application, and they stack on top of each other whenever we enter a
// sub-screen.
//
// Because gintctl tries to have mostly indepent modules, but we still need to
// share a unique scene to properly exploit the nice features in JustUI, this
// header here provides the interface for pushing and popping screens.
//
// WARNING: gintctl has the unusual convention that test functions push screens
//          on top of the scene, and it's the main function that pops them.
//---

#ifndef _GINTCTL_UI
#define _GINTCTL_UI

#include <justui/jscene.h>
/* All custom widgets */
#include <gintctl/widgets/gmenu.h>
#include <gintctl/widgets/gscreen.h>
#include <gintctl/widgets/gtable.h>

/* Get a pointer to the global scene. Normally just for `jscene_run()`. */
jscene *gintctl_scene(void);

/* Used only by main function. */
void gintctl_scene_init(void);
void gintctl_scene_deinit(void);

/* Push a screen on the scene stack and focus it. */
gscreen *gintctl_scene_push(gscreen *s);

/* Pop the last screen from the scene stack. Normally used only by main
   function (unless some test function has nested screens). */
void gintctl_scene_pop(void);

#endif /* _GINTCTL_UI */
