/* SDL3 audio backend for the Aurora target. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "types.h"
#include "pc_audio_ptr.h"
#include "pc_settings.h"
#include "jaudio_NES/audiothread.h"

#if UINTPTR_MAX > 0xFFFFFFFFu
uintptr_t pc_audio_ptr_base = 0;
#endif

#define PC_AUDIO_SAMPLE_RATE 32000
#define RING_BUF_SAMPLES 32768
#define RING_BUF_MASK (RING_BUF_SAMPLES - 1)
#define AUDIO_PRODUCE_THRESHOLD 4480
#define AUDIO_CALLBACK_CHUNK_SAMPLES 4096

static s16 ring_buffer[RING_BUF_SAMPLES];
static SDL_AtomicInt ring_write_pos;
static SDL_AtomicInt ring_read_pos;
static SDL_AtomicInt audio_thread_running;
static SDL_AudioStream* audio_stream;
static SDL_Thread* audio_producer_thread;

typedef void (*AIDMACallback)(void);
static AIDMACallback ai_dma_callback;
static u32 ai_dsp_sample_rate = PC_AUDIO_SAMPLE_RATE;

int pc_audio_get_buffer_fill(void) {
    return SDL_GetAtomicInt(&ring_write_pos) - SDL_GetAtomicInt(&ring_read_pos);
}

static int pc_audio_producer_func(void* data) {
    (void)data;
    while (SDL_GetAtomicInt(&audio_thread_running)) {
        if (pc_audio_get_buffer_fill() < AUDIO_PRODUCE_THRESHOLD) {
            pc_audio_process_frame();
        } else {
            SDL_Delay(1);
        }
    }
    return 0;
}

void pc_audio_start_producer_thread(void) {
    if (audio_producer_thread) return;
    SDL_SetAtomicInt(&audio_thread_running, 1);
    audio_producer_thread = SDL_CreateThread(pc_audio_producer_func, "AudioProducer", NULL);
    if (audio_producer_thread) {
        printf("[AUDIO] SDL3 producer thread started\n");
    } else {
        printf("[AUDIO] Failed to create producer thread: %s\n", SDL_GetError());
    }
}

static void pc_audio_stream_callback(void* userdata, SDL_AudioStream* stream,
                                     int additional_amount, int total_amount) {
    (void)userdata;
    (void)total_amount;
    s16 output[AUDIO_CALLBACK_CHUNK_SAMPLES];

    while (additional_amount > 0) {
        int wanted = additional_amount / (int)sizeof(s16);
        if (wanted > AUDIO_CALLBACK_CHUNK_SAMPLES) wanted = AUDIO_CALLBACK_CHUNK_SAMPLES;
        wanted &= ~1;
        if (wanted <= 0) break;

        u32 wp = (u32)SDL_GetAtomicInt(&ring_write_pos);
        SDL_MemoryBarrierAcquire();
        u32 rp = (u32)SDL_GetAtomicInt(&ring_read_pos);
        u32 used = wp - rp;
        if (used > RING_BUF_SAMPLES) {
            rp = (wp - RING_BUF_SAMPLES) & ~1u;
            used = wp - rp;
        }

        int available = (int)used & ~1;
        int copied = available < wanted ? available : wanted;
        for (int i = 0; i < copied; ++i) {
            output[i] = ring_buffer[(rp + (u32)i) & RING_BUF_MASK];
        }
        if (copied < wanted) {
            memset(output + copied, 0, (size_t)(wanted - copied) * sizeof(s16));
        }
        SDL_MemoryBarrierRelease();
        SDL_SetAtomicInt(&ring_read_pos, (int)(rp + (u32)copied));

        int bytes = wanted * (int)sizeof(s16);
        if (!SDL_PutAudioStreamData(stream, output, bytes)) break;
        additional_amount -= bytes;
    }
}

void AIInit(u8* stack) {
    (void)stack;
    if (audio_stream) return;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        printf("[AUDIO] Failed to initialize SDL3 audio: %s\n", SDL_GetError());
        return;
    }

    const SDL_AudioSpec wanted = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = PC_AUDIO_SAMPLE_RATE,
    };
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                             &wanted, pc_audio_stream_callback, NULL);
    if (audio_stream) {
        printf("[AUDIO] Opened SDL3 stream: %d Hz, stereo S16\n", PC_AUDIO_SAMPLE_RATE);
    } else {
        printf("[AUDIO] Failed to open SDL3 stream: %s\n", SDL_GetError());
    }
}

void AIInitDMA(uintptr_t addr, u32 size) {
    s16* src = (s16*)addr;
    u32 samples = (size / sizeof(s16)) & ~1u;
    u32 wp = (u32)SDL_GetAtomicInt(&ring_write_pos);
    u32 rp = (u32)SDL_GetAtomicInt(&ring_read_pos);
    SDL_MemoryBarrierAcquire();
    u32 used = wp - rp;
    u32 free_samples = used < RING_BUF_SAMPLES ? RING_BUF_SAMPLES - used : 0;
    if (samples > free_samples) samples = free_samples & ~1u;

    int volume = g_pc_settings.master_volume;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    for (u32 i = 0; i < samples; ++i) {
        int sample = ((int)src[i] * volume) / 100;
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        ring_buffer[(wp + i) & RING_BUF_MASK] = (s16)sample;
    }
    SDL_MemoryBarrierRelease();
    SDL_SetAtomicInt(&ring_write_pos, (int)(wp + samples));
}

void AIStartDMA(void) {
    if (audio_stream) SDL_ResumeAudioStreamDevice(audio_stream);
}

void AIStopDMA(void) {
    if (audio_stream) SDL_PauseAudioStreamDevice(audio_stream);
}

void pc_audio_set_paused(int paused) {
    if (!audio_stream) return;
    if (paused) SDL_PauseAudioStreamDevice(audio_stream);
    else SDL_ResumeAudioStreamDevice(audio_stream);
}

u32 AIGetDMAStartAddr(void) { return 0; }
u16 AIGetDMALength(void) { return 0; }
u32 AIGetStreamTrigger(void) { return 0; }
u32 AIGetStreamSampleCount(void) { return 0; }
void AISetStreamPlayState(u32 state) { (void)state; }
u32 AIGetStreamPlayState(void) { return 0; }
void AISetStreamSampleRate(u32 rate) { (void)rate; }
u32 AIGetStreamSampleRate(void) { return PC_AUDIO_SAMPLE_RATE; }
void AISetStreamVolLeft(u8 volume) { (void)volume; }
void AISetStreamVolRight(u8 volume) { (void)volume; }
u8 AIGetStreamVolLeft(void) { return 0; }
u8 AIGetStreamVolRight(void) { return 0; }
void AIResetStreamSampleCount(void) {}
void AISetDSPSampleRate(u32 rate) { ai_dsp_sample_rate = rate; }
u32 AIGetDSPSampleRate(void) { return ai_dsp_sample_rate; }

void* AIRegisterDMACallback(void* callback) {
    void* previous = (void*)ai_dma_callback;
    ai_dma_callback = (AIDMACallback)callback;
    return previous;
}

void DSPInit(void) {}
BOOL DSPCheckMailToDSP(void) { return FALSE; }
BOOL DSPCheckMailFromDSP(void) { return FALSE; }
u32 DSPReadMailFromDSP(void) { return 0; }
void DSPSendMailToDSP(u32 mail) { (void)mail; }
void DSPAssertInt(void) {}
void* DSPAddTask(void* task) { return task; }

int pc_audio_is_active(void) { return audio_stream != NULL; }

void pc_audio_shutdown(void) {
    SDL_SetAtomicInt(&audio_thread_running, 0);
    if (audio_producer_thread) {
        SDL_WaitThread(audio_producer_thread, NULL);
        audio_producer_thread = NULL;
    }
    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
}
