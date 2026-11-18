// ============================================================================
// Application: MRE Professional Audio Suite
// Target: MediaTek MRE 2.0 / 2.5 / 3.0 (C/C++ Runtime)
// Dependencies: Standard MRE SDK Headers Only
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

// ----------------------------------------------------------------------------
// Audio Device Definitions (MediaTek MED Subsystem)
// ----------------------------------------------------------------------------
#ifndef VM_DEVICE_SPEAKER
#define VM_DEVICE_SPEAKER       0   // Keypad Tone / Buzzer
#endif
#ifndef VM_DEVICE_SPEAKER2
#define VM_DEVICE_SPEAKER2      2   // Earphone / Headset
#endif
#ifndef VM_DEVICE_LOUDSPEAKER
#define VM_DEVICE_LOUDSPEAKER   3   // Handsfree Loudspeaker
#endif
#ifndef VM_DEVICE_SPEAKER_BOTH
#define VM_DEVICE_SPEAKER_BOTH  4   // Both Loudspeaker and Earphone
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

#define MAX_TRACKS 128
#define MAX_LRC_FILES 64
#define MAX_LRC_LINES 256
#define MAX_LRC_TEXT_LEN 72
#define ENGINE_TIMER_MS 100          // 10 FPS engine tick
#define NUMERIC_INPUT_TIMEOUT_MS 1200

// ----------------------------------------------------------------------------
// Color Helpers & Theme Definition
// ----------------------------------------------------------------------------
static vm_graphic_color make_color(VMUINT8 r, VMUINT8 g, VMUINT8 b) {
    vm_graphic_color c;
    c.vm_color_565 = VM_COLOR_888_TO_565(r, g, b);
    c.vm_color_888 = ((VMUINT32)b << 16) | ((VMUINT32)g << 8) | (VMUINT32)r;
    return c;
}

enum ThemeID {
    THEME_BASIC_LIGHT = 0,
    THEME_BASIC_DARK,
    THEME_MINT,
    THEME_OCEAN,
    THEME_RED_VELVET,
    THEME_BUSINESS_GOLD, // Classic Nokia E52 Style
    THEME_SUNBEAM_GLOW,
    THEME_COUNT
};

struct Theme {
    const char* name;
    vm_graphic_color bg;
    vm_graphic_color header_bg;
    vm_graphic_color header_text;
    vm_graphic_color item_text;
    vm_graphic_color item_playing;
    vm_graphic_color item_selected_bg;
    vm_graphic_color item_selected_text;
    vm_graphic_color marquee_bg;
    vm_graphic_color marquee_text;
    vm_graphic_color softkey_bg;
    vm_graphic_color softkey_text;
    vm_graphic_color popup_bg;
    vm_graphic_color popup_border;
};

static Theme g_themes[THEME_COUNT] = {
    { "Basic Light",
      make_color(220, 222, 228), make_color(180, 185, 195), make_color(15, 15, 15),
      make_color(25, 25, 25), make_color(0, 130, 50), make_color(0, 100, 200),
      make_color(255, 255, 255), make_color(200, 205, 215), make_color(10, 10, 10),
      make_color(180, 185, 195), make_color(0, 0, 0),
      make_color(235, 238, 245), make_color(100, 105, 115) },
    
    { "Basic Dark",
      make_color(16, 16, 16), make_color(28, 28, 28), make_color(240, 240, 240),
      make_color(200, 200, 200), make_color(50, 205, 50), make_color(50, 50, 60),
      make_color(255, 255, 255), make_color(22, 22, 22), make_color(255, 170, 0),
      make_color(28, 28, 28), make_color(220, 220, 220),
      make_color(32, 32, 32), make_color(75, 75, 75) },

    { "Mint",
      make_color(20, 52, 38), make_color(12, 34, 24), make_color(170, 240, 205),
      make_color(140, 215, 175), make_color(45, 255, 140), make_color(34, 88, 64),
      make_color(255, 255, 255), make_color(14, 40, 28), make_color(120, 245, 180),
      make_color(12, 34, 24), make_color(170, 240, 205),
      make_color(18, 46, 34), make_color(45, 120, 85) },

    { "Ocean",
      make_color(10, 25, 47), make_color(15, 42, 74), make_color(100, 255, 218),
      make_color(136, 146, 176), make_color(0, 255, 180), make_color(23, 62, 104),
      make_color(255, 255, 255), make_color(15, 35, 60), make_color(100, 255, 218),
      make_color(15, 42, 74), make_color(100, 255, 218),
      make_color(17, 45, 80), make_color(0, 150, 255) },

    { "Red Velvet",
      make_color(32, 8, 14), make_color(64, 12, 24), make_color(255, 210, 220),
      make_color(220, 185, 195), make_color(255, 65, 85), make_color(115, 24, 44),
      make_color(255, 255, 255), make_color(48, 10, 20), make_color(255, 180, 195),
      make_color(64, 12, 24), make_color(255, 210, 220),
      make_color(45, 10, 18), make_color(160, 30, 60) },

    { "Business Gold",
      make_color(36, 30, 24), make_color(58, 48, 36), make_color(225, 190, 75),
      make_color(210, 200, 185), make_color(255, 215, 0), make_color(95, 75, 52),
      make_color(255, 245, 210), make_color(46, 38, 28), make_color(235, 195, 85),
      make_color(58, 48, 36), make_color(225, 190, 75),
      make_color(48, 40, 32), make_color(150, 120, 55) },

    { "Sunbeam Glow",
      make_color(82, 38, 0), make_color(52, 22, 0), make_color(255, 220, 90),
      make_color(255, 200, 120), make_color(255, 235, 0), make_color(140, 65, 0),
      make_color(255, 255, 255), make_color(64, 28, 0), make_color(255, 225, 50),
      make_color(52, 22, 0), make_color(255, 220, 90),
      make_color(66, 30, 0), make_color(190, 95, 0) }
};

// ----------------------------------------------------------------------------
// Audio Formats, Loop Modes, Sort Modes & Device Output Definitions
// ----------------------------------------------------------------------------
enum AudioFormatMask {
    FMT_MP3  = 0x01,
    FMT_MIDI = 0x02,
    FMT_AAC  = 0x04,
    FMT_AMR  = 0x08,
    FMT_WAV  = 0x10,
    FMT_AWB  = 0x20,
    FMT_VM   = 0x40,
    FMT_IMY  = 0x80
};

enum LoopMode {
    LOOP_LIST = 0,
    LOOP_ONE,
    LOOP_NONE,
    LOOP_COUNT
};

static const char* LOOP_NAMES[LOOP_COUNT] = {
    "List",
    "One",
    "None"
};

enum SortMode {
    SORT_FILENAME = 0,
    SORT_TITLE,
    SORT_ARTIST,
    SORT_ALBUM,
    SORT_TRACK_NUM,
    SORT_YEAR,
    SORT_FILESIZE,
    SORT_FORMAT,
    SORT_COUNT
};

static const char* SORT_NAMES[SORT_COUNT] = {
    "Filename (A-Z)",
    "Title (A-Z)",
    "Artist (A-Z)",
    "Album (A-Z)",
    "Track # (0-9)",
    "Year (0-9)",
    "File Size",
    "Format"
};

static const char* SORT_SHORT_NAMES[SORT_COUNT] = {
    "File",
    "Title",
    "Artist",
    "Album",
    "Track#",
    "Year",
    "Size",
    "Fmt"
};

enum ScreensaverID {
    SS_NONE = 0,
    SS_STARFIELD,
    SS_EQUALIZER,
    SS_BOUNCING_ORB,
    SS_DIGITAL_RAIN,
    SS_COUNT
};

static const char* SCREENSAVER_NAMES[SS_COUNT] = {
    "Lyrics Only",
    "Starfield",
    "Equalizer",
    "Bouncing Orb",
    "Digital Rain"
};

enum AudioDeviceProfile {
    DEV_SPEAKER_BOTH = 0,    // Default from MediaTek's mmm.c sample (value 4)
    DEV_LOUDSPEAKER,         // Handsfree Loudspeaker (value 3)
    DEV_EARPHONE,            // Headset / Earphone (value 2)
    DEV_BUZZER,              // Keypad Tone / Buzzer (value 0)
    DEV_PROFILE_COUNT
};

static const char* DEVICE_PROFILE_NAMES[DEV_PROFILE_COUNT] = {
    "Both (Loud+Ear)",
    "Loudspeaker",
    "Earphone",
    "Buzzer"
};

static const VMUINT DEVICE_PROFILE_VALUES[DEV_PROFILE_COUNT] = {
    VM_DEVICE_SPEAKER_BOTH,
    VM_DEVICE_LOUDSPEAKER,
    VM_DEVICE_SPEAKER2,
    VM_DEVICE_SPEAKER
};

struct TrackInfo {
    VMWCHAR filename[128];
    VMUINT  filesize;
    VMUINT8 format;
    VMWCHAR title[48];
    VMWCHAR artist[36];
    VMWCHAR album[36];
    VMUINT16 year;
    VMUINT16 track_num;
    bool id3_parsed;
    bool has_id3;
};

struct LrcLine {
    VMINT32 time_ms;
    VMWCHAR text[MAX_LRC_TEXT_LEN];
};

struct StarPoint {
    int x;
    int y;
    int z;
};

enum PlaybackState {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED
};

enum AppUIMode {
    MODE_TRACKLIST,
    MODE_MENU,
    MODE_VOLUME_SELECT,
    MODE_DEVICE_SELECT,
    MODE_TIME_PICKER,
    MODE_THEME_SELECT,
    MODE_FORMAT_FILTER,
    MODE_SORT_SELECT,
    MODE_KARAOKE,
    MODE_LRC_PICKER,
    MODE_POPUP_CONFIRM_EXIT,
    MODE_POPUP_ABOUT,
    MODE_POPUP_ERROR
};

#define MENU_ITEM_COUNT 13

struct AppContext {
    VMINT layer_hdl;
    int screen_w;
    int screen_h;
    int font_h;
    
    int header_h;
    int footer_h;
    int marquee_h;
    
    AppUIMode ui_mode;
    int cur_theme;
    VMUINT8 enabled_formats;
    AudioDeviceProfile cur_device;
    LoopMode loop_mode;
    int volume;                      // 0..6
    bool id3_mode;
    SortMode sort_mode;
    bool show_clock;
    bool scroll_on_pause;
    
    VMWCHAR app_dir[260];
    TrackInfo tracks[MAX_TRACKS];
    int track_count;
    int selected_track;
    int scroll_offset;
    
    char numeric_buf[8];
    int  numeric_len;
    VMINT numeric_timer;
    
    PlaybackState play_state;
    int active_track_idx;
    VMUINT total_duration_sec;       // Length stored directly in SECONDS
    VMUINT current_time_sec;         // Current playback stored directly in SECONDS
    VMUINT current_time_ms;          // Milliseconds for LRC sync
    
    VMUINT8* ram_audio_buf;
    VMUINT   ram_audio_size;
    VMINT    midi_handle;
    VMINT    audio_client_handle;
    
    // Character-offset scrolling
    int sel_char_offset;
    int marq_char_offset;
    int scroll_tick_counter;
    int clock_tick_counter;
    VMINT engine_timer;
    
    int time_digits[6]; // HHMMSS
    int time_cursor;
    
    int menu_cursor;
    int menu_scroll;
    
    // Karaoke / LRC / Screensaver state
    LrcLine lrc_lines[MAX_LRC_LINES];
    int lrc_count;
    int lrc_file_offset_ms;
    int lrc_user_offset_ms;
    int lrc_track_idx;
    VMWCHAR lrc_cur_name[128];
    VMWCHAR lrc_files[MAX_LRC_FILES][128];
    int lrc_file_count;
    ScreensaverID screensaver_id;
    int ss_frame;
    StarPoint ss_stars[24];
    int ss_eq_heights[16];
    int ss_orb_x, ss_orb_y, ss_orb_dx, ss_orb_dy;
    int ss_rain_y[20];
    
    char error_title[32];
    char error_line1[64];
    char error_line2[64];
    char error_line3[64];
};

static AppContext g_app;

// ----------------------------------------------------------------------------
// Forward Declarations
// ----------------------------------------------------------------------------
static void refresh_track_library();
static void sort_track_library();
static void ensure_all_id3_parsed();
static void start_playback(int track_index, VMUINT start_time_ms);
static void stop_playback();
static void pause_playback();
static void resume_playback();
static void apply_timestamp_jump();
static void set_ui_mode(AppUIMode mode);
static void set_ui_mode_with_cursor(AppUIMode mode, int cursor);
static void trigger_repaint();
static void format_time_string_sec(VMUINT total_s, VMWCHAR* out_str);
static void digital_audio_callback(VMINT result);
static void midi_audio_callback(VMINT handle, VMINT event);
static void audio_interrupt_callback(VMINT handle, VMINT result);
static void auto_load_lrc_for_track(int track_idx);
static void scan_lrc_files();
static void init_screensaver_state();

// ----------------------------------------------------------------------------
// Safe String Utilities
// ----------------------------------------------------------------------------
static void safe_ascii_to_ucs2(VMWSTR dst, int max_chars, const char* src) {
    if (!dst || max_chars <= 0) return;
    int i = 0;
    if (src) {
        while (src[i] != '\0' && i < max_chars - 1) {
            dst[i] = (VMWCHAR)(unsigned char)src[i];
            i++;
        }
    }
    dst[i] = 0;
}

