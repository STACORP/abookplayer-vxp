// ============================================================================
// Application: MRE Professional Audio Suite (Ver. 1.2 - Modular)
// Target: MediaTek MRE 2.0 / 2.5 / 3.0 (C/C++ Runtime)
// ============================================================================

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmmm.h"
#include "vmtimer.h"
#include "vmstdlib.h"

#ifndef VM_DEVICE_SPEAKER
#define VM_DEVICE_SPEAKER       0
#endif
#ifndef VM_DEVICE_SPEAKER2
#define VM_DEVICE_SPEAKER2      2
#endif
#ifndef VM_DEVICE_LOUDSPEAKER
#define VM_DEVICE_LOUDSPEAKER   3
#endif
#ifndef VM_DEVICE_SPEAKER_BOTH
#define VM_DEVICE_SPEAKER_BOTH  4
#endif

#ifndef VM_FORMAT_MIDI
#define VM_FORMAT_MIDI          1
#endif
#ifndef VM_FORMAT_WAV
#define VM_FORMAT_WAV           2
#endif
#ifndef VM_FORMAT_AMR
#define VM_FORMAT_AMR           3
#endif
#ifndef VM_FORMAT_MP3
#define VM_FORMAT_MP3           4
#endif
#ifndef VM_FORMAT_AAC
#define VM_FORMAT_AAC           5
#endif

#ifndef VM_FS_ATTR_DIR
#define VM_FS_ATTR_DIR          0x10
#endif
#ifndef VM_FS_ATTR_VOLUME
#define VM_FS_ATTR_VOLUME       0x08
#endif

#define MAX_TRACKS               256
#define TRACK_ALLOC_STEP         32
#define MAX_SCAN_DIRS            64
#define MAX_FOLDER_ENTRIES       192
#define FOLDER_ALLOC_STEP        16
#define MAX_LRC_LINES            256
#define MAX_LRC_TEXT_LEN         72
#define MAX_HELP_LINES           96
#define TEXT_LINE_STRIDE         96
#define STYLE_HL_ON              ((VMWCHAR)0x0001)
#define STYLE_HL_OFF             ((VMWCHAR)0x0002)
#define ENGINE_TIMER_MS          50
#define NUMERIC_INPUT_TIMEOUT_MS 1200

// ============================================================================
// MODULE 1: Color Helpers & Themes
// ============================================================================
static vm_graphic_color make_color(VMUINT8 r, VMUINT8 g, VMUINT8 b) {
    vm_graphic_color c;
    c.vm_color_565 = VM_COLOR_888_TO_565(r, g, b);
    c.vm_color_888 = ((VMUINT32)b << 16) | ((VMUINT32)g << 8) | (VMUINT32)r;
    return c;
}

static vm_graphic_color lerp_color(vm_graphic_color a, vm_graphic_color b, int pct_b) {
    if (pct_b <= 0) return a;
    if (pct_b >= 100) return b;
    int ar = (int)(a.vm_color_888 & 0xFF), ag = (int)((a.vm_color_888 >> 8) & 0xFF), ab = (int)((a.vm_color_888 >> 16) & 0xFF);
    int br = (int)(b.vm_color_888 & 0xFF), bg = (int)((b.vm_color_888 >> 8) & 0xFF), bb = (int)((b.vm_color_888 >> 16) & 0xFF);
    return make_color(
        (VMUINT8)(ar + ((br - ar) * pct_b) / 100),
        (VMUINT8)(ag + ((bg - ag) * pct_b) / 100),
        (VMUINT8)(ab + ((bb - ab) * pct_b) / 100)
    );
}

static int color_luma(vm_graphic_color c) {
    int r = (int)(c.vm_color_888 & 0xFF), g = (int)((c.vm_color_888 >> 8) & 0xFF), b = (int)((c.vm_color_888 >> 16) & 0xFF);
    return (299 * r + 587 * g + 114 * b) / 1000;
}

enum ThemeID {
    THEME_BASIC_LIGHT = 0, THEME_BASIC_DARK, THEME_MINT, THEME_OCEAN,
    THEME_RED_VELVET, THEME_BUSINESS_GOLD, THEME_SUNBEAM_GLOW, THEME_COUNT
};

struct Theme {
    const char* name;
    vm_graphic_color bg, header_bg, header_text, item_text, item_playing, item_playing_selected;
    vm_graphic_color item_selected_bg, item_selected_text, marquee_bg, marquee_text;
    vm_graphic_color softkey_bg, softkey_text, popup_bg, popup_border;
};

static const Theme g_themes[THEME_COUNT] = {
    { "Basic Light",
      make_color(224,226,232), make_color(185,192,204), make_color(15,15,15),
      make_color(25,25,25), make_color(0,128,45), make_color(150,255,165),
      make_color(0,95,195), make_color(255,255,255), make_color(202,208,220), make_color(10,10,10),
      make_color(185,192,204), make_color(0,0,0), make_color(240,243,248), make_color(90,98,112) },
    { "Basic Dark",
      make_color(16,16,18), make_color(32,34,38), make_color(242,242,245),
      make_color(205,208,214), make_color(60,220,75), make_color(160,255,120),
      make_color(42,95,165), make_color(255,255,255), make_color(24,26,30), make_color(255,180,30),
      make_color(32,34,38), make_color(230,230,235), make_color(30,32,38), make_color(110,120,138) },
    { "Mint",
      make_color(20,52,38), make_color(12,34,24), make_color(175,245,210),
      make_color(150,220,182), make_color(55,255,145), make_color(255,245,130),
      make_color(38,105,76), make_color(255,255,255), make_color(14,40,28), make_color(125,245,185),
      make_color(12,34,24), make_color(175,245,210), make_color(18,46,34), make_color(65,145,105) },
    { "Ocean",
      make_color(10,25,47), make_color(15,42,74), make_color(115,255,225),
      make_color(155,170,200), make_color(0,255,185), make_color(255,240,120),
      make_color(28,78,132), make_color(255,255,255), make_color(15,35,60), make_color(115,255,225),
      make_color(15,42,74), make_color(115,255,225), make_color(17,45,80), make_color(20,160,255) },
    { "Red Velvet",
      make_color(32,8,14), make_color(64,12,24), make_color(255,215,225),
      make_color(225,190,200), make_color(255,110,120), make_color(255,235,130),
      make_color(130,28,52), make_color(255,255,255), make_color(48,10,20), make_color(255,190,205),
      make_color(64,12,24), make_color(255,215,225), make_color(45,10,18), make_color(185,48,78) },
    { "Business Gold",
      make_color(36,30,24), make_color(58,48,36), make_color(230,198,85),
      make_color(214,204,190), make_color(255,220,40), make_color(150,255,140),
      make_color(105,82,56), make_color(255,248,220), make_color(46,38,28), make_color(240,200,90),
      make_color(58,48,36), make_color(230,198,85), make_color(48,40,32), make_color(165,132,62) },
    { "Sunbeam Glow",
      make_color(82,38,0), make_color(52,22,0), make_color(255,225,100),
      make_color(255,205,130), make_color(255,240,40), make_color(160,255,140),
      make_color(150,72,0), make_color(255,255,255), make_color(64,28,0), make_color(255,230,60),
      make_color(52,22,0), make_color(255,225,100), make_color(66,30,0), make_color(205,108,15) }
};

// ============================================================================
// MODULE 2: Core Definitions & Modular Subsystem Structs
// ============================================================================
enum AudioFormatMask {
    FMT_MP3 = 0x01, FMT_MIDI = 0x02, FMT_AAC = 0x04, FMT_AMR = 0x08,
    FMT_WAV = 0x10, FMT_AWB  = 0x20, FMT_VM  = 0x40, FMT_IMY = 0x80
};

enum LoopMode { LOOP_LIST = 0, LOOP_ONE, LOOP_NONE, LOOP_COUNT };
static const char* LOOP_NAMES[LOOP_COUNT] = { "List", "One", "None" };

enum SortMode {
    SORT_FILENAME = 0, SORT_TITLE, SORT_ARTIST, SORT_ALBUM,
    SORT_YEAR, SORT_FILESIZE, SORT_FORMAT, SORT_COUNT
};
static const char* SORT_NAMES[SORT_COUNT] = {
    "Filename (A-Z)", "Title (A-Z)", "Artist (A-Z)", "Album (A-Z)",
    "Year (0-9)", "File Size", "Format"
};
static const char* SORT_SHORT_NAMES[SORT_COUNT] = {
    "File", "Title", "Artist", "Album", "Year", "Size", "Fmt"
};

enum ScreensaverID {
    SS_NONE = 0, SS_STARFIELD, SS_EQUALIZER, SS_BOUNCING_ORB,
    SS_DIGITAL_RAIN, SS_BOKEH_SNOW, SS_COUNT
};
static const char* SCREENSAVER_NAMES[SS_COUNT] = {
    "Lyrics Only", "Starfield", "Equalizer", "Bouncing Orb", "Digital Rain", "Bokeh Snow"
};

enum AudioDeviceProfile {
    DEV_SPEAKER_BOTH = 0, DEV_LOUDSPEAKER, DEV_EARPHONE, DEV_BUZZER, DEV_PROFILE_COUNT
};
static const char* DEVICE_PROFILE_NAMES[DEV_PROFILE_COUNT] = {
    "Both (Loud+Ear)", "Loudspeaker", "Earphone", "Buzzer"
};
static const VMUINT DEVICE_PROFILE_VALUES[DEV_PROFILE_COUNT] = {
    VM_DEVICE_SPEAKER_BOTH, VM_DEVICE_LOUDSPEAKER, VM_DEVICE_SPEAKER2, VM_DEVICE_SPEAKER
};

struct TrackBase {
    VMWCHAR filename[128]; // relative path from lib_dir (e.g. "song.mp3" or "Pop\song.mp3")
    VMUINT  filesize;
    VMUINT8 format;
    VMUINT16 id3_slot;
};

struct Id3Tag {
    VMWCHAR title[48];
    VMWCHAR artist[36];
    VMWCHAR album[36];
    VMUINT16 year;
    VMUINT16 track_num;
    bool parsed;
    bool has_id3;
};

struct LrcLine {
    VMINT32 time_ms;
    VMWCHAR text[MAX_LRC_TEXT_LEN];
};

struct StarPoint { int x, y, z; };

struct KaraokeState {
    LrcLine lrc_lines[MAX_LRC_LINES];
    int lrc_count;
    int lrc_file_offset_ms;
    int lrc_track_idx;
    VMWCHAR lrc_cur_name[128];
    int ss_frame;
    StarPoint ss_stars[24];
    int ss_eq_heights[16];
    int ss_orb_x, ss_orb_y, ss_orb_dx, ss_orb_dy;
    int ss_rain_y[20];
    int ss_snow_x[4][36];
    int ss_snow_y[4][36];
    int ss_snow_wind_x;
    int ss_snow_wind_y;
    VMINT ss_snow_next_wind_tick;
};

enum FolderEntryType {
    FENT_LRC_AUTO = 0,
    FENT_LRC_NONE,
    FENT_UP,
    FENT_THIS,
    FENT_DRIVE,
    FENT_SUBDIR,
    FENT_FILE
};

struct FolderEntry {
    VMWCHAR name[128];
    VMUINT8 type;
};

struct FolderViewerState {
    VMWCHAR cur_path[260];
    bool lrc_mode;
    bool at_drive_list;
    VMWCHAR drive_paths[3][260];
    FolderEntry* entries;
    int entry_count;
    int entry_cap;
    int cursor;
    int scroll;
};

struct TextReaderState {
    int topic_idx;
    int scroll_line;
    int total_lines;
    VMWCHAR lines[MAX_HELP_LINES][TEXT_LINE_STRIDE];
};

enum PlaybackState { STATE_STOPPED, STATE_PLAYING, STATE_PAUSED };

enum AppUIMode {
    MODE_TRACKLIST,
    MODE_MENU,
    MODE_MODES_MENU,
    MODE_SETTINGS_MENU,
    MODE_FOLDER_VIEWER,
    MODE_FOLDER_OPTIONS,
    MODE_VOLUME_SELECT,
    MODE_DEVICE_SELECT,
    MODE_TIME_PICKER,
    MODE_THEME_SELECT,
    MODE_FORMAT_FILTER,
    MODE_SORT_SELECT,
    MODE_KARAOKE,
    MODE_HELP_INDEX,
    MODE_HELP_READER,
    MODE_FILE_TAGS,
    MODE_POPUP_CONFIRM_EXIT,
    MODE_POPUP_ABOUT,
    MODE_POPUP_ERROR
};

#define MENU_ITEM_COUNT          7
#define MODES_MENU_ITEM_COUNT    3
#define SETTINGS_MENU_ITEM_COUNT 10
#define FOLDER_OPT_ITEM_COUNT    4
#define HELP_TOPIC_COUNT         6

struct HelpArticle { const char* title; const char* body; };

static const HelpArticle g_help_articles[HELP_TOPIC_COUNT] = {
    {
        "1. Basics & Files",
        "By default, the app scans audio files in the {same folder} as this {.vxp} app (up to {256 tracks}). Use {Play from Folder} in {Modes} to browse other folders.\n\n{Supported:} MP3, MIDI (.mid/.midi), IMY (.imy), AAC, AMR, AWB, VM, WAV.\n\n{Tracklist Keys:}\n- {Up/Down:} Move selection\n- {* / #:} Skip -10 / +10 tracks\n- {0-9:} Type track number to jump\n- {OK:} Play or Pause selected track\n- {LSK:} Options | {RSK:} Exit"
    },
    {
        "2. Modes: Karaoke & LRC",
        "Open {Modes > Karaoke Mode} from Options. When a song plays, the app auto-loads {<trackname>.lrc} from the track's folder.\n\n{Manual Pick:} Press {LSK (Pick LRC)} inside Karaoke to select a .lrc file, return to Auto-Match, or clear lyrics.\n\n{LRC Features:} [mm:ss.xx], multi-timestamps, UTF-8, [offset:], and {<mm:ss.xx>} word highlighting.\n\n{Karaoke Keys:}\n- {Left/Right:} Switch Screensaver (Lyrics Only, Starfield, Equalizer, Bouncing Orb, Digital Rain, Bokeh Snow)\n- {Up/Down:} Nudge timing +/-500ms\n- {OK:} Play/Pause | {* / #:} Prev/Next"
    },
    {
        "3. Modes: Folder & ID3",
        "{Play from Folder:} Browse any folder or drive ([Phone], [Card], [App Folder]). Press {OK} on a folder to open it, or {LSK (Options)} for:\n- {Open:} Enter subfolder / go up\n- {Choose:} Load tracks in folder\n- {Choose with all subfolders:} Load folder & subfolders recursively\n- {Cancel:} Close popup\n\n{ID3 Tags:} Toggle in {Modes}, press {Left} in Tracklist, or tap {[ID3]}. Reads ID3v1/v2 (Title, Artist, Album, Year, Track #). Use {File Tags} in Options to inspect metadata."
    },
    {
        "4. Volume, Output & Power",
        "{Volume:} Ranges 0 (mute) to 6. Adjust in {Settings > Volume}, {Left/Right} on Volume row, hardware {Vol+/Vol-}, or tapping the bar.\n\n{Audio Output:} Both (Loud+Ear), Loudspeaker, Earphone, or Buzzer (device-dependent).\n\n{Keep Screen On:} In {Settings}, keeps the LCD backlight from turning off while the app is in foreground (restored on exit/background)."
    },
    {
        "5. Settings & Filters",
        "All preferences live in {Options > Settings}:\n- {Loop Mode:} Cycle {List}, {One}, or {None}.\n- {Shuffle:} When On in {List} loop, plays a random next track without reordering the list.\n- {Sort By:} Filename, Title, Artist, Album (by Track #), Year, Size, Format.\n- {Scroll on Pause:} Scroll long titles while paused.\n- {Show Clock:} Top-right HH:MM clock.\n- {Format Filter:} Toggle extensions.\n- {Themes:} 7 color palettes."
    },
    {
        "6. Touchscreen Controls",
        "- {Lists & Folder Viewer:} Drag vertically to scroll, tap to select, tap selected row to activate. Tap scrollbar to page up/down.\n- {Header:} Tap {[ID3]} badge to toggle ID3 mode; left half opens Sort.\n- {Now-Playing Dock:} Tap time to Jump Timestamp, {V#} for Volume, {[Loop/Shuf]} for Loop Mode, song name to Play/Pause.\n- {Karaoke:} Tap {< / >} for screensaver, {- / +} to sync, {|< / >|} for Prev/Next, bar to seek, center to Play/Pause."
    }
};

struct AppContext {
    VMINT layer_hdl;
    int screen_w, screen_h, font_h, font_baseline;
    int header_h, footer_h, marquee_h;

    AppUIMode ui_mode;
    AppUIMode return_mode;
    int return_cursor;

    int cur_theme;
    VMUINT8 enabled_formats;
    AudioDeviceProfile cur_device;
    LoopMode loop_mode;
    bool shuffle_mode;
    bool keep_screen_on;
    int volume;
    bool id3_mode;
    SortMode sort_mode;
    bool show_clock;
    bool scroll_on_pause;
    ScreensaverID screensaver_id;
    int lrc_user_offset_ms;

    VMWCHAR app_dir[260];
    VMWCHAR lib_dir[260];
    bool lib_recursive;

    TrackBase* tracks;
    int track_count;
    int track_capacity;
    int selected_track;
    int scroll_offset;

    Id3Tag* id3_tags;
    int id3_count;

    KaraokeState* karaoke;
    FolderViewerState* folder_viewer;
    TextReaderState* reader;

    char numeric_buf[8];
    int  numeric_len;
    VMINT numeric_timer;

    PlaybackState play_state;
    int active_track_idx;
    VMUINT total_duration_sec;
    VMUINT current_time_sec;
    VMUINT current_time_ms;

    VMUINT8* ram_audio_buf;
    VMUINT   ram_audio_size;
    VMINT    midi_handle;
    VMINT    audio_client_handle;
    bool     midi_type_set_imy;

    int sel_char_offset;
    int marq_char_offset;
    int scroll_tick_counter;
    int clock_tick_counter;
    VMINT engine_timer;

    int time_digits[6];
    int time_cursor;
    int menu_cursor;
    int menu_scroll;

    int pen_down_x, pen_down_y, pen_last_y;
    bool pen_moved;

    char error_title[32];
    char error_line1[64];
    char error_line2[64];
    char error_line3[64];
};

static AppContext g_app;

static void refresh_track_library();
static void sort_track_library();
static void start_playback(int track_index, VMUINT start_time_ms);
static void stop_playback();
static void pause_playback();
static void resume_playback();
static void trigger_repaint();
static void set_ui_mode(AppUIMode mode);
static void set_ui_mode_with_cursor(AppUIMode mode, int cursor);
static void digital_audio_callback(VMINT result);
static void midi_audio_callback(VMINT handle, VMINT event);
static void audio_interrupt_callback(VMINT handle, VMINT result);

// ============================================================================
// MODULE 3: Safe String, Encoding & Reusable Rich-Text Engine
// ============================================================================
static void safe_ascii_to_ucs2(VMWSTR dst, int max_chars, const char* src) {
    if (!dst || max_chars <= 0) return;
    int i = 0;
    if (src) {
        while (src[i] && i < max_chars - 1) { dst[i] = (VMWCHAR)(unsigned char)src[i]; i++; }
    }
    dst[i] = 0;
}

