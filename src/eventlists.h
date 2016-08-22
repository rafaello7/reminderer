#ifndef EVENTLISTS_H
#define EVENTLISTS_H

#include "reminderer_event.h"

/** NOTE that the functions below don't touch underlyng store file. Up to
 * update lists and store changes in file, use functions in eventstore.h
 */


struct evl_event;
struct evl_remind;

/* event item id (handler) */
typedef struct evl_event *evl_event_id;

/* remind item id (handler) */
typedef struct evl_remind *evl_remind_id;


/* columns of event list */
enum evl_evcolumns {
	EVL_EVCOL_COLOR,
	EVL_EVCOL_TYPE,
	EVL_EVCOL_DATE,
	EVL_EVCOL_TEXT,
    EVL_EVCOL_DELETED,
	EVL_EVCOL_PTR,
	EVL_EVCOL_COUNT
};

/* columns of remind list */
enum evl_rmdcolumns {
	EVL_RMDCOL_COLOR,
	EVL_RMDCOL_DAYS,
	EVL_RMDCOL_DATE,
	EVL_RMDCOL_TEXT,
    EVL_RMDCOL_DELETED,
	EVL_RMDCOL_PTR,
	EVL_RMDCOL_COUNT
};

/* columns of todo list */
enum evl_todocolumns {
    EVL_TODOCOL_COLOR,
	EVL_TODOCOL_MARK,
	EVL_TODOCOL_TEXT,
    EVL_TODOCOL_DELETED,
	EVL_TODOCOL_PTR,
	EVL_TODOCOL_COUNT
};

/* which list the reminderer event is on
 */
enum evl_iterlist {
    EVL_EVENTS,
    EVL_TODOS
};

void evl_init(void);
gboolean evl_remind_is_empty(void);

/* gets reminder list */
GtkListStore *evl_reminds_get(void);

/* gets preview list */
GtkListStore *evl_preview_get(void);

/* gets event list */
GtkListStore *evl_events_get(void);

/* gets todo list */
GtkListStore *evl_todos_get(void);

/* creates new event with the specified event data and stores it on event list.
 * Note that reminds are NOT derived.
 */
evl_event_id evl_event_new(const reminderer_event*);
const reminderer_event *evl_get_re(evl_event_id);

/* Updates event with the new data
 */
void evl_set_re(evl_event_id, const reminderer_event*);
void evl_event_destroy(evl_event_id);

evl_remind_id evl_remind_new(evl_event_id, const reminderer_remind*);
gint evl_remind_count(evl_event_id);
evl_remind_id evl_remind_get_at(evl_event_id, gint);
evl_event_id evl_get_eid(evl_remind_id);
const reminderer_remind *evl_get_rr(evl_remind_id);
void evl_set_rr(evl_remind_id, const reminderer_remind*);

/* Updates reminds derived from the event.
 */
void evl_derive_reminds(evl_event_id, const GDate *dateDeriveFrom);

void evl_events_foreach( void(*)(evl_event_id, gpointer), gpointer);

/* Invokes foreachFun for each event. When it returns TRUE, the event
 * is removed. When the foreachFun is NULL, all events are removed.
 */
void evl_events_foreach_remove( gboolean(*foreachFun)(evl_event_id, gpointer),
        gpointer);

evl_event_id   evl_eidFromList( enum evl_iterlist, GtkTreeIter*);
evl_event_id   evl_eidFromPath( enum evl_iterlist, GtkTreePath*);
evl_event_id   evl_eidFromRemindPath( GtkTreePath *rmdListPath);
evl_event_id   evl_eidFromPreviewPath( GtkTreePath *rmdListPath);
evl_event_id   evl_eidFromRemindList( GtkTreeIter *rmdListIter);
evl_event_id   evl_eidFromPreviewList( GtkTreeIter *pvwListIter);
evl_remind_id  evl_ridFromRemindList( GtkTreeIter*);
evl_remind_id  evl_ridFromPreviewList( GtkTreeIter*);
void evl_select_in_treeview(evl_event_id, GtkTreeView*);

gboolean evl_showdeleted_get(void);
void evl_showdeleted_set(gboolean);
void evl_changed_disp_preferences(void);

#endif /* EVENTLISTS_H */
