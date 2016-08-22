#ifndef EVENTSTORE_H
#define EVENTSTORE_H

#include "reminderer_event.h"
#include "eventlists.h"

void evs_init(int timeOffset);

/* Fills lists with data from store file. Current contents of lists is removed.
 * The parseErrors parameter is a location for errors buffer, to release by
 * g_free.
 */
void evs_load_file(const gchar *fname, gchar **parseErrors);

evl_event_id evs_event_add(const reminderer_event*);
void evs_event_set(evl_event_id, const reminderer_event*);
void evs_event_del(evl_event_id);
void evs_event_undel(evl_event_id);
void evs_remind_del(evl_remind_id);
void evs_remind_undel(evl_remind_id);
void evs_purge_deleted_events(void);

gboolean evs_remind_is_empty(void);
const GDate *evs_get_cur_date(void);

#endif /* EVENTSTORE_H */