static void safe_wstrncpy_zero(VMWSTR dst, const VMWCHAR* src, int max_chars) {
    if (!dst || max_chars <= 0) return;
    if (!src) { dst[0] = 0; return; }
    int i = 0;
    while (src[i] && i < max_chars - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void safe_wstrncat_zero(VMWSTR dst, const VMWCHAR* src, int max_chars) {
    if (!dst || !src || max_chars <= 0) return;
    int dlen = vm_wstrlen(dst), i = 0;
    while (src[i] && (dlen + i) < max_chars - 1) { dst[dlen + i] = src[i]; i++; }
    dst[dlen + i] = 0;
}

static int wstr_icmp(const VMWCHAR* a, const VMWCHAR* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        VMWCHAR c1 = *a, c2 = *b;
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return (int)c1 - (int)c2;
        a++; b++;
    }
    return (int)(*a) - (int)(*b);
}

static bool ascii_istartswith_nocase(const VMUINT8* buf, int buf_len, const char* prefix) {
    int plen = (int)strlen(prefix);
    if (buf_len < plen) return false;
    for (int i = 0; i < plen; i++) {
        char c1 = (char)buf[i], c2 = prefix[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

static bool ends_with_ignore_case(const VMWCHAR* str, const char* ext) {
    VMWCHAR ext_ucs2[16];
    safe_ascii_to_ucs2(ext_ucs2, 16, ext);
    int ls = vm_wstrlen((VMWSTR)str), le = vm_wstrlen(ext_ucs2);
    if (ls < le) return false;
    for (int i = 0; i < le; i++) {
        VMWCHAR c1 = str[ls - le + i], c2 = ext_ucs2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

static void normalize_dir_path(VMWSTR dst, const VMWCHAR* src, int max_chars) {
    safe_wstrncpy_zero(dst, src, max_chars);
    if (dst[0] >= 'a' && dst[0] <= 'z') dst[0] = dst[0] - 'a' + 'A';
    int len = vm_wstrlen(dst);
    while (len > 0 && (dst[len - 1] == '\\' || dst[len - 1] == '/')) dst[--len] = 0;
}

static void make_canonical_path(VMWSTR dst, const VMWCHAR* folder, const VMWCHAR* filename) {
    normalize_dir_path(dst, folder, 260);
    if (!filename || !filename[0]) return;
    VMWCHAR slash[2] = { '\\', 0 };
    safe_wstrncat_zero(dst, slash, 260);
    safe_wstrncat_zero(dst, filename, 260);
}

static VMUINT8 match_audio_format_ext(const VMWCHAR* name) {
    if (ends_with_ignore_case(name, ".mp3")) return FMT_MP3;
    if (ends_with_ignore_case(name, ".mid") || ends_with_ignore_case(name, ".midi")) return FMT_MIDI;
    if (ends_with_ignore_case(name, ".imy")) return FMT_IMY;
    if (ends_with_ignore_case(name, ".aac")) return FMT_AAC;
    if (ends_with_ignore_case(name, ".amr")) return FMT_AMR;
    if (ends_with_ignore_case(name, ".awb")) return FMT_AWB;
    if (ends_with_ignore_case(name, ".vm"))  return FMT_VM;
    if (ends_with_ignore_case(name, ".wav")) return FMT_WAV;
    return 0;
}

static void utf8_to_ucs2(const VMUINT8* src, int byte_len, VMWCHAR* dst, int max_chars) {
    if (!dst || max_chars <= 0) return;
    int out = 0, i = 0;
    while (i < byte_len && src[i] != 0 && out < max_chars - 1) {
        VMUINT8 b0 = src[i];
        if (b0 < 0x80) { dst[out++] = (VMWCHAR)b0; i++; }
        else if ((b0 & 0xE0) == 0xC0 && i + 1 < byte_len) {
            dst[out++] = (VMWCHAR)(((VMUINT16)(b0 & 0x1F) << 6) | (src[i + 1] & 0x3F)); i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < byte_len) {
            dst[out++] = (VMWCHAR)(((VMUINT16)(b0 & 0x0F) << 12) | ((VMUINT16)(src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F)); i += 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < byte_len) {
            dst[out++] = (VMWCHAR)'?'; i += 4;
        } else { dst[out++] = (VMWCHAR)b0; i++; }
    }
    dst[out] = 0;
}

static void trim_ucs2(VMWCHAR* str) {
    if (!str) return;
    int len = vm_wstrlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) str[--len] = 0;
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t') start++;
    if (start > 0) {
        int i = 0;
        while (str[start + i]) { str[i] = str[start + i]; i++; }
        str[i] = 0;
    }
}

static void utf8_markup_to_ucs2(const char* utf8_src, VMWCHAR* dst, int max_chars, bool parse_braces) {
    utf8_to_ucs2((const VMUINT8*)utf8_src, (int)strlen(utf8_src), dst, max_chars);
    if (!parse_braces) return;
    for (int i = 0; dst[i] != 0; i++) {
        if (dst[i] == '{') dst[i] = STYLE_HL_ON;
        else if (dst[i] == '}') dst[i] = STYLE_HL_OFF;
    }
}

static int measure_styled_width(const VMWCHAR* s, int len) {
    VMWCHAR vis[TEXT_LINE_STRIDE];
    int n = 0;
    for (int i = 0; i < len && s[i] != 0 && n < TEXT_LINE_STRIDE - 1; i++) {
        if (s[i] != STYLE_HL_ON && s[i] != STYLE_HL_OFF) vis[n++] = s[i];
    }
    vis[n] = 0;
    return (n > 0) ? vm_graphic_get_string_width(vis) : 0;
}

static void draw_styled_ucs2_line(int x, int y, const VMWCHAR* line, vm_graphic_color normal_col, vm_graphic_color hl_col) {
    if (!line || !line[0]) return;
    VMWCHAR vis_all[TEXT_LINE_STRIDE];
    int vn = 0;
    for (int k = 0; line[k] != 0 && vn < TEXT_LINE_STRIDE - 1; k++) {
        if (line[k] != STYLE_HL_ON && line[k] != STYLE_HL_OFF) vis_all[vn++] = line[k];
    }
    vis_all[vn] = 0;
    if (vn == 0) return;

    VMINT base = vm_graphic_get_string_baseline(vis_all);
    if (base < g_app.font_baseline) base = g_app.font_baseline;

    int cur_x = x, i = 0;
    bool hl = false;
    while (line[i] != 0) {
        if (line[i] == STYLE_HL_ON) { hl = true; i++; continue; }
        if (line[i] == STYLE_HL_OFF) { hl = false; i++; continue; }
        VMWCHAR seg[TEXT_LINE_STRIDE];
        int sn = 0;
        while (line[i] != 0 && line[i] != STYLE_HL_ON && line[i] != STYLE_HL_OFF && sn < TEXT_LINE_STRIDE - 1) {
            seg[sn++] = line[i++];
        }
        seg[sn] = 0;
        if (sn > 0) {
            vm_graphic_color c = hl ? hl_col : normal_col;
            vm_graphic_setcolor(&c);
            vm_graphic_textout_to_layer_by_baseline(g_app.layer_hdl, cur_x, y, seg, sn, base);
            cur_x += vm_graphic_get_string_width(seg);
        }
    }
}

static int wrap_rich_text(const VMWCHAR* src, int max_w_px, int max_lines, bool ellipsis_on_trunc,
                          VMWCHAR* out_buf, int line_stride) {
    if (!src || !out_buf || max_w_px <= 10 || max_lines <= 0 || line_stride < 8) return 0;

    int total_len = vm_wstrlen((VMWSTR)src);
    int cur_pos = 0, line_count = 0;
    bool hl_active = false;
    VMWCHAR dots[4] = { '.', '.', '.', 0 };
    int dots_w = vm_graphic_get_string_width(dots);

    while (cur_pos < total_len && line_count < max_lines) {
        VMWCHAR* out_line = out_buf + line_count * line_stride;

        if (src[cur_pos] == '\n') { out_line[0] = 0; line_count++; cur_pos++; continue; }

        while (cur_pos < total_len && (src[cur_pos] == ' ' || src[cur_pos] == STYLE_HL_ON || src[cur_pos] == STYLE_HL_OFF)) {
            if (src[cur_pos] == STYLE_HL_ON) hl_active = true;
            else if (src[cur_pos] == STYLE_HL_OFF) hl_active = false;
            cur_pos++;
        }
        if (cur_pos >= total_len) break;
        if (src[cur_pos] == '\n') { out_line[0] = 0; line_count++; cur_pos++; continue; }

        int end_pos = cur_pos, last_space = -1, vis_chars = 0;
        bool hit_hard_break = false;

        while (end_pos < total_len && (end_pos - cur_pos) < line_stride - 6) {
            VMWCHAR ch = src[end_pos];
            if (ch == '\n') { hit_hard_break = true; break; }
            if (ch == STYLE_HL_ON || ch == STYLE_HL_OFF) { end_pos++; continue; }
            if (measure_styled_width(src + cur_pos, end_pos - cur_pos + 1) > max_w_px) break;
            if (ch == ' ') last_space = end_pos;
            vis_chars++;
            end_pos++;
        }

        if (ellipsis_on_trunc && line_count == max_lines - 1 && !hit_hard_break && end_pos < total_len) {
            int fit_end = end_pos;
            while (fit_end > cur_pos && measure_styled_width(src + cur_pos, fit_end - cur_pos) + dots_w > max_w_px) fit_end--;
            int k = 0;
            if (hl_active && k < line_stride - 5) out_line[k++] = STYLE_HL_ON;
            for (int p = cur_pos; p < fit_end && k < line_stride - 5; p++) {
                out_line[k++] = src[p];
                if (src[p] == STYLE_HL_ON) hl_active = true;
                else if (src[p] == STYLE_HL_OFF) hl_active = false;
            }
            if (hl_active && k < line_stride - 4) out_line[k++] = STYLE_HL_OFF;
            out_line[k++] = '.'; out_line[k++] = '.'; out_line[k++] = '.'; out_line[k] = 0;
            line_count++;
            break;
        }

        int break_pos = end_pos;
        if (!hit_hard_break && end_pos < total_len && (end_pos - cur_pos) < line_stride - 6) {
            if (last_space > cur_pos) break_pos = last_space;
            else if (vis_chars == 0 && end_pos == cur_pos) break_pos = cur_pos + 1;
        }

        int k = 0;
        if (hl_active && k < line_stride - 2) out_line[k++] = STYLE_HL_ON;
        for (int p = cur_pos; p < break_pos && k < line_stride - 2; p++) {
            out_line[k++] = src[p];
            if (src[p] == STYLE_HL_ON) hl_active = true;
            else if (src[p] == STYLE_HL_OFF) hl_active = false;
        }
        if (hl_active && k < line_stride - 1) out_line[k++] = STYLE_HL_OFF;
        out_line[k] = 0;
        line_count++;

        cur_pos = break_pos;
        while (cur_pos < total_len && src[cur_pos] == ' ') cur_pos++;
        if (cur_pos < total_len && src[cur_pos] == '\n') cur_pos++;
    }
    return line_count;
}

// ============================================================================
// MODULE 4: On-Demand ID3 Tag Subsystem
// ============================================================================
static void decode_id3_text_frame(const VMUINT8* payload, int len, VMWCHAR* out, int max_chars) {
    if (!payload || len <= 0 || !out || max_chars <= 0) return;
    out[0] = 0;
    VMUINT8 enc = payload[0];
    const VMUINT8* txt = payload + 1;
    int tlen = len - 1;
    if (tlen <= 0) return;

    if (enc == 0) {
        int n = 0;
        for (int i = 0; i < tlen && txt[i] != 0 && n < max_chars - 1; i++) out[n++] = (VMWCHAR)txt[i];
        out[n] = 0;
    } else if (enc == 1 && tlen >= 2) {
        bool be = (txt[0] == 0xFE && txt[1] == 0xFF);
        int start = ((txt[0] == 0xFE && txt[1] == 0xFF) || (txt[0] == 0xFF && txt[1] == 0xFE)) ? 2 : 0;
        int n = 0;
        for (int i = start; i + 1 < tlen && n < max_chars - 1; i += 2) {
            VMWCHAR ch = be ? (((VMWCHAR)txt[i] << 8) | txt[i + 1]) : (((VMWCHAR)txt[i + 1] << 8) | txt[i]);
            if (ch == 0) break;
            out[n++] = ch;
        }
        out[n] = 0;
    } else if (enc == 2) {
        int n = 0;
        for (int i = 0; i + 1 < tlen && n < max_chars - 1; i += 2) {
            VMWCHAR ch = ((VMWCHAR)txt[i] << 8) | txt[i + 1];
            if (ch == 0) break;
            out[n++] = ch;
        }
        out[n] = 0;
    } else if (enc == 3) {
        utf8_to_ucs2(txt, tlen, out, max_chars);
    }
    trim_ucs2(out);
}

static VMUINT16 ucs2_parse_uint(const VMWCHAR* str) {
    VMUINT val = 0; int i = 0;
    if (!str) return 0;
    while (str[i] == ' ') i++;
    while (str[i] >= '0' && str[i] <= '9') {
        val = val * 10 + (VMUINT)(str[i] - '0');
        i++; if (val > 65000) break;
    }
    return (VMUINT16)val;
}

static void parse_id3_into_slot(const TrackBase* tb, Id3Tag* tag) {
    if (!tb || !tag || tag->parsed) return;
    memset(tag, 0, sizeof(Id3Tag));
    tag->parsed = true;

    VMWCHAR full_path[260];
    make_canonical_path(full_path, g_app.lib_dir, tb->filename);
    VMFILE f = vm_file_open(full_path, MODE_READ, 1);
    if (f < 0) return;

    VMUINT8 hdr[10]; VMUINT nread = 0;
    if (vm_file_read(f, hdr, 10, &nread) > 0 && nread == 10 &&
        hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && hdr[3] >= 2 && hdr[3] <= 4) {
        VMUINT8 ver = hdr[3], flags = hdr[5];
        VMUINT tag_size = ((VMUINT)(hdr[6] & 0x7F) << 21) | ((VMUINT)(hdr[7] & 0x7F) << 14) |
                          ((VMUINT)(hdr[8] & 0x7F) << 7)  | ((VMUINT)(hdr[9] & 0x7F));
        VMUINT pos = 10, tag_end = 10 + tag_size;
        if (tag_end > tb->filesize) tag_end = tb->filesize;

        if ((flags & 0x40) && ver >= 3 && pos + 4 <= tag_end) {
            VMUINT8 ext[4];
            if (vm_file_read(f, ext, 4, &nread) > 0 && nread == 4) {
                VMUINT ext_sz = (ver == 4)
                    ? (((VMUINT)(ext[0] & 0x7F) << 21) | ((VMUINT)(ext[1] & 0x7F) << 14) | ((VMUINT)(ext[2] & 0x7F) << 7) | ((VMUINT)(ext[3] & 0x7F)))
                    : (((VMUINT)ext[0] << 24) | ((VMUINT)ext[1] << 16) | ((VMUINT)ext[2] << 8) | (VMUINT)ext[3]);
                pos += (ver == 4) ? ext_sz : (4 + ext_sz);
                vm_file_seek(f, (VMINT)pos, BASE_BEGIN);
            }
        }

        VMUINT8 fhdr[10], payload[256];
        int fhdr_len = (ver == 2) ? 6 : 10;
        while (pos + (VMUINT)fhdr_len <= tag_end) {
            if (vm_file_read(f, fhdr, fhdr_len, &nread) <= 0 || (int)nread != fhdr_len) break;
            if (fhdr[0] == 0) break;
            VMUINT fsize = (ver == 2)
                ? (((VMUINT)fhdr[3] << 16) | ((VMUINT)fhdr[4] << 8) | (VMUINT)fhdr[5])
                : ((ver == 4)
                    ? (((VMUINT)(fhdr[4] & 0x7F) << 21) | ((VMUINT)(fhdr[5] & 0x7F) << 14) | ((VMUINT)(fhdr[6] & 0x7F) << 7) | ((VMUINT)(fhdr[7] & 0x7F)))
                    : (((VMUINT)fhdr[4] << 24) | ((VMUINT)fhdr[5] << 16) | ((VMUINT)fhdr[6] << 8) | (VMUINT)fhdr[7]));
            pos += (VMUINT)fhdr_len;
            if (fsize == 0 || pos + fsize > tag_end) break;

            int want = 0;
            if (ver == 2) {
                if (memcmp(fhdr, "TT2", 3) == 0) want = 1; else if (memcmp(fhdr, "TP1", 3) == 0) want = 2;
                else if (memcmp(fhdr, "TAL", 3) == 0) want = 3; else if (memcmp(fhdr, "TYE", 3) == 0) want = 4; else if (memcmp(fhdr, "TRK", 3) == 0) want = 5;
            } else {
                if (memcmp(fhdr, "TIT2", 4) == 0) want = 1; else if (memcmp(fhdr, "TPE1", 4) == 0) want = 2;
                else if (memcmp(fhdr, "TALB", 4) == 0) want = 3; else if (memcmp(fhdr, "TYER", 4) == 0 || memcmp(fhdr, "TDRC", 4) == 0) want = 4; else if (memcmp(fhdr, "TRCK", 4) == 0) want = 5;
            }
            if (want > 0) {
                VMUINT rlen = (fsize < sizeof(payload)) ? fsize : sizeof(payload);
                if (vm_file_read(f, payload, rlen, &nread) > 0 && nread > 1) {
                    VMWCHAR tmp[48];
                    if (want == 1 && !tag->title[0]) decode_id3_text_frame(payload, (int)nread, tag->title, 48);
                    else if (want == 2 && !tag->artist[0]) decode_id3_text_frame(payload, (int)nread, tag->artist, 36);
                    else if (want == 3 && !tag->album[0]) decode_id3_text_frame(payload, (int)nread, tag->album, 36);
                    else if (want == 4 && !tag->year) { decode_id3_text_frame(payload, (int)nread, tmp, 48); tag->year = ucs2_parse_uint(tmp); }
                    else if (want == 5 && !tag->track_num) { decode_id3_text_frame(payload, (int)nread, tmp, 48); tag->track_num = ucs2_parse_uint(tmp); }
                }
            }
            pos += fsize;
            if (tag->title[0] && tag->artist[0] && tag->album[0] && tag->year && tag->track_num) break;
            vm_file_seek(f, (VMINT)pos, BASE_BEGIN);
        }
    }

    if (tb->filesize >= 128 && (!tag->title[0] || !tag->artist[0] || !tag->album[0] || !tag->year || !tag->track_num)) {
        if (vm_file_seek(f, (VMINT)(tb->filesize - 128), BASE_BEGIN) == 0) {
            VMUINT8 v1[128];
            if (vm_file_read(f, v1, 128, &nread) > 0 && nread == 128 && v1[0] == 'T' && v1[1] == 'A' && v1[2] == 'G') {
                char tc[32];
                if (!tag->title[0])  { memcpy(tc, v1 + 3, 30);  tc[30] = 0; safe_ascii_to_ucs2(tag->title, 48, tc);  trim_ucs2(tag->title); }
                if (!tag->artist[0]) { memcpy(tc, v1 + 33, 30); tc[30] = 0; safe_ascii_to_ucs2(tag->artist, 36, tc); trim_ucs2(tag->artist); }
                if (!tag->album[0])  { memcpy(tc, v1 + 63, 30); tc[30] = 0; safe_ascii_to_ucs2(tag->album, 36, tc);  trim_ucs2(tag->album); }
                if (!tag->year)      { memcpy(tc, v1 + 93, 4);  tc[4] = 0;  tag->year = (VMUINT16)atoi(tc); }
                if (!tag->track_num && v1[125] == 0 && v1[126] != 0) tag->track_num = (VMUINT16)v1[126];
            }
        }
    }
    vm_file_close(f);
    if (tag->title[0] || tag->artist[0] || tag->album[0] || tag->year || tag->track_num) tag->has_id3 = true;
}

static void id3_unload_all() {
    if (g_app.id3_tags) { vm_free(g_app.id3_tags); g_app.id3_tags = NULL; g_app.id3_count = 0; }
    for (int i = 0; i < g_app.track_count; i++) g_app.tracks[i].id3_slot = 0xFFFF;
}

static bool id3_load_all() {
    if (g_app.track_count <= 0) return true;
    if (g_app.id3_tags && g_app.id3_count == g_app.track_count) return true;
    id3_unload_all();
    g_app.id3_tags = (Id3Tag*)vm_calloc(g_app.track_count * (int)sizeof(Id3Tag));
    if (!g_app.id3_tags) return false;
    g_app.id3_count = g_app.track_count;
    for (int i = 0; i < g_app.track_count; i++) {
        g_app.tracks[i].id3_slot = (VMUINT16)i;
        parse_id3_into_slot(&g_app.tracks[i], &g_app.id3_tags[i]);
    }
    return true;
}

static const Id3Tag* get_track_id3(const TrackBase* tb) {
    if (!tb || !g_app.id3_tags || tb->id3_slot >= (VMUINT16)g_app.id3_count) return NULL;
    return &g_app.id3_tags[tb->id3_slot];
}

static void get_track_display_name(int idx, VMWCHAR* out, int max_chars) {
    if (idx < 0 || idx >= g_app.track_count || !out || max_chars <= 0) return;
    const TrackBase* tb = &g_app.tracks[idx];
    const Id3Tag* tag = g_app.id3_mode ? get_track_id3(tb) : NULL;
    if (!tag || !tag->has_id3 || (!tag->title[0] && !tag->artist[0])) { safe_wstrncpy_zero(out, tb->filename, max_chars); return; }
    if (tag->title[0] && tag->artist[0]) {
        VMWCHAR sep[8]; safe_ascii_to_ucs2(sep, 8, " - ");
        safe_wstrncpy_zero(out, tag->title, max_chars); safe_wstrncat_zero(out, sep, max_chars); safe_wstrncat_zero(out, tag->artist, max_chars);
    } else safe_wstrncpy_zero(out, tag->title[0] ? tag->title : tag->artist, max_chars);
}

// ============================================================================
// MODULE 5: Dynamic Track Library, Recursive Scanner & Sorting
// ============================================================================
static bool is_entry_directory(const VMWCHAR* full_path, VMUINT filesize) {
    VMINT attr = vm_file_get_attributes((VMWSTR)full_path);
    if (attr >= 0) {
        if (attr & VM_FS_ATTR_DIR) return true;
        return false;
    }
    if (filesize == 0) {
        VMWCHAR sub_pat[260], wild[8];
        safe_ascii_to_ucs2(wild, 8, "*.*");
        make_canonical_path(sub_pat, full_path, wild);
        vm_fileinfo_ext sub_info;
        VMINT sh = vm_find_first_ext(sub_pat, &sub_info);
        if (sh >= 0) { vm_find_close_ext(sh); return true; }
    }
    return false;
}

static void tracks_unload() {
    id3_unload_all();
    if (g_app.tracks) { vm_free(g_app.tracks); g_app.tracks = NULL; }
    g_app.track_count = 0; g_app.track_capacity = 0;
}

static bool tracks_push_back(const VMWCHAR* fname, VMUINT fsize, VMUINT8 fmt) {
    if (g_app.track_count >= MAX_TRACKS) return false;
    if (g_app.track_count >= g_app.track_capacity) {
        int new_cap = g_app.track_capacity + TRACK_ALLOC_STEP; if (new_cap > MAX_TRACKS) new_cap = MAX_TRACKS;
        TrackBase* nb = g_app.tracks
            ? (TrackBase*)vm_realloc(g_app.tracks, new_cap * (int)sizeof(TrackBase))
            : (TrackBase*)vm_malloc(new_cap * (int)sizeof(TrackBase));
        if (!nb) return false;
        g_app.tracks = nb; g_app.track_capacity = new_cap;
    }
    TrackBase* t = &g_app.tracks[g_app.track_count++];
    safe_wstrncpy_zero(t->filename, fname, 128);
    t->filesize = fsize; t->format = fmt; t->id3_slot = 0xFFFF;
    return true;
}

static int compare_tracks(const TrackBase* a, const TrackBase* b) {
    int cmp = 0;
    const Id3Tag* ia = get_track_id3(a), *ib = get_track_id3(b);
    switch (g_app.sort_mode) {
        case SORT_TITLE:
            cmp = wstr_icmp((ia && ia->title[0]) ? ia->title : a->filename, (ib && ib->title[0]) ? ib->title : b->filename);
            break;
        case SORT_ARTIST:
            cmp = wstr_icmp((ia && ia->artist[0]) ? ia->artist : a->filename, (ib && ib->artist[0]) ? ib->artist : b->filename);
            break;
        case SORT_ALBUM:
            cmp = wstr_icmp((ia && ia->album[0]) ? ia->album : a->filename, (ib && ib->album[0]) ? ib->album : b->filename);
            if (cmp == 0) {
                VMUINT16 ta = (ia && ia->track_num) ? ia->track_num : 9999;
                VMUINT16 tb = (ib && ib->track_num) ? ib->track_num : 9999;
                if (ta != tb) cmp = (ta < tb) ? -1 : 1;
            }
            break;
        case SORT_YEAR: {
            VMUINT16 ya = (ia && ia->year) ? ia->year : 9999, yb = (ib && ib->year) ? ib->year : 9999;
            if (ya != yb) cmp = (ya < yb) ? -1 : 1;
            break;
        }
        case SORT_FILESIZE:
            if (a->filesize != b->filesize) cmp = (a->filesize < b->filesize) ? -1 : 1;
            break;
        case SORT_FORMAT:
            if (a->format != b->format) cmp = (a->format < b->format) ? -1 : 1;
            break;
        case SORT_FILENAME:
        default: break;
    }
    if (cmp == 0) cmp = wstr_icmp(a->filename, b->filename);
    return cmp;
}

static void sort_track_library() {
    if (g_app.track_count <= 1 || !g_app.tracks) return;
    bool needs_id3 = (g_app.sort_mode == SORT_TITLE || g_app.sort_mode == SORT_ARTIST ||
                      g_app.sort_mode == SORT_ALBUM || g_app.sort_mode == SORT_YEAR);
    if (needs_id3 || g_app.id3_mode) id3_load_all();

    VMWCHAR sel_name[128], act_name[128]; sel_name[0] = 0; act_name[0] = 0;
    if (g_app.selected_track >= 0 && g_app.selected_track < g_app.track_count) safe_wstrncpy_zero(sel_name, g_app.tracks[g_app.selected_track].filename, 128);
    if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) safe_wstrncpy_zero(act_name, g_app.tracks[g_app.active_track_idx].filename, 128);

    for (int i = 1; i < g_app.track_count; i++) {
        TrackBase key = g_app.tracks[i]; int j = i - 1;
        while (j >= 0 && compare_tracks(&g_app.tracks[j], &key) > 0) { g_app.tracks[j + 1] = g_app.tracks[j]; j--; }
        g_app.tracks[j + 1] = key;
    }

    if (sel_name[0]) { for (int i = 0; i < g_app.track_count; i++) if (vm_wstrcmp(g_app.tracks[i].filename, sel_name) == 0) { g_app.selected_track = i; break; } }
    if (act_name[0]) { for (int i = 0; i < g_app.track_count; i++) if (vm_wstrcmp(g_app.tracks[i].filename, act_name) == 0) { g_app.active_track_idx = i; break; } }
    if (g_app.karaoke && g_app.karaoke->lrc_track_idx >= 0 && act_name[0]) g_app.karaoke->lrc_track_idx = g_app.active_track_idx;
    if (!g_app.id3_mode) id3_unload_all();
}

static void scan_directory_tree(const VMWCHAR* root_dir, bool recursive) {
    if (!root_dir || !root_dir[0]) return;

    typedef VMWCHAR RelDirItem[128];
    RelDirItem* dir_queue = NULL;
    int q_head = 0, q_tail = 0, q_cap = 1;

    if (recursive) {
        dir_queue = (RelDirItem*)vm_calloc(MAX_SCAN_DIRS * (int)sizeof(RelDirItem));
        if (dir_queue) { q_cap = MAX_SCAN_DIRS; }
    }

    VMWCHAR single_root[128];
    single_root[0] = 0;
    q_tail = 1;

    VMWCHAR wild[8], slash[2];
    safe_ascii_to_ucs2(wild, 8, "*.*");
    slash[0] = '\\'; slash[1] = 0;
    vm_fileinfo_ext info;

    while (q_head < q_tail && g_app.track_count < MAX_TRACKS) {
        const VMWCHAR* rel_dir = dir_queue ? dir_queue[q_head] : single_root;
        q_head++;

        VMWCHAR cur_dir[260], search_pattern[260];
        if (rel_dir[0] == 0) normalize_dir_path(cur_dir, root_dir, 260);
        else make_canonical_path(cur_dir, root_dir, rel_dir);

        make_canonical_path(search_pattern, cur_dir, wild);
        VMINT handle = vm_find_first_ext(search_pattern, &info);
        if (handle < 0) continue;

        do {
            if (info.filefullname[0] == '.') continue;
            VMWCHAR full_path[260], rel_entry[128];
            make_canonical_path(full_path, cur_dir, info.filefullname);

            if (rel_dir[0] == 0) safe_wstrncpy_zero(rel_entry, info.filefullname, 128);
            else {
                if (vm_wstrlen((VMWSTR)rel_dir) + 1 + vm_wstrlen(info.filefullname) >= 127) continue;
                safe_wstrncpy_zero(rel_entry, rel_dir, 128);
                safe_wstrncat_zero(rel_entry, slash, 128);
                safe_wstrncat_zero(rel_entry, info.filefullname, 128);
            }

            if (is_entry_directory(full_path, info.filesize)) {
                if (recursive && dir_queue && q_tail < q_cap && vm_wstrlen(rel_entry) < 120) {
                    safe_wstrncpy_zero(dir_queue[q_tail++], rel_entry, 128);
                }
                continue;
            }

            VMUINT8 fmt = match_audio_format_ext(info.filefullname);
            if ((fmt & g_app.enabled_formats) != 0) {
                VMFILE test_f = vm_file_open(full_path, MODE_READ, 1);
                if (test_f >= 0) {
                    vm_file_close(test_f);
                    if (!tracks_push_back(rel_entry, info.filesize, fmt)) break;
                }
            }
        } while (vm_find_next_ext(handle, &info) == 0 && g_app.track_count < MAX_TRACKS);
        vm_find_close_ext(handle);
    }

    if (dir_queue) vm_free(dir_queue);
}

static void refresh_track_library_dir(const VMWCHAR* new_dir, bool recursive) {
    VMWCHAR prev_active_full[260];
    prev_active_full[0] = 0;
    if (g_app.play_state != STATE_STOPPED && g_app.active_track_idx >= 0 &&
        g_app.active_track_idx < g_app.track_count && g_app.tracks) {
        make_canonical_path(prev_active_full, g_app.lib_dir, g_app.tracks[g_app.active_track_idx].filename);
    }

    if (new_dir && new_dir[0]) {
        normalize_dir_path(g_app.lib_dir, new_dir, 260);
        g_app.lib_recursive = recursive;
    } else if (!g_app.lib_dir[0]) {
        normalize_dir_path(g_app.lib_dir, g_app.app_dir, 260);
    }

    tracks_unload();
    g_app.selected_track = 0; g_app.scroll_offset = 0; g_app.sel_char_offset = 0;
    g_app.active_track_idx = -1;

    if (g_app.lib_dir[0]) scan_directory_tree(g_app.lib_dir, g_app.lib_recursive);
    sort_track_library();

    if (prev_active_full[0]) {
        bool found = false;
        VMWCHAR cand[260];
        for (int i = 0; i < g_app.track_count; i++) {
            make_canonical_path(cand, g_app.lib_dir, g_app.tracks[i].filename);
            if (wstr_icmp(cand, prev_active_full) == 0) {
                g_app.active_track_idx = i;
                found = true;
                break;
            }
        }
        if (!found) stop_playback();
    }
}

static void refresh_track_library() {
    refresh_track_library_dir(NULL, g_app.lib_recursive);
}

// ============================================================================
// MODULE 6: On-Demand Folder Viewer Mode ("Play from Folder")
// ============================================================================
static void folder_viewer_unload() {
    if (g_app.folder_viewer) {
        if (g_app.folder_viewer->entries) { vm_free(g_app.folder_viewer->entries); g_app.folder_viewer->entries = NULL; }
        vm_free(g_app.folder_viewer);
        g_app.folder_viewer = NULL;
    }
}

static bool folder_viewer_push_entry(const VMWCHAR* name, VMUINT8 type) {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv || fv->entry_count >= MAX_FOLDER_ENTRIES) return false;
    if (fv->entry_count >= fv->entry_cap) {
        int new_cap = fv->entry_cap + FOLDER_ALLOC_STEP;
        if (new_cap > MAX_FOLDER_ENTRIES) new_cap = MAX_FOLDER_ENTRIES;
        FolderEntry* nb = fv->entries
            ? (FolderEntry*)vm_realloc(fv->entries, new_cap * (int)sizeof(FolderEntry))
            : (FolderEntry*)vm_malloc(new_cap * (int)sizeof(FolderEntry));
        if (!nb) return false;
        fv->entries = nb; fv->entry_cap = new_cap;
    }
    FolderEntry* e = &fv->entries[fv->entry_count++];
    safe_wstrncpy_zero(e->name, name, 128);
    e->type = type;
    return true;
}

static void folder_viewer_scan() {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv) return;
    fv->entry_count = 0; fv->cursor = 0; fv->scroll = 0;

    if (fv->at_drive_list) {
        VMWCHAR wlabel[64];
        safe_ascii_to_ucs2(wlabel, 64, "[App Folder]");
        safe_wstrncpy_zero(fv->drive_paths[0], g_app.app_dir, 260);
        folder_viewer_push_entry(wlabel, FENT_DRIVE);

        int dslot = 1;
        VMINT sys_drv = vm_get_system_driver();
        if (sys_drv >= 0) {
            char lb[32], dp[8];
            sprintf(lb, "[Phone] %c:\\", (char)sys_drv);
            sprintf(dp, "%c:", (char)sys_drv);
            safe_ascii_to_ucs2(wlabel, 64, lb);
            safe_ascii_to_ucs2(fv->drive_paths[dslot], 260, dp);
            folder_viewer_push_entry(wlabel, FENT_DRIVE);
            dslot++;
        }
        VMINT rem_drv = vm_get_removable_driver();
        if (rem_drv >= 0 && rem_drv != sys_drv && dslot < 3) {
            char lb[32], dp[8];
            sprintf(lb, "[Card] %c:\\", (char)rem_drv);
            sprintf(dp, "%c:", (char)rem_drv);
            safe_ascii_to_ucs2(wlabel, 64, lb);
            safe_ascii_to_ucs2(fv->drive_paths[dslot], 260, dp);
            folder_viewer_push_entry(wlabel, FENT_DRIVE);
            dslot++;
        }
        return;
    }

    VMWCHAR wtmp[64];
    if (fv->lrc_mode) {
        safe_ascii_to_ucs2(wtmp, 64, "<Auto-Match Song>");
        folder_viewer_push_entry(wtmp, FENT_LRC_AUTO);
        safe_ascii_to_ucs2(wtmp, 64, "<None (Clear LRC)>");
        folder_viewer_push_entry(wtmp, FENT_LRC_NONE);
        safe_ascii_to_ucs2(wtmp, 64, ".. (Up)");
        folder_viewer_push_entry(wtmp, FENT_UP);
    } else {
        safe_ascii_to_ucs2(wtmp, 64, ".. (Up)");
        folder_viewer_push_entry(wtmp, FENT_UP);
        // Remove the next 2 lines if you removed ". (This Folder)" per #1:
        safe_ascii_to_ucs2(wtmp, 64, ". (This Folder)");
        folder_viewer_push_entry(wtmp, FENT_THIS);
    }
    int pinned = fv->entry_count;

    VMWCHAR search_pattern[260], wild[8];
    safe_ascii_to_ucs2(wild, 8, "*.*");
    make_canonical_path(search_pattern, fv->cur_path, wild);

    vm_fileinfo_ext info;
    VMINT handle = vm_find_first_ext(search_pattern, &info);
    if (handle >= 0) {
        do {
            if (info.filefullname[0] == '.') continue;
            VMWCHAR full_path[260];
            make_canonical_path(full_path, fv->cur_path, info.filefullname);
            VMINT attr = vm_file_get_attributes(full_path);
            if (attr >= 0 && (attr & VM_FS_ATTR_VOLUME) && !(attr & VM_FS_ATTR_DIR)) continue;

            if (is_entry_directory(full_path, info.filesize)) {
                if (!folder_viewer_push_entry(info.filefullname, FENT_SUBDIR)) break;
            } else if (fv->lrc_mode ? ends_with_ignore_case(info.filefullname, ".lrc")
                                    : (match_audio_format_ext(info.filefullname) != 0)) {
                if (!folder_viewer_push_entry(info.filefullname, FENT_FILE)) break;
            }
        } while (vm_find_next_ext(handle, &info) == 0);
        vm_find_close_ext(handle);
    }

    for (int i = pinned + 1; i < fv->entry_count; i++) {
        FolderEntry key = fv->entries[i];
        int j = i - 1;
        while (j >= pinned) {
            bool swap_needed = false;
            if (fv->entries[j].type > key.type) swap_needed = true;
            else if (fv->entries[j].type == key.type && wstr_icmp(fv->entries[j].name, key.name) > 0) swap_needed = true;
            if (!swap_needed) break;
            fv->entries[j + 1] = fv->entries[j];
            j--;
        }
        fv->entries[j + 1] = key;
    }
}

static bool folder_viewer_load(bool lrc_mode) {
    folder_viewer_unload();
    g_app.folder_viewer = (FolderViewerState*)vm_calloc((int)sizeof(FolderViewerState));
    if (!g_app.folder_viewer) return false;
    g_app.folder_viewer->lrc_mode = lrc_mode;

    VMWCHAR start_dir[260];
    normalize_dir_path(start_dir, g_app.lib_dir[0] ? g_app.lib_dir : g_app.app_dir, 260);
    if (lrc_mode) {
        int target = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
        if (target >= 0 && target < g_app.track_count && g_app.tracks) {
            VMWCHAR sub[128];
            safe_wstrncpy_zero(sub, g_app.tracks[target].filename, 128);
            int ls = -1;
            for (int i = vm_wstrlen(sub) - 1; i >= 0; i--) if (sub[i] == '\\' || sub[i] == '/') { ls = i; break; }
            if (ls > 0) {
                sub[ls] = 0;
                make_canonical_path(start_dir, g_app.lib_dir, sub);
            }
        }
    }
    normalize_dir_path(g_app.folder_viewer->cur_path, start_dir, 260);
    g_app.folder_viewer->at_drive_list = (g_app.folder_viewer->cur_path[0] == 0);
    folder_viewer_scan();
    return true;
}

static void folder_viewer_go_up() {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv || fv->at_drive_list) return;
    int len = vm_wstrlen(fv->cur_path);
    int last_sep = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (fv->cur_path[i] == '\\' || fv->cur_path[i] == '/') { last_sep = i; break; }
    }
    if (last_sep <= 0) fv->at_drive_list = true;
    else fv->cur_path[last_sep] = 0;
    folder_viewer_scan();
}

static void folder_viewer_open_selected() {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv || fv->cursor < 0 || fv->cursor >= fv->entry_count) return;
    const FolderEntry* e = &fv->entries[fv->cursor];
    if (e->type == FENT_UP) { folder_viewer_go_up(); }
    else if (e->type == FENT_DRIVE) {
        if (fv->cursor >= 0 && fv->cursor < 3) {
            normalize_dir_path(fv->cur_path, fv->drive_paths[fv->cursor], 260);
            fv->at_drive_list = false;
            folder_viewer_scan();
        }
    } else if (e->type == FENT_SUBDIR) {
        VMWCHAR next_path[260];
        make_canonical_path(next_path, fv->cur_path, e->name);
        safe_wstrncpy_zero(fv->cur_path, next_path, 260);
        fv->at_drive_list = false;
        folder_viewer_scan();
    }
}

