#define _GNU_SOURCE

#include "types.h"
#include "pc_assets.h"
#include "pc_disc.h"
#include "pc_settings.h"
#include "pc_typing.h"
#include "pc_virtual_input.h"

#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/main.h>

#include <dolphin/pad.h>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_stdinc.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef BELLPAD_IOS
#include "BellpadGameOverlay.h"
#endif

struct SDL_Window* g_pc_window = NULL;
SDL_GLContext g_pc_gl_context = NULL;
int g_pc_running = 1;
int g_pc_no_framelimit = 0;
int g_pc_fast_forward = 0;
int g_pc_verbose = 1;
int g_pc_time_override = -1;
int g_pc_min_override = -1;
int g_pc_sec_override = -1;
int g_pc_weather_override = -1;
int g_pc_weather_intensity_override = 2;
int g_pc_window_w = 640;
int g_pc_window_h = 480;
int g_pc_widescreen_stretch = 0;
uintptr_t pc_image_base = 0;
uintptr_t pc_image_end = 0;

static bool s_aurora_initialized;
static jmp_buf* s_crash_jump;
static volatile uintptr_t s_crash_address;

extern void ac_entry(void);
extern int boot_main(int argc, const char** argv);
extern void pc_audio_shutdown(void);
extern void pc_audio_mq_shutdown(void);
extern void pc_audio_set_paused(int paused);
extern void VISetFrameBufferScale(float scale);

static void bellpad_configure_default_keyboard(void) {
    u32 count = 0;
    if (PADGetKeyButtonBindings(PAD_CHAN0, &count) != NULL) {
        return;
    }

    PADKeyButtonBinding buttons[PAD_BUTTON_COUNT] = {
        {SDL_SCANCODE_SPACE, PAD_BUTTON_A},
        {SDL_SCANCODE_LSHIFT, PAD_BUTTON_B},
        {SDL_SCANCODE_X, PAD_BUTTON_X},
        {SDL_SCANCODE_Y, PAD_BUTTON_Y},
        {SDL_SCANCODE_RETURN, PAD_BUTTON_START},
        {SDL_SCANCODE_Z, PAD_TRIGGER_Z},
        {SDL_SCANCODE_Q, PAD_TRIGGER_L},
        {SDL_SCANCODE_E, PAD_TRIGGER_R},
        {SDL_SCANCODE_I, PAD_BUTTON_UP},
        {SDL_SCANCODE_K, PAD_BUTTON_DOWN},
        {SDL_SCANCODE_J, PAD_BUTTON_LEFT},
        {SDL_SCANCODE_L, PAD_BUTTON_RIGHT},
    };
    PADKeyAxisBinding axes[PAD_AXIS_COUNT] = {
        {SDL_SCANCODE_D, PAD_AXIS_LEFT_X_POS, 100},
        {SDL_SCANCODE_A, PAD_AXIS_LEFT_X_NEG, 100},
        {SDL_SCANCODE_W, PAD_AXIS_LEFT_Y_POS, 100},
        {SDL_SCANCODE_S, PAD_AXIS_LEFT_Y_NEG, 100},
        {SDL_SCANCODE_RIGHT, PAD_AXIS_RIGHT_X_POS, 100},
        {SDL_SCANCODE_LEFT, PAD_AXIS_RIGHT_X_NEG, 100},
        {SDL_SCANCODE_UP, PAD_AXIS_RIGHT_Y_POS, 100},
        {SDL_SCANCODE_DOWN, PAD_AXIS_RIGHT_Y_NEG, 100},
        {SDL_SCANCODE_Q, PAD_AXIS_TRIGGER_L, 100},
        {SDL_SCANCODE_E, PAD_AXIS_TRIGGER_R, 100},
    };

    PADSetKeyButtonBindings(PAD_CHAN0, buttons);
    PADSetKeyAxisBindings(PAD_CHAN0, axes);
    PADSetKeyboardActive(PAD_CHAN0, TRUE);
}

static void bellpad_crash_handler(int signal_number, siginfo_t* info, void* context) {
    (void)context;
    if (s_crash_jump) {
        s_crash_address = (uintptr_t)info->si_addr;
        jmp_buf* destination = s_crash_jump;
        s_crash_jump = NULL;
        longjmp(*destination, 1);
    }
    signal(signal_number, SIG_DFL);
    raise(signal_number);
}

