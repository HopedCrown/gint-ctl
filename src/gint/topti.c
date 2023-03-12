#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

/* gintctl_gint_topti(): Test text rendering */
void gintctl_gint_topti(void)
{
	int key = 0;

	/* Text position parameters */
	int x = _(1,6), y=_(0,22);
	int dy = _(8,15);

	char const *strings[] = {
		"Text rendering",
		"Eurêka! UTF-8!",
		#ifdef FX9860G
		"∀Δ, ∀f : P(Δ)→Δ,",
		"∃(A,B)⊆Δ, f(A)=f(B)",
		#else
		"∀Δ, ∀f : P(Δ)→Δ, ∃(A,B)⊆Δ, f(A)=f(B)",
		#endif
		"2 = Λα.λf.λn.f (f n)",
		NULL,
	};

	int total = 0;
	while(strings[total]) total++;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		for(int i = 0; strings[i]; i++)
			dtext(x, y + i * dy, C_BLACK, strings[i]);
		dupdate();

		key = getkey().key;
		if(key == KEY_LEFT  && x >= -DWIDTH)  x--;
		if(key == KEY_RIGHT && x <=  DWIDTH)  x++;
		if(key == KEY_UP    && y >= -DHEIGHT) y--;
		if(key == KEY_DOWN  && y <=  DHEIGHT) y++;
	}
}
