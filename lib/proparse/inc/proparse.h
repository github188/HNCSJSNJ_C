#ifndef _PROPARSE_H_
#define _PROPARSE_H_

#include <map>
#include "proparseexport.h"

class CProParse
{
public:
    ~CProParse();
    static CProParse *GetInstance(void);
    static bool ReleaseInstance(void);

private:
    CProParse();
    static CProParse *m_pInstance;

public:
    bool SeperateRevData(BYTE *Buff, int BuffLen, FrameStruct **Frames);
    PROPARSE_CMD_TYPE ParseSingleFrame(FrameStruct *Frames);
    int FormSndFrame(BYTE *SndBuff, BYTE *OrgBuff, int OrgLen, BYTE ConFuncCode);
    int GetCheckSum(const BYTE *buf, const int len);

    bool RegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type, FrameHandler handler);
    bool UnRegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type);
    bool HandleFrameByCmd(PROPARSE_CMD_TYPE cmd, FrameStruct *tmpFrame);

private:
    std::map<PROPARSE_CMD_TYPE, FrameHandler>     m_MapFunc;
};

#endif /*_PROPARSE_H_*/