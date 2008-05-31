/* Interface */

#include "enna.h"
#include <player.h>


static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

static void _class_init(int dummy);
static void _class_shutdown(int dummy);
static int _class_file_set(const char *uri);
static int _class_play(void);
static int _class_pause(void);
static int _class_stop(void);
static double _class_position_get();
static double _class_length_get();
static Enna_Metadata *_class_metadata_get(void);
static void _class_event_cb_set(void (*event_cb)(void *data, enna_mediaplayer_event_t event), void *data);

static Enna_Class_MediaplayerBackend class =
  {
    "libplayer",
    1,
    {
      _class_init,
      _class_shutdown,
      _class_file_set,
      _class_play,
      _class_pause,
      _class_stop,
      _class_position_get,
      _class_length_get,
      _class_metadata_get,
      _class_event_cb_set,
    }
  };

typedef struct _Enna_Module_libplayer Enna_Module_libplayer;

struct _Enna_Module_libplayer
{
   Evas *evas;
   player_t *player;
   Enna_Module *em;
   void (*event_cb)(void *data, enna_mediaplayer_event_t event);
   void *event_cb_data;
};

static Enna_Module_libplayer *mod;

EAPI Enna_Module_Api module_api =
  {
    ENNA_MODULE_VERSION,
    "libplayer"
  };

static void _class_init(int dummy)
{
   printf("libplayer class init\n");
}

static void _class_shutdown(int dummy)
{
   player_playback_stop (mod->player);
   player_uninit (mod->player);
}

static int _class_file_set(const char *uri)
{
   mrl_t *mrl;
   mrl_resource_local_args_t *args;

   args = calloc (1, sizeof (mrl_resource_local_args_t));
   args->location = strdup (uri);

   mrl = mrl_new (mod->player, PLAYER_MRL_RESOURCE_FILE, args);
   if (!mrl)
    return 1;
   
   player_mrl_set (mod->player, mrl);
   return 0;
}

static int _class_play(void)
{
   player_playback_start (mod->player);
   return 0;
}

static int _class_stop(void)
{
   player_playback_stop (mod->player);
   return 0;
}

static int _class_pause(void)
{
   player_playback_pause (mod->player);
   return 0;
}

static double _class_position_get()
{
   double time_pos;

   time_pos = (double) player_get_time_pos (mod->player) / 1000.0;
   return time_pos < 0.0 ? 0.0 : time_pos;
}

static double _class_length_get()
{
   return (double) mrl_get_property (mod->player, NULL, PLAYER_PROPERTY_LENGTH) / 1000.0;
}

static Enna_Metadata *_class_metadata_get(void)
{
   Enna_Metadata *meta;

   meta = calloc (1, sizeof (Enna_Metadata));
   meta->title = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_TITLE);
   meta->artist = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_ARTIST);
   meta->album = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_ALBUM);
   meta->year = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_YEAR);
   meta->genre = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_GENRE);
   meta->comment = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_COMMENT);
   meta->discid = NULL;
   meta->track = mrl_get_metadata (mod->player, NULL, PLAYER_METADATA_TRACK);

   return meta;
}

static void _class_event_cb_set(void (*event_cb)(void *data, enna_mediaplayer_event_t event), void *data)
{
   /* FIXME: function to call when end of stream is send by libplayer */

   mod->event_cb_data = data;
   mod->event_cb = event_cb;
}

/* Module interface */

static int
_event_cb (player_event_t e, void *data)
{
   if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
     printf ("PLAYBACK FINISHED\n");

   return 0;
}

static int
em_init(Enna_Module *em)
{
   mod = calloc(1, sizeof(Enna_Module_libplayer));
   mod->em = em;
   mod->evas = em->evas;
   mod->player =
     player_init (PLAYER_TYPE_MPLAYER, PLAYER_AO_ALSA, PLAYER_VO_NULL,
                  PLAYER_MSG_WARNING, _event_cb);
   enna_mediaplayer_backend_register(&class);
   return 1;
}


static int
em_shutdown(Enna_Module *em)
{

   _class_shutdown(0);
   free(mod);
   return 1;
}

EAPI void
module_init(Enna_Module *em)
{
   if (!em)
     return;

   if (!em_init(em))
     return;
}

EAPI void
module_shutdown(Enna_Module *em)
{
   em_shutdown(em);
}