static void folder_viewer_choose_selected(bool recursive) {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv) return;
    VMWCHAR target[260]; target[0] = 0;

    if (fv->cursor >= 0 && fv->cursor < fv->entry_count) {
        const FolderEntry* e = &fv->entries[fv->cursor];
        if (fv->at_drive_list && e->type == FENT_DRIVE && fv->cursor < 3) {
            normalize_dir_path(target, fv->drive_paths[fv->cursor], 260);
        } else if (e->type == FENT_SUBDIR) {
            make_canonical_path(target, fv->cur_path, e->name);
        } else {
            normalize_dir_path(target, fv->cur_path, 260);
        }
    } else {
        normalize_dir_path(target, fv->cur_path, 260);
    }

    if (target[0]) refresh_track_library_dir(target, recursive);

    set_ui_mode(MODE_TRACKLIST);
}

// ============================================================================
// MODULE 7: On-Demand Karaoke, LRC & Screensavers
// ============================================================================
static const int SNOW_COUNTS[4] = { 36, 26, 18, 10 };
static const int SNOW_SIZES[4]  = { 3, 5, 7, 11 };

static unsigned int ss_rand_next = 1234567;
static int ss_rand(int max_val) {
    ss_rand_next = ss_rand_next * 1103515245 + 12345;
    return (int)((ss_rand_next >> 16) % (unsigned int)(max_val > 0 ? max_val : 1));
}

static void change_snow_wind() {
    if (!g_app.karaoke) return;
    g_app.karaoke->ss_snow_wind_x = (ss_rand(640) - 320) * 3 / 2;
    g_app.karaoke->ss_snow_wind_y = (180 + ss_rand(380)) * 3 / 2;
    g_app.karaoke->ss_snow_next_wind_tick = vm_get_tick_count() + (3 + ss_rand(13)) * 60 * 1000;
}

static void init_screensaver_state() {
    if (!g_app.karaoke) return;
    KaraokeState* k = g_app.karaoke; k->ss_frame = 0;
    int sw = g_app.screen_w > 0 ? g_app.screen_w : 240, sh = g_app.screen_h > 0 ? g_app.screen_h : 320;
    for (int i = 0; i < 24; i++) { k->ss_stars[i].x = ss_rand(sw) - sw / 2; k->ss_stars[i].y = ss_rand(sh) - sh / 2; k->ss_stars[i].z = 1 + ss_rand(15); }
    for (int i = 0; i < 16; i++) k->ss_eq_heights[i] = 4;
    k->ss_orb_x = sw / 3; k->ss_orb_y = g_app.header_h + 40; k->ss_orb_dx = 2; k->ss_orb_dy = 1;
    for (int i = 0; i < 20; i++) k->ss_rain_y[i] = ss_rand(sh);
    for (int l = 0; l < 4; l++) for (int i = 0; i < SNOW_COUNTS[l]; i++) { k->ss_snow_x[l][i] = ss_rand(sw) << 8; k->ss_snow_y[l][i] = ss_rand(sh) << 8; }
    change_snow_wind();
}

static void clear_lrc_data() {
    if (!g_app.karaoke) return;
    g_app.karaoke->lrc_count = 0; g_app.karaoke->lrc_file_offset_ms = 0;
    g_app.karaoke->lrc_cur_name[0] = 0; g_app.lrc_user_offset_ms = 0;
}

static void process_lrc_raw_line(const char* raw) {
    if (!g_app.karaoke || !raw || !raw[0]) return;
    if ((VMUINT8)raw[0] == 0xEF && (VMUINT8)raw[1] == 0xBB && (VMUINT8)raw[2] == 0xBF) raw += 3;
    if (strncmp(raw, "[offset:", 8) == 0) { g_app.karaoke->lrc_file_offset_ms = atoi(raw + 8); return; }

    VMINT32 stamps[12]; int stamp_count = 0; const char* p = raw;
    while (*p == '[') {
        const char* close_br = strchr(p, ']'); if (!close_br) break;
        if ((int)(close_br - p - 1) >= 4 && p[1] >= '0' && p[1] <= '9') {
            int mins = 0, secs = 0, ms = 0; const char* q = p + 1;
            while (q < close_br && *q >= '0' && *q <= '9') mins = mins * 10 + (*q++ - '0');
            if (q < close_br && *q == ':') {
                q++;
                while (q < close_br && *q >= '0' && *q <= '9') secs = secs * 10 + (*q++ - '0');
                if (q < close_br && (*q == '.' || *q == ':')) {
                    q++; int d = 0, f = 0;
                    while (q < close_br && *q >= '0' && *q <= '9' && d < 3) { f = f * 10 + (*q++ - '0'); d++; }
                    ms = (d == 1) ? f * 100 : ((d == 2) ? f * 10 : f);
                }
                if (stamp_count < 12) stamps[stamp_count++] = (VMINT32)(mins * 60000 + secs * 1000 + ms);
            }
        }
        p = close_br + 1;
    }
    if (stamp_count == 0) return;
    while (*p == ' ' || *p == '\t') p++;

    VMWCHAR text_w[MAX_LRC_TEXT_LEN];
    utf8_to_ucs2((const VMUINT8*)p, (int)strlen(p), text_w, MAX_LRC_TEXT_LEN); trim_ucs2(text_w);
    KaraokeState* k = g_app.karaoke;
    for (int i = 0; i < stamp_count && k->lrc_count < MAX_LRC_LINES; i++) {
        k->lrc_lines[k->lrc_count].time_ms = stamps[i];
        safe_wstrncpy_zero(k->lrc_lines[k->lrc_count].text, text_w, MAX_LRC_TEXT_LEN);
        k->lrc_count++;
    }
}

static bool load_lrc_file_path(const VMWCHAR* full_path) {
    if (!g_app.karaoke) return false;
    clear_lrc_data(); if (!full_path || !full_path[0]) return false;
    VMFILE f = vm_file_open((VMWSTR)full_path, MODE_READ, 1); if (f < 0) return false;

    char line_buf[320]; int line_pos = 0; VMUINT8 chunk[256]; VMUINT nread = 0;
    while (vm_file_read(f, chunk, sizeof(chunk), &nread) > 0 && nread > 0) {
        for (VMUINT i = 0; i < nread; i++) {
            char c = (char)chunk[i];
            if (c == '\r' || c == '\n') { if (line_pos > 0) { line_buf[line_pos] = 0; process_lrc_raw_line(line_buf); line_pos = 0; } }
            else if (line_pos < (int)sizeof(line_buf) - 1) line_buf[line_pos++] = c;
        }
        if (g_app.karaoke->lrc_count >= MAX_LRC_LINES) break;
    }
    if (line_pos > 0 && g_app.karaoke->lrc_count < MAX_LRC_LINES) { line_buf[line_pos] = 0; process_lrc_raw_line(line_buf); }
    vm_file_close(f);

    KaraokeState* k = g_app.karaoke;
    for (int i = 1; i < k->lrc_count; i++) {
        LrcLine key = k->lrc_lines[i]; int j = i - 1;
        while (j >= 0 && k->lrc_lines[j].time_ms > key.time_ms) { k->lrc_lines[j + 1] = k->lrc_lines[j]; j--; }
        k->lrc_lines[j + 1] = key;
    }
    safe_wstrncpy_zero(k->lrc_cur_name, full_path, 128);
    return (k->lrc_count > 0);
}

static bool load_lrc_file(const VMWCHAR* lrc_filename) {
    if (!lrc_filename || !lrc_filename[0]) return false;
    VMWCHAR full_path[260]; make_canonical_path(full_path, g_app.lib_dir, lrc_filename);
    return load_lrc_file_path(full_path);
}