void pc_crash_set_jmpbuf(jmp_buf* destination) {
    static bool installed;
    if (!installed) {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_sigaction = bellpad_crash_handler;
        action.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        installed = true;
    }
    s_crash_jump = destination;
}

uintptr_t pc_crash_get_addr(void) {
    return s_crash_address;
}

uintptr_t pc_crash_get_data_addr(void) {
    return s_crash_address;
}

void pc_gx_restore_after_nes(void) {
}

static void bellpad_log(AuroraLogLevel level, const char* module, const char* message, unsigned int length) {
    FILE* stream = level >= LOG_ERROR ? stderr : stdout;
    fprintf(stream, "[Aurora:%s] %.*s\n", module ? module : "", (int)length, message ? message : "");
    fflush(stream);
    if (level == LOG_FATAL) {
        abort();
    }
}

static void bellpad_find_image_range(void) {
    Dl_info info;
    if (!dladdr((void*)bellpad_find_image_range, &info) || !info.dli_fbase) {
        return;
    }

    pc_image_base = (uintptr_t)info.dli_fbase;
    const struct mach_header_64* header = (const struct mach_header_64*)info.dli_fbase;
    const struct load_command* command = (const struct load_command*)(header + 1);
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    for (u32 index = 0; index < header->ncmds; ++index) {
        if (command->cmd == LC_SEGMENT_64) {
            const struct segment_command_64* segment = (const struct segment_command_64*)command;
            if (strcmp(segment->segname, SEG_PAGEZERO) != 0) {
                uintptr_t end = (uintptr_t)slide + (uintptr_t)segment->vmaddr + (uintptr_t)segment->vmsize;
                if (end > pc_image_end) {
                    pc_image_end = end;
                }
            }
        }
        command = (const struct load_command*)((const u8*)command + command->cmdsize);
    }
}

int pc_platform_poll_events(void) {
    pc_rtc_poll_host_clock();
#ifdef BELLPAD_IOS
    static float applied_framebuffer_scale = -1.0f;
    float requested_framebuffer_scale = bellpad_get_framebuffer_scale();
    if (requested_framebuffer_scale != applied_framebuffer_scale) {
        VISetFrameBufferScale(requested_framebuffer_scale);
        applied_framebuffer_scale = requested_framebuffer_scale;
        if (requested_framebuffer_scale == 0.0f) {
            printf("[Graphics] Render resolution: Native\n");
        } else {
            printf("[Graphics] Render resolution: %.0fx\n", requested_framebuffer_scale);
        }
    }
    pc_typing_update();
    if (g_pc_editor_active && !g_pc_typing_mode) {
        pc_typing_begin();
    }
    bellpad_set_native_text_active(g_pc_typing_mode && g_pc_editor_active);

    char native_text[128];
    int native_command = BELLPAD_NATIVE_TEXT_NONE;
    while (bellpad_poll_native_text_event(native_text, sizeof(native_text),
                                           &native_command)) {
        if (native_text[0]) {
            pc_typing_commit_utf8(native_text);
        } else if (native_command == BELLPAD_NATIVE_TEXT_BACKSPACE) {
            pc_typing_command(PC_TYPING_CMD_BACKSPACE);
        } else if (native_command == BELLPAD_NATIVE_TEXT_ENTER) {
            pc_typing_command(PC_TYPING_CMD_ENTER);
        }
    }
#endif
    pc_pad_poll_normalized_state();
    const AuroraEvent* event = aurora_update();
    while (event && event->type != AURORA_NONE) {
        if (event->type == AURORA_EXIT) {
            g_pc_running = 0;
        } else if (event->type == AURORA_PAUSED) {
            pc_audio_set_paused(1);
        } else if (event->type == AURORA_UNPAUSED) {
            pc_audio_set_paused(0);
        } else if (event->type == AURORA_WINDOW_RESIZED) {
            g_pc_window_w = (int)event->windowSize.native_fb_width;
            g_pc_window_h = (int)event->windowSize.native_fb_height;
            printf("[Graphics] Internal framebuffer %ux%u; drawable %ux%u\n",
                   event->windowSize.fb_width, event->windowSize.fb_height,
                   event->windowSize.native_fb_width, event->windowSize.native_fb_height);
        }
        ++event;
    }
#ifdef BELLPAD_IOS
    if (bellpad_consume_will_resign_active()) {
        pc_audio_set_paused(1);
        printf("[Lifecycle] Audio paused after UIApplicationWillResignActive\n");
    }
    int did_become_active = bellpad_consume_did_become_active();
    int host_clock_changed = bellpad_consume_host_clock_changed();
    if (did_become_active || host_clock_changed) {
        s64 adjustment = pc_rtc_sync_to_host();
        printf("[Lifecycle] RTC synchronized after %s (adjustment %.3f seconds)\n",
               did_become_active ? "UIApplicationDidBecomeActive" : "host clock/time-zone change",
               (double)adjustment / (double)GC_TIMER_CLOCK);
    }
    if (did_become_active) {
        pc_audio_set_paused(0);
        printf("[Lifecycle] Audio resumed after UIApplicationDidBecomeActive\n");
    }
#endif
    return g_pc_running;
}

