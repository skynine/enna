#include <string.h>

#include "codecs.h"

static const struct {
    const char *codec_id;
    const char *codec_name;
} codecs_mapping[] = {

    /* Audio Codecs */
    { "wma9dmo", "WMA 9" },
    { "wmadmo", "WMA" },
    { "wma9spdmo", "WMA 9 Speech" },
    { "wma9spdshow", "WMA 9 Speech" },
    { "ffqdm2", "QDM2" },
    { "qdmc", "QuickTime QDMC/QDM2" },
    { "qclp", "QuickTime QCLP" },
    { "qtmace3", "QuickTime MACE3" },
    { "qtmace6", "QuickTime MACE6" },
    { "ffra144", "RealAudio 1.0" },
    { "ffra288", "RealAudio 2.0" },
    { "ffcook", "COOK" },
    { "ffatrc", "Atrac 3" },
    { "ra144", "RealAudio 1.0" },
    { "ra144win", "RealAudio 1.0" },
    { "ra144mac", "RealAudio 1.0" },
    { "ra288", "RealAudio 2.0" },
    { "ra288win", "RealAudio 2.0" },
    { "ra288mac", "RealAudio 2.0" },
    { "ra10cook", "RealPlayer 10 COOK" },
    { "racook", "RealAudio COOK" },
    { "ra10cookwin", "RealAudio 10 COOK" },
    { "racookwin", "RealAudio COOK" },
    { "racookmac", "RealAudio COOK" },
    { "rasipr", "RealAudio Sipro" },
    { "ra10sipr", "RealAudio 10 Sipro" },
    { "ra10siprwin", "RealAudio 10 Sipro" },
    { "rasiprwin", "RealAudio Sipro" },
    { "rasiprmac", "RealAudio Sipro" },
    { "raatrc", "RealAudio ATRAC3" },
    { "ra10atrc", "RealAudio 10 ATRAC3" },
    { "ra10atrcwin", "RealAudio 10 ATRAC3" },
    { "raatrcwin", "RealAudio ATRAC3" },
    { "raatrcmac", "RealAudio ATRAC3" },
    { "ffadpcmimaamv", "AMV IMA ADPCM" },
    { "ffadpcmimaqt", "QT IMA ADPCM" },
    { "ffadpcmimawav", "WAV IMA ADPCM" },
    { "imaadpcm", "IMA ADPCM" },
    { "ffadpcmms", "MS ADPCM" },
    { "msadpcm", "MS ADPCM" },
    { "ffadpcmimadk4", "DK4 IMA ADPCM" },
    { "dk4adpcm", "Duck DK4 ADPCM" },
    { "ffadpcmimadk3", "DK3 IMA ADPCM" },
    { "dk3adpcm", "Duck DK3 ADPCM" },
    { "ffroqaudio", "Id RoQ" },
    { "ffsmkaud", "Smacker" },
    { "ffdsicinaudio", "Delphine CIN" },
    { "ff4xmadmpcm", "4XM ADPCM" },
    { "ffadpcmimaws", "Westwood IMA ADPCM" },
    { "ffwssnd1", "Westwood SND1" },
    { "ffinterplaydpcm", "Interplay DPCM" },
    { "ffadpcmea", "EA ADPCM" },
    { "ffadpcmeamaxis", "EA MAXIS XA ADPCM" },
    { "ffadpcmxa", "XA ADPCM" },
    { "ffxandpcm", "XAN DPCM" },
    { "ffadpcmthp", "THP ADPCM" },
    { "libdv", "Raw DV" },
    { "ffdv", "DV" },
    { "faad", "AAC" },
    { "ffaac", "AAC" },
    { "ffflac", "FLAC" },
    { "ffalac", "ALAC" },
    { "fftta", "True Audio (TTA)" },
    { "ffwavpack", "WavPack" },
    { "ffshorten", "Shorten" },
    { "ffape", "Monkey's Audio" },
    { "ffmlp", "MLP" },
    { "ffnellymoser", "Nellymoser" },
    { "pcm", "Uncompressed PCM" },
    { "divx", "DivX audio (WMA)" },
    { "msadpcmacm", "MS ADPCM" },
    { "mp3", "MP3" },
    { "ffpcmdaud", "D-Cinema" },
    { "ffwmav1", "DivX audio v1" },
    { "ffwmav2", "DivX audio v2" },
    { "ffmac3", "Macintosh Audio Compression and Expansion 3:1" },
    { "ffmac6", "Macintosh Audio Compression and Expansion 6:1" },
    { "ffsonic", "Sonic" },
    { "ffmp3on4", "Multi-channel MP3 on MP4" },
    { "ffmp3", "MP3" },
    { "ffmp3adu", "MP3 adu" },
    { "ffmp2", "MP2" },
    { "mad", "MP3" },
    { "mp3acm", "MP3" },
    { "imaadpcmacm", "IMA ADPCM" },
    { "msgsm", "MS GSM" },
    { "msgsmacm", "MS GSM" },
    { "msnaudio", "MSN AUDIO" },
    { "alaw", "aLaw" },
    { "ulaw", "uLaw" },
    { "dvdpcm", "Uncompressed DVD/VOB LPCM" },
    { "a52", "AC3" },
    { "ffac3", "AC-3" },
    { "ffeac3", "E-AC-3" },
    { "dts", "DTS" },
    { "ffdca", "DTS" },
    { "ffmusepack7", "Musepack sv7" },
    { "ffmusepack8", "Musepack sv8" },
    { "musepack", "Musepack" },
    { "ffamrnb", "AMR Narrowband" },
    { "ffamrwb", "AMR Wideband" },
    { "ffadcpmswf", "ADPCM Flash-variant" },
    { "voxware", "VoxWare" },
    { "acelp", "ACELP.net Sipro Lab" },
    { "ffimc", "Intel Music Coder" },
    { "imc", "Intel Music Coder" },
    { "iac25", "Indeo audio" },
    { "ffctadp32", "Creative ADPCM" },
    { "ctadp32", "Creative ADPCM" },
    { "sc4", "SC4 : Micronas speech" },
    { "hwac3", "AC-3" },
    { "hwdts", "DTS" },
    { "ffvorbis", "Vorbis" },
    { "vorbis", "OggVorbis" },
    { "vorbisacm", "OggVorbis ACM" },
    { "speex", "Speex" },
    { "vivoaudio", "Vivo G.723/Siren" },
    { "g72x", "G.711/G.721/G.723" },
    { "ffg726", "Sharp G.726" },
    { "g726", "Sharp G.726" },
    { "atrac3", "Sony ATRAC3" },
    { "ALF2", "ALF2" },
    { "fftruespeech", "TrueSpeech" },
    { "truespeech", "DSP Group TrueSpeech" },
    { "voxwarert24", "VoxWare RT24 speech" },
    { "lhacm", "Lernout & Hauspie CELP and SBC" },
    { "TwinVQ", "NTTLabs VQF" },
    { "hwmpa", "MPEG audio" },
    { "msnsiren", "MSN siren" },

    /* Video Codecs */
  
    { NULL, NULL }
};

char *
get_codec_name (char *codec_id)
{
    int i;

    if (!codec_id)
        return strdup ("");
  
    for (i = 0; codecs_mapping[i].codec_id; i++)
        if (!strcmp (codec_id, codecs_mapping[i].codec_id))
            return strdup (codecs_mapping[i].codec_name);

    return strdup (codec_id);
}
