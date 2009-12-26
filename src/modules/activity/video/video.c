/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * FIXME : Remove unused object and fix navigation : it's actually not possible
 * to return from video playback !
 * Fix state machine
 * Enable Position set
 * EOS is not used !!!!
 */

#include <Edje.h>
#include <Elementary.h>
#include <Ecore_Input.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "activity.h"
#include "content.h"
#include "image.h"
#include "mainmenu.h"
#include "logs.h"
#include "vfs.h"
#include "view_list.h"
#include "browser.h"
#include "mediaplayer.h"
#include "volumes.h"
#include "buffer.h"
#include "metadata.h"
#include "utils.h"
#include "mediaplayer_obj.h"

#include "video.h"
#include "video_flags.h"
#include "video_infos.h"
#include "video_resume.h"
#include "video_picture.h"

#define ENNA_MODULE_NAME "video"

#define TIMER_DELAY 10.0

static void browser_cb_root(void *data, Evas_Object *obj, void *event_info);
static void browser_cb_select(void *data, Evas_Object *obj, void *event_info);
static void browser_cb_hilight(void *data, Evas_Object *obj, void *event_info);
static void browser_cb_delay_hilight(void *data,
                                     Evas_Object *obj, void *event_info);
static void browse(void *data);

static void _create_menu(void);
static void _return_to_video_info_gui();

static int _eos_cb(void *data, int type, void *event);
static void video_infos_del(void);

typedef struct _Enna_Module_Video Enna_Module_Video;
typedef enum _VIDEO_STATE VIDEO_STATE;

enum _VIDEO_STATE
{
    MENU_VIEW,
    BROWSER_VIEW,
    VIDEOPLAYER_VIEW
};

struct _Enna_Module_Video
{
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_list;
    Evas_Object *o_browser;
    Evas_Object *o_backdrop;
    Evas_Object *o_snapshot;
    Evas_Object *o_panel_infos;
    Evas_Object *o_resume;
    Evas_Object *o_video_flags;
    Evas_Object *o_mediaplayer;
    Evas_Object *o_mediacontrols;
    Enna_Module *em;
    VIDEO_STATE state;
    Ecore_Event_Handler *eos_event_handler;
    Enna_Playlist *enna_playlist;
    char *o_current_uri;
    int infos_displayed;
    int resume_displayed;
    int controls_displayed;
    Enna_Volumes_Listener *vl;
    Ecore_Timer *controls_timer;
    Ecore_Event_Handler *mouse_button_event_handler;
    Ecore_Event_Handler *mouse_move_event_handler;
};

static Enna_Module_Video *mod;

static void
update_movies_counter(Eina_List *list)
{
    Enna_Vfs_File *f;
    Eina_List *l;
    int children = 0;
    char label[128] = { 0 };

    if (!list)
        goto end;

    EINA_LIST_FOREACH(list, l, f)
    {
        if (!f->is_directory && !f->is_menu)
            children++;
    }
    if (children)
        snprintf(label, sizeof(label), _("%d Movies"), children);
end:
    edje_object_part_text_set(mod->o_edje, "movies.counter.label", label);
}

static int
_controls_timer_cb(void *data)
{
    media_controls_display(0);
    mod->controls_timer = NULL;
    return 0;
}

static void
video_resize(void)
{
    Evas_Coord w, h, x, y;
    Evas_Coord h2 = 0;

    if (mod->controls_displayed)
        evas_object_geometry_get(mod->o_mediacontrols, NULL, NULL, NULL, &h2);

    evas_object_geometry_get(mod->o_mediaplayer, &x, &y, &w, &h);
    enna_mediaplayer_video_resize(x, y, w, h - h2);
}

void
media_controls_display(int show)
{
    if (show)
    {
        ENNA_TIMER_DEL(mod->controls_timer);
        mod->controls_timer =
            ecore_timer_add(TIMER_DELAY, _controls_timer_cb, NULL);
    }

    if (show == mod->controls_displayed)
        return;

    mod->controls_displayed = show;
    video_resize();
}

static void
menu_view_event(enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_EXIT:
        enna_content_hide();
        enna_mainmenu_show();
        break;
    case ENNA_INPUT_OK:
        browse (enna_list_selected_data_get(mod->o_list));
        break;
    default:
        enna_list_input_feed(mod->o_list, event);
        break;
    }
}