static void auto_load_lrc_for_track(int track_idx) {
    if (!g_app.karaoke) return;
    clear_lrc_data(); g_app.karaoke->lrc_track_idx = track_idx;
    if (track_idx < 0 || track_idx >= g_app.track_count) return;
    VMWCHAR lrc_name[128], ext[8]; safe_wstrncpy_zero(lrc_name, g_app.tracks[track_idx].filename, 120);
    int dot_pos = -1;
    for (int i = vm_wstrlen(lrc_name) - 1; i >= 0; i--) {
        if (lrc_name[i] == '\\' || lrc_name[i] == '/') break;
        if (lrc_name[i] == '.') { dot_pos = i; break; }
    }
    if (dot_pos > 0) lrc_name[dot_pos] = 0;
    safe_ascii_to_ucs2(ext, 8, ".lrc"); safe_wstrncat_zero(lrc_name, ext, 128);
    load_lrc_file(lrc_name);
}

static void karaoke_unload() { if (g_app.karaoke) { vm_free(g_app.karaoke); g_app.karaoke = NULL; } }

static bool karaoke_load(int target_track) {
    if (!g_app.karaoke) {
        g_app.karaoke = (KaraokeState*)vm_calloc((int)sizeof(KaraokeState));
        if (!g_app.karaoke) return false;
        g_app.karaoke->lrc_track_idx = -1; init_screensaver_state();
    }
    if (g_app.karaoke->lrc_track_idx != target_track) auto_load_lrc_for_track(target_track);
    return true;
}

static void update_screensaver_state() {
    if (!g_app.karaoke) return;
    KaraokeState* k = g_app.karaoke; k->ss_frame++;
    int sw = g_app.screen_w > 0 ? g_app.screen_w : 240, sh = g_app.screen_h > 0 ? g_app.screen_h : 320;
    int top_y = g_app.header_h + 2, bot_y = sh - g_app.footer_h - 2;
    int vh = (bot_y - top_y > 20) ? (bot_y - top_y) : 20;

    if (g_app.screensaver_id == SS_STARFIELD) {
        for (int i = 0; i < 24; i++) if (--k->ss_stars[i].z <= 0) { k->ss_stars[i].x = ss_rand(sw) - sw / 2; k->ss_stars[i].y = ss_rand(vh) - vh / 2; k->ss_stars[i].z = 16; }
    } else if (g_app.screensaver_id == SS_EQUALIZER) {
        int max_bar = (vh / 2 > 10) ? (vh / 2) : 10;
        for (int i = 0; i < 16; i++) k->ss_eq_heights[i] = (g_app.play_state == STATE_PLAYING) ? ((k->ss_eq_heights[i] + 4 + ss_rand(max_bar)) / 2) : ((k->ss_eq_heights[i] > 3) ? (k->ss_eq_heights[i] - 2) : 2);
    } else if (g_app.screensaver_id == SS_BOUNCING_ORB) {
        int r = 15; k->ss_orb_x += k->ss_orb_dx; k->ss_orb_y += k->ss_orb_dy;
        if (k->ss_orb_x < r) { k->ss_orb_x = r; k->ss_orb_dx = -k->ss_orb_dx; }
        if (k->ss_orb_x > sw - 1 - r) { k->ss_orb_x = sw - 1 - r; k->ss_orb_dx = -k->ss_orb_dx; }
        if (k->ss_orb_y < top_y + r) { k->ss_orb_y = top_y + r; k->ss_orb_dy = -k->ss_orb_dy; }
        if (k->ss_orb_y > bot_y - 1 - r) { k->ss_orb_y = bot_y - 1 - r; k->ss_orb_dy = -k->ss_orb_dy; }
    } else if (g_app.screensaver_id == SS_DIGITAL_RAIN) {
        for (int i = 0; i < 20; i++) { k->ss_rain_y[i] += 4 + (i % 3) * 2; if (k->ss_rain_y[i] > bot_y) k->ss_rain_y[i] = top_y - ss_rand(30); }
    } else if (g_app.screensaver_id == SS_BOKEH_SNOW) {
        if ((VMINT)(vm_get_tick_count() - k->ss_snow_next_wind_tick) >= 0) change_snow_wind();
        int wFP = sw << 8, topFP = top_y << 8, botFP = bot_y << 8;
        for (int l = 0; l < 4; l++) {
            int mult = 65 + l * 35, dx = (k->ss_snow_wind_x * mult) / 100, dy = (k->ss_snow_wind_y * mult) / 100, sizeFP = SNOW_SIZES[l] << 8;
            for (int i = 0; i < SNOW_COUNTS[l]; i++) {
                k->ss_snow_x[l][i] += dx; k->ss_snow_y[l][i] += dy;
                if (k->ss_snow_y[l][i] > botFP) { k->ss_snow_y[l][i] = topFP - sizeFP; k->ss_snow_x[l][i] = ss_rand(sw) << 8; }
                else if (k->ss_snow_y[l][i] < topFP - sizeFP) { k->ss_snow_y[l][i] = botFP; k->ss_snow_x[l][i] = ss_rand(sw) << 8; }
                if (k->ss_snow_x[l][i] > wFP) k->ss_snow_x[l][i] = -sizeFP; else if (k->ss_snow_x[l][i] < -sizeFP) k->ss_snow_x[l][i] = wFP;
            }
        }
    }
}

static void build_karaoke_styled_line(const VMWCHAR* raw, VMINT32 eff_ms, int file_off, int user_off,
                                      bool strip_only, VMWCHAR* out, int max_chars) {
    bool has_word_tags = false;
    for (int i = 0; raw[i] != 0; i++) if (raw[i] == '<' && raw[i + 1] >= '0' && raw[i + 1] <= '9') { has_word_tags = true; break; }
    if (!has_word_tags) {
        if (strip_only) { safe_wstrncpy_zero(out, raw, max_chars); return; }
        int k = 0; if (k < max_chars - 2) out[k++] = STYLE_HL_ON;
        for (int i = 0; raw[i] != 0 && k < max_chars - 2; i++) out[k++] = raw[i];
        if (k < max_chars - 1) out[k++] = STYLE_HL_OFF;
        out[k] = 0; return;
    }

    int k = 0; bool hl = !strip_only;
    if (hl && k < max_chars - 1) out[k++] = STYLE_HL_ON;
    for (int i = 0; raw[i] != 0 && k < max_chars - 2; ) {
        if (raw[i] == '<' && raw[i + 1] >= '0' && raw[i + 1] <= '9') {
            int j = i + 1, mins = 0, secs = 0, ms = 0;
            while (raw[j] >= '0' && raw[j] <= '9') mins = mins * 10 + (raw[j++] - '0');
            if (raw[j] == ':') {
                j++; while (raw[j] >= '0' && raw[j] <= '9') secs = secs * 10 + (raw[j++] - '0');
                if (raw[j] == '.' || raw[j] == ':') {
                    j++; int d = 0, f = 0;
                    while (raw[j] >= '0' && raw[j] <= '9' && d < 3) { f = f * 10 + (raw[j++] - '0'); d++; }
                    ms = (d == 1) ? f * 100 : ((d == 2) ? f * 10 : f);
                }
                if (raw[j] == '>') {
                    if (!strip_only) {
                        VMINT32 w_ms = (VMINT32)(mins * 60000 + secs * 1000 + ms);
                        bool should_hl = (w_ms - file_off + user_off <= eff_ms);
                        if (should_hl != hl && k < max_chars - 1) { out[k++] = should_hl ? STYLE_HL_ON : STYLE_HL_OFF; hl = should_hl; }
                    }
                    i = j + 1; continue;
                }
            }
        }
        out[k++] = raw[i++];
    }
    if (hl && k < max_chars - 1) out[k++] = STYLE_HL_OFF;
    out[k] = 0;
}

// ============================================================================
// MODULE 8: On-Demand Help & File Tags Reader (Load / Unload)
// ============================================================================
static void reader_unload() { if (g_app.reader) { vm_free(g_app.reader); g_app.reader = NULL; } }

static bool help_load(int topic_idx) {
    reader_unload();
    g_app.reader = (TextReaderState*)vm_calloc((int)sizeof(TextReaderState));
    if (!g_app.reader) return false;
    if (topic_idx < 0 || topic_idx >= HELP_TOPIC_COUNT) topic_idx = 0;
    g_app.reader->topic_idx = topic_idx;

    vm_graphic_set_font(VM_SMALL_FONT);
    int max_w = g_app.screen_w - 18; if (max_w < 60) max_w = 60;
    VMWCHAR wbody[1024];
    utf8_markup_to_ucs2(g_help_articles[topic_idx].body, wbody, 1024, true);
    g_app.reader->total_lines = wrap_rich_text(wbody, max_w, MAX_HELP_LINES, false, &g_app.reader->lines[0][0], TEXT_LINE_STRIDE);
    return true;
}

static const char* format_mask_name(VMUINT8 fmt) {
    switch (fmt) {
        case FMT_MP3: return "MP3"; case FMT_MIDI: return "MIDI"; case FMT_IMY: return "IMY (iMelody)";
        case FMT_AAC: return "AAC"; case FMT_AMR: return "AMR"; case FMT_AWB: return "AWB (AMR-WB)";
        case FMT_VM:  return "VM (VoiceMemo)"; case FMT_WAV: return "WAV"; default: return "Unknown";
    }
}

static void append_styled_kv(VMWCHAR* dst, int max_chars, const char* key, const VMWCHAR* val_w, const char* val_ascii) {
    VMWCHAR tmp[160];
    safe_ascii_to_ucs2(tmp, 160, key);
    for (int i = 0; tmp[i]; i++) { if (tmp[i] == '{') tmp[i] = STYLE_HL_ON; else if (tmp[i] == '}') tmp[i] = STYLE_HL_OFF; }
    safe_wstrncat_zero(dst, tmp, max_chars);
    if (val_w && val_w[0]) safe_wstrncat_zero(dst, val_w, max_chars);
    else if (val_ascii && val_ascii[0]) { safe_ascii_to_ucs2(tmp, 160, val_ascii); safe_wstrncat_zero(dst, tmp, max_chars); }
    else { safe_ascii_to_ucs2(tmp, 160, "-"); safe_wstrncat_zero(dst, tmp, max_chars); }
    safe_ascii_to_ucs2(tmp, 160, "\n");
    safe_wstrncat_zero(dst, tmp, max_chars);
}

static bool file_tags_load(int track_idx) {
    reader_unload();
    g_app.reader = (TextReaderState*)vm_calloc((int)sizeof(TextReaderState));
    if (!g_app.reader) return false;

    vm_graphic_set_font(VM_SMALL_FONT);
    int max_w = g_app.screen_w - 18; if (max_w < 60) max_w = 60;
    VMWCHAR wbody[768]; wbody[0] = 0;

    if (track_idx < 0 || track_idx >= g_app.track_count || !g_app.tracks) {
        utf8_markup_to_ucs2("{No track selected.}", wbody, 768, true);
    } else {
        const TrackBase* tb = &g_app.tracks[track_idx];
        Id3Tag local_tag;
        const Id3Tag* tag = get_track_id3(tb);
        if (!tag) { memset(&local_tag, 0, sizeof(Id3Tag)); parse_id3_into_slot(tb, &local_tag); tag = &local_tag; }

        char sz_buf[40], trk_buf[16], yr_buf[16];
        sprintf(sz_buf, "%u KB (%u B)", (unsigned int)(tb->filesize / 1024), (unsigned int)tb->filesize);
        if (tag->track_num > 0) sprintf(trk_buf, "%u", (unsigned int)tag->track_num); else strcpy(trk_buf, "-");
        if (tag->year > 0) sprintf(yr_buf, "%u", (unsigned int)tag->year); else strcpy(yr_buf, "-");

        append_styled_kv(wbody, 768, "{File:} ", tb->filename, NULL);
        append_styled_kv(wbody, 768, "{Format:} ", NULL, format_mask_name(tb->format));
        append_styled_kv(wbody, 768, "{Size:} ", NULL, sz_buf);
        append_styled_kv(wbody, 768, "\n{Title:} ", tag->title, NULL);
        append_styled_kv(wbody, 768, "{Artist:} ", tag->artist, NULL);
        append_styled_kv(wbody, 768, "{Album:} ", tag->album, NULL);
        append_styled_kv(wbody, 768, "{Track #:} ", NULL, trk_buf);
        append_styled_kv(wbody, 768, "{Year:} ", NULL, yr_buf);
    }
    g_app.reader->total_lines = wrap_rich_text(wbody, max_w, MAX_HELP_LINES, false, &g_app.reader->lines[0][0], TEXT_LINE_STRIDE);
    return true;
}

// ============================================================================
// MODULE 9: Audio Playback Engine & Power Saving Control
// ============================================================================
static void apply_power_saving_mode() {
    vm_switch_power_saving_mode(g_app.keep_screen_on ? turn_off_mode : turn_on_mode);
}

static void apply_live_volume(int vol) {
    if (vol < 0) vol = 0; if (vol > 6) vol = 6;
    g_app.volume = vol; vm_set_volume(g_app.volume);
}

static void show_audio_error(const char* title, const char* l1, const char* l2, const char* l3) {
    strncpy(g_app.error_title, title, sizeof(g_app.error_title) - 1); g_app.error_title[sizeof(g_app.error_title) - 1] = 0;
    strncpy(g_app.error_line1, l1, sizeof(g_app.error_line1) - 1); g_app.error_line1[sizeof(g_app.error_line1) - 1] = 0;
    strncpy(g_app.error_line2, l2, sizeof(g_app.error_line2) - 1); g_app.error_line2[sizeof(g_app.error_line2) - 1] = 0;
    strncpy(g_app.error_line3, l3, sizeof(g_app.error_line3) - 1); g_app.error_line3[sizeof(g_app.error_line3) - 1] = 0;
    set_ui_mode(MODE_POPUP_ERROR); g_app.play_state = STATE_STOPPED;
}

static void stop_playback() {
    if (g_app.play_state != STATE_STOPPED) {
        g_app.play_state = STATE_STOPPED; g_app.current_time_sec = 0; g_app.current_time_ms = 0;
        if (g_app.midi_handle >= 0) { vm_midi_stop(g_app.midi_handle); g_app.midi_handle = -1; }
        else vm_audio_stop(digital_audio_callback);
    }
    if (g_app.ram_audio_buf) { vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; g_app.ram_audio_size = 0; }
}

static void pause_playback() {
    if (g_app.play_state == STATE_PLAYING) {
        if (g_app.midi_handle >= 0) vm_midi_pause(g_app.midi_handle); else vm_audio_pause(digital_audio_callback);
        g_app.play_state = STATE_PAUSED;
    }
}

static void resume_playback() {
    if (g_app.play_state == STATE_PAUSED) {
        vm_set_volume(g_app.volume);
        if (g_app.midi_handle >= 0) vm_midi_resume(g_app.midi_handle); else vm_audio_resume(digital_audio_callback);
        vm_set_volume(g_app.volume); g_app.play_state = STATE_PLAYING;
    }
}

static int pick_next_list_track(int cur_idx) {
    if (g_app.track_count <= 0) return -1;
    if (g_app.track_count == 1) return 0;
    if (g_app.shuffle_mode) {
        ss_rand_next ^= (unsigned int)vm_get_tick_count();
        int base = (cur_idx >= 0 && cur_idx < g_app.track_count) ? cur_idx : 0;
        return (base + 1 + ss_rand(g_app.track_count - 1)) % g_app.track_count;
    }
    return (cur_idx + 1) % g_app.track_count;
}

static void handle_track_finished() {
    if (g_app.loop_mode == LOOP_ONE) start_playback(g_app.active_track_idx, 0);
    else if (g_app.loop_mode == LOOP_LIST && g_app.track_count > 0) start_playback(pick_next_list_track(g_app.active_track_idx), 0);
    else stop_playback();
    trigger_repaint();
}

static void audio_interrupt_callback(VMINT handle, VMINT result) {
    if (result == VM_AUDIO_RESULT_INTERRUPT) pause_playback(); else if (result == VM_AUDIO_RESULT_INTERRUPT_RESUME) resume_playback();
}
static void digital_audio_callback(VMINT result) { if (result == VM_AUDIO_RESULT_END_OF_FILE) handle_track_finished(); }
static void midi_audio_callback(VMINT handle, VMINT event) { if (g_app.play_state != STATE_STOPPED && (event == VM_STATE_MIDI_STOP || event == VM_AUDIO_RESULT_END_OF_FILE)) handle_track_finished(); }

static void start_playback(int track_index, VMUINT start_time_ms) {
    if (track_index < 0 || track_index >= g_app.track_count || !g_app.tracks) return;
    stop_playback();

    TrackBase* t = &g_app.tracks[track_index];
    VMWCHAR full_path[260]; make_canonical_path(full_path, g_app.lib_dir, t->filename);
    VMUINT out_dev = DEVICE_PROFILE_VALUES[g_app.cur_device];
    if (g_app.karaoke && g_app.karaoke->lrc_track_idx != track_index) auto_load_lrc_for_track(track_index);

    vm_set_volume(g_app.volume);

    if (t->format == FMT_MIDI || t->format == FMT_IMY) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f < 0) { show_audio_error("File Read Error", "vm_file_open failed", "Check card/file name", ""); return; }
        VMUINT file_size = 0; vm_file_getfilesize(f, &file_size);
        if (file_size < ((t->format == FMT_IMY) ? 10u : 14u)) { vm_file_close(f); show_audio_error((t->format == FMT_IMY) ? "Corrupt IMY" : "Corrupt MIDI", "File size too small", "", ""); return; }
        g_app.ram_audio_buf = (VMUINT8*)vm_malloc((file_size + 7) & ~3);
        if (!g_app.ram_audio_buf) { vm_file_close(f); show_audio_error("Memory Error", "vm_malloc failed", "Out of ASM heap", ""); return; }
        VMUINT nread = 0; VMINT actual_b = vm_file_read(f, g_app.ram_audio_buf, file_size, &nread); vm_file_close(f);
        g_app.ram_audio_size = (actual_b > 0) ? (VMUINT)actual_b : nread;
        if (g_app.ram_audio_size == 0) { vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; show_audio_error("Disk I/O Error", "0 Bytes read", "", ""); return; }

        if (t->format == FMT_MIDI && memcmp(g_app.ram_audio_buf, "MThd", 4) != 0) {
            vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; show_audio_error("Header Error", "Missing 'MThd' marker", "Not a valid MIDI", ""); return;
        } else if (t->format == FMT_IMY && !ascii_istartswith_nocase(g_app.ram_audio_buf, (int)g_app.ram_audio_size, "BEGIN:IMELODY")) {
            vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; show_audio_error("Header Error", "Missing BEGIN:IMELODY", "Not a valid IMY", ""); return;
        }

        VMINT midi_dur = 0;
if (t->format == FMT_IMY) {
#ifdef VM_FORMAT_IMELODY
            midi_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, VM_FORMAT_IMELODY);
#endif
        } else {
            midi_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, VM_FORMAT_MIDI);
        }
        if (midi_dur <= 0) midi_dur = vm_audio_duration(full_path);
        g_app.total_duration_sec = (midi_dur > 0) ? ((midi_dur > 100000) ? ((VMUINT)midi_dur / 1000) : (VMUINT)midi_dur) : 0;

#ifdef VM_FORMAT_IMELODY
        if (t->format == FMT_IMY) { vm_midi_set_type(VM_FORMAT_IMELODY); g_app.midi_type_set_imy = true; }
        else if (g_app.midi_type_set_imy) { vm_midi_set_type(VM_FORMAT_MIDI); g_app.midi_type_set_imy = false; }
#endif

        vm_set_volume(g_app.volume);
        VMINT ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms, 1, out_dev, midi_audio_callback);
        if (ret < 0 && out_dev != VM_DEVICE_SPEAKER_BOTH) ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms,1, VM_DEVICE_SPEAKER_BOTH, midi_audio_callback);
        if (ret < 0) ret = vm_midi_play_by_bytes(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, 1, midi_audio_callback);

        if (ret < 0 && t->format == FMT_IMY) {
            VMINT fret = vm_audio_play_file_ex(full_path, start_time_ms, out_dev, digital_audio_callback);
            if (fret != VM_AUDIO_SUCCEED && out_dev != VM_DEVICE_SPEAKER_BOTH) fret = vm_audio_play_file_ex(full_path, 0, VM_DEVICE_SPEAKER_BOTH, digital_audio_callback);
            if (fret != VM_AUDIO_SUCCEED) fret = vm_audio_play_file(full_path, digital_audio_callback);
            if (fret == VM_AUDIO_SUCCEED) {
                vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; g_app.ram_audio_size = 0; g_app.midi_handle = -1;
                vm_set_volume(g_app.volume); g_app.play_state = STATE_PLAYING; g_app.active_track_idx = track_index;
                g_app.current_time_ms = start_time_ms; g_app.current_time_sec = start_time_ms / 1000; g_app.marq_char_offset = 0;
                return;
            }
        }

        if (ret < 0) {
            vm_free(g_app.ram_audio_buf); g_app.ram_audio_buf = NULL; g_app.ram_audio_size = 0;
            char l1[64], l2[64]; sprintf(l1, "Output: %s", DEVICE_PROFILE_NAMES[g_app.cur_device]); sprintf(l2, "MIDI DSP Code: %d", (int)ret);
            show_audio_error((t->format == FMT_IMY) ? "IMY Synth Failed" : "MIDI Synth Failed", l1, l2, "Synthesizer rejected bytes");
            return;
        }
        g_app.midi_handle = ret; vm_set_volume(g_app.volume);
        g_app.play_state = STATE_PLAYING; g_app.active_track_idx = track_index;
        g_app.current_time_ms = start_time_ms; g_app.current_time_sec = start_time_ms / 1000; g_app.marq_char_offset = 0;
        return;
    }

    VMINT dur_sec = vm_audio_duration(full_path);
    g_app.total_duration_sec = (dur_sec > 0) ? ((dur_sec > 100000) ? ((VMUINT)dur_sec / 1000) : (VMUINT)dur_sec) : 0;

    VMINT ret = VM_AUDIO_FAILED;
    if (start_time_ms > 0) ret = vm_audio_play_file_ex(full_path, start_time_ms, out_dev, digital_audio_callback);
    if (ret != VM_AUDIO_SUCCEED) ret = vm_audio_play_file_ex(full_path, 0, out_dev, digital_audio_callback);
    if (ret != VM_AUDIO_SUCCEED && out_dev != VM_DEVICE_SPEAKER_BOTH) ret = vm_audio_play_file_ex(full_path, 0, VM_DEVICE_SPEAKER_BOTH, digital_audio_callback);
    if (ret != VM_AUDIO_SUCCEED) ret = vm_audio_play_file(full_path, digital_audio_callback);

    if (ret != VM_AUDIO_SUCCEED && t->filesize <= (1024 * 1024 * 2)) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f >= 0) {
            g_app.ram_audio_buf = (VMUINT8*)vm_malloc((t->filesize + 7) & ~3);
            if (g_app.ram_audio_buf) {
                VMUINT nread = 0; VMINT actual_b = vm_file_read(f, g_app.ram_audio_buf, t->filesize, &nread);
                g_app.ram_audio_size = (actual_b > 0) ? (VMUINT)actual_b : nread;
                VMUINT8 fmt = (t->format == FMT_AAC) ? VM_FORMAT_AAC : ((t->format == FMT_AMR) ? VM_FORMAT_AMR : ((t->format == FMT_WAV) ? VM_FORMAT_WAV : VM_FORMAT_MP3));
                if (g_app.total_duration_sec == 0) { VMINT b_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, fmt); if (b_dur > 0) g_app.total_duration_sec = (b_dur > 100000) ? ((VMUINT)b_dur / 1000) : (VMUINT)b_dur; }
                ret = vm_audio_play_bytes(g_app.ram_audio_buf, g_app.ram_audio_size, fmt, 0, out_dev, digital_audio_callback);
                if (ret != VM_AUDIO_SUCCEED) ret = vm_audio_play_bytes_no_block(g_app.ram_audio_buf, g_app.ram_audio_size, fmt, out_dev, digital_audio_callback);
            }
            vm_file_close(f);
        }
    }

    if (ret != VM_AUDIO_SUCCEED) {
        char l1[64], l2[64], l3[64];
        sprintf(l1, "Output: %s", DEVICE_PROFILE_NAMES[g_app.cur_device]); sprintf(l2, "Return Code: %d", (int)ret); sprintf(l3, "Size: %u KB", (unsigned int)(t->filesize / 1024));
        show_audio_error("Playback Failed", l1, l2, l3); return;
    }
    vm_set_volume(g_app.volume);
    g_app.current_time_ms = start_time_ms; g_app.current_time_sec = start_time_ms / 1000;
    g_app.play_state = STATE_PLAYING; g_app.active_track_idx = track_index; g_app.marq_char_offset = 0;
}

