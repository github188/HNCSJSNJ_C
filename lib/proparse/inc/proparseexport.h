#ifndef _PROPARSE_EXPORT_H_
#define _PROPARSE_EXPORT_H_

#include "proparsecmd.h"

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _T_BYTE
#define _T_BYTE
typedef unsigned char   BYTE;
#endif

typedef struct Frame {
  int FrameLen;
  BYTE Framedata[500];
  Frame *next;
} __attribute__((packed)) FrameStruct;
extern FrameStruct *FrameList;

bool PP_InputBuffer(BYTE *Buff, int BuffLen);
int PP_FormSndFrame(BYTE *SndBuff, BYTE *DataBuff, int OrgLen, BYTE ConFuncCode);

bool PP_SeperateRevData(BYTE *Buff, int BuffLen, FrameStruct **Frames);
int PP_ParseSingleFrame(FrameStruct *Frames);
int PP_FormSndFrame(BYTE *SndBuff, BYTE *OrgBuff, int OrgLen, BYTE ConFuncCode);
int PP_GetCheckSum(const BYTE *buf, const int len);

typedef bool (*FrameHandler)(FrameStruct *tmpFrame);
bool PP_RegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type, FrameHandler handler);
bool PP_UnRegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type);

#ifdef __cplusplus
}
#endif

#endif /*_PROPARSE_EXPORT_H_*/