static void
_seek_video(double value)
{
    int pos = 0;
    double seek = 0.0;

    pos = enna_mediaplayer_position_percent_get();
    seek = ((double) pos + value) / 100.0;
    enna_mediaplayer_seek(seek);
    enna_mediaplayer_position_update(mod->o_mediacontrols);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Seek value : %f", seek);
}

static void
videoplayer_view_event_no_display (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_OK:
        media_controls_display(!mod->controls_displayed);
        break;
    case ENNA_INPUT_SPACE:
        enna_mediaplayer_play(mod->enna_playlist);
        break;
    case ENNA_INPUT_RIGHT:
        _seek_video(+1);
        break;
    case ENNA_INPUT_LEFT:
        _seek_video(-1);
        break;
    case ENNA_INPUT_UP:
        _seek_video(+5);
        break;
    case ENNA_INPUT_DOWN:
        _seek_video(-5);
        break;
    case ENNA_INPUT_PLUS:
        enna_mediaplayer_default_increase_volume();
        break;
    case ENNA_INPUT_MINUS:
        enna_mediaplayer_default_decrease_volume();
        break;
    case ENNA_INPUT_KEY_M:
        enna_mediaplayer_mute();
        break;
    case ENNA_INPUT_KEY_K:
        enna_mediaplayer_audio_previous();
        break;
    case ENNA_INPUT_KEY_L:
        enna_mediaplayer_audio_next();
        break;
    case ENNA_INPUT_KEY_P:
        enna_mediaplayer_audio_increase_delay();
        break;
    case ENNA_INPUT_KEY_O:
        enna_mediaplayer_audio_decrease_delay();
        break;
    case ENNA_INPUT_KEY_S:
        enna_mediaplayer_subtitle_set_visibility();
        break;
    case ENNA_INPUT_KEY_G:
        enna_mediaplayer_subtitle_previous();
        break;
    case ENNA_INPUT_KEY_Y:
        enna_mediaplayer_subtitle_next();
        break;
    case ENNA_INPUT_KEY_A:
        enna_mediaplayer_subtitle_set_alignment();
        break;
    case ENNA_INPUT_KEY_T:
        enna_mediaplayer_subtitle_increase_position();
        break;
    case ENNA_INPUT_KEY_R:
        enna_mediaplayer_subtitle_decrease_position();
        break;
    case ENNA_INPUT_KEY_J:
        enna_mediaplayer_subtitle_increase_scale();
        break;
    case ENNA_INPUT_KEY_I:
        enna_mediaplayer_subtitle_decrease_scale();
        break;
    case ENNA_INPUT_KEY_X:
        enna_mediaplayer_subtitle_increase_delay();
        break;
    case ENNA_INPUT_KEY_Z:
        enna_mediaplayer_subtitle_decrease_delay();
        break;
    case ENNA_INPUT_KEY_W:
        enna_mediaplayer_set_framedrop();
        break;
    default:
        break;
    }
}

static void
videoplayer_view_event(enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_QUIT:
    case ENNA_INPUT_EXIT:
        _return_to_video_info_gui();
        break;
    default:
        videoplayer_view_event_no_display(event);
        if (mod->controls_displayed)
            media_controls_display(1);
        break;
    }
}

static void
popup_resume_display(int show)
{
    if (show)
    {
        edje_object_signal_emit(mod->o_edje, "resume,show", "enna");
        mod->resume_displayed = 1;
    }
    else
    {
        edje_object_signal_emit(mod->o_edje, "resume,hide", "enna");
        mod->resume_displayed = 0;
    }
}

static void
_return_to_video_info_gui()
{
    Enna_Metadata *m;
    double pos;

    media_controls_display(0);
    ENNA_TIMER_DEL(mod->controls_timer);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_mediacontrols);
    popup_resume_display (0);
    m = enna_mediaplayer_metadata_get(mod->enna_playlist);
    pos = enna_mediaplayer_position_get();
    enna_metadata_set_position(m, pos);
    enna_mediaplayer_stop();
    mod->state = BROWSER_VIEW;
}

static int
_eos_cb(void *data, int type, void *event)
{
    _return_to_video_info_gui();
    return 1;
}

/****************************************************************************/
/*                               Backdrop                                   */
/****************************************************************************/

