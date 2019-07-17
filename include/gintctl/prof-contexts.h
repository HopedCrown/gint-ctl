//---
//	gintctl:prof-contexts - All contexts under which profiling is used
//---

#ifndef GINTCTL_PROF_CONTEXTS
#define GINTCTL_PROF_CONTEXTS

/* libprof requires its user to declare the number of execution contexts that
   are being profiled. The number and IDs is maintained here */

enum {
	PROFCTX_BASICS = 0,

	PROFCTX_DCLEAR,
	PROFCTX_DUPDATE,
	PROFCTX_DRECT1,
	PROFCTX_DRECT2,
	PROFCTX_DRECT3,

	/* Last element and bound checker */
	PROFCTX_COUNT,
};

#endif /* GINTCTL_PROF_CONTEXTS */