static void safe_wstrncpy_zero(VMWSTR dst, const VMWCHAR* src, int max_chars) {
    if (!dst || max_chars <= 0) return;
    if (!src) {
        dst[0] = 0;
        return;
    }
    int i = 0;
    while (src[i] != 0 && i < max_chars - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void safe_wstrncat_zero(VMWSTR dst, const VMWCHAR* src, int max_chars) {
    if (!dst || !src || max_chars <= 0) return;
    int dlen = vm_wstrlen(dst);
    int i = 0;
    while (src[i] != 0 && (dlen + i) < max_chars - 1) {
        dst[dlen + i] = src[i];
        i++;
    }
    dst[dlen + i] = 0;
}

static int wstr_icmp(const VMWCHAR* a, const VMWCHAR* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        VMWCHAR c1 = *a;
        VMWCHAR c2 = *b;
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return (int)c1 - (int)c2;
        a++;
        b++;
    }
    return (int)(*a) - (int)(*b);
}

static bool ascii_istartswith_nocase(const VMUINT8* buf, int buf_len, const char* prefix) {
    int plen = (int)strlen(prefix);
    if (buf_len < plen) return false;
    for (int i = 0; i < plen; i++) {
        char c1 = (char)buf[i];
        char c2 = prefix[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

static bool ends_with_ignore_case(const VMWCHAR* str, const char* ext) {
    VMWCHAR ext_ucs2[16];
    safe_ascii_to_ucs2(ext_ucs2, 16, ext);
    int len_str = vm_wstrlen((VMWSTR)str);
    int len_ext = vm_wstrlen(ext_ucs2);
    if (len_str < len_ext) return false;
    
    for (int i = 0; i < len_ext; i++) {
        VMWCHAR c1 = str[len_str - len_ext + i];
        VMWCHAR c2 = ext_ucs2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

static void make_canonical_path(VMWSTR dst, const VMWCHAR* folder, const VMWCHAR* filename) {
    vm_wstrcpy(dst, (VMWSTR)folder);

    // CRITICAL: Ensure drive letter is uppercase (e:\ -> E:\)
    if (dst[0] >= (VMWCHAR)'a' && dst[0] <= (VMWCHAR)'z') {
        dst[0] = dst[0] - (VMWCHAR)'a' + (VMWCHAR)'A';
    }

    int len = vm_wstrlen(dst);
    while (len > 0 && (dst[len - 1] == (VMWCHAR)'\\' || dst[len - 1] == (VMWCHAR)'/')) {
        dst[len - 1] = 0;
        len--;
    }
    VMWCHAR slash[2] = { (VMWCHAR)'\\', 0 };
    vm_wstrcat(dst, slash);
    vm_wstrcat(dst, (VMWSTR)filename);
}

static void utf8_to_ucs2(const VMUINT8* src, int byte_len, VMWCHAR* dst, int max_chars) {
    if (!dst || max_chars <= 0) return;
    int out = 0;
    int i = 0;
    while (i < byte_len && src[i] != 0 && out < max_chars - 1) {
        VMUINT8 b0 = src[i];
        if (b0 < 0x80) {
            dst[out++] = (VMWCHAR)b0;
            i++;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < byte_len) {
            VMUINT16 cp = ((VMUINT16)(b0 & 0x1F) << 6) | (src[i + 1] & 0x3F);
            dst[out++] = (VMWCHAR)cp;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < byte_len) {
            VMUINT16 cp = ((VMUINT16)(b0 & 0x0F) << 12) |
                          ((VMUINT16)(src[i + 1] & 0x3F) << 6) |
                          (src[i + 2] & 0x3F);
            dst[out++] = (VMWCHAR)cp;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < byte_len) {
            dst[out++] = (VMWCHAR)'?';
            i += 4;
        } else {
            dst[out++] = (VMWCHAR)b0;
            i++;
        }
    }
    dst[out] = 0;
}

static void trim_ucs2(VMWCHAR* str) {
    if (!str) return;
    int len = vm_wstrlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = 0;
        len--;
    }
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t') start++;
    if (start > 0) {
        int i = 0;
        while (str[start + i] != 0) {
            str[i] = str[start + i];
            i++;
        }
        str[i] = 0;
    }
}

// ----------------------------------------------------------------------------
// ID3 Tag Reader (ID3v2.2 / v2.3 / v2.4 + ID3v1 / v1.1)
// ----------------------------------------------------------------------------
static void decode_id3_text_frame(const VMUINT8* payload, int len, VMWCHAR* out, int max_chars) {
    if (!payload || len <= 0 || !out || max_chars <= 0) return;
    out[0] = 0;

    VMUINT8 enc = payload[0];
    const VMUINT8* txt = payload + 1;
    int tlen = len - 1;
    if (tlen <= 0) return;

    if (enc == 0) {
        // ISO-8859-1 / ASCII
        int n = 0;
        for (int i = 0; i < tlen && txt[i] != 0 && n < max_chars - 1; i++) {
            out[n++] = (VMWCHAR)txt[i];
        }
        out[n] = 0;
    } else if (enc == 1 && tlen >= 2) {
        // UTF-16 with BOM
        bool big_endian = false;
        int start = 0;
        if (txt[0] == 0xFE && txt[1] == 0xFF) {
            big_endian = true;
            start = 2;
        } else if (txt[0] == 0xFF && txt[1] == 0xFE) {
            big_endian = false;
            start = 2;
        }
        int n = 0;
        for (int i = start; i + 1 < tlen && n < max_chars - 1; i += 2) {
            VMWCHAR ch = big_endian ? (((VMWCHAR)txt[i] << 8) | txt[i + 1]) : (((VMWCHAR)txt[i + 1] << 8) | txt[i]);
            if (ch == 0) break;
            out[n++] = ch;
        }
        out[n] = 0;
    } else if (enc == 2) {
        // UTF-16BE without BOM
        int n = 0;
        for (int i = 0; i + 1 < tlen && n < max_chars - 1; i += 2) {
            VMWCHAR ch = ((VMWCHAR)txt[i] << 8) | txt[i + 1];
            if (ch == 0) break;
            out[n++] = ch;
        }
        out[n] = 0;
    } else if (enc == 3) {
        // UTF-8
        utf8_to_ucs2(txt, tlen, out, max_chars);
    }
    trim_ucs2(out);
}

static VMUINT16 ucs2_parse_uint(const VMWCHAR* str) {
    if (!str) return 0;
    VMUINT val = 0;
    int i = 0;
    while (str[i] == ' ') i++;
    while (str[i] >= '0' && str[i] <= '9') {
        val = val * 10 + (VMUINT)(str[i] - '0');
        i++;
        if (val > 65000) break;
    }
    return (VMUINT16)val;
}

static void parse_id3_for_track(TrackInfo* t) {
    if (!t || t->id3_parsed) return;
    t->id3_parsed = true;
    t->has_id3 = false;
    t->title[0] = 0;
    t->artist[0] = 0;
    t->album[0] = 0;
    t->year = 0;
    t->track_num = 0;

    VMWCHAR full_path[260];
    make_canonical_path(full_path, g_app.app_dir, t->filename);

    VMFILE f = vm_file_open(full_path, MODE_READ, 1);
    if (f < 0) return;

    // 1. Parse ID3v2 header at file start
    VMUINT8 hdr[10];
    VMUINT nread = 0;
    if (vm_file_read(f, hdr, 10, &nread) > 0 && nread == 10) {
        if (hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3' && hdr[3] >= 2 && hdr[3] <= 4) {
            VMUINT8 ver = hdr[3];
            VMUINT8 flags = hdr[5];
            VMUINT tag_size = ((VMUINT)(hdr[6] & 0x7F) << 21) |
                              ((VMUINT)(hdr[7] & 0x7F) << 14) |
                              ((VMUINT)(hdr[8] & 0x7F) << 7) |
                              ((VMUINT)(hdr[9] & 0x7F));
            VMUINT pos = 10;
            VMUINT tag_end = 10 + tag_size;
            if (tag_end > t->filesize) tag_end = t->filesize;

            // Skip optional extended header (v2.3 / v2.4)
            if ((flags & 0x40) && ver >= 3 && pos + 4 <= tag_end) {
                VMUINT8 ext[4];
                if (vm_file_read(f, ext, 4, &nread) > 0 && nread == 4) {
                    VMUINT ext_sz;
                    if (ver == 4) {
                        ext_sz = ((VMUINT)(ext[0] & 0x7F) << 21) | ((VMUINT)(ext[1] & 0x7F) << 14) |
                                 ((VMUINT)(ext[2] & 0x7F) << 7)  | ((VMUINT)(ext[3] & 0x7F));
                        pos += ext_sz;
                    } else {
                        ext_sz = ((VMUINT)ext[0] << 24) | ((VMUINT)ext[1] << 16) |
                                 ((VMUINT)ext[2] << 8)  | (VMUINT)ext[3];
                        pos += 4 + ext_sz;
                    }
                    vm_file_seek(f, (VMINT)pos, BASE_BEGIN);
                }
            }

            VMUINT8 fhdr[10];
            VMUINT8 payload[256];
            int fhdr_len = (ver == 2) ? 6 : 10;

            while (pos + (VMUINT)fhdr_len <= tag_end) {
                if (vm_file_read(f, fhdr, fhdr_len, &nread) <= 0 || (int)nread != fhdr_len) break;
                if (fhdr[0] == 0) break; // Padding

                VMUINT fsize = 0;
                if (ver == 2) {
                    fsize = ((VMUINT)fhdr[3] << 16) | ((VMUINT)fhdr[4] << 8) | (VMUINT)fhdr[5];
                } else if (ver == 4) {
                    fsize = ((VMUINT)(fhdr[4] & 0x7F) << 21) | ((VMUINT)(fhdr[5] & 0x7F) << 14) |
                            ((VMUINT)(fhdr[6] & 0x7F) << 7)  | ((VMUINT)(fhdr[7] & 0x7F));
                } else {
                    fsize = ((VMUINT)fhdr[4] << 24) | ((VMUINT)fhdr[5] << 16) |
                            ((VMUINT)fhdr[6] << 8)  | (VMUINT)fhdr[7];
                }

                pos += (VMUINT)fhdr_len;
                if (fsize == 0 || pos + fsize > tag_end) break;

                int want = 0; // 1=title, 2=artist, 3=album, 4=year, 5=track
                if (ver == 2) {
                    if (memcmp(fhdr, "TT2", 3) == 0) want = 1;
                    else if (memcmp(fhdr, "TP1", 3) == 0) want = 2;
                    else if (memcmp(fhdr, "TAL", 3) == 0) want = 3;
                    else if (memcmp(fhdr, "TYE", 3) == 0) want = 4;
                    else if (memcmp(fhdr, "TRK", 3) == 0) want = 5;
                } else {
                    if (memcmp(fhdr, "TIT2", 4) == 0) want = 1;
                    else if (memcmp(fhdr, "TPE1", 4) == 0) want = 2;
                    else if (memcmp(fhdr, "TALB", 4) == 0) want = 3;
                    else if (memcmp(fhdr, "TYER", 4) == 0 || memcmp(fhdr, "TDRC", 4) == 0) want = 4;
                    else if (memcmp(fhdr, "TRCK", 4) == 0) want = 5;
                }

                if (want > 0) {
                    VMUINT rlen = (fsize < sizeof(payload)) ? fsize : sizeof(payload);
                    if (vm_file_read(f, payload, rlen, &nread) > 0 && nread > 1) {
                        VMWCHAR tmp[48];
                        if (want == 1 && t->title[0] == 0) {
                            decode_id3_text_frame(payload, (int)nread, t->title, 48);
                        } else if (want == 2 && t->artist[0] == 0) {
                            decode_id3_text_frame(payload, (int)nread, t->artist, 36);
                        } else if (want == 3 && t->album[0] == 0) {
                            decode_id3_text_frame(payload, (int)nread, t->album, 36);
                        } else if (want == 4 && t->year == 0) {
                            decode_id3_text_frame(payload, (int)nread, tmp, 48);
                            t->year = ucs2_parse_uint(tmp);
                        } else if (want == 5 && t->track_num == 0) {
                            decode_id3_text_frame(payload, (int)nread, tmp, 48);
                            t->track_num = ucs2_parse_uint(tmp);
                        }
                    }
                }

                pos += fsize;
                if (t->title[0] && t->artist[0] && t->album[0] && t->year && t->track_num) {
                    break;
                }
                vm_file_seek(f, (VMINT)pos, BASE_BEGIN);
            }
        }
    }

    // 2. Fallback to ID3v1 at end of file (last 128 bytes)
    if (t->filesize >= 128 && (!t->title[0] || !t->artist[0] || !t->album[0] || !t->year || !t->track_num)) {
        if (vm_file_seek(f, (VMINT)(t->filesize - 128), BASE_BEGIN) == 0) {
            VMUINT8 v1[128];
            if (vm_file_read(f, v1, 128, &nread) > 0 && nread == 128) {
                if (v1[0] == 'T' && v1[1] == 'A' && v1[2] == 'G') {
                    char tmp_c[32];
                    if (t->title[0] == 0) {
                        memcpy(tmp_c, v1 + 3, 30);
                        tmp_c[30] = 0;
                        safe_ascii_to_ucs2(t->title, 48, tmp_c);
                        trim_ucs2(t->title);
                    }
                    if (t->artist[0] == 0) {
                        memcpy(tmp_c, v1 + 33, 30);
                        tmp_c[30] = 0;
                        safe_ascii_to_ucs2(t->artist, 36, tmp_c);
                        trim_ucs2(t->artist);
                    }
                    if (t->album[0] == 0) {
                        memcpy(tmp_c, v1 + 63, 30);
                        tmp_c[30] = 0;
                        safe_ascii_to_ucs2(t->album, 36, tmp_c);
                        trim_ucs2(t->album);
                    }
                    if (t->year == 0) {
                        memcpy(tmp_c, v1 + 93, 4);
                        tmp_c[4] = 0;
                        t->year = (VMUINT16)atoi(tmp_c);
                    }
                    if (t->track_num == 0 && v1[125] == 0 && v1[126] != 0) {
                        t->track_num = (VMUINT16)v1[126];
                    }
                }
            }
        }
    }

    vm_file_close(f);

    if (t->title[0] != 0 || t->artist[0] != 0 || t->album[0] != 0 || t->year != 0 || t->track_num != 0) {
        t->has_id3 = true;
    }
}

static void ensure_all_id3_parsed() {
    for (int i = 0; i < g_app.track_count; i++) {
        if (!g_app.tracks[i].id3_parsed) {
            parse_id3_for_track(&g_app.tracks[i]);
        }
    }
}

static void get_track_display_name(const TrackInfo* t, VMWCHAR* out, int max_chars) {
    if (!t || !out || max_chars <= 0) return;
    if (!g_app.id3_mode || !t->has_id3 || (t->title[0] == 0 && t->artist[0] == 0)) {
        safe_wstrncpy_zero(out, t->filename, max_chars);
        return;
    }

    if (t->title[0] != 0 && t->artist[0] != 0) {
        VMWCHAR sep[8];
        safe_ascii_to_ucs2(sep, 8, " - ");
        safe_wstrncpy_zero(out, t->title, max_chars);
        safe_wstrncat_zero(out, sep, max_chars);
        safe_wstrncat_zero(out, t->artist, max_chars);
    } else if (t->title[0] != 0) {
        safe_wstrncpy_zero(out, t->title, max_chars);
    } else {
        safe_wstrncpy_zero(out, t->artist, max_chars);
    }
}

// ----------------------------------------------------------------------------
// Track Sorting
// ----------------------------------------------------------------------------
static int compare_tracks(const TrackInfo* a, const TrackInfo* b) {
    int cmp = 0;
    switch (g_app.sort_mode) {
        case SORT_TITLE: {
            const VMWCHAR* sa = (a->title[0] != 0) ? a->title : a->filename;
            const VMWCHAR* sb = (b->title[0] != 0) ? b->title : b->filename;
            cmp = wstr_icmp(sa, sb);
            break;
        }
        case SORT_ARTIST: {
            const VMWCHAR* sa = (a->artist[0] != 0) ? a->artist : a->filename;
            const VMWCHAR* sb = (b->artist[0] != 0) ? b->artist : b->filename;
            cmp = wstr_icmp(sa, sb);
            break;
        }
        case SORT_ALBUM: {
            const VMWCHAR* sa = (a->album[0] != 0) ? a->album : a->filename;
            const VMWCHAR* sb = (b->album[0] != 0) ? b->album : b->filename;
            cmp = wstr_icmp(sa, sb);
            break;
        }
        case SORT_TRACK_NUM: {
            VMUINT16 ta = (a->track_num > 0) ? a->track_num : 9999;
            VMUINT16 tb = (b->track_num > 0) ? b->track_num : 9999;
            if (ta != tb) cmp = (ta < tb) ? -1 : 1;
            break;
        }
        case SORT_YEAR: {
            VMUINT16 ya = (a->year > 0) ? a->year : 9999;
            VMUINT16 yb = (b->year > 0) ? b->year : 9999;
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
        default:
            break;
    }
    if (cmp == 0) {
        cmp = wstr_icmp(a->filename, b->filename);
    }
    return cmp;
}

static void sort_track_library() {
    if (g_app.track_count <= 1) return;

    if (g_app.sort_mode == SORT_TITLE || g_app.sort_mode == SORT_ARTIST ||
        g_app.sort_mode == SORT_ALBUM || g_app.sort_mode == SORT_TRACK_NUM ||
        g_app.sort_mode == SORT_YEAR) {
        ensure_all_id3_parsed();
    }

    VMWCHAR sel_name[128];
    VMWCHAR act_name[128];
    sel_name[0] = 0;
    act_name[0] = 0;
    if (g_app.selected_track >= 0 && g_app.selected_track < g_app.track_count) {
        safe_wstrncpy_zero(sel_name, g_app.tracks[g_app.selected_track].filename, 128);
    }
    if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
        safe_wstrncpy_zero(act_name, g_app.tracks[g_app.active_track_idx].filename, 128);
    }

    // Insertion sort (fast and zero extra stack/heap)
    for (int i = 1; i < g_app.track_count; i++) {
        TrackInfo key = g_app.tracks[i];
        int j = i - 1;
        while (j >= 0 && compare_tracks(&g_app.tracks[j], &key) > 0) {
            g_app.tracks[j + 1] = g_app.tracks[j];
            j--;
        }
        g_app.tracks[j + 1] = key;
    }

    if (sel_name[0] != 0) {
        for (int i = 0; i < g_app.track_count; i++) {
            if (vm_wstrcmp(g_app.tracks[i].filename, sel_name) == 0) {
                g_app.selected_track = i;
                break;
            }
        }
    }
    if (act_name[0] != 0) {
        for (int i = 0; i < g_app.track_count; i++) {
            if (vm_wstrcmp(g_app.tracks[i].filename, act_name) == 0) {
                g_app.active_track_idx = i;
                break;
            }
        }
    }
    if (g_app.lrc_track_idx >= 0 && act_name[0] != 0) {
        g_app.lrc_track_idx = g_app.active_track_idx;
    }
}

// ----------------------------------------------------------------------------
// Local Directory Scanning Only (Sandboxed)
// ----------------------------------------------------------------------------
static void scan_directory(const VMWCHAR* base_dir) {
    VMWCHAR search_pattern[260];
    vm_wstrcpy(search_pattern, (VMWSTR)base_dir);
    
    int len = vm_wstrlen(search_pattern);
    while (len > 0 && (search_pattern[len - 1] == (VMWCHAR)'\\' || search_pattern[len - 1] == (VMWCHAR)'/')) {
        search_pattern[len - 1] = 0;
        len--;
    }
    VMWCHAR wild[8];
    safe_ascii_to_ucs2(wild, 8, "\\*.*");
    vm_wstrcat(search_pattern, wild);

    vm_fileinfo_ext info;
    VMINT handle = vm_find_first_ext(search_pattern, &info);
    if (handle < 0) return;

    do {
        if (info.filefullname[0] == '.') continue;

        VMUINT8 fmt = 0;
        if (ends_with_ignore_case(info.filefullname, ".mp3")) fmt = FMT_MP3;
        else if (ends_with_ignore_case(info.filefullname, ".mid") || 
                 ends_with_ignore_case(info.filefullname, ".midi")) fmt = FMT_MIDI;
        else if (ends_with_ignore_case(info.filefullname, ".imy")) fmt = FMT_IMY;
        else if (ends_with_ignore_case(info.filefullname, ".aac")) fmt = FMT_AAC;
        else if (ends_with_ignore_case(info.filefullname, ".amr")) fmt = FMT_AMR;
        else if (ends_with_ignore_case(info.filefullname, ".awb")) fmt = FMT_AWB;
        else if (ends_with_ignore_case(info.filefullname, ".vm"))  fmt = FMT_VM;
        else if (ends_with_ignore_case(info.filefullname, ".wav")) fmt = FMT_WAV;

        if ((fmt & g_app.enabled_formats) != 0) {
            TrackInfo* t = &g_app.tracks[g_app.track_count];
            memset(t, 0, sizeof(TrackInfo));
            safe_wstrncpy_zero(t->filename, info.filefullname, 128);

            VMWCHAR full_path[260];
            make_canonical_path(full_path, base_dir, t->filename);
            
            VMFILE test_f = vm_file_open(full_path, MODE_READ, 1);
            if (test_f >= 0) {
                vm_file_close(test_f);
                t->filesize = info.filesize;
                t->format = fmt;
                g_app.track_count++;
                if (g_app.track_count >= MAX_TRACKS) break;
            }
        }
    } while (vm_find_next_ext(handle, &info) == 0);

    vm_find_close_ext(handle);
}

static void refresh_track_library() {
    g_app.track_count = 0;
    g_app.selected_track = 0;
    g_app.scroll_offset = 0;
    g_app.sel_char_offset = 0;

    VMWCHAR exec_path[260];
    if (vm_get_exec_filename(exec_path) == 0) {
        vm_get_path(exec_path, g_app.app_dir);
        scan_directory(g_app.app_dir);
    }
    if (g_app.id3_mode) {
        ensure_all_id3_parsed();
    }
    sort_track_library();
}

// ----------------------------------------------------------------------------
// LRC Parser & Karaoke Support
// ----------------------------------------------------------------------------
static void clear_lrc_data() {
    g_app.lrc_count = 0;
    g_app.lrc_file_offset_ms = 0;
    g_app.lrc_user_offset_ms = 0;
    g_app.lrc_cur_name[0] = 0;
}

static void process_lrc_raw_line(const char* raw) {
    if (!raw || raw[0] == 0) return;

    // Skip UTF-8 BOM if present
    if ((VMUINT8)raw[0] == 0xEF && (VMUINT8)raw[1] == 0xBB && (VMUINT8)raw[2] == 0xBF) {
        raw += 3;
    }

    // Check [offset:...]
    if (strncmp(raw, "[offset:", 8) == 0) {
        g_app.lrc_file_offset_ms = atoi(raw + 8);
        return;
    }

    VMINT32 stamps[12];
    int stamp_count = 0;
    const char* p = raw;

    while (*p == '[') {
        const char* close_br = strchr(p, ']');
        if (!close_br) break;

        int tag_len = (int)(close_br - p - 1);
        if (tag_len >= 4 && p[1] >= '0' && p[1] <= '9') {
            int mins = 0, secs = 0, ms = 0;
            const char* q = p + 1;
            while (q < close_br && *q >= '0' && *q <= '9') {
                mins = mins * 10 + (*q - '0');
                q++;
            }
            if (q < close_br && *q == ':') {
                q++;
                while (q < close_br && *q >= '0' && *q <= '9') {
                    secs = secs * 10 + (*q - '0');
                    q++;
                }
                if (q < close_br && (*q == '.' || *q == ':')) {
                    q++;
                    int digits = 0;
                    int frac = 0;
                    while (q < close_br && *q >= '0' && *q <= '9' && digits < 3) {
                        frac = frac * 10 + (*q - '0');
                        digits++;
                        q++;
                    }
                    if (digits == 1) ms = frac * 100;
                    else if (digits == 2) ms = frac * 10;
                    else ms = frac;
                }
                if (stamp_count < 12) {
                    stamps[stamp_count++] = (VMINT32)(mins * 60000 + secs * 1000 + ms);
                }
            }
        }
        p = close_br + 1;
    }

    if (stamp_count == 0) return;

    while (*p == ' ' || *p == '\t') p++;

    VMWCHAR text_w[MAX_LRC_TEXT_LEN];
    utf8_to_ucs2((const VMUINT8*)p, (int)strlen(p), text_w, MAX_LRC_TEXT_LEN);
    trim_ucs2(text_w);

    for (int i = 0; i < stamp_count; i++) {
        if (g_app.lrc_count >= MAX_LRC_LINES) break;
        g_app.lrc_lines[g_app.lrc_count].time_ms = stamps[i];
        safe_wstrncpy_zero(g_app.lrc_lines[g_app.lrc_count].text, text_w, MAX_LRC_TEXT_LEN);
        g_app.lrc_count++;
    }
}

static bool load_lrc_file(const VMWCHAR* lrc_filename) {
    clear_lrc_data();
    if (!lrc_filename || lrc_filename[0] == 0) return false;

    VMWCHAR full_path[260];
    make_canonical_path(full_path, g_app.app_dir, lrc_filename);

    VMFILE f = vm_file_open(full_path, MODE_READ, 1);
    if (f < 0) return false;

    char line_buf[320];
    int line_pos = 0;
    VMUINT8 chunk[256];
    VMUINT nread = 0;
    VMINT r;

    while ((r = vm_file_read(f, chunk, sizeof(chunk), &nread)) > 0 && nread > 0) {
        for (VMUINT i = 0; i < nread; i++) {
            char c = (char)chunk[i];
            if (c == '\r' || c == '\n') {
                if (line_pos > 0) {
                    line_buf[line_pos] = 0;
                    process_lrc_raw_line(line_buf);
                    line_pos = 0;
                }
            } else if (line_pos < (int)sizeof(line_buf) - 1) {
                line_buf[line_pos++] = c;
            }
        }
        if (g_app.lrc_count >= MAX_LRC_LINES) break;
    }
    if (line_pos > 0 && g_app.lrc_count < MAX_LRC_LINES) {
        line_buf[line_pos] = 0;
        process_lrc_raw_line(line_buf);
    }

    vm_file_close(f);

    // Sort lines chronologically by time_ms
    for (int i = 1; i < g_app.lrc_count; i++) {
        LrcLine key = g_app.lrc_lines[i];
        int j = i - 1;
        while (j >= 0 && g_app.lrc_lines[j].time_ms > key.time_ms) {
            g_app.lrc_lines[j + 1] = g_app.lrc_lines[j];
            j--;
        }
        g_app.lrc_lines[j + 1] = key;
    }

    safe_wstrncpy_zero(g_app.lrc_cur_name, lrc_filename, 128);
    return (g_app.lrc_count > 0);
}

static void auto_load_lrc_for_track(int track_idx) {
    clear_lrc_data();
    g_app.lrc_track_idx = track_idx;
    if (track_idx < 0 || track_idx >= g_app.track_count) return;

    VMWCHAR lrc_name[128];
    safe_wstrncpy_zero(lrc_name, g_app.tracks[track_idx].filename, 120);
    int len = vm_wstrlen(lrc_name);
    int dot_pos = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (lrc_name[i] == (VMWCHAR)'.') {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos > 0) {
        lrc_name[dot_pos] = 0;
    }
    VMWCHAR ext[8];
    safe_ascii_to_ucs2(ext, 8, ".lrc");
    safe_wstrncat_zero(lrc_name, ext, 128);

    load_lrc_file(lrc_name);
}

static void scan_lrc_files() {
    g_app.lrc_file_count = 0;
    VMWCHAR search_pattern[260];
    vm_wstrcpy(search_pattern, g_app.app_dir);

    int len = vm_wstrlen(search_pattern);
    while (len > 0 && (search_pattern[len - 1] == (VMWCHAR)'\\' || search_pattern[len - 1] == (VMWCHAR)'/')) {
        search_pattern[len - 1] = 0;
        len--;
    }
    VMWCHAR wild[12];
    safe_ascii_to_ucs2(wild, 12, "\\*.lrc");
    vm_wstrcat(search_pattern, wild);

    vm_fileinfo_ext info;
    VMINT handle = vm_find_first_ext(search_pattern, &info);
    if (handle < 0) return;

    do {
        if (info.filefullname[0] == '.') continue;
        if (ends_with_ignore_case(info.filefullname, ".lrc")) {
            safe_wstrncpy_zero(g_app.lrc_files[g_app.lrc_file_count], info.filefullname, 128);
            g_app.lrc_file_count++;
            if (g_app.lrc_file_count >= MAX_LRC_FILES) break;
        }
    } while (vm_find_next_ext(handle, &info) == 0);

    vm_find_close_ext(handle);
}

// ----------------------------------------------------------------------------
// Screensaver Animation State & Updater
// ----------------------------------------------------------------------------
static unsigned int ss_rand_next = 1234567;
static int ss_rand(int max_val) {
    ss_rand_next = ss_rand_next * 1103515245 + 12345;
    return (int)((ss_rand_next >> 16) % (unsigned int)(max_val > 0 ? max_val : 1));
}

static void init_screensaver_state() {
    g_app.ss_frame = 0;
    int sw = g_app.screen_w > 0 ? g_app.screen_w : 240;
    int sh = g_app.screen_h > 0 ? g_app.screen_h : 320;

    for (int i = 0; i < 24; i++) {
        g_app.ss_stars[i].x = ss_rand(sw) - sw / 2;
        g_app.ss_stars[i].y = ss_rand(sh) - sh / 2;
        g_app.ss_stars[i].z = 1 + ss_rand(15);
    }
    for (int i = 0; i < 16; i++) {
        g_app.ss_eq_heights[i] = 4;
    }
    g_app.ss_orb_x = sw / 3;
    g_app.ss_orb_y = g_app.header_h + 30;
    g_app.ss_orb_dx = 3;
    g_app.ss_orb_dy = 2;
    for (int i = 0; i < 20; i++) {
        g_app.ss_rain_y[i] = ss_rand(sh);
    }
}

static void update_screensaver_state() {
    g_app.ss_frame++;
    int sw = g_app.screen_w > 0 ? g_app.screen_w : 240;
    int sh = g_app.screen_h > 0 ? g_app.screen_h : 320;
    int top_y = g_app.header_h + 2;
    int bot_y = sh - g_app.footer_h - 2;
    int vh = bot_y - top_y;
    if (vh < 20) vh = 20;

    if (g_app.screensaver_id == SS_STARFIELD) {
        for (int i = 0; i < 24; i++) {
            g_app.ss_stars[i].z--;
            if (g_app.ss_stars[i].z <= 0) {
                g_app.ss_stars[i].x = ss_rand(sw) - sw / 2;
                g_app.ss_stars[i].y = ss_rand(vh) - vh / 2;
                g_app.ss_stars[i].z = 16;
            }
        }
    } else if (g_app.screensaver_id == SS_EQUALIZER) {
        int max_bar = vh / 2;
        if (max_bar < 10) max_bar = 10;
        for (int i = 0; i < 16; i++) {
            if (g_app.play_state == STATE_PLAYING) {
                int target = 4 + ss_rand(max_bar);
                g_app.ss_eq_heights[i] = (g_app.ss_eq_heights[i] + target) / 2;
            } else {
                if (g_app.ss_eq_heights[i] > 3) g_app.ss_eq_heights[i] -= 2;
                else g_app.ss_eq_heights[i] = 2;
            }
        }
    } else if (g_app.screensaver_id == SS_BOUNCING_ORB) {
        int r = 14;
        g_app.ss_orb_x += g_app.ss_orb_dx;
        g_app.ss_orb_y += g_app.ss_orb_dy;
        if (g_app.ss_orb_x < r) { g_app.ss_orb_x = r; g_app.ss_orb_dx = -g_app.ss_orb_dx; }
        if (g_app.ss_orb_x > sw - r) { g_app.ss_orb_x = sw - r; g_app.ss_orb_dx = -g_app.ss_orb_dx; }
        if (g_app.ss_orb_y < top_y + r) { g_app.ss_orb_y = top_y + r; g_app.ss_orb_dy = -g_app.ss_orb_dy; }
        if (g_app.ss_orb_y > bot_y - r) { g_app.ss_orb_y = bot_y - r; g_app.ss_orb_dy = -g_app.ss_orb_dy; }
    } else if (g_app.screensaver_id == SS_DIGITAL_RAIN) {
        for (int i = 0; i < 20; i++) {
            g_app.ss_rain_y[i] += 4 + (i % 3) * 2;
            if (g_app.ss_rain_y[i] > bot_y) {
                g_app.ss_rain_y[i] = top_y - ss_rand(30);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Audio Callbacks & Loop Handling
// ----------------------------------------------------------------------------
static void handle_track_finished() {
    if (g_app.loop_mode == LOOP_ONE) {
        start_playback(g_app.active_track_idx, 0);
    } else if (g_app.loop_mode == LOOP_LIST) {
        if (g_app.track_count > 0) {
            int next_idx = (g_app.active_track_idx + 1) % g_app.track_count;
            start_playback(next_idx, 0);
        } else {
            stop_playback();
        }
    } else {
        stop_playback();
    }
    trigger_repaint();
}

static void audio_interrupt_callback(VMINT handle, VMINT result) {
    if (result == VM_AUDIO_RESULT_INTERRUPT) {
        pause_playback();
    } else if (result == VM_AUDIO_RESULT_INTERRUPT_RESUME) {
        resume_playback();
    }
}

static void digital_audio_callback(VMINT result) {
    if (result == VM_AUDIO_RESULT_END_OF_FILE) {
        handle_track_finished();
    }
}

static void midi_audio_callback(VMINT handle, VMINT event) {
    if (event == VM_STATE_MIDI_STOP) {
        handle_track_finished();
    }
}

static void stop_playback() {
    if (g_app.play_state != STATE_STOPPED) {
        if (g_app.midi_handle >= 0) {
            vm_midi_stop(g_app.midi_handle);
            g_app.midi_handle = -1;
        } else {
            vm_audio_stop(digital_audio_callback);
        }
        g_app.play_state = STATE_STOPPED;
        g_app.current_time_sec = 0;
        g_app.current_time_ms = 0;
    }

    if (g_app.ram_audio_buf != NULL) {
        vm_free(g_app.ram_audio_buf);
        g_app.ram_audio_buf = NULL;
        g_app.ram_audio_size = 0;
    }
}

static void pause_playback() {
    if (g_app.play_state == STATE_PLAYING) {
        if (g_app.midi_handle >= 0) {
            vm_midi_pause(g_app.midi_handle);
        } else {
            vm_audio_pause(digital_audio_callback);
        }
        g_app.play_state = STATE_PAUSED;
    }
}

static void resume_playback() {
    if (g_app.play_state == STATE_PAUSED) {
        if (g_app.midi_handle >= 0) {
            vm_midi_resume(g_app.midi_handle);
        } else {
            vm_audio_resume(digital_audio_callback);
        }
        g_app.play_state = STATE_PLAYING;
    }
}

static void show_audio_error(const char* title, const char* l1, const char* l2, const char* l3) {
    strncpy(g_app.error_title, title, sizeof(g_app.error_title) - 1);
    g_app.error_title[sizeof(g_app.error_title) - 1] = 0;
    strncpy(g_app.error_line1, l1, sizeof(g_app.error_line1) - 1);
    g_app.error_line1[sizeof(g_app.error_line1) - 1] = 0;
    strncpy(g_app.error_line2, l2, sizeof(g_app.error_line2) - 1);
    g_app.error_line2[sizeof(g_app.error_line2) - 1] = 0;
    strncpy(g_app.error_line3, l3, sizeof(g_app.error_line3) - 1);
    g_app.error_line3[sizeof(g_app.error_line3) - 1] = 0;
    set_ui_mode(MODE_POPUP_ERROR);
    g_app.play_state = STATE_STOPPED;
}

// ----------------------------------------------------------------------------
// Audio Playback Pipeline
// ----------------------------------------------------------------------------
static void start_playback(int track_index, VMUINT start_time_ms) {
    if (track_index < 0 || track_index >= g_app.track_count) return;

    stop_playback();

    TrackInfo* t = &g_app.tracks[track_index];
    VMWCHAR full_path[260];
    make_canonical_path(full_path, g_app.app_dir, t->filename);

    VMUINT out_dev = DEVICE_PROFILE_VALUES[g_app.cur_device];

    if (g_app.lrc_track_idx != track_index) {
        auto_load_lrc_for_track(track_index);
    }

    // ========================================================================
    // PIPELINE 1: MIDI & IMY (iMelody) Synthesizer Execution
    // ========================================================================
    if (t->format == FMT_MIDI || t->format == FMT_IMY) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f < 0) {
            show_audio_error("File Read Error", "vm_file_open failed", "Check card/file name", "");
            return;
        }

        VMUINT file_size = 0;
        vm_file_getfilesize(f, &file_size);
        VMUINT min_sz = (t->format == FMT_IMY) ? 10 : 14;
        if (file_size < min_sz) {
            vm_file_close(f);
            show_audio_error(
                (t->format == FMT_IMY) ? "Corrupt IMY" : "Corrupt MIDI",
                "File size too small",
                (t->format == FMT_IMY) ? "Invalid iMelody file" : "Invalid SMF header",
                "");
            return;
        }

        VMUINT alloc_size = (file_size + 7) & ~3;
        g_app.ram_audio_buf = (VMUINT8*)vm_malloc(alloc_size);
        if (!g_app.ram_audio_buf) {
            vm_file_close(f);
            show_audio_error("Memory Error", "vm_malloc failed", "Out of ASM heap", "");
            return;
        }

        VMUINT nread = 0;
        VMINT actual_b = vm_file_read(f, g_app.ram_audio_buf, file_size, &nread);
        vm_file_close(f);

        g_app.ram_audio_size = (actual_b > 0) ? (VMUINT)actual_b : nread;

        if (g_app.ram_audio_size == 0) {
            vm_free(g_app.ram_audio_buf);
            g_app.ram_audio_buf = NULL;
            show_audio_error("Disk I/O Error", "0 Bytes read from file", "", "");
            return;
        }

        if (t->format == FMT_MIDI) {
            if (g_app.ram_audio_buf[0] != 'M' || g_app.ram_audio_buf[1] != 'T' ||
                g_app.ram_audio_buf[2] != 'h' || g_app.ram_audio_buf[3] != 'd') {
                vm_free(g_app.ram_audio_buf);
                g_app.ram_audio_buf = NULL;
                show_audio_error("Header Error", "Missing 'MThd' marker", "Not a valid MIDI", "");
                return;
            }
        } else {
            if (!ascii_istartswith_nocase(g_app.ram_audio_buf, (int)g_app.ram_audio_size, "BEGIN:IMELODY")) {
                vm_free(g_app.ram_audio_buf);
                g_app.ram_audio_buf = NULL;
                show_audio_error("Header Error", "Missing BEGIN:IMELODY", "Not a valid IMY", "");
                return;
            }
        }

        // Query MIDI / IMY duration (without dividing by 1000)
        VMINT midi_dur = 0;
        if (t->format == FMT_IMY) {
#ifdef VM_FORMAT_IMELODY
            midi_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, VM_FORMAT_IMELODY);
#endif
            if (midi_dur <= 0) {
                midi_dur = vm_audio_duration(full_path);
            }
        } else {
            midi_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, VM_FORMAT_MIDI);
        }

        if (midi_dur > 0) {
            g_app.total_duration_sec = (VMUINT)midi_dur;
        } else {
            g_app.total_duration_sec = 0;
        }

#ifdef VM_FORMAT_IMELODY
        vm_midi_set_type((t->format == FMT_IMY) ? VM_FORMAT_IMELODY : VM_FORMAT_MIDI);
#else
        vm_midi_set_type(VM_FORMAT_MIDI);
#endif

        // Play explicitly via selected device
        VMINT ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms, 0, out_dev, midi_audio_callback);
        if (ret < 0 && out_dev != VM_DEVICE_SPEAKER_BOTH) {
            ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms, 0, VM_DEVICE_SPEAKER_BOTH, midi_audio_callback);
        }
        if (ret < 0) {
            ret = vm_midi_play_by_bytes(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, 0, midi_audio_callback);
        }

        if (ret < 0 && t->format == FMT_IMY) {
            // Fallback to file-based player if device's MIDI DSP rejects IMY bytes
            VMINT fret = vm_audio_play_file_ex(full_path, start_time_ms, out_dev, digital_audio_callback);
            if (fret != VM_AUDIO_SUCCEED && out_dev != VM_DEVICE_SPEAKER_BOTH) {
                fret = vm_audio_play_file_ex(full_path, 0, VM_DEVICE_SPEAKER_BOTH, digital_audio_callback);
            }
            if (fret != VM_AUDIO_SUCCEED) {
                fret = vm_audio_play_file(full_path, digital_audio_callback);
            }
            if (fret == VM_AUDIO_SUCCEED) {
                vm_free(g_app.ram_audio_buf);
                g_app.ram_audio_buf = NULL;
                g_app.ram_audio_size = 0;
                g_app.midi_handle = -1;
                vm_set_volume(g_app.volume);
                g_app.play_state = STATE_PLAYING;
                g_app.active_track_idx = track_index;
                g_app.current_time_ms = start_time_ms;
                g_app.current_time_sec = start_time_ms / 1000;
                g_app.marq_char_offset = 0;
                return;
            }
        }

        if (ret < 0) {
            vm_free(g_app.ram_audio_buf);
            g_app.ram_audio_buf = NULL;
            g_app.ram_audio_size = 0;
            char l1[64], l2[64];
            sprintf(l1, "Output: %s", DEVICE_PROFILE_NAMES[g_app.cur_device]);
            sprintf(l2, "MIDI DSP Code: %d", (int)ret);
            show_audio_error((t->format == FMT_IMY) ? "IMY Synth Failed" : "MIDI Synth Failed", l1, l2, "Synthesizer rejected bytes");
            return;
        }

        g_app.midi_handle = ret;
        vm_set_volume(g_app.volume);
        g_app.play_state = STATE_PLAYING;
        g_app.active_track_idx = track_index;
        g_app.current_time_ms = start_time_ms;
        g_app.current_time_sec = start_time_ms / 1000;
        g_app.marq_char_offset = 0;
        return;
    }

    // ========================================================================
    // PIPELINE 2: Digital Audio Stream Execution (MP3, AWB, VM, AAC, AMR, WAV)
    // ========================================================================
    // Query duration BEFORE playback begins (vm_audio_duration returns SECONDS!)
    VMINT dur_sec = vm_audio_duration(full_path);
    if (dur_sec > 0) {
        g_app.total_duration_sec = (dur_sec > 100000) ? ((VMUINT)dur_sec / 1000) : (VMUINT)dur_sec;
    } else {
        g_app.total_duration_sec = 0;
    }

    VMINT ret = VM_AUDIO_FAILED;

    if (start_time_ms > 0) {
        ret = vm_audio_play_file_ex(full_path, start_time_ms, out_dev, digital_audio_callback);
    }
    if (ret != VM_AUDIO_SUCCEED) {
        ret = vm_audio_play_file_ex(full_path, 0, out_dev, digital_audio_callback);
    }
    if (ret != VM_AUDIO_SUCCEED && out_dev != VM_DEVICE_SPEAKER_BOTH) {
        ret = vm_audio_play_file_ex(full_path, 0, VM_DEVICE_SPEAKER_BOTH, digital_audio_callback);
    }
    if (ret != VM_AUDIO_SUCCEED) {
        ret = vm_audio_play_file(full_path, digital_audio_callback);
    }

    // In-memory fallback if file player is rejected by security sandbox (AWB & VM same rights as MP3)
    if (ret != VM_AUDIO_SUCCEED && t->filesize <= (1024 * 1024 * 2)) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f >= 0) {
            VMUINT alloc_size = (t->filesize + 7) & ~3;
            g_app.ram_audio_buf = (VMUINT8*)vm_malloc(alloc_size);
            if (g_app.ram_audio_buf) {
                VMUINT nread = 0;
                VMINT actual_b = vm_file_read(f, g_app.ram_audio_buf, t->filesize, &nread);
                g_app.ram_audio_size = (actual_b > 0) ? (VMUINT)actual_b : nread;

                VMUINT8 fmt = VM_FORMAT_MP3;
                if (t->format == FMT_AAC) fmt = VM_FORMAT_AAC;
                else if (t->format == FMT_AMR) fmt = VM_FORMAT_AMR;
                else if (t->format == FMT_WAV) fmt = VM_FORMAT_WAV;
                // FMT_MP3, FMT_AWB, FMT_VM use the MP3/digital stream fallback path

                if (g_app.total_duration_sec == 0) {
                    VMINT b_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, fmt);
                    if (b_dur > 0) {
                        g_app.total_duration_sec = (b_dur > 100000) ? ((VMUINT)b_dur / 1000) : (VMUINT)b_dur;
                    }
                }

                ret = vm_audio_play_bytes(g_app.ram_audio_buf, g_app.ram_audio_size, fmt, 0, out_dev, digital_audio_callback);
                if (ret != VM_AUDIO_SUCCEED) {
                    ret = vm_audio_play_bytes_no_block(g_app.ram_audio_buf, g_app.ram_audio_size, fmt, out_dev, digital_audio_callback);
                }
            }
            vm_file_close(f);
        }
    }

    if (ret != VM_AUDIO_SUCCEED) {
        char l1[64], l2[64], l3[64];
        sprintf(l1, "Output: %s", DEVICE_PROFILE_NAMES[g_app.cur_device]);
        sprintf(l2, "Return Code: %d", (int)ret);
        sprintf(l3, "Size: %u KB", (unsigned int)(t->filesize / 1024));
        show_audio_error("Playback Failed", l1, l2, l3);
        return;
    }

    vm_set_volume(g_app.volume);
    g_app.current_time_ms = start_time_ms;
    g_app.current_time_sec = start_time_ms / 1000;
    g_app.play_state = STATE_PLAYING;
    g_app.active_track_idx = track_index;
    g_app.marq_char_offset = 0;
}

