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
#ifndef VM_FORMAT_MP3
#define VM_FORMAT_MP3           4
#endif
#ifndef VM_FORMAT_AAC
#define VM_FORMAT_AAC           5
#endif

#define MAX_TRACKS 128
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
// Audio Formats, Loop Modes & Device Output Definitions
// ----------------------------------------------------------------------------
enum AudioFormatMask {
    FMT_MP3  = 0x01,
    FMT_MIDI = 0x02,
    FMT_AAC  = 0x04,
    FMT_AMR  = 0x08,
    FMT_WAV  = 0x10
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
};

enum PlaybackState {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED
};

enum AppUIMode {
    MODE_TRACKLIST,
    MODE_MENU,
    MODE_DEVICE_SELECT,
    MODE_TIME_PICKER,
    MODE_THEME_SELECT,
    MODE_FORMAT_FILTER,
    MODE_POPUP_CONFIRM_EXIT,
    MODE_POPUP_ABOUT,
    MODE_POPUP_ERROR
};

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
    VMUINT current_time_sec;        // Current playback stored directly in SECONDS
    
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
static void start_playback(int track_index, VMUINT start_time_ms);
static void stop_playback();
static void pause_playback();
static void resume_playback();
static void apply_timestamp_jump();
static void set_ui_mode(AppUIMode mode);
static void trigger_repaint();
static void format_time_string_sec(VMUINT total_s, VMWCHAR* out_str);
static void digital_audio_callback(VMINT result);
static void midi_audio_callback(VMINT handle, VMINT event);
static void audio_interrupt_callback(VMINT handle, VMINT result);

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
        else if (ends_with_ignore_case(info.filefullname, ".aac")) fmt = FMT_AAC;
        else if (ends_with_ignore_case(info.filefullname, ".amr")) fmt = FMT_AMR;
        else if (ends_with_ignore_case(info.filefullname, ".wav")) fmt = FMT_WAV;

        if ((fmt & g_app.enabled_formats) != 0) {
            TrackInfo* t = &g_app.tracks[g_app.track_count];
            vm_wstrncpy(t->filename, info.filefullname, 127);
            t->filename[127] = 0;

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
    strncpy(g_app.error_line1, l1, sizeof(g_app.error_line1) - 1);
    strncpy(g_app.error_line2, l2, sizeof(g_app.error_line2) - 1);
    strncpy(g_app.error_line3, l3, sizeof(g_app.error_line3) - 1);
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

    // ========================================================================
    // PIPELINE 1: MIDI Synthesizer Execution (Vogins Sample Architecture)
    // ========================================================================
    if (t->format == FMT_MIDI) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f < 0) {
            show_audio_error("File Read Error", "vm_file_open failed", "Check card/file name", "");
            return;
        }

        VMUINT file_size = 0;
        vm_file_getfilesize(f, &file_size);
        if (file_size < 14) {
            vm_file_close(f);
            show_audio_error("Corrupt MIDI", "File size is under 14B", "Invalid SMF header", "");
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

        if (g_app.ram_audio_buf[0] != 'M' || g_app.ram_audio_buf[1] != 'T' ||
            g_app.ram_audio_buf[2] != 'h' || g_app.ram_audio_buf[3] != 'd') {
            vm_free(g_app.ram_audio_buf);
            g_app.ram_audio_buf = NULL;
            show_audio_error("Header Error", "Missing 'MThd' marker", "Not a valid MIDI", "");
            return;
        }

        // Query MIDI duration from bytes
        VMINT midi_dur = vm_audio_bytes_duration(g_app.ram_audio_buf, g_app.ram_audio_size, VM_FORMAT_MIDI);
        if (midi_dur > 0) {
            g_app.total_duration_sec = (midi_dur > 100000) ? ((VMUINT)midi_dur / 1000) : (VMUINT)midi_dur;
        } else {
            g_app.total_duration_sec = 0;
        }

        // Play explicitly via selected device
        VMINT ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms, 0, out_dev, midi_audio_callback);
        if (ret < 0 && out_dev != VM_DEVICE_SPEAKER_BOTH) {
            ret = vm_midi_play_by_bytes_ex(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, start_time_ms, 0, VM_DEVICE_SPEAKER_BOTH, midi_audio_callback);
        }
        if (ret < 0) {
            ret = vm_midi_play_by_bytes(g_app.ram_audio_buf, (VMINT)g_app.ram_audio_size, 0, midi_audio_callback);
        }

        if (ret < 0) {
            vm_free(g_app.ram_audio_buf);
            g_app.ram_audio_buf = NULL;
            g_app.ram_audio_size = 0;
            char l1[64], l2[64];
            sprintf(l1, "Output: %s", DEVICE_PROFILE_NAMES[g_app.cur_device]);
            sprintf(l2, "MIDI DSP Code: %d", (int)ret);
            show_audio_error("MIDI Synth Failed", l1, l2, "Synthesizer rejected bytes");
            return;
        }

        g_app.midi_handle = ret;
        vm_set_volume(4);
        g_app.play_state = STATE_PLAYING;
        g_app.active_track_idx = track_index;
        g_app.current_time_sec = start_time_ms / 1000;
        g_app.marq_char_offset = 0;
        return;
    }

    // ========================================================================
    // PIPELINE 2: Digital Audio Stream Execution (MediaTek mmm.c Architecture)
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

    // In-memory fallback if file player is rejected by security sandbox
    if (ret != VM_AUDIO_SUCCEED && t->filesize <= (1024 * 1024 * 2)) {
        VMFILE f = vm_file_open(full_path, MODE_READ, 1);
        if (f >= 0) {
            VMUINT alloc_size = (t->filesize + 7) & ~3;
            g_app.ram_audio_buf = (VMUINT8*)vm_malloc(alloc_size);
            if (g_app.ram_audio_buf) {
                VMUINT nread = 0;
                VMINT actual_b = vm_file_read(f, g_app.ram_audio_buf, t->filesize, &nread);
                g_app.ram_audio_size = (actual_b > 0) ? (VMUINT)actual_b : nread;

                VMUINT8 fmt = (t->format == FMT_AAC) ? VM_FORMAT_AAC : VM_FORMAT_MP3;
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

    vm_set_volume(4);
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
        if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
            TrackInfo* t = &g_app.tracks[g_app.active_track_idx];
            if (t->format == FMT_MIDI) {
                VMUINT mtime = 0;
                if (g_app.midi_handle >= 0 && vm_midi_get_time(g_app.midi_handle, &mtime) == 0) {
                    // vm_midi_get_time returns milliseconds -> convert to seconds
                    g_app.current_time_sec = (mtime > 360000) ? (mtime / 1000) : (mtime / 1000);
                } else {
                    g_app.current_time_sec++;
                }
            } else {
                VMINT cur = vm_audio_get_time();
                if (cur >= 0) {
                    // vm_audio_get_time returns milliseconds -> convert to seconds
                    g_app.current_time_sec = (VMUINT)cur / 1000;
                }
            }
        }
        needs_repaint = true;
    }

    // 2. Character-window scroll shift every 3 ticks (~300 ms)
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

    // 3. System Clock 1-second refresh
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
// High-Speed Text Truncation
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
    vm_wstrncpy(buf, (VMWSTR)str, len);
    buf[len] = 0;

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

    // 1. Header & Live Clock
    vm_graphic_setcolor(&th->header_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, 0, sw, g_app.header_h);
    vm_graphic_setcolor(&th->popup_border);
    vm_graphic_line_ex(g_app.layer_hdl, 0, g_app.header_h - 1, sw - 1, g_app.header_h - 1);

    char title_buf[64];
    const char* header_title = "Audio Player";
    switch (g_app.ui_mode) {
        case MODE_TRACKLIST:
            sprintf(title_buf, "Tracks (%d/%d)", g_app.track_count > 0 ? g_app.selected_track + 1 : 0, g_app.track_count);
            header_title = title_buf;
            break;
        case MODE_MENU:
            header_title = "Options Menu";
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
        case MODE_THEME_SELECT:
            header_title = "Select Theme";
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
    draw_ascii(6, header_text_y, header_title, th->header_text);

    // Top-right Numeric Jump Badge OR Live Clock
    if (g_app.numeric_len > 0) {
        char jump_buf[32];
        sprintf(jump_buf, "#%s_", g_app.numeric_buf);
        VMWCHAR w_jump[32];
        safe_ascii_to_ucs2(w_jump, 32, jump_buf);
        int jw = vm_graphic_get_string_width(w_jump);

        vm_graphic_setcolor(&th->item_selected_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, sw - jw - 10, 2, jw + 6, g_app.header_h - 4);
        draw_text(sw - jw - 7, header_text_y, w_jump, th->item_selected_text);
    } else if (g_app.show_clock) {
        vm_time_t sys_t;
        if (vm_get_time(&sys_t) == 0) {
            char clk_buf[16];
            sprintf(clk_buf, "%02d:%02d", sys_t.hour, sys_t.min);
            VMWCHAR w_clk[16];
            safe_ascii_to_ucs2(w_clk, 16, clk_buf);
            int cw = vm_graphic_get_string_width(w_clk);
            draw_text(sw - cw - 6, header_text_y, w_clk, th->header_text);
        }
    }

    // 2. Tracklist Viewport (Consistent Coloring Applied)
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

            // Selection bar background
            if (is_sel) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, 2, iy, sw - 10, item_h);
            }

            // Fixed prefix: "> 1. ", "|| 1. ", or "  1. "
            char prefix[16];
            if (is_playing) {
                sprintf(prefix, "%s%d. ", (g_app.play_state == STATE_PLAYING ? "> " : "|| "), idx + 1);
            } else {
                sprintf(prefix, "  %d. ", idx + 1);
            }

            VMWCHAR wprefix[16];
            safe_ascii_to_ucs2(wprefix, 16, prefix);
            int prefix_w = vm_graphic_get_string_width(wprefix);

            // CONSISTENT COLORING:
            // If playing, both prefix AND filename are ALWAYS 0F0 (item_playing)
            // If not playing, both are fff (item_selected_text) when selected, or item_text when normal
            vm_graphic_color row_col;
            if (is_playing) {
                row_col = th->item_playing;
            } else if (is_sel) {
                row_col = th->item_selected_text;
            } else {
                row_col = th->item_text;
            }

            // Prefix stays anchored on left
            draw_text(5, iy + 2, wprefix, row_col);

            // Filename portion (scrolls smoothly when selected, NO blank canvas!)
            int name_x = 5 + prefix_w;
            int name_view_w = (sw - 10) - name_x;

            int full_len = vm_wstrlen(g_app.tracks[idx].filename);
            int full_w = vm_graphic_get_string_width(g_app.tracks[idx].filename);

            if (is_sel && full_w > name_view_w) {
                int offset = g_app.sel_char_offset % (full_len + 5);
                VMWCHAR* p = g_app.tracks[idx].filename;
                if (offset < full_len) p += offset;
                draw_text_truncated(name_x, iy + 2, p, name_view_w, row_col);
            } else {
                draw_text_truncated(name_x, iy + 2, g_app.tracks[idx].filename, name_view_w, row_col);
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

        // Line 1: Timestamps + Loop Mode Indicator
        VMWCHAR cur_w[16], tot_w[16], sep1[8];
        format_time_string_sec(g_app.current_time_sec, cur_w);
        format_time_string_sec(g_app.total_duration_sec, tot_w);
        safe_ascii_to_ucs2(sep1, 8, " / ");

        VMWCHAR line1_str[64];
        vm_wstrcpy(line1_str, cur_w);
        vm_wstrcat(line1_str, sep1);
        vm_wstrcat(line1_str, tot_w);
        draw_text(6, my + 2, line1_str, th->marquee_text);

        char loop_tag[16];
        sprintf(loop_tag, "[%s]", LOOP_NAMES[g_app.loop_mode]);
        VMWCHAR wloop[16];
        safe_ascii_to_ucs2(wloop, 16, loop_tag);
        int lw = vm_graphic_get_string_width(wloop);
        draw_text(sw - lw - 6, my + 2, wloop, th->item_playing);

        // Line 2: Scrolling track filename
        if (g_app.active_track_idx >= 0 && g_app.active_track_idx < g_app.track_count) {
            int line2_y = my + fh + 3;
            int max_w = sw - 12;
            int full_len = vm_wstrlen(g_app.tracks[g_app.active_track_idx].filename);
            int full_w = vm_graphic_get_string_width(g_app.tracks[g_app.active_track_idx].filename);

            if (full_w <= max_w) {
                draw_text(6, line2_y, g_app.tracks[g_app.active_track_idx].filename, th->item_playing);
            } else {
                int offset = g_app.marq_char_offset % (full_len + 5);
                VMWCHAR* p = g_app.tracks[g_app.active_track_idx].filename;
                if (offset < full_len) p += offset;
                draw_text_truncated(6, line2_y, p, max_w, th->item_playing);
            }
        }
    }

    // 4. Overlays & Modals
    if (g_app.ui_mode == MODE_MENU) {
        int row_h = fh + 4;
        int mw = sw - 24;
        int mh = row_h * 9 + 14;
        int mx = 12;
        int my = (sh - mh) / 2;
        if (my < 4) my = 4;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, mx, my, mw, mh);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, mx, my, mw, mh);

        char loop_item[32], pause_scroll_item[32], clk_item[32];
        sprintf(loop_item, "Loop Mode: %s", LOOP_NAMES[g_app.loop_mode]);
        sprintf(pause_scroll_item, "Scroll on Pause: %s", g_app.scroll_on_pause ? "On" : "Off");
        sprintf(clk_item, "Show Clock: %s", g_app.show_clock ? "On" : "Off");

        // RESTORED: Jump Timestamp is Option 1 (2nd option!)
        const char* menu_items[] = {
            g_app.play_state == STATE_PLAYING ? "Pause" : "Play",
            "Jump Timestamp",
            "Audio Output",
            loop_item,
            pause_scroll_item,
            clk_item,
            "Format Filter",
            "Themes",
            "About"
        };
        for (int i = 0; i < 9; i++) {
            int iy = my + 7 + i * row_h;
            if (i == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, mx + 3, iy - 1, mw - 6, row_h);
                draw_ascii(mx + 8, iy + 1, menu_items[i], th->item_selected_text);
            } else {
                draw_ascii(mx + 8, iy + 1, menu_items[i], th->item_text);
            }
        }
    } 
    else if (g_app.ui_mode == MODE_DEVICE_SELECT) {
        int row_h = fh + 5;
        int pw = sw - 20;
        int ph = row_h * DEV_PROFILE_COUNT + fh + 20;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 8, py + 5, "Select Output Device:", th->item_text);

        for (int i = 0; i < DEV_PROFILE_COUNT; i++) {
            int iy = py + fh + 10 + i * row_h;
            char dev_line[48];
            sprintf(dev_line, "%s %s", (i == (int)g_app.cur_device ? "*" : " "), DEVICE_PROFILE_NAMES[i]);

            if (i == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, px + 4, iy - 1, pw - 8, row_h);
                draw_ascii(px + 8, iy + 1, dev_line, th->item_selected_text);
            } else {
                draw_ascii(px + 8, iy + 1, dev_line, th->item_text);
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
        int row_h = fh + 5;
        int pw = sw - 20;
        int ph = row_h * 5 + fh + 20;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 8, py + 5, "Supported Formats:", th->item_text);

        const char* fmt_names[] = { "MP3", "MIDI", "AAC", "AMR (Voice)", "WAV" };
        VMUINT8 masks[] = { FMT_MP3, FMT_MIDI, FMT_AAC, FMT_AMR, FMT_WAV };

        for (int i = 0; i < 5; i++) {
            int iy = py + fh + 10 + i * row_h;
            bool active = (g_app.enabled_formats & masks[i]) != 0;
            char box[32];
            sprintf(box, "[%c] %s", active ? 'X' : ' ', fmt_names[i]);

            if (i == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, px + 4, iy - 1, pw - 8, row_h);
                draw_ascii(px + 8, iy + 1, box, th->item_selected_text);
            } else {
                draw_ascii(px + 8, iy + 1, box, th->item_text);
            }
        }
    }
    else if (g_app.ui_mode == MODE_THEME_SELECT) {
        int row_h = fh + 5;
        int pw = sw - 20;
        int ph = row_h * THEME_COUNT + fh + 20;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 8, py + 5, "Select Color Theme:", th->item_text);

        for (int i = 0; i < THEME_COUNT; i++) {
            int iy = py + fh + 10 + i * row_h;
            char theme_line[32];
            sprintf(theme_line, "%s %s", (i == g_app.cur_theme ? "*" : " "), g_themes[i].name);

            if (i == g_app.menu_cursor) {
                vm_graphic_setcolor(&th->item_selected_bg);
                vm_graphic_fill_rect_ex(g_app.layer_hdl, px + 4, iy - 1, pw - 8, row_h);
                draw_ascii(px + 8, iy + 1, theme_line, th->item_selected_text);
            } else {
                draw_ascii(px + 8, iy + 1, theme_line, th->item_text);
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
        int pw = sw - 20;
        int ph = fh * 3 + 24;
        int px = 10;
        int py = (sh - ph) / 2;

        vm_graphic_setcolor(&th->popup_bg);
        vm_graphic_fill_rect_ex(g_app.layer_hdl, px, py, pw, ph);
        vm_graphic_setcolor(&th->popup_border);
        vm_graphic_rect_ex(g_app.layer_hdl, px, py, pw, ph);

        draw_ascii(px + 12, py + 10, "http://stacorp.mw.lt", th->item_playing);
        draw_ascii(px + 12, py + 14 + fh, "Made with Gemini 3.8", th->item_text);
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

        draw_ascii(px + 10, py + 6, g_app.error_title, th->item_playing);
        draw_ascii(px + 10, py + 8 + fh, g_app.error_line1, th->item_text);
        draw_ascii(px + 10, py + 10 + fh * 2, g_app.error_line2, th->item_text);
        draw_ascii(px + 10, py + 12 + fh * 3, g_app.error_line3, th->item_text);
        draw_ascii(px + 10, py + 16 + fh * 4, "[Press OK/RSK to Close]", th->header_text);
    }

    // 5. Softkey Bar: RESTORED to (LSK = Options, RSK = Exit)
    int footer_y = sh - g_app.footer_h;
    vm_graphic_setcolor(&th->softkey_bg);
    vm_graphic_fill_rect_ex(g_app.layer_hdl, 0, footer_y, sw, g_app.footer_h);
    vm_graphic_setcolor(&th->popup_border);
    vm_graphic_line_ex(g_app.layer_hdl, 0, footer_y, sw - 1, footer_y);

    const char* lsk_label = "Options";
    const char* rsk_label = "Exit";

    if (g_app.ui_mode == MODE_MENU || g_app.ui_mode == MODE_DEVICE_SELECT) {
        lsk_label = "Select";
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

static void set_ui_mode(AppUIMode mode) {
    g_app.ui_mode = mode;
    g_app.menu_cursor = 0;
    trigger_repaint();
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
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR) {
            set_ui_mode(MODE_TRACKLIST);
        }
        return;
    }

    // Dismiss Error / About
    if (g_app.ui_mode == MODE_POPUP_ABOUT || g_app.ui_mode == MODE_POPUP_ERROR) {
        if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_OK || 
            keycode == VM_KEY_CLEAR || keycode == VM_KEY_LEFT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
        }
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
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
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
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_FORMAT_FILTER) {
        VMUINT8 masks[] = { FMT_MP3, FMT_MIDI, FMT_AAC, FMT_AMR, FMT_WAV };
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + 4) % 5;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % 5;
        } else if (keycode == VM_KEY_OK || keycode == VM_KEY_LEFT_SOFTKEY) {
            g_app.enabled_formats ^= masks[g_app.menu_cursor];
            refresh_track_library();
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
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
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
        }
        trigger_repaint();
        return;
    }

    if (g_app.ui_mode == MODE_MENU) {
        if (keycode == VM_KEY_UP) {
            g_app.menu_cursor = (g_app.menu_cursor + 8) % 9;
        } else if (keycode == VM_KEY_DOWN) {
            g_app.menu_cursor = (g_app.menu_cursor + 1) % 9;
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
                    break;
                case 1: // Jump Timestamp (RESTORED AS SECOND OPTION)
                    prepare_time_picker(g_app.current_time_sec);
                    set_ui_mode(MODE_TIME_PICKER);
                    break;
                case 2: // Audio Output Device
                    g_app.menu_cursor = (int)g_app.cur_device;
                    set_ui_mode(MODE_DEVICE_SELECT);
                    break;
                case 3: // Loop Mode Cycle
                    g_app.loop_mode = (LoopMode)((g_app.loop_mode + 1) % LOOP_COUNT);
                    break;
                case 4: // Scroll on Pause Toggle
                    g_app.scroll_on_pause = !g_app.scroll_on_pause;
                    break;
                case 5: // Show Clock Toggle
                    g_app.show_clock = !g_app.show_clock;
                    break;
                case 6: // Format Filter
                    set_ui_mode(MODE_FORMAT_FILTER);
                    break;
                case 7: // Themes
                    set_ui_mode(MODE_THEME_SELECT);
                    break;
                case 8: // About
                    set_ui_mode(MODE_POPUP_ABOUT);
                    break;
            }
        } else if (keycode == VM_KEY_RIGHT_SOFTKEY) {
            set_ui_mode(MODE_TRACKLIST);
        }
        trigger_repaint();
        return;
    }

    // Tracklist Primary Controls
    if (keycode == VM_KEY_RIGHT_SOFTKEY || keycode == VM_KEY_CLEAR || keycode == VM_KEY_BACK) {
        // RSK = Exit (Reverted)
        set_ui_mode(MODE_POPUP_CONFIRM_EXIT);
        return;
    }

    if (keycode == VM_KEY_LEFT_SOFTKEY) {
        set_ui_mode(MODE_MENU);
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
            g_app.enabled_formats = FMT_MP3 | FMT_MIDI | FMT_AAC | FMT_AMR | FMT_WAV;
            g_app.cur_device = DEV_SPEAKER_BOTH; // Default Both Speakers
            g_app.loop_mode = LOOP_LIST;
            g_app.show_clock = true;
            g_app.scroll_on_pause = true;
            g_app.ui_mode = MODE_TRACKLIST;
            g_app.active_track_idx = -1;
            g_app.midi_handle = -1;

            g_app.layer_hdl = vm_graphic_create_layer(0, 0, g_app.screen_w, g_app.screen_h, VM_NO_TRANS_COLOR);
            vm_graphic_active_layer(g_app.layer_hdl);

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