static void
backdrop_show(Enna_Metadata *m)
{
    char *file = NULL;
    int from_vfs = 1;
    char *backdrop;

    backdrop = enna_metadata_meta_get(m, "fanart", 1);
    if (backdrop)
    {
        char dst[1024] = { 0 };

        if (*backdrop == '/')
            snprintf(dst, sizeof (dst), "%s", backdrop);
        else
            snprintf(dst, sizeof (dst), "%s/.enna/fanarts/%s",
                     enna_util_user_home_get (), backdrop);
        file = strdup(dst);

        enna_video_picture_set(mod->o_backdrop, file, from_vfs);
        evas_object_show(mod->o_backdrop);
        edje_object_part_swallow(mod->o_edje,
                                 "backdrop.swallow", mod->o_backdrop);
        ENNA_FREE(backdrop);
        ENNA_FREE(file);
    }
    else
        enna_video_picture_unset(mod->o_backdrop);
}

/****************************************************************************/
/*                               Snapshot                                   */
/****************************************************************************/

static void
snapshot_show(Enna_Metadata *m, int dir)
{
    char *file = NULL;
    int from_vfs = 1;
    char *snapshot;

    enna_video_picture_unset(mod->o_snapshot);
    snapshot = enna_metadata_meta_get(m, "fanart", 1);
    if (snapshot)
    {
        char dst[1024] = { 0 };

        if (*snapshot == '/')
            snprintf(dst, sizeof(dst), "%s", snapshot);
        else
            snprintf(dst, sizeof(dst), "%s/.enna/fanarts/%s",
                     enna_util_user_home_get(), snapshot);
        file = strdup(dst);
    }
    else
    {
        file = strdup(dir ? "cover/video/dir" : "cover/video/file");
        from_vfs = 0;
    }

    enna_video_picture_set(mod->o_snapshot, file, from_vfs);
    evas_object_show(mod->o_snapshot);
    edje_object_part_swallow(mod->o_edje,
                             "snapshot.swallow", mod->o_snapshot);

    ENNA_FREE(snapshot);
    ENNA_FREE(file);
}

static void
panel_infos_display(int show)
{
    if (show)
    {
        edje_object_signal_emit(mod->o_edje, "infos,show", "enna");
        mod->infos_displayed = 1;
    }
    else
    {
        edje_object_signal_emit(mod->o_edje, "infos,hide", "enna");
        mod->infos_displayed = 0;
    }
}

/****************************************************************************/
/*                                Browser                                   */
/****************************************************************************/

static void
browser_view_event(enna_input event)
{
    /* handle resume popup, if any */
    if (mod->resume_displayed)
    {
        if (event == ENNA_INPUT_EXIT)
            popup_resume_display(0);
        else
            video_resume_input_feed(mod->o_resume, event);
        return;
    }

    if (event == ENNA_INPUT_KEY_I)
    {
        panel_infos_display(!mod->infos_displayed);
        return;
    }

    if (event == ENNA_INPUT_EXIT)
    {
        video_infos_del();
        update_movies_counter(NULL);
    }
    enna_browser_input_feed(mod->o_browser, event);
}

static void
browser_cb_root(void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del(mod->o_browser, "root", browser_cb_root);
    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", browser_cb_select);
    evas_object_smart_callback_del(mod->o_browser,
                                   "hilight", browser_cb_hilight);
    evas_object_smart_callback_del(mod->o_browser,
                                   "delay,hilight", browser_cb_delay_hilight);

    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_infos);
    ENNA_OBJECT_DEL(mod->o_resume);

    _create_menu();
}

static void
_mediaplayer_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    video_resize();
}

static void
_mediaplayer_mouse_move_cb(void *data,
                           Evas *e, Evas_Object *obj, void *event_info)
{
    media_controls_display(1);
}

static void
_mediaplayer_mouse_up_cb(void *data,
                         Evas *e, Evas_Object *obj, void *event_info)
{
    media_controls_display(1);
}

static int
_mediaplayer_mouse_move_libplayer_cb(void *data, int type, void *event)
{
    Ecore_Event_Mouse_Move *e = event;
    Evas_Coord y;

    evas_object_geometry_get(mod->o_mediacontrols, NULL, &y, NULL, NULL);

    if ((e->window != enna->ee_winid) && (e->y >= y))
    {
        media_controls_display(1);
        return 1;
    }
    return 0;
}