static void format_time_string_sec(VMUINT total_s, VMWCHAR* out_str) {
    char buf[32]; sprintf(buf, "%02d:%02d:%02d", (int)(total_s / 3600), (int)((total_s % 3600) / 60), (int)(total_s % 60));
    safe_ascii_to_ucs2(out_str, 16, buf);
}

static void prepare_time_picker(VMUINT initial_s) {
    int h = (initial_s / 3600) % 100, m = (initial_s % 3600) / 60, s = initial_s % 60;
    g_app.time_digits[0] = h / 10; g_app.time_digits[1] = h % 10; g_app.time_digits[2] = m / 10;
    g_app.time_digits[3] = m % 10; g_app.time_digits[4] = s / 10; g_app.time_digits[5] = s % 10;
    g_app.time_cursor = 0;
}

static void apply_timestamp_jump() {
    int h = g_app.time_digits[0] * 10 + g_app.time_digits[1], m = g_app.time_digits[2] * 10 + g_app.time_digits[3], s = g_app.time_digits[4] * 10 + g_app.time_digits[5];
    if (s > 59) s = 59; if (m > 59) m = 59;
    VMUINT target_sec = (VMUINT)(h * 3600 + m * 60 + s);
    if (g_app.total_duration_sec > 0 && target_sec > g_app.total_duration_sec) target_sec = g_app.total_duration_sec;
    int tt = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
    if (tt >= 0 && tt < g_app.track_count) start_playback(tt, target_sec * 1000);
}

// ============================================================================
// MODULE 10: Timers & UI Rendering
// ============================================================================
static void timer_tick(VMINT tid) {
    bool needs_repaint = false;
    if (g_app.play_state == STATE_PLAYING) {
        if (g_app.midi_handle >= 0) {
            VMUINT mtime = 0;
            if (vm_midi_get_time(g_app.midi_handle, &mtime) == 0) { g_app.current_time_ms = mtime; g_app.current_time_sec = mtime / 1000; }
            else { g_app.current_time_ms += ENGINE_TIMER_MS; g_app.current_time_sec = g_app.current_time_ms / 1000; }
        } else {
            VMINT cur = vm_audio_get_time();
            if (cur >= 0) { g_app.current_time_ms = (VMUINT)cur; g_app.current_time_sec = (VMUINT)cur / 1000; }
            else { g_app.current_time_ms += ENGINE_TIMER_MS; g_app.current_time_sec = g_app.current_time_ms / 1000; }
        }
        needs_repaint = true;
    }

    if (g_app.ui_mode == MODE_KARAOKE && g_app.karaoke) { update_screensaver_state(); needs_repaint = true; }

    bool allow_scroll = (g_app.play_state == STATE_PLAYING) ||
                        (g_app.play_state == STATE_PAUSED && g_app.scroll_on_pause) ||
                        (g_app.play_state == STATE_STOPPED && (g_app.ui_mode == MODE_TRACKLIST || g_app.ui_mode == MODE_FOLDER_VIEWER));
    if (allow_scroll && ++g_app.scroll_tick_counter >= 6) {
        g_app.scroll_tick_counter = 0;
        if (g_app.play_state == STATE_PLAYING || (g_app.play_state == STATE_PAUSED && g_app.scroll_on_pause)) g_app.marq_char_offset++;
        if (g_app.ui_mode == MODE_TRACKLIST || g_app.ui_mode == MODE_FOLDER_VIEWER) g_app.sel_char_offset++;
        needs_repaint = true;
    }
    if (g_app.show_clock && ++g_app.clock_tick_counter >= 20) { g_app.clock_tick_counter = 0; needs_repaint = true; }
    if (needs_repaint) trigger_repaint();
}

static void numeric_timeout(VMINT tid) {
    if (g_app.numeric_len > 0) {
        int target = atoi(g_app.numeric_buf);
        if (target >= 1 && target <= g_app.track_count) {
            g_app.selected_track = target - 1;
            int br = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
            int ih = g_app.font_h + 4; if (ih <= 0) ih = 18;
            int vis = (g_app.screen_h - g_app.header_h - br) / ih; if (vis < 1) vis = 1;
            if (g_app.selected_track < g_app.scroll_offset) g_app.scroll_offset = g_app.selected_track;
            else if (g_app.selected_track >= g_app.scroll_offset + vis) g_app.scroll_offset = g_app.selected_track - vis + 1;
            if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;
            g_app.sel_char_offset = 0;
        }
        g_app.numeric_len = 0; g_app.numeric_buf[0] = 0; trigger_repaint();
    }
}

static void draw_text(int x, int y, const VMWCHAR* str, vm_graphic_color col) {
    if (x < 0) x = 0; if (y < 0) y = 0;
    vm_graphic_setcolor(&col); vm_graphic_textout_to_layer(g_app.layer_hdl, x, y, (VMWSTR)str, vm_wstrlen((VMWSTR)str));
}

static void draw_ascii(int x, int y, const char* str, vm_graphic_color col) {
    VMWCHAR wstr[128]; safe_ascii_to_ucs2(wstr, 128, str); draw_text(x, y, wstr, col);
}

static void draw_text_truncated(int x, int y, const VMWCHAR* str, int max_w, vm_graphic_color col) {
    if (x < 0) x = 0; if (y < 0) y = 0; if (max_w <= 0 || !str) return;
    int len = vm_wstrlen((VMWSTR)str); if (len <= 0) return;
    VMWCHAR buf[160]; if (len >= 159) len = 158; safe_wstrncpy_zero(buf, str, len + 1);
    int total_w = vm_graphic_get_string_width(buf);
    if (total_w > max_w) {
        int avg = total_w / len; if (avg <= 0) avg = 6;
        int est = max_w / avg; if (est >= len) est = len - 1; if (est < 1) est = 1;
        buf[est] = 0; while (est > 0 && vm_graphic_get_string_width(buf) > max_w) buf[--est] = 0;
        len = est;
    }
    if (len > 0) { vm_graphic_setcolor(&col); vm_graphic_textout_to_layer(g_app.layer_hdl, x, y, buf, len); }
}

static void draw_ascii_truncated(int x, int y, const char* str, int max_w, vm_graphic_color col) {
    VMWCHAR wstr[128]; safe_ascii_to_ucs2(wstr, 128, str); draw_text_truncated(x, y, wstr, max_w, col);
}

struct PopupListGeom { int px, py, pw, ph, title_h, row_h, visible_count, total_count; };

static PopupListGeom calc_popup_list_geom(int total_count, bool has_title) {
    PopupListGeom g;
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    g.total_count = total_count; g.row_h = fh + 4; if (g.row_h < 14) g.row_h = 14;
    g.title_h = has_title ? (fh + 8) : 0; g.pw = sw - 20; if (g.pw < 80) g.pw = sw; g.px = (sw - g.pw) / 2;
    int max_vis = ((sh - g_app.footer_h - 8) - g.title_h - 12) / g.row_h; if (max_vis < 1) max_vis = 1;
    g.visible_count = (total_count < max_vis) ? total_count : max_vis; if (g.visible_count < 1) g.visible_count = 1;
    g.ph = g.title_h + g.visible_count * g.row_h + 12; g.py = (sh - g_app.footer_h - g.ph) / 2; if (g.py < 4) g.py = 4;
    return g;
}

static void adjust_popup_scroll(int total_count, int visible_count) {
    if (visible_count < 1) visible_count = 1;
    if (total_count <= 0) { g_app.menu_cursor = 0; g_app.menu_scroll = 0; return; }
    if (g_app.menu_cursor < 0) g_app.menu_cursor = 0;
    if (g_app.menu_cursor >= total_count) g_app.menu_cursor = total_count - 1;
    if (g_app.menu_cursor < g_app.menu_scroll) g_app.menu_scroll = g_app.menu_cursor;
    else if (g_app.menu_cursor >= g_app.menu_scroll + visible_count) g_app.menu_scroll = g_app.menu_cursor - visible_count + 1;
    int max_s = total_count - visible_count; if (max_s < 0) max_s = 0;
    if (g_app.menu_scroll > max_s) g_app.menu_scroll = max_s;
    if (g_app.menu_scroll < 0) g_app.menu_scroll = 0;
}

static void draw_popup_list_frame_and_sb(const PopupListGeom* g, const char* title, const Theme* th) {
    vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g->px, g->py, g->pw, g->ph);
    vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, g->px, g->py, g->pw, g->ph);
    if (title && g->title_h > 0) {
        draw_ascii_truncated(g->px + 8, g->py + 5, title, g->pw - 16, th->header_text);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border);
        vm_graphic_line_ex(g_app.layer_hdl, g->px + 1, g->py + g->title_h + 1, g->px + g->pw - 2, g->py + g->title_h + 1);
    }
    if (g->total_count > g->visible_count && g->visible_count > 0) {
        int sb_x = g->px + g->pw - 7, sb_y = g->py + g->title_h + 6, sb_h = g->visible_count * g->row_h;
        vm_graphic_color tc = lerp_color(th->popup_bg, th->popup_border, 45);
        vm_graphic_setcolor(&tc); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 4, sb_h);
        int th_h = (g->visible_count * sb_h) / g->total_count; if (th_h < 6) th_h = 6;
        int max_s = g->total_count - g->visible_count;
        int ty = sb_y + (max_s > 0 ? (g_app.menu_scroll * (sb_h - th_h)) / max_s : 0);
        vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, ty, 4, th_h);
    }
}

static int get_active_popup_total_items() {
    switch (g_app.ui_mode) {
        case MODE_MENU:           return MENU_ITEM_COUNT;
        case MODE_MODES_MENU:     return MODES_MENU_ITEM_COUNT;
        case MODE_SETTINGS_MENU:  return SETTINGS_MENU_ITEM_COUNT;
        case MODE_FOLDER_OPTIONS: return FOLDER_OPT_ITEM_COUNT;
        case MODE_DEVICE_SELECT:  return DEV_PROFILE_COUNT;
        case MODE_THEME_SELECT:   return THEME_COUNT;
        case MODE_FORMAT_FILTER:  return 8;
        case MODE_SORT_SELECT:    return SORT_COUNT;
        case MODE_HELP_INDEX:     return HELP_TOPIC_COUNT;
        default:                  return 0;
    }
}

static bool active_popup_has_title() { return (g_app.ui_mode != MODE_MENU); }

static void draw_now_playing_dock(const Theme* th) {
    if (g_app.play_state == STATE_STOPPED) return;
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    int my = sh - g_app.footer_h - g_app.marquee_h;
    vm_graphic_setcolor((vm_graphic_color*)&th->marquee_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, my, sw, g_app.marquee_h);
    vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_line_ex(g_app.layer_hdl, 0, my, sw - 1, my);
    VMWCHAR cur_w[16], tot_w[16], sep1[8], l1[64], wl[24];
    format_time_string_sec(g_app.current_time_sec, cur_w); format_time_string_sec(g_app.total_duration_sec, tot_w);
    safe_ascii_to_ucs2(sep1, 8, " / "); vm_wstrcpy(l1, cur_w); vm_wstrcat(l1, sep1); vm_wstrcat(l1, tot_w);
    draw_text(6, my + 2, l1, th->marquee_text);
    const char* ltag = (g_app.loop_mode == LOOP_LIST && g_app.shuffle_mode) ? "Shuf" : LOOP_NAMES[g_app.loop_mode];
    char lt[24]; sprintf(lt, "V%d [%s]", g_app.volume, ltag);
    safe_ascii_to_ucs2(wl, 24, lt); draw_text(sw - vm_graphic_get_string_width(wl) - 6, my + 2, wl, th->item_playing);
    if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
        int l2y = my + fh + 3, mw = sw - 12;
        VMWCHAR mn[128]; get_track_display_name(g_app.active_track_idx, mn, 128);
        int fl = vm_wstrlen(mn), fw = vm_graphic_get_string_width(mn);
        if (fw <= mw) draw_text(6, l2y, mn, th->item_playing);
        else { int off = g_app.marq_char_offset % (fl + 5); draw_text_truncated(6, l2y, (off < fl) ? (mn + off) : mn, mw, th->item_playing); }
    }
}

static void render_karaoke_view(const Theme* th) {
    if (!g_app.karaoke) return;
    KaraokeState* k = g_app.karaoke;
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    int top_y = g_app.header_h, bot_y = sh - g_app.footer_h, vh = bot_y - top_y;
    if (vh <= 0) return;

    if (g_app.screensaver_id == SS_STARFIELD) {
        int cx = sw / 2, cy = top_y + vh / 2; vm_graphic_color c_far = lerp_color(th->bg, th->header_text, 45);
        for (int i = 0; i < 24; i++) {
            int z = k->ss_stars[i].z > 0 ? k->ss_stars[i].z : 1;
            int sx = cx + (k->ss_stars[i].x * 16) / z, sy = cy + (k->ss_stars[i].y * 16) / z;
            if (sx >= 2 && sx < sw - 2 && sy >= top_y + 2 && sy < bot_y - 2) {
                vm_graphic_color sc = (z < 6) ? th->header_text : ((z < 11) ? th->item_playing : c_far);
                vm_graphic_setcolor(&sc); int sz = (z < 5) ? 3 : ((z < 10) ? 2 : 1);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, sx, sy, sz, sz);
            }
        }
    } else if (g_app.screensaver_id == SS_EQUALIZER) {
        int bars = 12, bar_w = (sw - 20) / bars; if (bar_w < 4) bar_w = 4;
        int base_y = bot_y - fh - 16;
        if (base_y > top_y + 20) {
            vm_graphic_color body = lerp_color(th->bg, th->item_selected_bg, 65);
            for (int i = 0; i < bars; i++) {
                int bh = k->ss_eq_heights[i % 16]; if (bh > base_y - top_y - 8) bh = base_y - top_y - 8;
                int bx = 10 + i * bar_w;
                vm_graphic_setcolor(&body); vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, base_y - bh, bar_w - 2, bh);
                vm_graphic_setcolor((vm_graphic_color*)&th->item_playing); vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, base_y - bh, bar_w - 2, (bh >= 3) ? 3 : bh);
            }
        }
    } else if (g_app.screensaver_id == SS_BOUNCING_ORB) {
        int ox = k->ss_orb_x, oy = k->ss_orb_y;
        vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_ellipse_ex(g_app.layer_hdl, ox - 12, oy - 12, 25, 25);
        vm_graphic_setcolor((vm_graphic_color*)&th->item_playing); vm_graphic_ellipse_ex(g_app.layer_hdl, ox - 14, oy - 14, 29, 29);
        vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_text); vm_graphic_fill_ellipse_ex(g_app.layer_hdl, ox - 3, oy - 3, 7, 7);
    } else if (g_app.screensaver_id == SS_DIGITAL_RAIN) {
        int cols = 14, col_w = sw / cols; if (col_w < 8) col_w = 8;
        const char glyphs[] = "01AF*+";
        vm_graphic_color tc[4] = { (color_luma(th->bg) > 128) ? th->header_text : th->item_selected_text, th->item_playing, lerp_color(th->item_playing, th->bg, 38), lerp_color(th->item_playing, th->bg, 68) };
        for (int i = 0; i < cols && i < 20; i++) {
            int gx = 4 + i * col_w;
            for (int t = 0; t < 4; t++) {
                int gy = k->ss_rain_y[i] - t * (fh + 2);
                if (gy >= top_y && gy + fh < bot_y) { char ch[2] = { glyphs[(k->ss_frame + i + t) % 6], 0 }; draw_ascii(gx, gy, ch, tc[t]); }
            }
        }
    } else if (g_app.screensaver_id == SS_BOKEH_SNOW) {
        vm_graphic_set_layer_clip(g_app.layer_hdl, 0, (VMINT16)top_y, (VMINT16)(sw - 1), (VMINT16)(bot_y - 1));
        vm_graphic_color target = lerp_color(th->item_selected_bg, th->header_text, 40);
        const int pcts[4] = { 18, 28, 40, 54 };
        for (int l = 0; l < 4; l++) {
            vm_graphic_color c = lerp_color(th->bg, target, pcts[l]);
            vm_graphic_setcolor(&c);
            int sz = SNOW_SIZES[l];
            for (int i = 0; i < SNOW_COUNTS[l]; i++) vm_graphic_fill_ellipse_ex(g_app.layer_hdl, k->ss_snow_x[l][i] >> 8, k->ss_snow_y[l][i] >> 8, sz, sz);
        }
        vm_graphic_set_layer_clip(g_app.layer_hdl, 0, 0, (VMINT16)(sw - 1), (VMINT16)(sh - 1));
    }

    VMWCHAR disp_name[128];
    int show_idx = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
    if (show_idx >= 0 && show_idx < g_app.track_count) get_track_display_name(show_idx, disp_name, 128);
    else safe_ascii_to_ucs2(disp_name, 128, "No Track Selected");
    int nw = sw - 12;
    int fl = vm_wstrlen(disp_name), fw = vm_graphic_get_string_width(disp_name);
    if (fw <= nw) draw_text(6, top_y + 3, disp_name, th->header_text);
    else {
        int off = g_app.marq_char_offset % (fl + 5);
        draw_text_truncated(6, top_y + 3, (off < fl) ? (disp_name + off) : disp_name, nw, th->header_text);
    }

    int row2_y = top_y + fh + 5;
    char ss_info[48], offs_info[24];
    sprintf(ss_info, "< %s >", SCREENSAVER_NAMES[g_app.screensaver_id]);
    draw_ascii_truncated(6, row2_y, ss_info, sw / 2 + 12, th->marquee_text);

    sprintf(offs_info, "[-] %+.1fs [+]", g_app.lrc_user_offset_ms / 1000.0f);
    VMWCHAR woffs[24]; safe_ascii_to_ucs2(woffs, 24, offs_info);
    draw_text(sw - vm_graphic_get_string_width(woffs) - 6, row2_y, woffs, th->item_playing);

    int lyr_top = top_y + fh * 2 + 12, lyr_bot = bot_y - fh - 16, lyr_h = lyr_bot - lyr_top, line_step = fh + 2;
    if (k->lrc_count > 0 && lyr_h > line_step) {
        VMINT32 eff_ms = (VMINT32)g_app.current_time_ms - k->lrc_file_offset_ms + g_app.lrc_user_offset_ms;
        if (eff_ms < 0) eff_ms = 0;
        int active_idx = -1;
        for (int i = 0; i < k->lrc_count; i++) { if (k->lrc_lines[i].time_ms <= eff_ms) active_idx = i; else break; }

        int total_fit = lyr_h / line_step; if (total_fit < 1) total_fit = 1;
        int active_max = (total_fit >= 6) ? 4 : ((total_fit >= 4) ? 3 : ((total_fit >= 2) ? 2 : 1));
        VMWCHAR styled_act[MAX_LRC_TEXT_LEN + 16], act_lines[4][TEXT_LINE_STRIDE], clean_prev[MAX_LRC_TEXT_LEN];
        int act_cnt = 0;

        if (active_idx >= 0) {
            build_karaoke_styled_line(k->lrc_lines[active_idx].text, eff_ms, k->lrc_file_offset_ms, g_app.lrc_user_offset_ms, false, styled_act, MAX_LRC_TEXT_LEN + 16);
            act_cnt = wrap_rich_text(styled_act, sw - 16, active_max, true, &act_lines[0][0], TEXT_LINE_STRIDE);
        } else {
            safe_ascii_to_ucs2(act_lines[0], TEXT_LINE_STRIDE, "~ Instrumental ~");
            act_cnt = 1;
        }

        int act_block_h = act_cnt * line_step, act_y = lyr_top + (lyr_h - act_block_h) / 2;

        if (active_idx > 0 && act_y - line_step - 4 >= lyr_top) {
            build_karaoke_styled_line(k->lrc_lines[active_idx - 1].text, 0, 0, 0, true, clean_prev, MAX_LRC_TEXT_LEN);
            int pw = vm_graphic_get_string_width(clean_prev), px = (pw < sw - 16) ? (sw - pw) / 2 : 8;
            draw_text_truncated(px, act_y - line_step - 4, clean_prev, sw - 16, th->item_text);
        }
        for (int li = 0; li < act_cnt; li++) {
            int lw = measure_styled_width(act_lines[li], TEXT_LINE_STRIDE), lx = (lw < sw - 16) ? (sw - lw) / 2 : 8;
            if (active_idx >= 0) draw_styled_ucs2_line(lx, act_y + li * line_step, act_lines[li], th->item_text, th->item_playing);
            else draw_text(lx, act_y + li * line_step, act_lines[li], th->marquee_text);
        }
        int ny = act_y + act_block_h + 6;
        for (int nxt = 1; nxt <= 2 && active_idx + nxt < k->lrc_count && ny + line_step <= lyr_bot; nxt++) {
            build_karaoke_styled_line(k->lrc_lines[active_idx + nxt].text, 0, 0, 0, true, clean_prev, MAX_LRC_TEXT_LEN);
            int nw = vm_graphic_get_string_width(clean_prev), nx = (nw < sw - 16) ? (sw - nw) / 2 : 8;
            draw_text_truncated(nx, ny, clean_prev, sw - 16, th->item_text);
            ny += line_step + 2;
        }
    } else {
        int cy = lyr_top + lyr_h / 2 - fh; if (cy < lyr_top) cy = lyr_top;
        VMWCHAR msg1[32], msg2[32];
        safe_ascii_to_ucs2(msg1, 32, "No LRC Loaded"); safe_ascii_to_ucs2(msg2, 32, "Press LSK to Pick .lrc");
        draw_text((sw - vm_graphic_get_string_width(msg1)) / 2, cy, msg1, th->item_text);
        draw_text((sw - vm_graphic_get_string_width(msg2)) / 2, cy + fh + 4, msg2, th->header_text);
    }

    int bar_y = bot_y - fh - 10;
    vm_graphic_color bar_track = lerp_color(th->bg, th->popup_border, 45);
    vm_graphic_setcolor(&bar_track); vm_graphic_fill_rect_ex(g_app.layer_hdl, 8, bar_y, sw - 16, 5);
    if (g_app.total_duration_sec > 0) {
        int fill_w = (int)((g_app.current_time_sec * (VMUINT)(sw - 16)) / g_app.total_duration_sec);
        if (fill_w > sw - 16) fill_w = sw - 16;
        vm_graphic_setcolor((vm_graphic_color*)&th->item_playing); vm_graphic_fill_rect_ex(g_app.layer_hdl, 8, bar_y, fill_w, 5);
    }

    VMWCHAR cur_w[16], tot_w[16], sep1[8], time_line[48], w_st[24];
    format_time_string_sec(g_app.current_time_sec, cur_w); format_time_string_sec(g_app.total_duration_sec, tot_w);
    safe_ascii_to_ucs2(sep1, 8, " / "); vm_wstrcpy(time_line, cur_w); vm_wstrcat(time_line, sep1); vm_wstrcat(time_line, tot_w);
    draw_text(8, bot_y - fh - 3, time_line, th->marquee_text);

    safe_ascii_to_ucs2(w_st, 24, (g_app.play_state == STATE_PLAYING) ? "|<  ||  >|" : "|<  >   >|");
    draw_text(sw - vm_graphic_get_string_width(w_st) - 8, bot_y - fh - 3, w_st, th->item_playing);
}