static void format_time_string_sec(VMUINT total_s, VMWCHAR* out_str) {
    int h = total_s / 3600;
    int m = (total_s % 3600) / 60;
    int s = total_s % 60;
    char buf[32];
    sprintf(buf, "%02d:%02d:%02d", h, m, s);
    safe_ascii_to_ucs2(out_str, 16, buf);
}

static void prepare_time_picker(VMUINT initial_s) {
    int h = (initial_s / 3600) % 100;
    int m = (initial_s % 3600) / 60;
    int s = initial_s % 60;

    g_app.time_digits[0] = h / 10;
    g_app.time_digits[1] = h % 10;
    g_app.time_digits[2] = m / 10;
    g_app.time_digits[3] = m % 10;
    g_app.time_digits[4] = s / 10;
    g_app.time_digits[5] = s % 10;
    g_app.time_cursor = 0;
}

static void apply_timestamp_jump() {
    int h = g_app.time_digits[0] * 10 + g_app.time_digits[1];
    int m = g_app.time_digits[2] * 10 + g_app.time_digits[3];
    int s = g_app.time_digits[4] * 10 + g_app.time_digits[5];
    
    if (s > 59) s = 59;
    if (m > 59) m = 59;
    
    VMUINT target_sec = ((h * 3600) + (m * 60) + s);
    if (g_app.total_duration_sec > 0 && target_sec > g_app.total_duration_sec) {
        target_sec = g_app.total_duration_sec;
    }

    int target_track = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
    if (target_track >= 0 && target_track < g_app.track_count) {
        start_playback(target_track, target_sec * 1000);
    }
}