static int
_mediaplayer_mouse_down_libplayer_cb(void *data, int type, void *event)
{
    Ecore_Event_Mouse_Button *e = event;
    Evas_Coord y;

    evas_object_geometry_get(mod->o_mediacontrols, NULL, &y, NULL, NULL);

    if ((e->window != enna->ee_winid) && (e->y >= y))
    {
        media_controls_display(1);
        return 1;
    }
    return 0;
}

void
movie_start_playback(int resume)
{
    mod->state = VIDEOPLAYER_VIEW;

    ENNA_EVENT_HANDLER_DEL(mod->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_move_event_handler);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    mod->o_mediaplayer = evas_object_rectangle_add(enna->evas);
    evas_object_color_set(mod->o_mediaplayer, 0, 0, 0, 255);
    edje_object_part_swallow(mod->o_edje,
                             "fullscreen.swallow", mod->o_mediaplayer);
    evas_object_event_callback_add(mod->o_mediaplayer, EVAS_CALLBACK_RESIZE,
                                   _mediaplayer_resize_cb, NULL);

    ENNA_OBJECT_DEL(mod->o_mediacontrols);

    mod->o_mediacontrols =
        enna_mediaplayer_obj_add(enna->evas, mod->enna_playlist);
    edje_object_part_swallow(mod->o_edje,
                             "controls.swallow", mod->o_mediacontrols);
    enna_mediaplayer_obj_layout_set(mod->o_mediacontrols, "layout,video");

    mod->mouse_button_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                _mediaplayer_mouse_down_libplayer_cb, NULL);
    mod->mouse_move_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                _mediaplayer_mouse_move_libplayer_cb, NULL);

    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_MOUSE_MOVE,
                                   _mediaplayer_mouse_move_cb, NULL);
    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_MOUSE_UP,
                                   _mediaplayer_mouse_up_cb, NULL);
    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_RESIZE,
                                   _mediaplayer_resize_cb, NULL);

    enna_mediaplayer_stop();
    enna_mediaplayer_play(mod->enna_playlist);
#if 0
    if (resume)
    {
        Enna_Metadata *m;
        m = enna_mediaplayer_metadata_get(mod->enna_playlist);
        enna_mediaplayer_position_set(m->position);
    }
#endif
    popup_resume_display(0);
}

static void
browser_cb_select(void *data, Evas_Object *obj, void *event_info)
{
    int i = 0;
    Enna_Vfs_File *f;
    Eina_List *l;
    Browser_Selected_File_Data *ev = event_info;

    if (!ev || !ev->file) return;

    if (ev->file->is_directory || ev->file->is_menu)
    {
        enna_log (ENNA_MSG_EVENT,
                  ENNA_MODULE_NAME, "Directory Selected %s", ev->file->uri);
        update_movies_counter(ev->files);
    }
    else
    {
        Enna_Metadata *m;

        enna_log(ENNA_MSG_EVENT,
                 ENNA_MODULE_NAME, "File Selected %s", ev->file->uri);
        enna_mediaplayer_playlist_clear(mod->enna_playlist);

        /* File selected, create mediaplayer */
        EINA_LIST_FOREACH(ev->files, l, f)
        {
            if (!f->is_directory && !f->is_menu)
            {
                enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                         "Append : %s %s to playlist", f->label, f->uri);
                enna_mediaplayer_uri_append(mod->enna_playlist,
                                            f->uri, f->label);

                if (!strcmp(f->uri, ev->file->uri))
                {
                    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                              "Select : %s %d in playlist", f->uri, i);
                    enna_mediaplayer_select_nth(mod->enna_playlist,i);

                    if (mod->o_current_uri)
                        free(mod->o_current_uri);
                    mod->o_current_uri = strdup(f->uri);
                }
                i++;
            }
        }

        /* fetch new stream's metadata */
        m = enna_mediaplayer_metadata_get(mod->enna_playlist);
#if 0
        if (m->position)
        {
            /* stream has already been played once, show resume popup */
            popup_resume_display(1);
        }
        else
#endif
            movie_start_playback(0);
    }
    free(ev);
}