static void render_folder_viewer(const Theme* th) {
    FolderViewerState* fv = g_app.folder_viewer;
    if (!fv) return;
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
    int body_h = sh - g_app.header_h - bottom_reserved;

    if (body_h > 0 && fv->entries) {
        int item_h = fh + 4; if (item_h <= 0) item_h = 18;
        int vis = body_h / item_h; if (vis < 1) vis = 1;
        int max_s = fv->entry_count - vis; if (max_s < 0) max_s = 0;
        if (fv->scroll > max_s) fv->scroll = max_s;
        if (fv->scroll < 0) fv->scroll = 0;

        for (int i = 0; i < vis; i++) {
            int idx = fv->scroll + i; if (idx >= fv->entry_count) break;
            int iy = g_app.header_h + i * item_h; if (iy + item_h > g_app.header_h + body_h) break;
            const FolderEntry* e = &fv->entries[idx];
            bool is_sel = (idx == fv->cursor);
            if (is_sel) { vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 2, iy, sw - 10, item_h); }

            const char* pfx = "";
            if (e->type == FENT_SUBDIR) pfx = "[D] ";
            else if (e->type == FENT_FILE) pfx = "  ";
            VMWCHAR wpfx[16]; safe_ascii_to_ucs2(wpfx, 16, pfx);
            int pfx_w = pfx[0] ? vm_graphic_get_string_width(wpfx) : 0;

            vm_graphic_color rc = is_sel ? th->item_selected_text
                        : ((e->type == FENT_FILE && !fv->lrc_mode) ? lerp_color(th->item_text, th->bg, 30) : th->item_text);
                    if ((e->type == FENT_THIS || e->type == FENT_LRC_AUTO || e->type == FENT_LRC_NONE) && !is_sel) rc = th->item_playing;


            if (pfx_w > 0) draw_text(5, iy + 2, wpfx, rc);
            int nx = 5 + pfx_w, nw = (sw - 10) - nx;
            int fl = vm_wstrlen((VMWSTR)e->name), fw = vm_graphic_get_string_width((VMWSTR)e->name);
            if (is_sel && fw > nw) {
                int off = g_app.sel_char_offset % (fl + 5);
                draw_text_truncated(nx, iy + 2, (off < fl) ? (e->name + off) : e->name, nw, rc);
            } else {
                draw_text_truncated(nx, iy + 2, e->name, nw, rc);
            }
        }
        if (fv->entry_count > vis && vis > 0) {
            int sb_x = sw - 6, sb_y = g_app.header_h + 2, sb_h = body_h - 4;
            vm_graphic_color tc = lerp_color(th->bg, th->popup_border, 45);
            vm_graphic_setcolor(&tc); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 4, sb_h);
            int th_h = (vis * sb_h) / fv->entry_count; if (th_h < 8) th_h = 8;
            int ty = sb_y + (max_s > 0 ? (fv->scroll * (sb_h - th_h)) / max_s : 0);
            vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, ty, 4, th_h);
        }
    }
    draw_now_playing_dock(th);
}

static void render_screen() {
    vm_graphic_active_layer(g_app.layer_hdl);
    vm_graphic_set_font(VM_SMALL_FONT);

    const Theme* th = &g_themes[g_app.cur_theme];
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;

    vm_graphic_setcolor((vm_graphic_color*)&th->bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, 0, sw, sh);
    vm_graphic_setcolor((vm_graphic_color*)&th->header_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, 0, sw, g_app.header_h);
    vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_line_ex(g_app.layer_hdl, 0, g_app.header_h - 1, sw - 1, g_app.header_h - 1);

    char title_buf[64];
    const char* header_title = "ABookPlayer";
    switch (g_app.ui_mode) {
        case MODE_TRACKLIST:
            sprintf(title_buf, g_app.id3_mode ? "ID3 (%d/%d)" : "Tracks (%d/%d)",
                    g_app.track_count > 0 ? g_app.selected_track + 1 : 0, g_app.track_count);
            header_title = title_buf; break;
        case MODE_MENU:               header_title = "ABookPlayer"; break;
        case MODE_MODES_MENU:         header_title = "Modes"; break;
        case MODE_SETTINGS_MENU:      header_title = "Settings"; break;
        case MODE_FOLDER_VIEWER:      header_title = "Play from Folder"; break;
        case MODE_FOLDER_OPTIONS:     header_title = "Folder Options"; break;
        case MODE_VOLUME_SELECT:      header_title = "Volume"; break;
        case MODE_DEVICE_SELECT:      header_title = "Audio Output"; break;
        case MODE_TIME_PICKER:        header_title = "Jump to Timestamp"; break;
        case MODE_FORMAT_FILTER:      header_title = "Audio Formats"; break;
        case MODE_SORT_SELECT:        header_title = "Sort Tracks"; break;
        case MODE_THEME_SELECT:       header_title = "Select Theme"; break;
        case MODE_KARAOKE:            header_title = "Karaoke Mode"; break;
        case MODE_HELP_INDEX:         header_title = "Help Topics"; break;
        case MODE_HELP_READER:        header_title = g_app.reader ? g_help_articles[g_app.reader->topic_idx].title : "Help"; break;
        case MODE_FILE_TAGS:          header_title = "This File's Tags"; break;
        case MODE_POPUP_CONFIRM_EXIT: header_title = "Exit"; break;
        case MODE_POPUP_ABOUT:        header_title = "About"; break;
        case MODE_POPUP_ERROR:        header_title = "Audio Notice"; break;
    }

    int header_text_y = (g_app.header_h - fh) / 2, right_used_w = 6;
    if (g_app.numeric_len > 0) {
        char jb[32]; sprintf(jb, "#%s_", g_app.numeric_buf);
        VMWCHAR wj[32]; safe_ascii_to_ucs2(wj, 32, jb);
        int jw = vm_graphic_get_string_width(wj);
        vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, sw - jw - 10, 2, jw + 6, g_app.header_h - 4);
        draw_text(sw - jw - 7, header_text_y, wj, th->item_selected_text);
        right_used_w = jw + 14;
    } else {
        if (g_app.show_clock) {
            vm_time_t sys_t;
            if (vm_get_time(&sys_t) == 0) {
                char cb[16]; sprintf(cb, "%02d:%02d", sys_t.hour, sys_t.min);
                VMWCHAR wc[16]; safe_ascii_to_ucs2(wc, 16, cb);
                int cw = vm_graphic_get_string_width(wc);
                draw_text(sw - cw - 6, header_text_y, wc, th->header_text);
                right_used_w += cw + 6;
            }
        }
        if (g_app.ui_mode == MODE_TRACKLIST && sw >= 128) {
            VMWCHAR wi[12]; safe_ascii_to_ucs2(wi, 12, g_app.id3_mode ? "ID3*" : "ID3");
            int iw = vm_graphic_get_string_width(wi), bx = sw - right_used_w - iw - 8;
            if (g_app.id3_mode) {
                vm_graphic_color bbg = th->item_playing, bfg = (color_luma(bbg) > 135) ? make_color(10, 10, 10) : make_color(255, 255, 255);
                vm_graphic_setcolor(&bbg); vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
                vm_graphic_setcolor(&bfg); vm_graphic_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
                draw_text(bx + 3, header_text_y, wi, bfg);
            } else {
                vm_graphic_color bbg = th->header_bg, bfg = lerp_color(th->header_bg, th->header_text, 55);
                vm_graphic_setcolor(&bbg); vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
                vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
                draw_text(bx + 3, header_text_y, wi, bfg);
            }
            right_used_w += iw + 12;
        }
    }

    if ((g_app.ui_mode == MODE_FOLDER_VIEWER || g_app.ui_mode == MODE_FOLDER_OPTIONS) && g_app.folder_viewer) {
        if (g_app.folder_viewer->at_drive_list) {
            draw_ascii_truncated(6, header_text_y, "Select Drive:", sw - right_used_w - 8, th->header_text);
        } else {
            draw_text_truncated(6, header_text_y, g_app.folder_viewer->cur_path, sw - right_used_w - 8, th->header_text);
        }
    } else {
        draw_ascii_truncated(6, header_text_y, header_title, sw - right_used_w - 8, th->header_text);
    }

    if (g_app.ui_mode == MODE_KARAOKE) {
        render_karaoke_view(th);
    } else if ((g_app.ui_mode == MODE_FOLDER_VIEWER || g_app.ui_mode == MODE_FOLDER_OPTIONS) && g_app.folder_viewer) {
        render_folder_viewer(th);
    } else if ((g_app.ui_mode == MODE_HELP_READER || g_app.ui_mode == MODE_FILE_TAGS) && g_app.reader) {
        int body_top = g_app.header_h + 4, body_bot = sh - g_app.footer_h - 4;
        int line_h = fh + 3; if (line_h < 14) line_h = 14;
        int vis = (body_bot - body_top > 0) ? ((body_bot - body_top) / line_h) : 1; if (vis < 1) vis = 1;
        int max_s = g_app.reader->total_lines - vis; if (max_s < 0) max_s = 0;
        if (g_app.reader->scroll_line > max_s) g_app.reader->scroll_line = max_s;
        if (g_app.reader->scroll_line < 0) g_app.reader->scroll_line = 0;

        for (int i = 0; i < vis; i++) {
            int idx = g_app.reader->scroll_line + i; if (idx >= g_app.reader->total_lines) break;
            if (g_app.reader->lines[idx][0]) draw_styled_ucs2_line(6, body_top + i * line_h, g_app.reader->lines[idx], th->item_text, th->item_playing);
        }
        if (g_app.reader->total_lines > vis) {
            int sb_x = sw - 6, sb_y = body_top, sb_h = body_bot - body_top;
            vm_graphic_color tc = lerp_color(th->bg, th->popup_border, 45);
            vm_graphic_setcolor(&tc); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 4, sb_h);
            int th_h = (vis * sb_h) / g_app.reader->total_lines; if (th_h < 8) th_h = 8;
            int ty = sb_y + (max_s > 0 ? (g_app.reader->scroll_line * (sb_h - th_h)) / max_s : 0);
            vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, ty, 4, th_h);
        }
    } else {
        int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int body_h = sh - g_app.header_h - bottom_reserved;
        if (body_h > 0 && g_app.tracks) {
            int item_h = fh + 4; if (item_h <= 0) item_h = 18;
            int vis = body_h / item_h;
            for (int i = 0; i < vis; i++) {
                int idx = g_app.scroll_offset + i; if (idx >= g_app.track_count) break;
                int iy = g_app.header_h + i * item_h; if (iy + item_h > g_app.header_h + body_h) break;
                bool is_sel = (idx == g_app.selected_track), is_play = (g_app.play_state != STATE_STOPPED && idx == g_app.active_track_idx);
                if (is_sel) { vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 2, iy, sw - 10, item_h); }

                char prefix[16];
                if (is_play) sprintf(prefix, "%s%d. ", (g_app.play_state == STATE_PLAYING ? "> " : "|| "), idx + 1);
                else sprintf(prefix, "  %d. ", idx + 1);
                VMWCHAR wprefix[16]; safe_ascii_to_ucs2(wprefix, 16, prefix);
                int prefix_w = vm_graphic_get_string_width(wprefix);
                vm_graphic_color rc = (is_play && is_sel) ? th->item_playing_selected : (is_play ? th->item_playing : (is_sel ? th->item_selected_text : th->item_text));
                draw_text(5, iy + 2, wprefix, rc);

                int nx = 5 + prefix_w, nw = (sw - 10) - nx;
                VMWCHAR dn[128]; get_track_display_name(idx, dn, 128);
                int fl = vm_wstrlen(dn), fw = vm_graphic_get_string_width(dn);
                if (is_sel && fw > nw) {
                    int off = g_app.sel_char_offset % (fl + 5);
                    draw_text_truncated(nx, iy + 2, (off < fl) ? (dn + off) : dn, nw, rc);
                } else draw_text_truncated(nx, iy + 2, dn, nw, rc);
            }
            if (g_app.track_count > vis && vis > 0) {
                int sb_x = sw - 6, sb_y = g_app.header_h + 2, sb_h = body_h - 4;
                vm_graphic_color tc = lerp_color(th->bg, th->popup_border, 45);
                vm_graphic_setcolor(&tc); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 4, sb_h);
                int th_h = (vis * sb_h) / g_app.track_count; if (th_h < 8) th_h = 8;
                int max_s = g_app.track_count - vis;
                int ty = sb_y + (max_s > 0 ? (g_app.scroll_offset * (sb_h - th_h)) / max_s : 0);
                vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, ty, 4, th_h);
            }
        }
        draw_now_playing_dock(th);
    }

    if (g_app.ui_mode == MODE_MENU) {
        PopupListGeom g = calc_popup_list_geom(MENU_ITEM_COUNT, false);
        adjust_popup_scroll(MENU_ITEM_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, NULL, th);
        const char* items[MENU_ITEM_COUNT] = {
            g_app.play_state == STATE_PLAYING ? "Pause" : "Play",
            "Jump Timestamp",
            "File Tags",
            "Modes >",
            "Settings >",
            "Help",
            "About"
        };
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i; if (idx >= MENU_ITEM_COUNT) break;
            int iy = g.py + 6 + i * g.row_h;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 3, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_selected_text);
            } else draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_text);
        }
    } else if (g_app.ui_mode == MODE_MODES_MENU) {
        PopupListGeom g = calc_popup_list_geom(MODES_MENU_ITEM_COUNT, true);
        adjust_popup_scroll(MODES_MENU_ITEM_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Modes:", th);
        char ii[32]; sprintf(ii, "ID3 Tags: %s", g_app.id3_mode ? "On" : "Off");
        const char* items[MODES_MENU_ITEM_COUNT] = { "Karaoke Mode", ii, "Play from Folder" };
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i; if (idx >= MODES_MENU_ITEM_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_selected_text);
            } else draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_text);
        }
    } else if (g_app.ui_mode == MODE_SETTINGS_MENU) {
        PopupListGeom g = calc_popup_list_geom(SETTINGS_MENU_ITEM_COUNT, true);
        adjust_popup_scroll(SETTINGS_MENU_ITEM_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Settings:", th);
        char vi[32], li[32], shi[32], si[32], ki[32], pi[32], ci[32];
        sprintf(vi,  "Volume: %d/6", g_app.volume);
        sprintf(li,  "Loop Mode: %s", LOOP_NAMES[g_app.loop_mode]);
        sprintf(shi, "Shuffle: %s", g_app.shuffle_mode ? "On" : "Off");
        sprintf(si,  "Sort By: %s", SORT_SHORT_NAMES[g_app.sort_mode]);
        sprintf(ki,  "Keep Screen On: %s", g_app.keep_screen_on ? "On" : "Off");
        sprintf(pi,  "Scroll on Pause: %s", g_app.scroll_on_pause ? "On" : "Off");
        sprintf(ci,  "Show Clock: %s", g_app.show_clock ? "On" : "Off");
        const char* items[SETTINGS_MENU_ITEM_COUNT] = {
            vi, "Audio Output", li, shi, si, ki, pi, ci, "Format Filter", "Themes"
        };
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i; if (idx >= SETTINGS_MENU_ITEM_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_selected_text);
            } else draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_text);
        }
    } else if (g_app.ui_mode == MODE_FOLDER_OPTIONS) {
        PopupListGeom g = calc_popup_list_geom(FOLDER_OPT_ITEM_COUNT, true);
        adjust_popup_scroll(FOLDER_OPT_ITEM_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Folder Options:", th);
        const char* items[FOLDER_OPT_ITEM_COUNT] = { "Open", "Choose", "Choose with all subfolders", "Cancel" };
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i; if (idx >= FOLDER_OPT_ITEM_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_selected_text);
            } else draw_ascii_truncated(g.px + 8, iy + 1, items[idx], g.pw - 18, th->item_text);
        }
    } else if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        int pw = sw - 24, ph = fh * 3 + 38, px = 12, py = (sh - g_app.footer_h - ph) / 2; if (py < 4) py = 4;
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        char vl[32]; sprintf(vl, "Volume Level: %d / 6", g_app.volume); draw_ascii(px + 10, py + 8, vl, th->header_text);

        VMWCHAR w_m[2] = { '-', 0 }, w_p[2] = { '+', 0 };
        int mw = vm_graphic_get_string_width(w_m), gap = 3, seg_w = ((pw - 56) - 5 * gap) / 6; if (seg_w < 4) seg_w = 4;
        int total_w = 6 * seg_w + 5 * gap, bar_x = px + (pw - total_w) / 2, bar_h = fh + 2, bar_y = py + fh + 16, sym_y = bar_y + (bar_h - fh) / 2;
        draw_text(bar_x - mw - 8, sym_y, w_m, th->header_text); draw_text(bar_x + total_w + 8, sym_y, w_p, th->header_text);
        vm_graphic_color ec = lerp_color(th->popup_bg, th->popup_border, 35);
        for (int s = 0; s < 6; s++) {
            int sx = bar_x + s * (seg_w + gap); vm_graphic_color sc = (s < g_app.volume) ? th->item_playing : ec;
            vm_graphic_setcolor(&sc); vm_graphic_fill_rect_ex(g_app.layer_hdl, sx, bar_y, seg_w, bar_h);
            vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, sx, bar_y, seg_w, bar_h);
        }
        draw_ascii_truncated(px + 10, py + fh * 2 + 24, "Tap bar, Left/Right or 0-6", pw - 20, th->item_text);
    } else if (g_app.ui_mode == MODE_DEVICE_SELECT || g_app.ui_mode == MODE_THEME_SELECT ||
               g_app.ui_mode == MODE_FORMAT_FILTER || g_app.ui_mode == MODE_SORT_SELECT ||
               g_app.ui_mode == MODE_HELP_INDEX) {
        int total = get_active_popup_total_items();
        PopupListGeom g = calc_popup_list_geom(total, true);
        adjust_popup_scroll(total, g.visible_count);
        const char* pt = (g_app.ui_mode == MODE_DEVICE_SELECT) ? "Select Output Device:"
                       : (g_app.ui_mode == MODE_THEME_SELECT) ? "Select Color Theme:"
                       : (g_app.ui_mode == MODE_FORMAT_FILTER) ? "Supported Formats:"
                       : (g_app.ui_mode == MODE_SORT_SELECT) ? "Sort Tracks By:"
                       : (g_app.ui_mode == MODE_HELP_INDEX) ? "Help Topics:" : "Pick LRC Lyric File:";
        draw_popup_list_frame_and_sb(&g, pt, th);

        const char* fn[] = { "MP3", "MIDI", "IMY (iMelody)", "AAC", "AMR (Voice)", "AWB (AMR-WB)", "VM (VoiceMemo)", "WAV" };
        VMUINT8 mk[] = { FMT_MP3, FMT_MIDI, FMT_IMY, FMT_AAC, FMT_AMR, FMT_AWB, FMT_VM, FMT_WAV };
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i; if (idx >= total) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            vm_graphic_color col = (idx == g_app.menu_cursor) ? th->item_selected_text : th->item_text;
            if (idx == g_app.menu_cursor) { vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h); }
            char ln[64];
            if (g_app.ui_mode == MODE_DEVICE_SELECT) { sprintf(ln, "%s %s", (idx == (int)g_app.cur_device ? "*" : " "), DEVICE_PROFILE_NAMES[idx]); draw_ascii_truncated(g.px + 8, iy + 1, ln, g.pw - 18, col); }
            else if (g_app.ui_mode == MODE_THEME_SELECT) { sprintf(ln, "%s %s", (idx == g_app.cur_theme ? "*" : " "), g_themes[idx].name); draw_ascii_truncated(g.px + 8, iy + 1, ln, g.pw - 18, col); }
            else if (g_app.ui_mode == MODE_FORMAT_FILTER) { sprintf(ln, "[%c] %s", (g_app.enabled_formats & mk[idx]) ? 'X' : ' ', fn[idx]); draw_ascii_truncated(g.px + 8, iy + 1, ln, g.pw - 18, col); }
            else if (g_app.ui_mode == MODE_SORT_SELECT) { sprintf(ln, "%s %s", (idx == (int)g_app.sort_mode ? "*" : " "), SORT_NAMES[idx]); draw_ascii_truncated(g.px + 8, iy + 1, ln, g.pw - 18, col); }
            else if (g_app.ui_mode == MODE_HELP_INDEX) draw_ascii_truncated(g.px + 8, iy + 1, g_help_articles[idx].title, g.pw - 18, col);
        }
    } else if (g_app.ui_mode == MODE_TIME_PICKER) {
        int pw = sw - 20, ph = fh * 5 + 32, px = 10, py = (sh - g_app.footer_h - ph) / 2; if (py < 4) py = 4;
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        draw_ascii(px + 8, py + 6, "Enter Timestamp:", th->header_text);

        VMWCHAR w_z[2] = { '0', 0 }, w_c[2] = { ':', 0 };
        int dw = vm_graphic_get_string_width(w_z); if (dw <= 0) dw = 8;
        int cw = vm_graphic_get_string_width(w_c); if (cw <= 0) cw = 5;
        int cell_w = dw + 8, cx = px + (pw - (6 * cell_w + 2 * (cw + 8))) / 2;
        int uy = py + fh + 10, dy = uy + fh + 4, vy = dy + fh + 6;
        for (int i = 0; i < 6; i++) {
            if (i == 2 || i == 4) { draw_ascii(cx + 4, dy, ":", th->header_text); cx += cw + 8; }
            char ds[2] = { (char)('0' + g_app.time_digits[i]), 0 };
            draw_ascii(cx + 4, uy, "^", (i == g_app.time_cursor) ? th->item_playing : th->item_text);
            if (i == g_app.time_cursor) { vm_graphic_setcolor((vm_graphic_color*)&th->item_selected_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, cx, dy - 2, cell_w, fh + 4); draw_ascii(cx + 4, dy, ds, th->item_selected_text); }
            else draw_ascii(cx + 4, dy, ds, th->item_text);
            draw_ascii(cx + 4, vy, "v", (i == g_app.time_cursor) ? th->item_playing : th->item_text);
            cx += cell_w;
        }
        draw_ascii_truncated(px + 8, vy + fh + 6, "Tap ^ / v or digits", pw - 16, th->item_text);
    } else if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) {
        int pw = sw - 40, ph = fh * 2 + 20, px = (sw - pw) / 2, py = (sh - ph) / 2;
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        draw_ascii_truncated(px + 14, py + (ph - fh) / 2, "Exit ABookPlayer?", pw - 28, th->item_text);
    } else if (g_app.ui_mode == MODE_POPUP_ABOUT) {
        const char* av = "Made with Gemini 4 Hydrogen. Ver. 1.2";
        VMWCHAR wa[64]; safe_ascii_to_ucs2(wa, 64, av);
        int pw = sw - 20; bool one = (vm_graphic_get_string_width(wa) <= pw - 24);
        int ph = fh * (one ? 3 : 4) + 26, px = 10, py = (sh - ph) / 2;
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        draw_ascii_truncated(px + 12, py + 10, "http://stacorp.mw.lt", pw - 24, th->item_playing);
        if (one) draw_ascii(px + 12, py + 14 + fh, av, th->item_text);
        else { draw_ascii_truncated(px + 12, py + 14 + fh, "Made with Gemini 4 Hydrogen.", pw - 24, th->item_text); draw_ascii_truncated(px + 12, py + 16 + fh * 2, "Ver. 1.2", pw - 24, th->item_text); }
    } else if (g_app.ui_mode == MODE_POPUP_ERROR) {
        int pw = sw - 20, ph = fh * 5 + 32, px = 10, py = (sh - ph) / 2;
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        draw_ascii_truncated(px + 10, py + 6, g_app.error_title, pw - 20, th->item_playing);
        draw_ascii_truncated(px + 10, py + 8 + fh, g_app.error_line1, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 10 + fh * 2, g_app.error_line2, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 12 + fh * 3, g_app.error_line3, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 16 + fh * 4, "[Press OK/RSK or Tap]", pw - 20, th->header_text);
    }

    int fy = sh - g_app.footer_h;
    vm_graphic_setcolor((vm_graphic_color*)&th->softkey_bg); vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, fy, sw, g_app.footer_h);
    vm_graphic_setcolor((vm_graphic_color*)&th->popup_border); vm_graphic_line_ex(g_app.layer_hdl, 0, fy, sw - 1, fy);

    const char *lsk = "Options", *rsk = "Exit";
    if (g_app.ui_mode == MODE_MENU || g_app.ui_mode == MODE_MODES_MENU || g_app.ui_mode == MODE_SETTINGS_MENU ||
        g_app.ui_mode == MODE_FOLDER_OPTIONS || g_app.ui_mode == MODE_DEVICE_SELECT || g_app.ui_mode == MODE_SORT_SELECT ||
        g_app.ui_mode == MODE_HELP_INDEX) { lsk = "Select"; rsk = "Back"; }
    else if (g_app.ui_mode == MODE_FOLDER_VIEWER) {
        lsk = (g_app.folder_viewer && g_app.folder_viewer->lrc_mode) ? "Select" : "Options";
        rsk = "Back";
    }
    else if (g_app.ui_mode == MODE_HELP_READER) { lsk = "Topics"; rsk = "Back"; }
    else if (g_app.ui_mode == MODE_FILE_TAGS) { lsk = ""; rsk = "Close"; }
    else if (g_app.ui_mode == MODE_VOLUME_SELECT) { lsk = "OK"; rsk = "Back"; }
    else if (g_app.ui_mode == MODE_KARAOKE) { lsk = "Pick LRC"; rsk = "Back"; }
    else if (g_app.ui_mode == MODE_TIME_PICKER) { lsk = "Jump"; rsk = "Cancel"; }
    else if (g_app.ui_mode == MODE_FORMAT_FILTER || g_app.ui_mode == MODE_THEME_SELECT) { lsk = "Toggle"; rsk = "Done"; }
    else if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) { lsk = "Yes"; rsk = "No"; }
    else if (g_app.ui_mode == MODE_POPUP_ABOUT) { lsk = ""; rsk = "Close"; }
    else if (g_app.ui_mode == MODE_POPUP_ERROR) { lsk = ""; rsk = "OK"; }

    int sy = fy + (g_app.footer_h - fh) / 2;
    if (lsk[0]) draw_ascii(10, sy, lsk, th->softkey_text);
    if (rsk[0]) { VMWCHAR wr[32]; safe_ascii_to_ucs2(wr, 32, rsk); draw_text(sw - vm_graphic_get_string_width(wr) - 10, sy, wr, th->softkey_text); }

    VMINT h = g_app.layer_hdl;
    vm_graphic_flush_layer(&h, 1);
}

