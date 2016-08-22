#ifndef REMINDERER_EVENT
#define REMINDERER_EVENT

enum RemindererEventType {
    RET_BYDATE,
    RET_PERIODIC,
    RET_TODO
};

/* Reminderer's event or todo
 */
typedef struct reminderer_event {
    enum RemindererEventType evType;

	guint evByDateYear;     /* the event year; 0 if no year */
    guint evByDateWeekdays; /* bit-or of recurring event weekdays:
                             * 1<<0 for Monday, 1<<1 for Tuesday,
                             * 1<<6 for Sunday */
    guint evByDateMonths;   /* bit-or of recurring event months:
                               1<<(M-1) for month M (1=January) */
    guint evByDateDays;     /* bit-or of recurring event days of month:
                               1<<(N-1) for day N */

    gint evPeriodicDays;    /* periodic event interval in days */
    gint evPeriodicOffset;  /* periodic event offset: a number between
                             * 0 and evPeriodicDays-1; meaning is as follows:
                             * event days are extrapolated back, till
                             * 1-Jan-2014; number of days since 1-Jan-2014
                             * of the first event day is the offset */

	gchar *text;			/* text to remind */
	gint remind_days;		/* number of days before event
                             * which the remind should appear */
	gboolean keep;			/* event: should remind be kept after event date?
	 						 * todo: should appear also on remind list? */
    gboolean evDeleted;     /* whether the event is deleted */
} reminderer_event;

typedef struct reminderer_remind {
	gint year;					/* event date */
	gint month;
	gint day;
    gboolean rrDeleted;           /* whether the remind is deleted */
} reminderer_remind;

void reminderer_event_init(reminderer_event *init);
void reminderer_event_destroy(reminderer_event*);
void reminderer_event_set(reminderer_event *dest, const reminderer_event *src);
void reminderer_event_set_text(reminderer_event*, const char *text);


#endif /* REMINDERER_EVENT */
