#ifndef DVDTHREAD_H
#define DVDTHREAD_H

#include "types.h"
#include "dolphin/dvd.h"

#ifdef TARGET_PC
typedef uintptr_t Jac_DVDAddress;
#else
typedef u32 Jac_DVDAddress;
#endif

typedef void (*Jac_DVDCallback)(Jac_DVDAddress);
typedef s32 (*TaskCallback)(void*);
typedef void (*ErrorCallback)(char*, u8* addr);

#ifdef __cplusplus
extern "C" {
#endif

extern void DVDT_ExtendPath(char* dst, char* ext);
extern s32 DVDT_AddTaskHigh(TaskCallback callback, void* stackp, size_t len);
extern s32 DVDT_AddTask(TaskCallback callback, void* stackp, size_t len);
extern void jac_dvdproc_init();
extern void* jac_dvdproc(void*);
extern s32 DVDT_LoadtoARAM_Main(void* arg);
extern s32 DVDT_LoadtoARAM(Jac_DVDAddress owner, char* name, Jac_DVDAddress dst, Jac_DVDAddress src, u32 length,
                           u32* status, Jac_DVDCallback callback);
extern s32 DVDT_ARAMtoDRAM_Main(void* arg);
extern s32 DVDT_DRAMtoARAM_Main(void* arg);
extern s32 DVDT_ARAMtoDRAM(Jac_DVDAddress owner, Jac_DVDAddress dst, Jac_DVDAddress src, u32 length, u32* status,
                           Jac_DVDCallback callback);
extern s32 DVDT_DRAMtoARAM(Jac_DVDAddress owner, Jac_DVDAddress dst, Jac_DVDAddress src, u32 length, u32* status,
                           Jac_DVDCallback callback);
extern s32 DVDT_CheckFile(char* file);
extern s32 DVDT_CheckPass(Jac_DVDAddress owner, u32* status, Jac_DVDCallback callback);
extern s32 Jac_CheckFile(char* file);
extern void Jac_RegisterDVDErrorCallback(ErrorCallback callback);
extern s32 Jac_RegisterFastOpen(char* file);
extern BOOL Jac_DVDOpen(char* name, DVDFileInfo* info);

#ifdef TARGET_PC
void pc_dvd_process_all_tasks(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