// ----------------------------------------------------------------------------
// Timer Routines (10 FPS Engine Tick)
// ----------------------------------------------------------------------------
static void timer_tick(VMINT tid) {
    bool needs_repaint = false;

    // 1. Playback timing update
    if (g_app.play_state == STATE_PLAYING) {
        if (g_app.midi_handle >= 0) {
            VMUINT mtime = 0;
            if (vm_midi_get_time(g_app.midi_handle, &mtime) == 0) {
                g_app.current_time_ms = mtime;
                g_app.current_time_sec = mtime / 1000;
            } else {
                g_app.current_time_ms += ENGINE_TIMER_MS;
                g_app.current_time_sec = g_app.current_time_ms / 1000;
            }
        } else {
            VMINT cur = vm_audio_get_time();
            if (cur >= 0) {
                g_app.current_time_ms = (VMUINT)cur;
                g_app.current_time_sec = (VMUINT)cur / 1000;
            } else {
                g_app.current_time_ms += ENGINE_TIMER_MS;
                g_app.current_time_sec = g_app.current_time_ms / 1000;
            }
        }
        needs_repaint = true;
    }

    // 2. Karaoke screensaver tick
    if (g_app.ui_mode == MODE_KARAOKE) {
        update_screensaver_state();
        needs_repaint = true;
    }

    // 3. Character-window scroll shift every 3 ticks (~300 ms)
    if (g_app.play_state == STATE_PLAYING || (g_app.play_state == STATE_PAUSED && g_app.scroll_on_pause)) {
        g_app.scroll_tick_counter++;
        if (g_app.scroll_tick_counter >= 3) {
            g_app.scroll_tick_counter = 0;
            g_app.marq_char_offset++;
            g_app.sel_char_offset++;
            needs_repaint = true;
        }
    } else if (g_app.ui_mode == MODE_TRACKLIST) {
        g_app.scroll_tick_counter++;
        if (g_app.scroll_tick_counter >= 3) {
            g_app.scroll_tick_counter = 0;
            g_app.sel_char_offset++;
            needs_repaint = true;
        }
    }

    // 4. System Clock 1-second refresh
    if (g_app.show_clock) {
        g_app.clock_tick_counter++;
        if (g_app.clock_tick_counter >= 10) {
            g_app.clock_tick_counter = 0;
            needs_repaint = true;
        }
    }

    if (needs_repaint) {
        trigger_repaint();
    }
}