static void
video_infos_display(Enna_Vfs_File *file, int delay)
{
    Enna_Metadata *m;

    if (!file)
        return;

    m = enna_metadata_meta_new(file->uri);
    if (!m)
        return;

    if (!delay)
    {
        char *title, *categories, *length, *synopsis;
        const char *label;

        /* title */
        title = enna_metadata_meta_get(m, "title", 1);
        label = title ? title : file->label;
        edje_object_part_text_set(mod->o_edje, "title.label", label);

        /* genre */
        categories = enna_metadata_meta_get(m, "category", 5);
        edje_object_part_text_set(mod->o_edje, "genre.label",
                                  categories ? categories : "");

        /* length */
        length = enna_metadata_meta_duration_get(m);
        edje_object_part_text_set(mod->o_edje, "length.label",
                                  length ? length : "");

        /* synopsis */
        synopsis = enna_metadata_meta_get(m, "synopsis", 1);
        edje_object_part_text_set(mod->o_edje, "synopsis.textblock",
                                  synopsis ? synopsis : "");
        edje_object_signal_emit(mod->o_edje, synopsis ?
                                "separator,show" : "separator,hide", "enna");

        ENNA_FREE(title);
        ENNA_FREE(categories);
        ENNA_FREE(length);
        ENNA_FREE(synopsis);
    }
    else
    {
        backdrop_show(m);
        snapshot_show(m, file->is_directory);

        enna_video_flags_update(mod->o_video_flags, m);

        enna_panel_infos_set_cover(mod->o_panel_infos, m);
        enna_panel_infos_set_text(mod->o_panel_infos, m);
        enna_panel_infos_set_rating(mod->o_panel_infos, m);
    }

    enna_metadata_meta_free(m);
}

static void
video_infos_del (void)
{
    edje_object_part_text_set(mod->o_edje, "title.label", "");
    edje_object_part_text_set(mod->o_edje, "genre.label", "");
    edje_object_part_text_set(mod->o_edje, "length.label", "");
    edje_object_part_text_set(mod->o_edje, "synopsis.textblock", "");
    panel_infos_display(0);
    popup_resume_display(0);
    enna_video_picture_set(mod->o_backdrop, NULL, 0);
    enna_video_picture_set(mod->o_snapshot, NULL, 0);
    enna_video_flags_update(mod->o_video_flags, NULL);
    edje_object_signal_emit(mod->o_edje, "separator,hide", "enna");
}

static void
browser_cb_delay_hilight(void *data, Evas_Object *obj, void *event_info)
{
    Browser_Selected_File_Data *ev = event_info;

    if (!ev || !ev->file)
        return;

    if (!ev->file->is_directory && !ev->file->is_menu)
    {
        /* ask for on-demand scan for local files */
        if (!strncmp(ev->file->uri, "file://", 7))
            enna_metadata_ondemand(ev->file->uri + 7);

        video_infos_display(ev->file, 1);
    }
}

static void
browser_cb_hilight(void *data, Evas_Object *obj, void *event_info)
{
    Browser_Selected_File_Data *ev = event_info;

    if (!ev || !ev->file)
        return;

    if (!ev->file->is_directory && !ev->file->is_menu)
        video_infos_display(ev->file, 0);
}

static void
browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if (!vfs)
        return;

    mod->o_browser = enna_browser_add(enna->evas);

    enna_browser_view_add(mod->o_browser, ENNA_BROWSER_VIEW_LIST);
    evas_object_smart_callback_add(mod->o_browser,
                                   "root", browser_cb_root, NULL);
    evas_object_smart_callback_add(mod->o_browser,
                                   "selected", browser_cb_select, NULL);
    evas_object_smart_callback_add(mod->o_browser,
                                   "hilight", browser_cb_hilight, NULL);
    evas_object_smart_callback_add (mod->o_browser, "delay,hilight",
                                    browser_cb_delay_hilight, NULL);

    edje_object_part_swallow(mod->o_edje,
                             "browser.swallow", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);
    if (mod->vl)
        enna_volumes_listener_del(mod->vl);
    mod->vl = NULL;
    ENNA_OBJECT_DEL(mod->o_list);

    ENNA_OBJECT_DEL(mod->o_panel_infos);
    mod->o_panel_infos = enna_panel_infos_add(enna->evas);
    edje_object_part_swallow(mod->o_edje,
                             "infos.panel.swallow", mod->o_panel_infos);

    ENNA_OBJECT_DEL(mod->o_resume);
    mod->o_resume = video_resume_add(enna->evas);
    edje_object_part_swallow(mod->o_edje,
                             "resume.swallow", mod->o_resume);

    ENNA_OBJECT_DEL(mod->o_video_flags);
    mod->o_video_flags = enna_video_flags_add(enna->evas);
    edje_object_part_swallow(mod->o_edje,
                             "infos.flags.swallow", mod->o_video_flags);

    mod->state = BROWSER_VIEW;
}

