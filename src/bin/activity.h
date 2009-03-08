#ifndef _ENNA_ACTIVITY_H_
#define _ENNA_ACTIVITY_H_

typedef enum _ENNA_CLASS_TYPE ENNA_CLASS_TYPE;

typedef struct _Enna_Class_Activity Enna_Class_Activity;

struct _Enna_Class_Activity
{
    const char *name;
    int pri;
    const char *label;
    const char *icon_file;
    const char *icon;
    struct
    {
        void (*class_init)(int dummy);
        void (*class_shutdown)(int dummy);
        void (*class_show)(int dummy);
        void (*class_hide)(int dummy);
        void (*class_event)(void *event_info);
    } func;
    Eina_List *categories;
};

int enna_activity_add(Enna_Class_Activity *class);
int enna_activity_del(const char *name);
void enna_activity_del_all (void);
Eina_List *enna_activities_get(void);
int enna_activity_init(const char *name);
int enna_activity_show(const char *name);
int enna_activity_shutdown(const char *name);
int enna_activity_hide(const char *name);
int enna_activity_event(Enna_Class_Activity *act, void *event_info);

#endif