static void numeric_timeout(VMINT tid) {
    if (g_app.numeric_len > 0) {
        int target = atoi(g_app.numeric_buf);
        if (target >= 1 && target <= g_app.track_count) {
            g_app.selected_track = target - 1;
            
            int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
            int body_h = g_app.screen_h - g_app.header_h - bottom_reserved;
            int item_h = g_app.font_h + 4;
            if (item_h <= 0) item_h = 18;
            int visible_items = (body_h > 0) ? (body_h / item_h) : 1;
            
            if (g_app.selected_track < g_app.scroll_offset) {
                g_app.scroll_offset = g_app.selected_track;
            } else if (g_app.selected_track >= g_app.scroll_offset + visible_items) {
                g_app.scroll_offset = g_app.selected_track - visible_items + 1;
            }
            if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;
            g_app.sel_char_offset = 0;
        }
        g_app.numeric_len = 0;
        g_app.numeric_buf[0] = 0;
        trigger_repaint();
    }
}

// ----------------------------------------------------------------------------
// High-Speed Text Rendering & Scrollable Popup List Geometry
// ----------------------------------------------------------------------------
static void draw_text(int x, int y, const VMWCHAR* str, vm_graphic_color col) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    vm_graphic_setcolor(&col);
    vm_graphic_textout_to_layer(g_app.layer_hdl, x, y, (VMWSTR)str, vm_wstrlen((VMWSTR)str));
}

static void draw_ascii(int x, int y, const char* str, vm_graphic_color col) {
    VMWCHAR wstr[128];
    safe_ascii_to_ucs2(wstr, 128, str);
    draw_text(x, y, wstr, col);
}

static void draw_text_truncated(int x, int y, const VMWCHAR* str, int max_w, vm_graphic_color col) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_w <= 0 || !str) return;

    int len = vm_wstrlen((VMWSTR)str);
    if (len <= 0) return;

    VMWCHAR buf[160];
    if (len >= 159) len = 158;
    safe_wstrncpy_zero(buf, str, len + 1);

    int total_w = vm_graphic_get_string_width(buf);
    if (total_w > max_w) {
        int avg_char_w = total_w / len;
        if (avg_char_w <= 0) avg_char_w = 6;
        int est_chars = max_w / avg_char_w;
        if (est_chars >= len) est_chars = len - 1;
        if (est_chars < 1) est_chars = 1;

        buf[est_chars] = 0;
        while (est_chars > 0 && vm_graphic_get_string_width(buf) > max_w) {
            est_chars--;
            buf[est_chars] = 0;
        }
        len = est_chars;
    }

    if (len > 0) {
        vm_graphic_setcolor(&col);
        vm_graphic_textout_to_layer(g_app.layer_hdl, x, y, buf, len);
    }
}

static void draw_ascii_truncated(int x, int y, const char* str, int max_w, vm_graphic_color col) {
    VMWCHAR wstr[128];
    safe_ascii_to_ucs2(wstr, 128, str);
    draw_text_truncated(x, y, wstr, max_w, col);
}

static void draw_text_centered_wrapped(int area_x, int area_w, int start_y, const VMWCHAR* str, int max_lines, int line_step, vm_graphic_color col) {
    if (!str || str[0] == 0 || area_w <= 10 || max_lines <= 0) return;

    int len = vm_wstrlen((VMWSTR)str);
    int pos = 0;
    for (int line = 0; line < max_lines && pos < len; line++) {
        int remain = len - pos;
        int take = remain;
        VMWCHAR seg[MAX_LRC_TEXT_LEN];
        if (take >= MAX_LRC_TEXT_LEN) take = MAX_LRC_TEXT_LEN - 1;
        safe_wstrncpy_zero(seg, str + pos, take + 1);

        if (vm_graphic_get_string_width(seg) > area_w) {
            while (take > 1 && vm_graphic_get_string_width(seg) > area_w) {
                take--;
                seg[take] = 0;
            }
            // Prefer breaking on space if available
            if (line < max_lines - 1) {
                int sp = -1;
                for (int k = take - 1; k > 0; k--) {
                    if (seg[k] == ' ') {
                        sp = k;
                        break;
                    }
                }
                if (sp > 0) {
                    take = sp;
                    seg[take] = 0;
                }
            }
        }

        int sw_line = vm_graphic_get_string_width(seg);
        int dx = area_x + (area_w - sw_line) / 2;
        if (dx < area_x) dx = area_x;
        draw_text(dx, start_y + line * line_step, seg, col);

        pos += take;
        while (pos < len && str[pos] == ' ') pos++;
    }
}

struct PopupListGeom {
    int px, py, pw, ph;
    int title_h;
    int row_h;
    int visible_count;
    int total_count;
};

static PopupListGeom calc_popup_list_geom(int total_count, bool has_title) {
    PopupListGeom g;
    int sw = g_app.screen_w;
    int sh = g_app.screen_h;
    int fh = g_app.font_h;

    g.total_count = total_count;
    g.row_h = fh + 4;
    if (g.row_h < 14) g.row_h = 14;
    g.title_h = has_title ? (fh + 8) : 0;
    g.pw = sw - 20;
    if (g.pw < 80) g.pw = sw;
    g.px = (sw - g.pw) / 2;

    int max_ph = sh - g_app.footer_h - 8;
    int avail_rows_h = max_ph - g.title_h - 12;
    int max_vis = (avail_rows_h > 0) ? (avail_rows_h / g.row_h) : 1;
    if (max_vis < 1) max_vis = 1;

    g.visible_count = (total_count < max_vis) ? total_count : max_vis;
    if (g.visible_count < 1) g.visible_count = 1;

    g.ph = g.title_h + g.visible_count * g.row_h + 12;
    g.py = (sh - g_app.footer_h - g.ph) / 2;
    if (g.py < 4) g.py = 4;
    return g;
}

static void adjust_popup_scroll(int total_count, int visible_count) {
    if (visible_count < 1) visible_count = 1;
    if (total_count <= 0) {
        g_app.menu_cursor = 0;
        g_app.menu_scroll = 0;
        return;
    }
    if (g_app.menu_cursor < 0) g_app.menu_cursor = 0;
    if (g_app.menu_cursor >= total_count) g_app.menu_cursor = total_count - 1;
    if (g_app.menu_cursor < g_app.menu_scroll) {
        g_app.menu_scroll = g_app.menu_cursor;
    } else if (g_app.menu_cursor >= g_app.menu_scroll + visible_count) {
        g_app.menu_scroll = g_app.menu_cursor - visible_count + 1;
    }
    int max_scroll = total_count - visible_count;
    if (max_scroll < 0) max_scroll = 0;
    if (g_app.menu_scroll > max_scroll) g_app.menu_scroll = max_scroll;
    if (g_app.menu_scroll < 0) g_app.menu_scroll = 0;
}

static void draw_popup_list_frame_and_sb(const PopupListGeom* g, const char* title, Theme* th) {
    vm_graphic_setcolor(&th->popup_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, g->px, g->py, g->pw, g->ph);
    vm_graphic_setcolor(&th->popup_border);
    vm_graphic_rect_ex(g_app.layer_hdl, g->px, g->py, g->pw, g->ph);

    if (title && g->title_h > 0) {
        draw_ascii_truncated(g->px + 8, g->py + 5, title, g->pw - 16, th->header_text);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_line_ex(g_app.layer_hdl, g->px + 1, g->py + g->title_h + 1, g->px + g->pw - 2, g->py + g->title_h + 1);
    }

    if (g->total_count > g->visible_count && g->visible_count > 0) {
        int sb_x = g->px + g->pw - 6;
        int sb_y = g->py + g->title_h + 6;
        int sb_h = g->visible_count * g->row_h;
        vm_graphic_setcolor(&th->header_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 3, sb_h);

        int thumb_h = (g->visible_count * sb_h) / g->total_count;
        if (thumb_h < 6) thumb_h = 6;
        int max_scroll = g->total_count - g->visible_count;
        int thumb_y = sb_y;
        if (max_scroll > 0) {
            thumb_y += (g_app.menu_scroll * (sb_h - thumb_h)) / max_scroll;
        }
        vm_graphic_setcolor(&th->item_selected_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, thumb_y, 3, thumb_h);
    }
}

static int get_active_popup_total_items() {
    switch (g_app.ui_mode) {
        case MODE_MENU:          return MENU_ITEM_COUNT;
        case MODE_DEVICE_SELECT: return DEV_PROFILE_COUNT;
        case MODE_THEME_SELECT:  return THEME_COUNT;
        case MODE_FORMAT_FILTER: return 8;
        case MODE_SORT_SELECT:   return SORT_COUNT;
        case MODE_LRC_PICKER:    return 2 + g_app.lrc_file_count;
        default:                 return 0;
    }
}

static bool active_popup_has_title() {
    return (g_app.ui_mode != MODE_MENU);
}