static void
_refresh_menu(void *data, Enna_Volume *volume)
{
    if (volume->type == VOLUME_TYPE_DVD_VIDEO)
        _create_menu();
}

/****************************************************************************/
/*                                  GUI                                     */
/****************************************************************************/

static void
_create_menu(void)
{
    Evas_Object *o;
    Eina_List *l, *categories;
    Enna_Class_Vfs *cat;

    ENNA_OBJECT_DEL(mod->o_list);
    if (mod->vl)
        enna_volumes_listener_del(mod->vl);
    mod->vl = NULL;
    /* Create List */
    o = enna_list_add(enna->evas);

    categories = enna_vfs_get(ENNA_CAPS_VIDEO);
    EINA_LIST_FOREACH(categories, l, cat)
    {
        Enna_Vfs_File *item;
        item = calloc(1, sizeof(Enna_Vfs_File));
        item->icon = (char*)eina_stringshare_add(cat->icon);
        item->label = (char*)eina_stringshare_add(gettext(cat->label));
        item->is_menu = 1;
        enna_list_file_append(o, item, browse, cat);
    }

    enna_list_select_nth(o, 0);
    mod->o_list = o;
    edje_object_part_swallow(mod->o_edje, "browser.swallow", o);
    video_infos_del();
    mod->vl =
        enna_volumes_listener_add("video", _refresh_menu, _refresh_menu, NULL);
}

static void
_create_gui(void)
{
    Evas_Object *o;

    mod->state = MENU_VIEW;
    o = edje_object_add(enna->evas);
    edje_object_file_set(o, enna_config_theme_get(), "activity/video");
    mod->o_edje = o;
    _create_menu();
}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_init(int dummy)
{
    _create_gui();
    enna_content_append(ENNA_MODULE_NAME, mod->o_edje);
}

static void
_class_show(int dummy)
{
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");

    switch (mod->state)
    {
    case BROWSER_VIEW:
    case MENU_VIEW:
        edje_object_signal_emit(mod->o_edje, "content,show", "enna");
        break;

    case VIDEOPLAYER_VIEW:
        break;
    default:
        enna_log(ENNA_MSG_ERROR,
                 ENNA_MODULE_NAME,  "State Unknown in video module");
    }
}

static void
_class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void
_class_event(enna_input event)
{
    switch (mod->state)
    {
    case MENU_VIEW:
        menu_view_event(event);
        break;
    case BROWSER_VIEW:
        browser_view_event(event);
        break;
    case VIDEOPLAYER_VIEW:
        videoplayer_view_event(event);
        break;
    default:
        break;
    }
}

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    1,
    N_("Video"),
    NULL,
    "icon/video",
    "background/video",
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event,
        NULL
    },
    NULL
};

static void
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Video));
    mod->em = em;
    em->mod = mod;

    mod->infos_displayed = 0;
    mod->resume_displayed = 0;
    mod->controls_displayed = 0;
    mod->o_backdrop = enna_video_picture_add(enna->evas);
    mod->o_snapshot = enna_video_picture_add(enna->evas);
    mod->eos_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, NULL);
    enna_activity_add(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
}

static void
em_shutdown(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_EVENT_HANDLER_DEL(mod->eos_event_handler);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_list);
    evas_object_smart_callback_del(mod->o_browser, "root", browser_cb_root);
    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", browser_cb_select);
    evas_object_smart_callback_del(mod->o_browser,
                                   "hilight", browser_cb_hilight);
    evas_object_smart_callback_del(mod->o_browser,
                                   "delay,hilight", browser_cb_delay_hilight);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_move_event_handler);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_mediacontrols);
    ENNA_OBJECT_DEL(mod->o_backdrop);
    ENNA_OBJECT_DEL(mod->o_snapshot);
    ENNA_OBJECT_DEL(mod->o_panel_infos);
    ENNA_OBJECT_DEL(mod->o_resume);
    ENNA_OBJECT_DEL(mod->o_video_flags);
    ENNA_FREE(mod->o_current_uri);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_video
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_video",
    N_("Video"),
    "icon/video",
    N_("This is the main video module"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    em_init(em);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    em_shutdown(em);
}
