#include "reminderer.h"
#include "reminderer_event.h"
#include <string.h>

void reminderer_event_init(reminderer_event *init)
{
	memset(init, 0, sizeof(reminderer_event));
	init->text = g_strdup("");
}

void reminderer_event_destroy(reminderer_event *re)
{
	g_free(re->text);
}

void reminderer_event_set(reminderer_event *dest, const reminderer_event *src)
{
	g_free(dest->text);
	*dest = *src;
	dest->text = g_strdup(src->text);
}

void reminderer_event_set_text(reminderer_event *re, const char *text)
{
	g_free(re->text);
	re->text = g_strdup(text);
}