// ----------------------------------------------------------------------------
// Karaoke & Screensaver Renderer
// ----------------------------------------------------------------------------
static void render_karaoke_view(Theme* th) {
    int sw = g_app.screen_w;
    int sh = g_app.screen_h;
    int fh = g_app.font_h;
    int top_y = g_app.header_h;
    int bot_y = sh - g_app.footer_h;
    int vh = bot_y - top_y;
    if (vh <= 0) return;

    // 1. Optional Screensaver Layer
    if (g_app.screensaver_id == SS_STARFIELD) {
        int cx = sw / 2;
        int cy = top_y + vh / 2;
        for (int i = 0; i < 24; i++) {
            int z = g_app.ss_stars[i].z;
            if (z <= 0) z = 1;
            int sx = cx + (g_app.ss_stars[i].x * 16) / z;
            int sy = cy + (g_app.ss_stars[i].y * 16) / z;
            if (sx >= 2 && sx < sw - 2 && sy >= top_y + 2 && sy < bot_y - 2) {
                vm_graphic_color sc = (z < 6) ? th->item_selected_text : ((z < 11) ? th->item_playing : th->item_text);
                vm_graphic_setcolor(&sc);
                int sz = (z < 5) ? 3 : ((z < 10) ? 2 : 1);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, sx, sy, sz, sz);
            }
        }
    } else if (g_app.screensaver_id == SS_EQUALIZER) {
        int bars = 12;
        int bar_w = (sw - 20) / bars;
        if (bar_w < 4) bar_w = 4;
        int gap = 2;
        int base_y = bot_y - fh - 16;
        if (base_y > top_y + 20) {
            for (int i = 0; i < bars; i++) {
                int bh = g_app.ss_eq_heights[i % 16];
                if (bh > base_y - top_y - 8) bh = base_y - top_y - 8;
                int bx = 10 + i * bar_w;
                vm_graphic_setcolor(&th->header_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, base_y - bh, bar_w - gap, bh);
                vm_graphic_setcolor(&th->item_playing);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, base_y - bh, bar_w - gap, (bh >= 3) ? 3 : bh);
            }
        }
    } else if (g_app.screensaver_id == SS_BOUNCING_ORB) {
        int ox = g_app.ss_orb_x;
        int oy = g_app.ss_orb_y;
        vm_graphic_setcolor(&th->item_selected_bg);
        vm_graphic_fill_ellipse_ex(g_app.layer_hdl, ox - 12, oy - 12, 24, 24);
        vm_graphic_setcolor(&th->item_playing);
        vm_graphic_ellipse_ex(g_app.layer_hdl, ox - 14, oy - 14, 28, 28);
    } else if (g_app.screensaver_id == SS_DIGITAL_RAIN) {
        int cols = 14;
        int col_w = sw / cols;
        if (col_w < 8) col_w = 8;
        const char glyphs[] = "01AF*+";
        for (int i = 0; i < cols && i < 20; i++) {
            int gx = 4 + i * col_w;
            for (int t = 0; t < 3; t++) {
                int gy = g_app.ss_rain_y[i] - t * (fh + 2);
                if (gy >= top_y && gy + fh < bot_y) {
                    char ch[2];
                    ch[0] = glyphs[(g_app.ss_frame + i + t) % 6];
                    ch[1] = 0;
                    draw_ascii(gx, gy, ch, (t == 0) ? th->item_playing : th->header_bg);
                }
            }
        }
    }

    // 2. Top Track & Screensaver Info Strip
    VMWCHAR disp_name[128];
    if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
        get_track_display_name(&g_app.tracks[g_app.active_track_idx], disp_name, 128);
    } else if (g_app.selected_track >= 0 && g_app.selected_track < g_app.track_count) {
        get_track_display_name(&g_app.tracks[g_app.selected_track], disp_name, 128);
    } else {
        safe_ascii_to_ucs2(disp_name, 128, "No Track Selected");
    }
    draw_text_truncated(6, top_y + 3, disp_name, sw - 12, th->header_text);

    char ss_info[64];
    if (g_app.lrc_user_offset_ms != 0) {
        sprintf(ss_info, "< %s >  Offs:%+dms", SCREENSAVER_NAMES[g_app.screensaver_id], g_app.lrc_user_offset_ms);
    } else {
        sprintf(ss_info, "< %s >", SCREENSAVER_NAMES[g_app.screensaver_id]);
    }
    draw_ascii_truncated(6, top_y + fh + 5, ss_info, sw - 12, th->marquee_text);

    // 3. Synchronized LRC Lyrics Area
    int lyr_top = top_y + fh * 2 + 12;
    int lyr_bot = bot_y - fh - 16;
    int lyr_h = lyr_bot - lyr_top;

    if (g_app.lrc_count > 0 && lyr_h > fh * 2) {
        VMINT32 eff_ms = (VMINT32)g_app.current_time_ms - g_app.lrc_file_offset_ms + g_app.lrc_user_offset_ms;
        if (eff_ms < 0) eff_ms = 0;

        int active_idx = -1;
        for (int i = 0; i < g_app.lrc_count; i++) {
            if (g_app.lrc_lines[i].time_ms <= eff_ms) {
                active_idx = i;
            } else {
                break;
            }
        }

        int center_y = lyr_top + (lyr_h / 2) - fh;
        if (active_idx > 0) {
            int prev_y = center_y - fh - 6;
            if (prev_y >= lyr_top) {
                draw_text_centered_wrapped(8, sw - 16, prev_y, g_app.lrc_lines[active_idx - 1].text, 1, fh + 2, th->item_text);
            }
        }

        if (active_idx >= 0) {
            draw_text_centered_wrapped(8, sw - 16, center_y, g_app.lrc_lines[active_idx].text, 2, fh + 2, th->item_playing);
        } else {
            VMWCHAR wait_w[32];
            safe_ascii_to_ucs2(wait_w, 32, "~ Instrumental ~");
            draw_text_centered_wrapped(8, sw - 16, center_y, wait_w, 1, fh + 2, th->marquee_text);
        }

        int next_y = center_y + fh * 2 + 10;
        if (active_idx + 1 < g_app.lrc_count && next_y + fh <= lyr_bot) {
            draw_text_centered_wrapped(8, sw - 16, next_y, g_app.lrc_lines[active_idx + 1].text, 1, fh + 2, th->item_text);
        }
        if (active_idx + 2 < g_app.lrc_count && next_y + fh * 2 + 4 <= lyr_bot) {
            draw_text_centered_wrapped(8, sw - 16, next_y + fh + 4, g_app.lrc_lines[active_idx + 2].text, 1, fh + 2, th->item_text);
        }
    } else {
        int cy = lyr_top + lyr_h / 2 - fh;
        if (cy < lyr_top) cy = lyr_top;
        VMWCHAR no_lrc[64];
        safe_ascii_to_ucs2(no_lrc, 64, "No LRC Loaded");
        draw_text_centered_wrapped(8, sw - 16, cy, no_lrc, 1, fh + 2, th->item_text);
        safe_ascii_to_ucs2(no_lrc, 64, "Press LSK to Pick .lrc");
        draw_text_centered_wrapped(8, sw - 16, cy + fh + 4, no_lrc, 1, fh + 2, th->header_text);
    }

    // 4. Bottom Progress Bar & Timestamps
    int bar_y = bot_y - fh - 10;
    vm_graphic_setcolor(&th->header_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 8, bar_y, sw - 16, 4);
    if (g_app.total_duration_sec > 0) {
        int fill_w = (int)((g_app.current_time_sec * (VMUINT)(sw - 16)) / g_app.total_duration_sec);
        if (fill_w > sw - 16) fill_w = sw - 16;
        vm_graphic_setcolor(&th->item_playing);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, 8, bar_y, fill_w, 4);
    }

    VMWCHAR cur_w[16], tot_w[16], sep1[8], time_line[48];
    format_time_string_sec(g_app.current_time_sec, cur_w);
    format_time_string_sec(g_app.total_duration_sec, tot_w);
    safe_ascii_to_ucs2(sep1, 8, " / ");
    vm_wstrcpy(time_line, cur_w);
    vm_wstrcat(time_line, sep1);
    vm_wstrcat(time_line, tot_w);
    draw_text(8, bot_y - fh - 4, time_line, th->marquee_text);

    const char* st_tag = (g_app.play_state == STATE_PLAYING) ? "PLAY" : ((g_app.play_state == STATE_PAUSED) ? "PAUSE" : "STOP");
    VMWCHAR w_st[16];
    safe_ascii_to_ucs2(w_st, 16, st_tag);
    int stw = vm_graphic_get_string_width(w_st);
    draw_text(sw - stw - 8, bot_y - fh - 4, w_st, th->item_playing);
}