static void trigger_repaint() { render_screen(); }

static void set_ui_mode_with_cursor(AppUIMode mode, int cursor) {
    if ((g_app.ui_mode == MODE_HELP_READER || g_app.ui_mode == MODE_FILE_TAGS) &&
        mode != MODE_HELP_READER && mode != MODE_FILE_TAGS) {
        reader_unload();
    }
    bool was_karaoke_or_lrc = (g_app.ui_mode == MODE_KARAOKE) ||
        (g_app.ui_mode == MODE_FOLDER_VIEWER && g_app.folder_viewer && g_app.folder_viewer->lrc_mode);
    bool is_karaoke_or_lrc = (mode == MODE_KARAOKE) ||
        (mode == MODE_FOLDER_VIEWER && g_app.folder_viewer && g_app.folder_viewer->lrc_mode);
    if (was_karaoke_or_lrc && !is_karaoke_or_lrc) {
        karaoke_unload();
    }
    if ((g_app.ui_mode == MODE_FOLDER_VIEWER || g_app.ui_mode == MODE_FOLDER_OPTIONS) &&
        mode != MODE_FOLDER_VIEWER && mode != MODE_FOLDER_OPTIONS) {
        folder_viewer_unload();
    }
    g_app.ui_mode = mode; g_app.menu_cursor = cursor; g_app.menu_scroll = 0;
    trigger_repaint();
}

static void set_ui_mode(AppUIMode mode) { set_ui_mode_with_cursor(mode, 0); }

// ============================================================================
// MODULE 11: Input Handlers & Lifecycle
// ============================================================================
static void handle_key_event(VMINT event, VMINT keycode) {
    if (event != VM_KEY_EVENT_DOWN && event != VM_KEY_EVENT_REPEAT) return;

    if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) {
        if (keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_OK) {
            stop_playback();
            vm_switch_power_saving_mode(turn_on_mode);
            vm_exit_app();
        }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR || keycode == VM_KEY_BACK) set_ui_mode(MODE_TRACKLIST);
        return;
    }
    if (g_app.ui_mode == MODE_POPUP_ABOUT || g_app.ui_mode == MODE_POPUP_ERROR) {
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_OK || keycode == VM_KEY_CLEAR ||
            keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_BACK) set_ui_mode(MODE_TRACKLIST);
        return;
    }
    if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        if (keycode == VM_KEY_LEFT || keycode == VM_KEY_DOWN || keycode == VM_KEY_VOL_DOWN) apply_live_volume(g_app.volume - 1);
        else if (keycode == VM_KEY_RIGHT || keycode == VM_KEY_UP || keycode == VM_KEY_VOL_UP) apply_live_volume(g_app.volume + 1);
        else if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM6) apply_live_volume(keycode - VM_KEY_NUM0);
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor);
            return;
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_HELP_INDEX) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + HELP_TOPIC_COUNT - 1) % HELP_TOPIC_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % HELP_TOPIC_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            if (help_load(g_app.menu_cursor)) set_ui_mode(MODE_HELP_READER);
            else show_audio_error("Memory Error", "Failed to load help", "", "");
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) { set_ui_mode_with_cursor(MODE_MENU, 5); return; }
        trigger_repaint(); return;
    }
    if ((g_app.ui_mode == MODE_HELP_READER || g_app.ui_mode == MODE_FILE_TAGS) && g_app.reader) {
        int lh = g_app.font_h + 3; if (lh < 14) lh = 14;
        int vis = (g_app.screen_h - g_app.header_h - g_app.footer_h - 8) / lh; if (vis < 1) vis = 1;
        int max_s = g_app.reader->total_lines - vis; if (max_s < 0) max_s = 0;
        if (keycode == VM_KEY_UP && g_app.reader->scroll_line > 0) g_app.reader->scroll_line--;
        else if (keycode == VM_KEY_DOWN && g_app.reader->scroll_line < max_s) g_app.reader->scroll_line++;
        else if (keycode == VM_KEY_LEFT || keycode == VM_KEY_STAR) { g_app.reader->scroll_line -= vis; if (g_app.reader->scroll_line < 0) g_app.reader->scroll_line = 0; }
        else if (keycode == VM_KEY_RIGHT || keycode == VM_KEY_POUND) { g_app.reader->scroll_line += vis; if (g_app.reader->scroll_line > max_s) g_app.reader->scroll_line = max_s; }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_OK || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            if (g_app.ui_mode == MODE_HELP_READER) set_ui_mode_with_cursor(MODE_HELP_INDEX, g_app.reader->topic_idx);
            else set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_KARAOKE) {
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) { set_ui_mode(MODE_TRACKLIST); return; }
        else if (keycode == VM_KEY_LEFT_SOFTKEY) { if (folder_viewer_load(true)) set_ui_mode(MODE_FOLDER_VIEWER); return; }
        else if (keycode == VM_KEY_LEFT) { g_app.screensaver_id = (ScreensaverID)((g_app.screensaver_id + SS_COUNT - 1) % SS_COUNT); init_screensaver_state(); }
        else if (keycode == VM_KEY_RIGHT) { g_app.screensaver_id = (ScreensaverID)((g_app.screensaver_id + 1) % SS_COUNT); init_screensaver_state(); }
        else if (keycode == VM_KEY_UP) g_app.lrc_user_offset_ms += 500;
        else if (keycode == VM_KEY_DOWN) g_app.lrc_user_offset_ms -= 500;
        else if (keycode == VM_KEY_VOL_UP) apply_live_volume(g_app.volume + 1);
        else if (keycode == VM_KEY_VOL_DOWN) apply_live_volume(g_app.volume - 1);
        else if (keycode == VM_KEY_OK) {
            if (g_app.play_state == STATE_PLAYING) pause_playback();
            else if (g_app.play_state == STATE_PAUSED) resume_playback();
            else if (g_app.track_count > 0) start_playback(g_app.selected_track, 0);
        } else if (keycode == VM_KEY_STAR && g_app.track_count > 0) {
            int cur = (g_app.active_track_idx >= 0 ? g_app.active_track_idx : g_app.selected_track);
            int prev = (g_app.loop_mode == LOOP_LIST && g_app.shuffle_mode)
                ? pick_next_list_track(cur)
                : (cur + g_app.track_count - 1) % g_app.track_count;
            g_app.selected_track = prev; start_playback(prev, 0);
        } else if (keycode == VM_KEY_POUND && g_app.track_count > 0) {
            int cur = (g_app.active_track_idx >= 0 ? g_app.active_track_idx : g_app.selected_track);
            int next = (g_app.loop_mode == LOOP_LIST && g_app.shuffle_mode)
                ? pick_next_list_track(cur)
                : (cur + 1) % g_app.track_count;
            g_app.selected_track = next; start_playback(next, 0);
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_FOLDER_OPTIONS) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + FOLDER_OPT_ITEM_COUNT - 1) % FOLDER_OPT_ITEM_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % FOLDER_OPT_ITEM_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            if (g_app.menu_cursor == 0) { folder_viewer_open_selected(); g_app.sel_char_offset = 0; set_ui_mode(MODE_FOLDER_VIEWER); }
            else if (g_app.menu_cursor == 1) folder_viewer_choose_selected(false);
            else if (g_app.menu_cursor == 2) folder_viewer_choose_selected(true);
            else set_ui_mode(MODE_FOLDER_VIEWER);
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            set_ui_mode(MODE_FOLDER_VIEWER); return;
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_FOLDER_VIEWER && g_app.folder_viewer) {
        FolderViewerState* fv = g_app.folder_viewer;
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            if (fv->at_drive_list) {
                set_ui_mode_with_cursor(fv->lrc_mode ? MODE_KARAOKE : MODE_MODES_MENU, fv->lrc_mode ? 0 : 2);
            } else {
                folder_viewer_go_up(); g_app.sel_char_offset = 0; trigger_repaint();
            }
            return;
        }
        if (keycode == VM_KEY_LEFT_SOFTKEY) {
            if (!fv->lrc_mode) { set_ui_mode(MODE_FOLDER_OPTIONS); return; }
            keycode = VM_KEY_OK;
        }
        if (keycode == VM_KEY_LEFT) { folder_viewer_go_up(); g_app.sel_char_offset = 0; trigger_repaint(); return; }
        if (keycode == VM_KEY_OK || keycode == VM_KEY_RIGHT) {
            if (fv->cursor >= 0 && fv->cursor < fv->entry_count) {
                const FolderEntry* e = &fv->entries[fv->cursor];
                if (fv->lrc_mode) {
                    int target = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
                    if (e->type == FENT_LRC_AUTO) { auto_load_lrc_for_track(target); set_ui_mode(MODE_KARAOKE); return; }
                    if (e->type == FENT_LRC_NONE) { clear_lrc_data(); if (g_app.karaoke) g_app.karaoke->lrc_track_idx = target; set_ui_mode(MODE_KARAOKE); return; }
                    if (e->type == FENT_FILE) {
                        VMWCHAR full_lrc[260]; make_canonical_path(full_lrc, fv->cur_path, e->name);
                        load_lrc_file_path(full_lrc);
                        if (g_app.karaoke) g_app.karaoke->lrc_track_idx = target;
                        set_ui_mode(MODE_KARAOKE); return;
                    }
                } else if (e->type == FENT_THIS) {
                    folder_viewer_choose_selected(false); return;
                }
                if (e->type == FENT_UP || e->type == FENT_DRIVE || e->type == FENT_SUBDIR) {
                    folder_viewer_open_selected(); g_app.sel_char_offset = 0;
                }
            }
            trigger_repaint(); return;
        }

        int prev_c = fv->cursor;
        if (keycode == VM_KEY_STAR) { fv->cursor -= 10; if (fv->cursor < 0) fv->cursor = 0; }
        else if (keycode == VM_KEY_POUND) { fv->cursor += 10; if (fv->cursor >= fv->entry_count) fv->cursor = fv->entry_count - 1; }
        else if (keycode == VM_KEY_UP && fv->cursor > 0) fv->cursor--;
        else if (keycode == VM_KEY_DOWN && fv->cursor < fv->entry_count - 1) fv->cursor++;
        if (fv->cursor != prev_c) g_app.sel_char_offset = 0;

        int br = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int ih = g_app.font_h + 4; if (ih <= 0) ih = 18;
        int vis = (g_app.screen_h - g_app.header_h - br) / ih; if (vis < 1) vis = 1;
        if (fv->cursor < fv->scroll) fv->scroll = fv->cursor;
        else if (fv->cursor >= fv->scroll + vis) fv->scroll = fv->cursor - vis + 1;
        if (fv->scroll < 0) fv->scroll = 0;
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_DEVICE_SELECT) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + DEV_PROFILE_COUNT - 1) % DEV_PROFILE_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % DEV_PROFILE_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) { g_app.cur_device = (AudioDeviceProfile)g_app.menu_cursor; set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_THEME_SELECT) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + THEME_COUNT - 1) % THEME_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % THEME_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) g_app.cur_theme = g_app.menu_cursor;
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_FORMAT_FILTER) {
        VMUINT8 masks[] = { FMT_MP3, FMT_MIDI, FMT_IMY, FMT_AAC, FMT_AMR, FMT_AWB, FMT_VM, FMT_WAV };
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + 7) % 8;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % 8;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) { g_app.enabled_formats ^= masks[g_app.menu_cursor]; refresh_track_library(); }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_SORT_SELECT) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + SORT_COUNT - 1) % SORT_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % SORT_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) { g_app.sort_mode = (SortMode)g_app.menu_cursor; sort_track_library(); set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_TIME_PICKER) {
        if (keycode == VM_KEY_LEFT) g_app.time_cursor = (g_app.time_cursor + 5) % 6;
        else if (keycode == VM_KEY_RIGHT) g_app.time_cursor = (g_app.time_cursor + 1) % 6;
        else if (keycode == VM_KEY_UP) g_app.time_digits[g_app.time_cursor] = (g_app.time_digits[g_app.time_cursor] + 1) % 10;
        else if (keycode == VM_KEY_DOWN) g_app.time_digits[g_app.time_cursor] = (g_app.time_digits[g_app.time_cursor] + 9) % 10;
        else if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM9) { g_app.time_digits[g_app.time_cursor] = keycode - VM_KEY_NUM0; g_app.time_cursor = (g_app.time_cursor + 1) % 6; }
        else if (keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_OK) { apply_timestamp_jump(); if (g_app.ui_mode != MODE_POPUP_ERROR) set_ui_mode(MODE_TRACKLIST); return; }
        else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode(MODE_TRACKLIST); return; }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_MODES_MENU) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + MODES_MENU_ITEM_COUNT - 1) % MODES_MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % MODES_MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            switch (g_app.menu_cursor) {
                case 0: {
                    int target = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
                    if (karaoke_load(target)) set_ui_mode(MODE_KARAOKE);
                    else show_audio_error("Memory Error", "Cannot open Karaoke", "", "");
                    return;
                }
                case 1:
                    g_app.id3_mode = !g_app.id3_mode;
                    if (g_app.id3_mode) id3_load_all(); else id3_unload_all();
                    g_app.sel_char_offset = 0; g_app.marq_char_offset = 0;
                    break;
                case 2:
                    if (folder_viewer_load(false)) set_ui_mode(MODE_FOLDER_VIEWER);
                    else show_audio_error("Memory Error", "Cannot open Folder", "", "");
                    return;
            }
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            set_ui_mode_with_cursor(MODE_MENU, 3); return;
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_SETTINGS_MENU) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + SETTINGS_MENU_ITEM_COUNT - 1) % SETTINGS_MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % SETTINGS_MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_LEFT || keycode == VM_KEY_RIGHT) {
            if (g_app.menu_cursor == 0) apply_live_volume(g_app.volume + (keycode == VM_KEY_RIGHT ? 1 : -1));
            else if (g_app.menu_cursor == 2) g_app.loop_mode = (LoopMode)((g_app.loop_mode + (keycode == VM_KEY_RIGHT ? 1 : LOOP_COUNT - 1)) % LOOP_COUNT);
            else if (g_app.menu_cursor == 3) g_app.shuffle_mode = !g_app.shuffle_mode;
            else if (g_app.menu_cursor == 5) { g_app.keep_screen_on = !g_app.keep_screen_on; apply_power_saving_mode(); }
            else if (g_app.menu_cursor == 6) g_app.scroll_on_pause = !g_app.scroll_on_pause;
            else if (g_app.menu_cursor == 7) g_app.show_clock = !g_app.show_clock;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            switch (g_app.menu_cursor) {
                case 0: g_app.return_mode = MODE_SETTINGS_MENU; g_app.return_cursor = 0; set_ui_mode(MODE_VOLUME_SELECT); return;
                case 1: g_app.return_mode = MODE_SETTINGS_MENU; g_app.return_cursor = 1; set_ui_mode_with_cursor(MODE_DEVICE_SELECT, (int)g_app.cur_device); return;
                case 2: g_app.loop_mode = (LoopMode)((g_app.loop_mode + 1) % LOOP_COUNT); break;
                case 3: g_app.shuffle_mode = !g_app.shuffle_mode; break;
                case 4: g_app.return_mode = MODE_SETTINGS_MENU; g_app.return_cursor = 4; set_ui_mode_with_cursor(MODE_SORT_SELECT, (int)g_app.sort_mode); return;
                case 5: g_app.keep_screen_on = !g_app.keep_screen_on; apply_power_saving_mode(); break;
                case 6: g_app.scroll_on_pause = !g_app.scroll_on_pause; break;
                case 7: g_app.show_clock = !g_app.show_clock; break;
                case 8: g_app.return_mode = MODE_SETTINGS_MENU; g_app.return_cursor = 8; set_ui_mode(MODE_FORMAT_FILTER); return;
                case 9: g_app.return_mode = MODE_SETTINGS_MENU; g_app.return_cursor = 9; set_ui_mode_with_cursor(MODE_THEME_SELECT, g_app.cur_theme); return;
            }
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            set_ui_mode_with_cursor(MODE_MENU, 4); return;
        }
        trigger_repaint(); return;
    }
    if (g_app.ui_mode == MODE_MENU) {
        if (keycode == VM_KEY_UP) g_app.menu_cursor = (g_app.menu_cursor + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_DOWN) g_app.menu_cursor = (g_app.menu_cursor + 1) % MENU_ITEM_COUNT;
        else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            switch (g_app.menu_cursor) {
                case 0:
                    if (g_app.play_state == STATE_PLAYING) { pause_playback(); set_ui_mode(MODE_TRACKLIST); }
                    else if (g_app.play_state == STATE_PAUSED) { resume_playback(); set_ui_mode(MODE_TRACKLIST); }
                    else { start_playback(g_app.selected_track, 0); if (g_app.ui_mode != MODE_POPUP_ERROR) set_ui_mode(MODE_TRACKLIST); }
                    return;
                case 1: prepare_time_picker(g_app.current_time_sec); set_ui_mode(MODE_TIME_PICKER); return;
                case 2:
                    if (file_tags_load(g_app.selected_track)) set_ui_mode(MODE_FILE_TAGS);
                    else show_audio_error("Memory Error", "Cannot load tags", "", "");
                    return;
                case 3: set_ui_mode(MODE_MODES_MENU); return;
                case 4: set_ui_mode(MODE_SETTINGS_MENU); return;
                case 5: set_ui_mode(MODE_HELP_INDEX); return;
                case 6: set_ui_mode(MODE_POPUP_ABOUT); return;
            }
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) { set_ui_mode(MODE_TRACKLIST); return; }
        trigger_repaint(); return;
    }

    if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR || keycode == VM_KEY_BACK) { set_ui_mode(MODE_POPUP_CONFIRM_EXIT); return; }
    if (keycode == VM_KEY_LEFT_SOFTKEY) { set_ui_mode(MODE_MENU); return; }
    if (keycode == VM_KEY_LEFT) { g_app.id3_mode = !g_app.id3_mode; if (g_app.id3_mode) id3_load_all(); else id3_unload_all(); g_app.sel_char_offset = 0; g_app.marq_char_offset = 0; trigger_repaint(); return; }
    if (keycode == VM_KEY_RIGHT) { g_app.return_mode = MODE_TRACKLIST; g_app.return_cursor = 0; set_ui_mode_with_cursor(MODE_SORT_SELECT, (int)g_app.sort_mode); return; }
    if (keycode == VM_KEY_VOL_UP) { apply_live_volume(g_app.volume + 1); trigger_repaint(); return; }
    if (keycode == VM_KEY_VOL_DOWN) { apply_live_volume(g_app.volume - 1); trigger_repaint(); return; }
    if (keycode == VM_KEY_OK) {
        if (g_app.play_state == STATE_PLAYING && g_app.active_track_idx == g_app.selected_track) pause_playback();
        else if (g_app.play_state == STATE_PAUSED && g_app.active_track_idx == g_app.selected_track) resume_playback();
        else start_playback(g_app.selected_track, 0);
        trigger_repaint(); return;
    }
    if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM9) {
        if (g_app.numeric_len < 4) {
            g_app.numeric_buf[g_app.numeric_len++] = '0' + (keycode - VM_KEY_NUM0); g_app.numeric_buf[g_app.numeric_len] = 0;
            if (g_app.numeric_timer != 0) vm_delete_timer_ex(g_app.numeric_timer);
            g_app.numeric_timer = vm_create_timer_ex(NUMERIC_INPUT_TIMEOUT_MS, numeric_timeout);
            trigger_repaint();
        }
        return;
    }

    int prev_sel = g_app.selected_track;
    if (keycode == VM_KEY_STAR) { g_app.selected_track -= 10; if (g_app.selected_track < 0) g_app.selected_track = 0; }
    else if (keycode == VM_KEY_POUND) { g_app.selected_track += 10; if (g_app.selected_track >= g_app.track_count) g_app.selected_track = g_app.track_count - 1; }
    if (keycode == VM_KEY_UP && g_app.selected_track > 0) g_app.selected_track--;
    else if (keycode == VM_KEY_DOWN && g_app.selected_track < g_app.track_count - 1) g_app.selected_track++;
    if (g_app.selected_track != prev_sel) g_app.sel_char_offset = 0;

    int br = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
    int item_h = g_app.font_h + 4; if (item_h <= 0) item_h = 18;
    int vis = (g_app.screen_h - g_app.header_h - br) / item_h; if (vis < 1) vis = 1;
    if (g_app.selected_track < g_app.scroll_offset) g_app.scroll_offset = g_app.selected_track;
    else if (g_app.selected_track >= g_app.scroll_offset + vis) g_app.scroll_offset = g_app.selected_track - vis + 1;
    if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;
    trigger_repaint();
}