void pc_platform_update_window_size(void) {
}

void pc_platform_swap_buffers(void) {
}

void pc_platform_shutdown(void) {
    pc_audio_shutdown();
    pc_audio_mq_shutdown();
    pc_disc_shutdown();
    if (s_aurora_initialized) {
        s_aurora_initialized = false;
        aurora_shutdown();
    }
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#ifdef BELLPAD_IOS
    bool explicit_disc_path = false;
#endif
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--disc") == 0 && index + 1 < argc) {
            if (!pc_disc_set_path(argv[++index])) {
                fprintf(stderr, "Disc image path is too long.\n");
                return 2;
            }
#ifdef BELLPAD_IOS
            explicit_disc_path = true;
#endif
        } else if (strcmp(argv[index], "--framelimit") == 0) {
            if (index + 1 >= argc || strcmp(argv[++index], "60") != 0) {
                fprintf(stderr, "Aurora integration is fixed to the native 60 Hz simulation rate.\n");
                return 2;
            }
        } else if (strcmp(argv[index], "--no-framelimit") == 0) {
            fprintf(stderr, "Uncapped simulation is unsupported.\n");
            return 2;
        }
    }

    bellpad_find_image_range();
    const AuroraConfig config = {
        .appName = "Bellpad Aurora",
        .desiredBackend = BACKEND_METAL,
        .vsync = true,
        .pauseOnFocusLost = true,
        .windowWidth = 640,
        .windowHeight = 480,
        .logCallback = bellpad_log,
        .logLevel = LOG_INFO,
        .mem1Size = 256 * 1024 * 1024,
        .mem2Size = 16 * 1024 * 1024,
    };
#ifdef BELLPAD_IOS
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
    AuroraInfo info = aurora_initialize(argc, argv, &config);
    s_aurora_initialized = true;
    g_pc_window = info.window;
    g_pc_window_w = (int)info.windowSize.native_fb_width;
    g_pc_window_h = (int)info.windowSize.native_fb_height;

#ifdef BELLPAD_IOS
    bellpad_install_game_overlay();
#endif

    char* writable_path = SDL_GetPrefPath("Bellpad", "Bellpad");
    if (!writable_path || chdir(writable_path) != 0) {
        fprintf(stderr, "Bellpad could not enter its writable Application Support directory.\n");
        if (writable_path) SDL_free(writable_path);
        pc_platform_shutdown();
        return 1;
    }
    printf("[Storage] Using %s\n", writable_path);
#ifdef BELLPAD_IOS
    if (!explicit_disc_path) {
        char imported_disc_path[1024];
        if (!bellpad_prepare_game_data_path(writable_path, imported_disc_path,
                                             sizeof(imported_disc_path)) ||
            !pc_disc_set_path(imported_disc_path)) {
            fprintf(stderr, "Bellpad could not prepare supported user game data.\n");
            SDL_free(writable_path);
            pc_platform_shutdown();
            return 1;
        }
        printf("[Storage] Using retained game data at %s\n", imported_disc_path);
    }
#endif
    SDL_free(writable_path);

    PADInit();
    bellpad_configure_default_keyboard();

    pc_settings_load();
    if (!pc_disc_init() || !pc_assets_init()) {
        fprintf(stderr, "Bellpad needs a supported user-supplied GAFE01 revision 0 image via --disc PATH.\n");
        pc_platform_shutdown();
        return 1;
    }

    ac_entry();
    boot_main(argc, (const char**)argv);
    pc_platform_shutdown();
    return 0;
}