// ----------------------------------------------------------------------------
// Core Renderer (100% Consistent Coloring & Anchored Scrolling)
// ----------------------------------------------------------------------------
static void render_screen() {
    vm_graphic_active_layer(g_app.layer_hdl);
    vm_graphic_set_font(VM_SMALL_FONT);

    Theme* th = &g_themes[g_app.cur_theme];
    int sw = g_app.screen_w;
    int sh = g_app.screen_h;
    int fh = g_app.font_h;

    // Background
    vm_graphic_setcolor(&th->bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, 0, sw, sh);

    // 1. Header & Live Clock / ID3 Badge
    vm_graphic_setcolor(&th->header_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, 0, sw, g_app.header_h);
    vm_graphic_setcolor(&th->popup_border);
    vm_graphic_line_ex(g_app.layer_hdl, 0, g_app.header_h - 1, sw - 1, g_app.header_h - 1);

    char title_buf[64];
    const char* header_title = "Audio Player";
    switch (g_app.ui_mode) {
        case MODE_TRACKLIST:
            if (g_app.id3_mode) {
                sprintf(title_buf, "ID3 (%d/%d)", g_app.track_count > 0 ? g_app.selected_track + 1 : 0, g_app.track_count);
            } else {
                sprintf(title_buf, "Tracks (%d/%d)", g_app.track_count > 0 ? g_app.selected_track + 1 : 0, g_app.track_count);
            }
            header_title = title_buf;
            break;
        case MODE_MENU:
            header_title = "Options Menu";
            break;
        case MODE_VOLUME_SELECT:
            header_title = "Volume";
            break;
        case MODE_DEVICE_SELECT:
            header_title = "Audio Output";
            break;
        case MODE_TIME_PICKER:
            header_title = "Jump to Timestamp";
            break;
        case MODE_FORMAT_FILTER:
            header_title = "Audio Formats";
            break;
        case MODE_SORT_SELECT:
            header_title = "Sort Tracks";
            break;
        case MODE_THEME_SELECT:
            header_title = "Select Theme";
            break;
        case MODE_KARAOKE:
            header_title = "Karaoke Mode";
            break;
        case MODE_LRC_PICKER:
            header_title = "Pick LRC File";
            break;
        case MODE_POPUP_CONFIRM_EXIT:
            header_title = "Exit";
            break;
        case MODE_POPUP_ABOUT:
            header_title = "About";
            break;
        case MODE_POPUP_ERROR:
            header_title = "Audio Notice";
            break;
    }

    int header_text_y = (g_app.header_h - fh) / 2;
    int right_used_w = 6;

    // Top-right Numeric Jump Badge OR Live Clock + ID3 Toggle Pill
    if (g_app.numeric_len > 0) {
        char jump_buf[32];
        sprintf(jump_buf, "#%s_", g_app.numeric_buf);
        VMWCHAR w_jump[32];
        safe_ascii_to_ucs2(w_jump, 32, jump_buf);
        int jw = vm_graphic_get_string_width(w_jump);

        vm_graphic_setcolor(&th->item_selected_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, sw - jw - 10, 2, jw + 6, g_app.header_h - 4);
        draw_text(sw - jw - 7, header_text_y, w_jump, th->item_selected_text);
        right_used_w = jw + 14;
    } else {
        if (g_app.show_clock) {
            vm_time_t sys_t;
            if (vm_get_time(&sys_t) == 0) {
                char clk_buf[16];
                sprintf(clk_buf, "%02d:%02d", sys_t.hour, sys_t.min);
                VMWCHAR w_clk[16];
                safe_ascii_to_ucs2(w_clk, 16, clk_buf);
                int cw = vm_graphic_get_string_width(w_clk);
                draw_text(sw - cw - 6, header_text_y, w_clk, th->header_text);
                right_used_w += cw + 6;
            }
        }
        if (g_app.ui_mode == MODE_TRACKLIST && sw >= 176) {
            VMWCHAR w_id3[12];
            safe_ascii_to_ucs2(w_id3, 12, "ID3");
            int iw = vm_graphic_get_string_width(w_id3);
            int bx = sw - right_used_w - iw - 8;
            vm_graphic_color bbg = g_app.id3_mode ? th->item_selected_bg : th->popup_bg;
            vm_graphic_color bfg = g_app.id3_mode ? th->item_selected_text : th->item_text;
            vm_graphic_setcolor(&bbg);
            vm_graphic_fill_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
            vm_graphic_setcolor(&th->popup_border);
            vm_graphic_rect_ex(g_app.layer_hdl, bx, 2, iw + 6, g_app.header_h - 4);
            draw_text(bx + 3, header_text_y, w_id3, bfg);
            right_used_w += iw + 12;
        }
    }

    draw_ascii_truncated(6, header_text_y, header_title, sw - right_used_w - 8, th->header_text);

    // 2. Main Body (Either Karaoke Mode OR Tracklist Viewport)
    if (g_app.ui_mode == MODE_KARAOKE) {
        render_karaoke_view(th);
    } else {
        int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int body_h = sh - g_app.header_h - bottom_reserved;

        if (body_h > 0) {
            int item_h = fh + 4;
            if (item_h <= 0) item_h = 18;
            int visible_items = body_h / item_h;

            for (int i = 0; i < visible_items; i++) {
                int idx = g_app.scroll_offset + i;
                if (idx >= g_app.track_count) break;

                int iy = g_app.header_h + i * item_h;
                if (iy + item_h > g_app.header_h + body_h) break;

                bool is_sel = (idx == g_app.selected_track);
                bool is_playing = (g_app.play_state != STATE_STOPPED && idx == g_app.active_track_idx);

                if (is_sel) {
                    vm_graphic_setcolor(&th->item_selected_bg);
                    vm_graphic_fill_rect_ex(g_app.layer_hdl, 2, iy, sw - 10, item_h);
                }

                char prefix[16];
                if (is_playing) {
                    sprintf(prefix, "%s%d. ", (g_app.play_state == STATE_PLAYING ? "> " : "|| "), idx + 1);
                } else {
                    sprintf(prefix, "  %d. ", idx + 1);
                }

                VMWCHAR wprefix[16];
                safe_ascii_to_ucs2(wprefix, 16, prefix);
                int prefix_w = vm_graphic_get_string_width(wprefix);

                vm_graphic_color row_col;
                if (is_playing) {
                    row_col = th->item_playing;
                } else if (is_sel) {
                    row_col = th->item_selected_text;
                } else {
                    row_col = th->item_text;
                }

                draw_text(5, iy + 2, wprefix, row_col);

                int name_x = 5 + prefix_w;
                int name_view_w = (sw - 10) - name_x;

                VMWCHAR disp_name[128];
                get_track_display_name(&g_app.tracks[idx], disp_name, 128);

                int full_len = vm_wstrlen(disp_name);
                int full_w = vm_graphic_get_string_width(disp_name);

                if (is_sel && full_w > name_view_w) {
                    int offset = g_app.sel_char_offset % (full_len + 5);
                    VMWCHAR* p = disp_name;
                    if (offset < full_len) p += offset;
                    draw_text_truncated(name_x, iy + 2, p, name_view_w, row_col);
                } else {
                    draw_text_truncated(name_x, iy + 2, disp_name, name_view_w, row_col);
                }
            }

            // Vertical Scrollbar
            if (g_app.track_count > visible_items && visible_items > 0) {
                int sb_x = sw - 6;
                int sb_y = g_app.header_h + 2;
                int sb_h = body_h - 4;
                
                vm_graphic_setcolor(&th->header_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, sb_y, 4, sb_h);

                int thumb_h = (visible_items * sb_h) / g_app.track_count;
                if (thumb_h < 8) thumb_h = 8;
                
                int max_scroll = g_app.track_count - visible_items;
                int thumb_y = sb_y;
                if (max_scroll > 0) {
                    thumb_y += (g_app.scroll_offset * (sb_h - thumb_h)) / max_scroll;
                }

                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, sb_x, thumb_y, 4, thumb_h);
            }
        }

        // 3. Two-Line Now-Playing Dock
        if (g_app.play_state != STATE_STOPPED) {
            int my = sh - g_app.footer_h - g_app.marquee_h;

            vm_graphic_setcolor(&th->marquee_bg);
            vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, my, sw, g_app.marquee_h);

            vm_graphic_setcolor(&th->popup_border);
            vm_graphic_line_ex(g_app.layer_hdl, 0, my, sw - 1, my);

            VMWCHAR cur_w[16], tot_w[16], sep1[8];
            format_time_string_sec(g_app.current_time_sec, cur_w);
            format_time_string_sec(g_app.total_duration_sec, tot_w);
            safe_ascii_to_ucs2(sep1, 8, " / ");

            VMWCHAR line1_str[64];
            vm_wstrcpy(line1_str, cur_w);
            vm_wstrcat(line1_str, sep1);
            vm_wstrcat(line1_str, tot_w);
            draw_text(6, my + 2, line1_str, th->marquee_text);

            char loop_tag[24];
            sprintf(loop_tag, "V%d [%s]", g_app.volume, LOOP_NAMES[g_app.loop_mode]);
            VMWCHAR wloop[24];
            safe_ascii_to_ucs2(wloop, 24, loop_tag);
            int lw = vm_graphic_get_string_width(wloop);
            draw_text(sw - lw - 6, my + 2, wloop, th->item_playing);

            if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
                int line2_y = my + fh + 3;
                int max_w = sw - 12;
                VMWCHAR marq_name[128];
                get_track_display_name(&g_app.tracks[g_app.active_track_idx], marq_name, 128);
                int full_len = vm_wstrlen(marq_name);
                int full_w = vm_graphic_get_string_width(marq_name);

                if (full_w <= max_w) {
                    draw_text(6, line2_y, marq_name, th->item_playing);
                } else {
                    int offset = g_app.marq_char_offset % (full_len + 5);
                    VMWCHAR* p = marq_name;
                    if (offset < full_len) p += offset;
                    draw_text_truncated(6, line2_y, p, max_w, th->item_playing);
                }
            }
        }
    }

    // 4. Overlays & Modals (All vertical lists clamped & scrollable!)
    if (g_app.ui_mode == MODE_MENU) {
        PopupListGeom g = calc_popup_list_geom(MENU_ITEM_COUNT, false);
        adjust_popup_scroll(MENU_ITEM_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, NULL, th);

        char vol_item[32], loop_item[32], id3_item[32], sort_item[32], pause_scroll_item[32], clk_item[32];
        sprintf(vol_item, "Volume: %d/6", g_app.volume);
        sprintf(loop_item, "Loop Mode: %s", LOOP_NAMES[g_app.loop_mode]);
        sprintf(id3_item, "ID3 Tags: %s", g_app.id3_mode ? "On" : "Off");
        sprintf(sort_item, "Sort By: %s", SORT_SHORT_NAMES[g_app.sort_mode]);
        sprintf(pause_scroll_item, "Scroll on Pause: %s", g_app.scroll_on_pause ? "On" : "Off");
        sprintf(clk_item, "Show Clock: %s", g_app.show_clock ? "On" : "Off");

        const char* menu_items[MENU_ITEM_COUNT] = {
            g_app.play_state == STATE_PLAYING ? "Pause" : "Play",
            "Karaoke Mode",
            "Jump Timestamp",
            vol_item,
            "Audio Output",
            loop_item,
            id3_item,
            sort_item,
            pause_scroll_item,
            clk_item,
            "Format Filter",
            "Themes",
            "About"
        };

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= MENU_ITEM_COUNT) break;
            int iy = g.py + 6 + i * g.row_h;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 3, iy - 1, g.pw - 11, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, menu_items[idx], text_max_w, th->item_selected_text);
            } else {
                draw_ascii_truncated(g.px + 8, iy + 1, menu_items[idx], text_max_w, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        int pw = sw - 24;
        int ph = fh * 3 + 38;
        int px = 12;
        int py = (sh - g_app.footer_h - ph) / 2;
        if (py < 4) py = 4;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        char vline[32];
        sprintf(vline, "Volume Level: %d / 6", g_app.volume);
        draw_ascii(px + 10, py + 8, vline, th->header_text);

        // Visual segmented bar (6 segments + mute indicator)
        int bar_x = px + 10;
        int bar_y = py + fh + 16;
        int bar_w = pw - 20;
        int seg_w = (bar_w - 5 * 3) / 6;
        if (seg_w < 4) seg_w = 4;
        for (int s = 0; s < 6; s++) {
            int sx = bar_x + s * (seg_w + 3);
            vm_graphic_color scol = (s < g_app.volume) ? th->item_playing : th->header_bg;
            vm_graphic_setcolor(&scol);
            vm_graphic_fill_rect_ex(g_app.layer_hdl, sx, bar_y, seg_w, fh + 2);
            vm_graphic_setcolor(&th->popup_border);
            vm_graphic_rect_ex(g_app.layer_hdl, sx, bar_y, seg_w, fh + 2);
        }

        draw_ascii_truncated(px + 10, py + fh * 2 + 24, "Left/Right or 0-6", pw - 20, th->item_text);
    }
    else if (g_app.ui_mode == MODE_DEVICE_SELECT) {
        PopupListGeom g = calc_popup_list_geom(DEV_PROFILE_COUNT, true);
        adjust_popup_scroll(DEV_PROFILE_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Select Output Device:", th);

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= DEV_PROFILE_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            char dev_line[48];
            sprintf(dev_line, "%s %s", (idx == (int)g_app.cur_device ? "*" : " "), DEVICE_PROFILE_NAMES[idx]);

            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, dev_line, text_max_w, th->item_selected_text);
            } else {
                draw_ascii_truncated(g.px + 8, iy + 1, dev_line, text_max_w, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_TIME_PICKER) {
        int pw = sw - 20;
        int ph = fh * 3 + 26;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 8, py + 6, "Enter Timestamp:", th->item_text);

        VMWCHAR w_zero[2] = { (VMWCHAR)'0', 0 };
        VMWCHAR w_colon[2] = { (VMWCHAR)':', 0 };
        int dw = vm_graphic_get_string_width(w_zero);
        if (dw <= 0) dw = 8;
        int cw = vm_graphic_get_string_width(w_colon);
        if (cw <= 0) cw = 5;
        int cell_w = dw + 6;
        
        int total_w = 6 * cell_w + 2 * (cw + 8);
        int start_x = px + (pw - total_w) / 2;
        int digit_y = py + fh + 10;

        int cur_x = start_x;
        for (int i = 0; i < 6; i++) {
            if (i == 2 || i == 4) {
                draw_ascii(cur_x + 4, digit_y, ":", th->header_text);
                cur_x += cw + 8;
            }

            char d_str[2] = { (char)('0' + g_app.time_digits[i]), 0 };
            if (i == g_app.time_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, cur_x, digit_y - 2, cell_w, fh + 4);
                draw_ascii(cur_x + 3, digit_y, d_str, th->item_selected_text);

                vm_graphic_setcolor(&th->item_playing);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, cur_x, digit_y + fh + 3, cell_w, 3);
            } else {
                draw_ascii(cur_x + 3, digit_y, d_str, th->item_text);
            }
            cur_x += cell_w;
        }
    }
    else if (g_app.ui_mode == MODE_FORMAT_FILTER) {
        PopupListGeom g = calc_popup_list_geom(8, true);
        adjust_popup_scroll(8, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Supported Formats:", th);

        const char* fmt_names[] = {
            "MP3", "MIDI", "IMY (iMelody)", "AAC", "AMR (Voice)", "AWB (AMR-WB)", "VM (VoiceMemo)", "WAV"
        };
        VMUINT8 masks[] = {
            FMT_MP3, FMT_MIDI, FMT_IMY, FMT_AAC, FMT_AMR, FMT_AWB, FMT_VM, FMT_WAV
        };

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= 8) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            bool active = (g_app.enabled_formats & masks[idx]) != 0;
            char box[40];
            sprintf(box, "[%c] %s", active ? 'X' : ' ', fmt_names[idx]);

            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, box, text_max_w, th->item_selected_text);
            } else {
                draw_ascii_truncated(g.px + 8, iy + 1, box, text_max_w, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_SORT_SELECT) {
        PopupListGeom g = calc_popup_list_geom(SORT_COUNT, true);
        adjust_popup_scroll(SORT_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Sort Tracks By:", th);

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= SORT_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            char line[48];
            sprintf(line, "%s %s", (idx == (int)g_app.sort_mode ? "*" : " "), SORT_NAMES[idx]);

            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, line, text_max_w, th->item_selected_text);
            } else {
                draw_ascii_truncated(g.px + 8, iy + 1, line, text_max_w, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_THEME_SELECT) {
        PopupListGeom g = calc_popup_list_geom(THEME_COUNT, true);
        adjust_popup_scroll(THEME_COUNT, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Select Color Theme:", th);

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= THEME_COUNT) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            char theme_line[36];
            sprintf(theme_line, "%s %s", (idx == g_app.cur_theme ? "*" : " "), g_themes[idx].name);

            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
                draw_ascii_truncated(g.px + 8, iy + 1, theme_line, text_max_w, th->item_selected_text);
            } else {
                draw_ascii_truncated(g.px + 8, iy + 1, theme_line, text_max_w, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_LRC_PICKER) {
        int total = 2 + g_app.lrc_file_count;
        PopupListGeom g = calc_popup_list_geom(total, true);
        adjust_popup_scroll(total, g.visible_count);
        draw_popup_list_frame_and_sb(&g, "Pick LRC Lyric File:", th);

        int text_max_w = g.pw - 18;
        for (int i = 0; i < g.visible_count; i++) {
            int idx = g_app.menu_scroll + i;
            if (idx >= total) break;
            int iy = g.py + g.title_h + 6 + i * g.row_h;
            vm_graphic_color col = (idx == g_app.menu_cursor) ? th->item_selected_text : th->item_text;
            if (idx == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, g.px + 4, iy - 1, g.pw - 12, g.row_h);
            }
            if (idx == 0) {
                draw_ascii_truncated(g.px + 8, iy + 1, "<Auto-Match Song>", text_max_w, col);
            } else if (idx == 1) {
                draw_ascii_truncated(g.px + 8, iy + 1, "<None (Clear LRC)>", text_max_w, col);
            } else {
                draw_text_truncated(g.px + 8, iy + 1, g_app.lrc_files[idx - 2], text_max_w, col);
            }
        }
    }
    else if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) {
        int pw = sw - 40;
        int ph = fh * 2 + 20;
        int px = (sw - pw) / 2;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 14, py + (ph - fh) / 2, "Exit Audio Player?", th->item_text);
    }
    else if (g_app.ui_mode == MODE_POPUP_ABOUT) {
        const char* about_ver = "Made with Gemini 4 Hydrogen. Ver. 1.1";
        VMWCHAR w_about[64];
        safe_ascii_to_ucs2(w_about, 64, about_ver);
        int pw = sw - 20;
        bool fits_one_line = (vm_graphic_get_string_width(w_about) <= pw - 24);
        int ph = fh * (fits_one_line ? 3 : 4) + 26;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 12, py + 10, "http://stacorp.mw.lt", th->item_playing);
        if (fits_one_line) {
            draw_ascii(px + 12, py + 14 + fh, about_ver, th->item_text);
        } else {
            draw_ascii_truncated(px + 12, py + 14 + fh, "Made with Gemini 4 Hydrogen.", pw - 24, th->item_text);
            draw_ascii_truncated(px + 12, py + 16 + fh * 2, "Ver. 1.1", pw - 24, th->item_text);
        }
    }
    else if (g_app.ui_mode == MODE_POPUP_ERROR) {
        int pw = sw - 20;
        int ph = fh * 5 + 32;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii_truncated(px + 10, py + 6, g_app.error_title, pw - 20, th->item_playing);
        draw_ascii_truncated(px + 10, py + 8 + fh, g_app.error_line1, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 10 + fh * 2, g_app.error_line2, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 12 + fh * 3, g_app.error_line3, pw - 20, th->item_text);
        draw_ascii_truncated(px + 10, py + 16 + fh * 4, "[Press OK/RSK to Close]", pw - 20, th->header_text);
    }

    // 5. Softkey Bar
    int footer_y = sh - g_app.footer_h;
    vm_graphic_setcolor(&th->softkey_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, footer_y, sw, g_app.footer_h);
    vm_graphic_setcolor(&th->popup_border);
    vm_graphic_line_ex(g_app.layer_hdl, 0, footer_y, sw - 1, footer_y);

    const char* lsk_label = "Options";
    const char* rsk_label = "Exit";

    if (g_app.ui_mode == MODE_MENU || g_app.ui_mode == MODE_DEVICE_SELECT ||
        g_app.ui_mode == MODE_SORT_SELECT || g_app.ui_mode == MODE_LRC_PICKER) {
        lsk_label = "Select";
        rsk_label = "Back";
    } else if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        lsk_label = "OK";
        rsk_label = "Back";
    } else if (g_app.ui_mode == MODE_KARAOKE) {
        lsk_label = "Pick LRC";
        rsk_label = "Back";
    } else if (g_app.ui_mode == MODE_TIME_PICKER) {
        lsk_label = "Jump";
        rsk_label = "Cancel";
    } else if (g_app.ui_mode == MODE_FORMAT_FILTER || g_app.ui_mode == MODE_THEME_SELECT) {
        lsk_label = "Toggle";
        rsk_label = "Done";
    } else if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) {
        lsk_label = "Yes";
        rsk_label = "No";
    } else if (g_app.ui_mode == MODE_POPUP_ABOUT) {
        lsk_label = "";
        rsk_label = "Close";
    } else if (g_app.ui_mode == MODE_POPUP_ERROR) {
        lsk_label = "";
        rsk_label = "OK";
    }

    int softkey_text_y = footer_y + (g_app.footer_h - fh) / 2;

    if (lsk_label[0] != '\0') {
        draw_ascii(10, softkey_text_y, lsk_label, th->softkey_text);
    }
    if (rsk_label[0] != '\0') {
        VMWCHAR w_rsk[32];
        safe_ascii_to_ucs2(w_rsk, 32, rsk_label);
        int rw = vm_graphic_get_string_width(w_rsk);
        draw_text(sw - rw - 10, softkey_text_y, w_rsk, th->softkey_text);
    }

    VMINT layer_hdl = g_app.layer_hdl;
    vm_graphic_flush_layer(&layer_hdl, 1);
}

static void trigger_repaint() {
    render_screen();
}

static void set_ui_mode_with_cursor(AppUIMode mode, int cursor) {
    g_app.ui_mode = mode;
    g_app.menu_cursor = cursor;
    g_app.menu_scroll = 0;
    trigger_repaint();
}

static void set_ui_mode(AppUIMode mode) {
    set_ui_mode_with_cursor(mode, 0);
}