static bool handle_dock_tap(VMINT x, VMINT y) {
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
    if (g_app.play_state != STATE_STOPPED && y >= sh - bottom_reserved && y < sh - g_app.footer_h) {
        int my = sh - g_app.footer_h - g_app.marquee_h;
        if (y < my + fh + 3) {
            if (x < sw / 2) { prepare_time_picker(g_app.current_time_sec); set_ui_mode(MODE_TIME_PICKER); }
            else if (x < (sw * 3) / 4) { g_app.return_mode = g_app.ui_mode; g_app.return_cursor = 0; set_ui_mode(MODE_VOLUME_SELECT); }
            else { g_app.loop_mode = (LoopMode)((g_app.loop_mode + 1) % LOOP_COUNT); trigger_repaint(); }
        } else { if (g_app.play_state == STATE_PLAYING) pause_playback(); else resume_playback(); trigger_repaint(); }
        return true;
    }
    return false;
}

static void handle_pen_tap_released(VMINT x, VMINT y) {
    int sw = g_app.screen_w, sh = g_app.screen_h, fh = g_app.font_h;
    if (y >= sh - g_app.footer_h) { handle_key_event(VM_KEY_EVENT_DOWN, (x < sw / 2) ? VM_KEY_LEFT_SOFTKEY : VM_KEY_RIGHT_SOFTKEY); return; }
    if (g_app.ui_mode == MODE_POPUP_ABOUT || g_app.ui_mode == MODE_POPUP_ERROR) { set_ui_mode(MODE_TRACKLIST); return; }
    if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) return;

    if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        int pw = sw - 24, ph = fh * 3 + 38, px = 12, py = (sh - g_app.footer_h - ph) / 2; if (py < 4) py = 4;
        if (x < px || x >= px + pw || y < py || y >= py + ph) { set_ui_mode_with_cursor(g_app.return_mode, g_app.return_cursor); return; }
        int gap = 3, seg_w = ((pw - 56) - 5 * gap) / 6; if (seg_w < 4) seg_w = 4;
        int total_w = 6 * seg_w + 5 * gap, bar_x = px + (pw - total_w) / 2;
        if (x < bar_x) apply_live_volume(g_app.volume - 1);
        else if (x >= bar_x + total_w) apply_live_volume(g_app.volume + 1);
        else apply_live_volume((x - bar_x) / (seg_w + gap) + 1);
        trigger_repaint(); return;
    }

    if (g_app.ui_mode == MODE_TIME_PICKER) {
        int pw = sw - 20, ph = fh * 5 + 32, px = 10, py = (sh - g_app.footer_h - ph) / 2; if (py < 4) py = 4;
        if (x < px || x >= px + pw || y < py || y >= py + ph) { set_ui_mode(MODE_TRACKLIST); return; }
        VMWCHAR w_z[2] = { '0', 0 }, w_c[2] = { ':', 0 };
        int dw = vm_graphic_get_string_width(w_z); if (dw <= 0) dw = 8;
        int cw = vm_graphic_get_string_width(w_c); if (cw <= 0) cw = 5;
        int cell_w = dw + 8, cur_x = px + (pw - (6 * cell_w + 2 * (cw + 8))) / 2;
        int uy = py + fh + 10, dy = uy + fh + 4, vy = dy + fh + 6;
        for (int i = 0; i < 6; i++) {
            if (i == 2 || i == 4) cur_x += cw + 8;
            if (x >= cur_x && x < cur_x + cell_w) {
                g_app.time_cursor = i;
                if (y >= vy - 2 && y <= vy + fh + 6) g_app.time_digits[i] = (g_app.time_digits[i] + 9) % 10;
                else g_app.time_digits[i] = (g_app.time_digits[i] + 1) % 10;
                trigger_repaint(); return;
            }
            cur_x += cell_w;
        }
        return;
    }

    int total = get_active_popup_total_items();
    if (total > 0) {
        PopupListGeom g = calc_popup_list_geom(total, active_popup_has_title());
        if (x < g.px || x >= g.px + g.pw || y < g.py || y >= g.py + g.ph) { handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_RIGHT_SOFTKEY); return; }
        int rt = g.py + g.title_h + 6, rb = rt + g.visible_count * g.row_h;
        if (y >= rt && y < rb) {
            if (total > g.visible_count && x >= g.px + g.pw - 14) {
                if (y < (rt + rb) / 2) { g_app.menu_scroll -= g.visible_count; if (g_app.menu_scroll < 0) g_app.menu_scroll = 0; }
                else { g_app.menu_scroll += g.visible_count; if (g_app.menu_scroll > total - g.visible_count) g_app.menu_scroll = total - g.visible_count; }
                g_app.menu_cursor = g_app.menu_scroll; trigger_repaint(); return;
            }
            int tapped = g_app.menu_scroll + (y - rt) / g.row_h;
            if (tapped >= 0 && tapped < total) {
                if (g_app.menu_cursor == tapped) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
                else { g_app.menu_cursor = tapped; trigger_repaint(); }
            }
        }
        return;
    }

    if (g_app.ui_mode == MODE_HELP_READER || g_app.ui_mode == MODE_FILE_TAGS) {
        int top = g_app.header_h + 4, bot = sh - g_app.footer_h - 4;
        if (y >= top && y < bot) handle_key_event(VM_KEY_EVENT_DOWN, (y < (top + bot) / 2) ? VM_KEY_STAR : VM_KEY_POUND);
        return;
    }

    if (g_app.ui_mode == MODE_KARAOKE && y >= g_app.header_h && y < sh - g_app.footer_h) {
        int top_y = g_app.header_h, bot_y = sh - g_app.footer_h, row2_y = top_y + fh + 5, bar_y = bot_y - fh - 10;
        if (y >= row2_y - 2 && y < row2_y + fh + 6) {
            if (x < sw / 4) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_LEFT);
            else if (x < sw / 2 + 12) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_RIGHT);
            else if (x < (sw * 3) / 4 + 8) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_DOWN);
            else handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_UP);
        } else if (y >= bar_y - 6 && y < bot_y - fh - 3 && g_app.total_duration_sec > 0) {
            int rel = x - 8; if (rel < 0) rel = 0; if (rel > sw - 16) rel = sw - 16;
            VMUINT ts = ((VMUINT)rel * g_app.total_duration_sec) / (VMUINT)(sw - 16);
            int tt = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
            if (tt >= 0 && tt < g_app.track_count) start_playback(tt, ts * 1000);
            trigger_repaint();
        } else if (y >= bot_y - fh - 6) {
            if (x >= sw - 54 && x < sw - 34) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_STAR);
            else if (x >= sw - 24) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_POUND);
            else handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
        } else handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
        return;
    }

    if (g_app.ui_mode == MODE_FOLDER_VIEWER && g_app.folder_viewer) {
        FolderViewerState* fv = g_app.folder_viewer;
        if (y < g_app.header_h) { handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_LEFT); return; }
        if (handle_dock_tap(x, y)) return;
        int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int body_h = sh - g_app.header_h - bottom_reserved, item_h = fh + 4; if (item_h <= 0) item_h = 18;
        int vis = (body_h > 0) ? (body_h / item_h) : 1;
        if (y >= g_app.header_h && y < sh - bottom_reserved) {
            if (fv->entry_count > vis && x >= sw - 12) {
                if (y < g_app.header_h + body_h / 2) fv->scroll -= vis; else fv->scroll += vis;
                int max_s = fv->entry_count - vis; if (max_s < 0) max_s = 0;
                if (fv->scroll < 0) fv->scroll = 0; if (fv->scroll > max_s) fv->scroll = max_s;
                trigger_repaint(); return;
            }
            int tapped = fv->scroll + ((y - g_app.header_h) / item_h);
            if (tapped >= 0 && tapped < fv->entry_count) {
                if (fv->cursor == tapped) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
                else { fv->cursor = tapped; g_app.sel_char_offset = 0; trigger_repaint(); }
            }
        }
        return;
    }

    if (g_app.ui_mode == MODE_TRACKLIST) {
        if (y < g_app.header_h) { handle_key_event(VM_KEY_EVENT_DOWN, (x >= sw / 2) ? VM_KEY_LEFT : VM_KEY_RIGHT); return; }
        if (handle_dock_tap(x, y)) return;
        int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int body_h = sh - g_app.header_h - bottom_reserved, item_h = fh + 4; if (item_h <= 0) item_h = 18;
        int vis = (body_h > 0) ? (body_h / item_h) : 1;
        if (y >= g_app.header_h && y < sh - bottom_reserved) {
            if (g_app.track_count > vis && x >= sw - 12) {
                if (y < g_app.header_h + body_h / 2) g_app.scroll_offset -= vis; else g_app.scroll_offset += vis;
                int max_s = g_app.track_count - vis; if (max_s < 0) max_s = 0;
                if (g_app.scroll_offset < 0) g_app.scroll_offset = 0; if (g_app.scroll_offset > max_s) g_app.scroll_offset = max_s;
                trigger_repaint(); return;
            }
            int tapped = g_app.scroll_offset + ((y - g_app.header_h) / item_h);
            if (tapped >= 0 && tapped < g_app.track_count) {
                if (g_app.selected_track == tapped) handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
                else { g_app.selected_track = tapped; g_app.sel_char_offset = 0; trigger_repaint(); }
            }
        }
    }
}

static void handle_pen_event(VMINT event, VMINT x, VMINT y) {
    if (event == VM_PEN_EVENT_TAP) { g_app.pen_down_x = x; g_app.pen_down_y = y; g_app.pen_last_y = y; g_app.pen_moved = false; return; }
    if (event == VM_PEN_EVENT_MOVE) {
        int dy_t = y - g_app.pen_down_y, dx_t = x - g_app.pen_down_x;
        if (dy_t < 0) dy_t = -dy_t; if (dx_t < 0) dx_t = -dx_t;
        if (!g_app.pen_moved && (dy_t > 8 || dx_t > 8)) g_app.pen_moved = true;
        if (g_app.pen_moved) {
            int step = g_app.font_h + 4; if (step < 14) step = 14;
            int rows = (g_app.pen_last_y - y) / step;
            if (rows != 0) {
                g_app.pen_last_y -= rows * step;
                if (g_app.ui_mode == MODE_TRACKLIST) {
                    int br = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
                    int vis = (g_app.screen_h - g_app.header_h - br) / step; if (vis < 1) vis = 1;
                    int max_s = g_app.track_count - vis; if (max_s < 0) max_s = 0;
                    g_app.scroll_offset += rows; if (g_app.scroll_offset < 0) g_app.scroll_offset = 0; if (g_app.scroll_offset > max_s) g_app.scroll_offset = max_s;
                    trigger_repaint();
                } else if (g_app.ui_mode == MODE_FOLDER_VIEWER && g_app.folder_viewer) {
                    FolderViewerState* fv = g_app.folder_viewer;
                    int br = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
                    int vis = (g_app.screen_h - g_app.header_h - br) / step; if (vis < 1) vis = 1;
                    int max_s = fv->entry_count - vis; if (max_s < 0) max_s = 0;
                    fv->scroll += rows; if (fv->scroll < 0) fv->scroll = 0; if (fv->scroll > max_s) fv->scroll = max_s;
                    trigger_repaint();
                } else if ((g_app.ui_mode == MODE_HELP_READER || g_app.ui_mode == MODE_FILE_TAGS) && g_app.reader) {
                    int lh = g_app.font_h + 3; if (lh < 14) lh = 14;
                    int vis = (g_app.screen_h - g_app.header_h - g_app.footer_h - 8) / lh; if (vis < 1) vis = 1;
                    int max_s = g_app.reader->total_lines - vis; if (max_s < 0) max_s = 0;
                    g_app.reader->scroll_line += rows; if (g_app.reader->scroll_line < 0) g_app.reader->scroll_line = 0; if (g_app.reader->scroll_line > max_s) g_app.reader->scroll_line = max_s;
                    trigger_repaint();
                } else {
                    int total = get_active_popup_total_items();
                    if (total > 0) {
                        PopupListGeom g = calc_popup_list_geom(total, active_popup_has_title());
                        int max_s = total - g.visible_count; if (max_s < 0) max_s = 0;
                        g_app.menu_scroll += rows; if (g_app.menu_scroll < 0) g_app.menu_scroll = 0; if (g_app.menu_scroll > max_s) g_app.menu_scroll = max_s;
                        if (g_app.menu_cursor < g_app.menu_scroll) g_app.menu_cursor = g_app.menu_scroll;
                        if (g_app.menu_cursor >= g_app.menu_scroll + g.visible_count) g_app.menu_cursor = g_app.menu_scroll + g.visible_count - 1;
                        trigger_repaint();
                    }
                }
            }
        }
        return;
    }
    if (event == VM_PEN_EVENT_RELEASE) { if (!g_app.pen_moved) handle_pen_tap_released(x, y); g_app.pen_moved = false; }
}


static void handle_sysevt(VMINT message, VMINT param) {
    switch (message) {
        case VM_MSG_CREATE: {
            memset(&g_app, 0, sizeof(AppContext));
            vm_audio_terminate_background_play();
            vm_audio_stop_all();
            vm_midi_stop_all();
            g_app.audio_client_handle = vm_audio_register_interrupt_callback(audio_interrupt_callback);
            ss_rand_next = (unsigned int)vm_get_tick_count() ^ 0xA5A55A5Au;

            g_app.screen_w = vm_graphic_get_screen_width();
            g_app.screen_h = vm_graphic_get_screen_height();
            vm_graphic_set_font(VM_SMALL_FONT);
            int fh = vm_graphic_get_character_height();
            g_app.font_h = (fh > 0) ? fh : 14;
            VMWCHAR w_ref[8]; safe_ascii_to_ucs2(w_ref, 8, "AgHp|");
            VMINT fb = vm_graphic_get_string_baseline(w_ref);
            g_app.font_baseline = (fb > 0) ? (int)fb : ((g_app.font_h * 4) / 5);

            g_app.header_h = g_app.font_h + 6;
            g_app.footer_h = g_app.font_h + 6;
            g_app.marquee_h = (g_app.font_h + 2) * 2 + 4;

            g_app.cur_theme = THEME_BASIC_DARK;
            g_app.enabled_formats = FMT_MP3 | FMT_MIDI | FMT_IMY | FMT_AAC | FMT_AMR | FMT_AWB | FMT_VM | FMT_WAV;
            g_app.cur_device = DEV_SPEAKER_BOTH;
            g_app.loop_mode = LOOP_LIST;
            g_app.shuffle_mode = false;
            g_app.keep_screen_on = false;

            VMINT cur_vol = vm_get_volume();
            g_app.volume = (cur_vol > 0 && cur_vol <= 6) ? (int)cur_vol : 4;
            vm_set_volume(g_app.volume);

            g_app.sort_mode = SORT_FILENAME;
            g_app.screensaver_id = SS_NONE;
            g_app.show_clock = true;
            g_app.scroll_on_pause = true;
            g_app.ui_mode = MODE_TRACKLIST;
            g_app.return_mode = MODE_TRACKLIST;
            g_app.active_track_idx = -1;
            g_app.midi_handle = -1;

            VMWCHAR exec_path[260];
            if (vm_get_exec_filename(exec_path) == 0) {
                vm_get_path(exec_path, g_app.app_dir);
                normalize_dir_path(g_app.app_dir, g_app.app_dir, 260);
                normalize_dir_path(g_app.lib_dir, g_app.app_dir, 260);
            }

            g_app.layer_hdl = vm_graphic_create_layer(0, 0, g_app.screen_w, g_app.screen_h, VM_NO_TRANS_COLOR);
            vm_graphic_active_layer(g_app.layer_hdl);

            apply_power_saving_mode();
            refresh_track_library();
            g_app.engine_timer = vm_create_timer(ENGINE_TIMER_MS, timer_tick);
            trigger_repaint();
            break;
        }
        case VM_MSG_ACTIVE:
            vm_graphic_active_layer(g_app.layer_hdl);
            apply_power_saving_mode();
            trigger_repaint();
            break;
        case VM_MSG_INACTIVE:
        case VM_MSG_HIDE:
            vm_switch_power_saving_mode(turn_on_mode);
            break;
        case VM_MSG_PAINT:
            trigger_repaint();
            break;
        case VM_MSG_QUIT:
            vm_switch_power_saving_mode(turn_on_mode);
            stop_playback();
            karaoke_unload();
            folder_viewer_unload();
            reader_unload();
            tracks_unload();
            if (g_app.audio_client_handle > 0) { vm_audio_clear_interrupt_callback(g_app.audio_client_handle); g_app.audio_client_handle = 0; }
            if (g_app.engine_timer != 0) vm_delete_timer(g_app.engine_timer);
            if (g_app.numeric_timer != 0) vm_delete_timer_ex(g_app.numeric_timer);
            if (g_app.layer_hdl >= 0) vm_graphic_delete_layer(g_app.layer_hdl);
            break;
    }
}

void vm_main(void) {
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_key_event);
    vm_reg_pen_callback(handle_pen_event);
}