// ----------------------------------------------------------------------------
// Keypad Event Handler
// ----------------------------------------------------------------------------
static void handle_key_event(VMINT event, VMINT keycode) {
    if (event != VM_KEY_EVENT_DOWN && event != VM_KEY_EVENT_REPEAT) {
        return;
    }

    if (g_app.ui_mode == MODE_POPUP_CONFIRM_EXIT) {
        if (keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_OK) {
            stop_playback();
            vm_exit_app();
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
        }
        return;
    }

    // Dismiss Error / About
    if (g_app.ui_mode == MODE_POPUP_ABOUT || g_app.ui_mode == MODE_POPUP_ERROR) {
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_OK || 
            keycode == VM_KEY_CLEAR || keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
        }
        return;
    }

    // Volume Control Modal
    if (g_app.ui_mode == MODE_VOLUME_SELECT) {
        if (keycode == VM_KEY_LEFT || keycode == VM_KEY_DOWN || keycode == VM_KEY_VOL_DOWN) {
            if (g_app.volume > 0) {
                g_app.volume--;
                vm_set_volume(g_app.volume);
            }
        } else if (keycode == VM_KEY_RIGHT || keycode == VM_KEY_UP || keycode == VM_KEY_VOL_UP) {
            if (g_app.volume < 6) {
                g_app.volume++;
                vm_set_volume(g_app.volume);
            }
        } else if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM6) {
            g_app.volume = keycode - VM_KEY_NUM0;
            vm_set_volume(g_app.volume);
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY ||
                   keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    // Karaoke Mode Controls
    if (g_app.ui_mode == MODE_KARAOKE) {
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK || keycode == VM_KEY_CLEAR) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        } else if (keycode == VM_KEY_LEFT_SOFTKEY) {
            scan_lrc_files();
            set_ui_mode(MODE_LRC_PICKER);
            return;
        } else if (keycode == VM_KEY_LEFT) {
            g_app.screensaver_id = (ScreensaverID)((g_app.screensaver_id + SS_COUNT - 1) % SS_COUNT);
            init_screensaver_state();
        } else if (keycode == VM_KEY_RIGHT) {
            g_app.screensaver_id = (ScreensaverID)((g_app.screensaver_id + 1) % SS_COUNT);
            init_screensaver_state();
        } else if (keycode == VM_KEY_UP) {
            g_app.lrc_user_offset_ms += 500;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.lrc_user_offset_ms -= 500;
        } else if (keycode == VM_KEY_OK) {
            if (g_app.play_state == STATE_PLAYING) {
                pause_playback();
            } else if (g_app.play_state == STATE_PAUSED) {
                resume_playback();
            } else if (g_app.track_count > 0) {
                start_playback(g_app.selected_track, 0);
            }
        } else if (keycode == VM_KEY_STAR && g_app.track_count > 0) {
            int prev = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
            prev = (prev + g_app.track_count - 1) % g_app.track_count;
            g_app.selected_track = prev;
            start_playback(prev, 0);
        } else if (keycode == VM_KEY_POUND && g_app.track_count > 0) {
            int next = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
            next = (next + 1) % g_app.track_count;
            g_app.selected_track = next;
            start_playback(next, 0);
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_LRC_PICKER) {
        int total = 2 + g_app.lrc_file_count;
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + total - 1) % total;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % total;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            if (g_app.menu_cursor == 0) {
                int target = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
                auto_load_lrc_for_track(target);
            } else if (g_app.menu_cursor == 1) {
                clear_lrc_data();
                g_app.lrc_track_idx = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
            } else if (g_app.menu_cursor >= 2 && g_app.menu_cursor < total) {
                load_lrc_file(g_app.lrc_files[g_app.menu_cursor - 2]);
                g_app.lrc_track_idx = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
            }
            set_ui_mode(MODE_KARAOKE);
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_KARAOKE);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_DEVICE_SELECT) {
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + DEV_PROFILE_COUNT - 1) % DEV_PROFILE_COUNT;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % DEV_PROFILE_COUNT;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            g_app.cur_device = (AudioDeviceProfile)g_app.menu_cursor;
            set_ui_mode(MODE_TRACKLIST);
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_THEME_SELECT) {
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + THEME_COUNT - 1) % THEME_COUNT;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % THEME_COUNT;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            g_app.cur_theme = g_app.menu_cursor;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_FORMAT_FILTER) {
        VMUINT8 masks[] = {
            FMT_MP3, FMT_MIDI, FMT_IMY, FMT_AAC, FMT_AMR, FMT_AWB, FMT_VM, FMT_WAV
        };
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + 7) % 8;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % 8;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            g_app.enabled_formats ^= masks[g_app.menu_cursor];
            refresh_track_library();
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_SORT_SELECT) {
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + SORT_COUNT - 1) % SORT_COUNT;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % SORT_COUNT;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            g_app.sort_mode = (SortMode)g_app.menu_cursor;
            sort_track_library();
            set_ui_mode(MODE_TRACKLIST);
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_TIME_PICKER) {
        if (keycode == VM_KEY_LEFT) {
            g_app.time_cursor = (g_app.time_cursor + 5) % 6;
        } else if (keycode == VM_KEY_RIGHT) {
            g_app.time_cursor = (g_app.time_cursor + 1) % 6;
        } else if (keycode == VM_KEY_UP) {
            g_app.time_digits[g_app.time_cursor] = (g_app.time_digits[g_app.time_cursor] + 1) % 10;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.time_digits[g_app.time_cursor] = (g_app.time_digits[g_app.time_cursor] + 9) % 10;
        } else if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM9) {
            g_app.time_digits[g_app.time_cursor] = (keycode - VM_KEY_NUM0);
            g_app.time_cursor = (g_app.time_cursor + 1) % 6;
        } else if (keycode == VM_KEY_LEFT_SOFTKEY || keycode == VM_KEY_OK) {
            apply_timestamp_jump();
            if (g_app.ui_mode != MODE_POPUP_ERROR) {
                set_ui_mode(MODE_TRACKLIST);
            }
            return;
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_MENU) {
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % MENU_ITEM_COUNT;
        } else if (keycode == VM_KEY_LEFT && g_app.menu_cursor == 3) {
            if (g_app.volume > 0) {
                g_app.volume--;
                vm_set_volume(g_app.volume);
            }
        } else if (keycode == VM_KEY_RIGHT && g_app.menu_cursor == 3) {
            if (g_app.volume < 6) {
                g_app.volume++;
                vm_set_volume(g_app.volume);
            }
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            switch (g_app.menu_cursor) {
                case 0: // Play / Pause
                    if (g_app.play_state == STATE_PLAYING) {
                        pause_playback();
                        set_ui_mode(MODE_TRACKLIST);
                    } else if (g_app.play_state == STATE_PAUSED) {
                        resume_playback();
                        set_ui_mode(MODE_TRACKLIST);
                    } else {
                        start_playback(g_app.selected_track, 0);
                        if (g_app.ui_mode != MODE_POPUP_ERROR) {
                            set_ui_mode(MODE_TRACKLIST);
                        }
                    }
                    return;
                case 1: // Karaoke Mode
                    {
                        int target = (g_app.active_track_idx >= 0) ? g_app.active_track_idx : g_app.selected_track;
                        if (g_app.lrc_track_idx != target) {
                            auto_load_lrc_for_track(target);
                        }
                        init_screensaver_state();
                        set_ui_mode(MODE_KARAOKE);
                    }
                    return;
                case 2: // Jump Timestamp
                    prepare_time_picker(g_app.current_time_sec);
                    set_ui_mode(MODE_TIME_PICKER);
                    return;
                case 3: // Volume Setting
                    set_ui_mode(MODE_VOLUME_SELECT);
                    return;
                case 4: // Audio Output Device
                    set_ui_mode_with_cursor(MODE_DEVICE_SELECT, (int)g_app.cur_device);
                    return;
                case 5: // Loop Mode Cycle
                    g_app.loop_mode = (LoopMode)((g_app.loop_mode + 1) % LOOP_COUNT);
                    break;
                case 6: // ID3 Tag Mode Toggle
                    g_app.id3_mode = !g_app.id3_mode;
                    if (g_app.id3_mode) {
                        ensure_all_id3_parsed();
                    }
                    g_app.sel_char_offset = 0;
                    g_app.marq_char_offset = 0;
                    break;
                case 7: // Sort By
                    set_ui_mode_with_cursor(MODE_SORT_SELECT, (int)g_app.sort_mode);
                    return;
                case 8: // Scroll on Pause Toggle
                    g_app.scroll_on_pause = !g_app.scroll_on_pause;
                    break;
                case 9: // Show Clock Toggle
                    g_app.show_clock = !g_app.show_clock;
                    break;
                case 10: // Format Filter
                    set_ui_mode(MODE_FORMAT_FILTER);
                    return;
                case 11: // Themes
                    set_ui_mode_with_cursor(MODE_THEME_SELECT, g_app.cur_theme);
                    return;
                case 12: // About
                    set_ui_mode(MODE_POPUP_ABOUT);
                    return;
            }
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_BACK) {
            set_ui_mode(MODE_TRACKLIST);
            return;
        }
        trigger_repaint();
        return;
    }

    // Tracklist Primary Controls
    if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR || keycode == VM_KEY_BACK) {
        set_ui_mode(MODE_POPUP_CONFIRM_EXIT);
        return;
    }

    if (keycode == VM_KEY_LEFT_SOFTKEY) {
        set_ui_mode(MODE_MENU);
        return;
    }

    if (keycode == VM_KEY_LEFT) {
        // Quick toggle ID3 tag mode
        g_app.id3_mode = !g_app.id3_mode;
        if (g_app.id3_mode) ensure_all_id3_parsed();
        g_app.sel_char_offset = 0;
        g_app.marq_char_offset = 0;
        trigger_repaint();
        return;
    }

    if (keycode == VM_KEY_RIGHT) {
        set_ui_mode_with_cursor(MODE_SORT_SELECT, (int)g_app.sort_mode);
        return;
    }

    if (keycode == VM_KEY_VOL_UP) {
        if (g_app.volume < 6) {
            g_app.volume++;
            vm_set_volume(g_app.volume);
            trigger_repaint();
        }
        return;
    }

    if (keycode == VM_KEY_VOL_DOWN) {
        if (g_app.volume > 0) {
            g_app.volume--;
            vm_set_volume(g_app.volume);
            trigger_repaint();
        }
        return;
    }

    if (keycode == VM_KEY_OK) {
        if (g_app.play_state == STATE_PLAYING && g_app.active_track_idx == g_app.selected_track) {
            pause_playback();
        } else if (g_app.play_state == STATE_PAUSED && g_app.active_track_idx == g_app.selected_track) {
            resume_playback();
        } else {
            start_playback(g_app.selected_track, 0);
        }
        trigger_repaint();
        return;
    }

    if (keycode >= VM_KEY_NUM0 && keycode <= VM_KEY_NUM9) {
        if (g_app.numeric_len < 4) {
            g_app.numeric_buf[g_app.numeric_len++] = '0' + (keycode - VM_KEY_NUM0);
            g_app.numeric_buf[g_app.numeric_len] = 0;

            if (g_app.numeric_timer != 0) {
                vm_delete_timer_ex(g_app.numeric_timer);
            }
            g_app.numeric_timer = vm_create_timer_ex(NUMERIC_INPUT_TIMEOUT_MS, numeric_timeout);
            trigger_repaint();
        }
        return;
    }

    int prev_sel = g_app.selected_track;

    if (keycode == VM_KEY_STAR) {
        g_app.selected_track -= 10;
        if (g_app.selected_track < 0) g_app.selected_track = 0;
    } else if (keycode == VM_KEY_POUND) {
        g_app.selected_track += 10;
        if (g_app.selected_track >= g_app.track_count) g_app.selected_track = g_app.track_count - 1;
    }

    if (keycode == VM_KEY_UP) {
        if (g_app.selected_track > 0) g_app.selected_track--;
    } else if (keycode == VM_KEY_DOWN) {
        if (g_app.selected_track < g_app.track_count - 1) g_app.selected_track++;
    }

    if (g_app.selected_track != prev_sel) {
        g_app.sel_char_offset = 0;
    }

    int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
    int body_h = g_app.screen_h - g_app.header_h - bottom_reserved;
    int item_h = g_app.font_h + 4;
    if (item_h <= 0) item_h = 18;
    int visible_items = (body_h > 0) ? (body_h / item_h) : 1;

    if (g_app.selected_track < g_app.scroll_offset) {
        g_app.scroll_offset = g_app.selected_track;
    } else if (g_app.selected_track >= g_app.scroll_offset + visible_items) {
        g_app.scroll_offset = g_app.selected_track - visible_items + 1;
    }
    if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;

    trigger_repaint();
}

// ----------------------------------------------------------------------------
// Pen (Touch Screen) Handling
// ----------------------------------------------------------------------------
static void handle_pen_event(VMINT event, VMINT x, VMINT y) {
    if (event == VM_PEN_EVENT_TAP) {
        if (y >= g_app.screen_h - g_app.footer_h) {
            if (x < g_app.screen_w / 2) {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_LEFT_SOFTKEY);
            } else {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_RIGHT_SOFTKEY);
            }
            return;
        }

        // Tapping the [ID3] button in the header in tracklist mode
        if (g_app.ui_mode == MODE_TRACKLIST && y < g_app.header_h && x >= g_app.screen_w / 2) {
            handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_LEFT);
            return;
        }

        // Touch in scrollable popup lists
        int total = get_active_popup_total_items();
        if (total > 0) {
            PopupListGeom g = calc_popup_list_geom(total, active_popup_has_title());
            int rows_top = g.py + g.title_h + 6;
            int rows_bot = rows_top + g.visible_count * g.row_h;
            if (x >= g.px && x < g.px + g.pw && y >= rows_top && y < rows_bot) {
                int tapped_idx = g_app.menu_scroll + (y - rows_top) / g.row_h;
                if (tapped_idx >= 0 && tapped_idx < total) {
                    if (g_app.menu_cursor == tapped_idx) {
                        handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
                    } else {
                        g_app.menu_cursor = tapped_idx;
                        trigger_repaint();
                    }
                }
            }
            return;
        }

        if (g_app.ui_mode == MODE_VOLUME_SELECT) {
            if (x < g_app.screen_w / 2) {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_LEFT);
            } else {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_RIGHT);
            }
            return;
        }

        if (g_app.ui_mode == MODE_KARAOKE && y >= g_app.header_h && y < g_app.screen_h - g_app.footer_h) {
            if (y < g_app.header_h + g_app.font_h * 2 + 10) {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_RIGHT);
            } else {
                handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
            }
            return;
        }

        int bottom_reserved = g_app.footer_h + (g_app.play_state != STATE_STOPPED ? g_app.marquee_h : 0);
        int item_h = g_app.font_h + 4;
        if (item_h <= 0) item_h = 18;
        
        if (g_app.ui_mode == MODE_TRACKLIST && y >= g_app.header_h && y < g_app.screen_h - bottom_reserved) {
            int tapped_idx = g_app.scroll_offset + ((y - g_app.header_h) / item_h);
            if (tapped_idx < g_app.track_count) {
                if (g_app.selected_track == tapped_idx) {
                    handle_key_event(VM_KEY_EVENT_DOWN, VM_KEY_OK);
                } else {
                    g_app.selected_track = tapped_idx;
                    g_app.sel_char_offset = 0;
                    trigger_repaint();
                }
            }
        }
    }
}
// ----------------------------------------------------------------------------
// Lifecycle Management
// ----------------------------------------------------------------------------
static void handle_sysevt(VMINT message, VMINT param) {
    switch (message) {
        case VM_MSG_CREATE: {
            memset(&g_app, 0, sizeof(AppContext));

            // Relinquish background audio lock
            vm_audio_terminate_background_play();
            vm_audio_stop_all();
            vm_midi_stop_all();

            // Acquire active audio client handle from MED
            g_app.audio_client_handle = vm_audio_register_interrupt_callback(audio_interrupt_callback);

            g_app.screen_w = vm_graphic_get_screen_width();
            g_app.screen_h = vm_graphic_get_screen_height();
            
            vm_graphic_set_font(VM_SMALL_FONT);
            int fh = vm_graphic_get_character_height();
            g_app.font_h = (fh > 0) ? fh : 14;
            
            g_app.header_h = g_app.font_h + 6;
            g_app.footer_h = g_app.font_h + 6;
            g_app.marquee_h = (g_app.font_h + 2) * 2 + 4; // 2-Line Marquee Bar

            g_app.cur_theme = THEME_BASIC_DARK; // Default Dark Theme
            g_app.enabled_formats = FMT_MP3 | FMT_MIDI | FMT_IMY | FMT_AAC | FMT_AMR | FMT_AWB | FMT_VM | FMT_WAV;
            g_app.cur_device = DEV_SPEAKER_BOTH; // Default Both Speakers
            g_app.loop_mode = LOOP_LIST;

            VMINT cur_vol = vm_get_volume();
            g_app.volume = (cur_vol >= 0 && cur_vol <= 6) ? (int)cur_vol : 4;

            g_app.id3_mode = false;
            g_app.sort_mode = SORT_FILENAME;
            g_app.screensaver_id = SS_NONE;
            g_app.lrc_track_idx = -1;
            g_app.show_clock = true;
            g_app.scroll_on_pause = true;
            g_app.ui_mode = MODE_TRACKLIST;
            g_app.active_track_idx = -1;
            g_app.midi_handle = -1;

            g_app.layer_hdl = vm_graphic_create_layer(0, 0, g_app.screen_w, g_app.screen_h, VM_NO_TRANS_COLOR);
            vm_graphic_active_layer(g_app.layer_hdl);

            init_screensaver_state();
            refresh_track_library();

            g_app.engine_timer = vm_create_timer(ENGINE_TIMER_MS, timer_tick);
            trigger_repaint();
            break;
        }
        case VM_MSG_ACTIVE:
            vm_graphic_active_layer(g_app.layer_hdl);
            trigger_repaint();
            break;
        case VM_MSG_PAINT:
            trigger_repaint();
            break;
        case VM_MSG_QUIT:
            stop_playback();
            if (g_app.audio_client_handle > 0) {
                vm_audio_clear_interrupt_callback(g_app.audio_client_handle);
                g_app.audio_client_handle = 0;
            }
            if (g_app.engine_timer != 0) vm_delete_timer(g_app.engine_timer);
            if (g_app.numeric_timer != 0) vm_delete_timer_ex(g_app.numeric_timer);
            if (g_app.layer_hdl >= 0) vm_graphic_delete_layer(g_app.layer_hdl);
            break;
    }
}

// ----------------------------------------------------------------------------
// Entry Point
// ----------------------------------------------------------------------------
void vm_main(void) {
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_key_event);
    vm_reg_pen_callback(handle_pen_event);
}
