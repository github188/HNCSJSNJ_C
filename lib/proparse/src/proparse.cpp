#include <stdio.h>
#include <string.h>
#include "logexport.h"
#include "proparse.h"

bool PP_InputBuffer(BYTE* Buff, int BuffLen)
{
    if(NULL == Buff || 0 >= BuffLen)
    {
        LogD("The input buffer is invalid");
        return false;
    }

    FrameStruct* tmp = NULL;
    FrameStruct* head = NULL;
    FrameStruct* FrameList = NULL;
    PROPARSE_CMD_TYPE cmdType = CT_ERROR;
    CProParse::GetInstance()->SeperateRevData(Buff, BuffLen, &FrameList);
    tmp = FrameList;
    while (NULL != tmp)
    {
        cmdType = CProParse::GetInstance()->ParseSingleFrame(tmp);
        LogD("cmdType is %d", (int)cmdType);
        CProParse::GetInstance()->HandleFrameByCmd(cmdType, tmp);
        tmp = tmp->next;
    }

    tmp = FrameList;
    while(NULL != tmp)
    {
        head = tmp->next;
        delete[] tmp;
        tmp = head;
    }

    tmp = NULL;
    head = NULL;
    FrameList = NULL;
    return true;
}

int PP_FormSndFrame(BYTE* SndBuff, BYTE* DataBuff, int OrgLen, BYTE ConFuncCode)
{
    return CProParse::GetInstance()->FormSndFrame(SndBuff, DataBuff, OrgLen, ConFuncCode);
}

bool PP_RegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type, FrameHandler handler)
{
    return CProParse::GetInstance()->RegisterFrameHandler(cmd_type, handler);
}

bool PP_UnRegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type)
{
    return CProParse::GetInstance()->UnRegisterFrameHandler(cmd_type);
}

bool PP_SeperateRevData(BYTE *Buff, int BuffLen, FrameStruct **Frames)
{
	return CProParse::GetInstance()->SeperateRevData(Buff, BuffLen, Frames);
}

int PP_ParseSingleFrame(FrameStruct *Frames)
{
	return CProParse::GetInstance()->ParseSingleFrame(Frames);
}

int PP_GetCheckSum(const BYTE *buf, const int len)
{
	return CProParse::GetInstance()->GetCheckSum(buf, len);
}

CProParse* CProParse::m_pInstance = NULL;

CProParse::CProParse()
{

}

CProParse::~CProParse()
{

}

CProParse* CProParse::GetInstance()
{
	if (NULL == m_pInstance)
	{
		m_pInstance = new CProParse();
	}

	return m_pInstance;

}

bool CProParse::ReleaseInstance()
{
	if (m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}

	return true;
}

bool CProParse::RegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type, FrameHandler handler)
{
    if (m_MapFunc.find(cmd_type) != m_MapFunc.end())
    {
        m_MapFunc[cmd_type] = handler;
    }
    else
    {
        m_MapFunc.insert(std::make_pair(cmd_type, handler));
    }

    return true;
}

bool CProParse::UnRegisterFrameHandler(PROPARSE_CMD_TYPE cmd_type)
{
    if (m_MapFunc.find(cmd_type) == m_MapFunc.end())
    {
        return true;
    }

    m_MapFunc.erase(cmd_type);
    return true;
}

bool CProParse::SeperateRevData(BYTE* inBuff, int inBuffLen, FrameStruct** Frames)
{
    int index = 0;
    LogD("---------Seperate received data---------");
    char temp[500] = {0};
    if (inBuffLen < 400)
    {
        sprintf(temp, "SeperateRevData: frame data len=%d, data:", inBuffLen);
        for (int i = 0; i < inBuffLen; i++)
        {
            sprintf(temp, "%s %02x", temp, inBuff[i]);
        }

        LogD(temp);
    }

    do
    {
        do
        {
            if((index + 11) > inBuffLen)
            {
                LogD("Frame is too short! Exiting...");
                return false;
            }
            if(inBuff[index] != 0x68 || inBuff[index+7] != 0x68)
            {
                index++;
            }
            else
            {
                break;
            }
        }
        while(1);

        int curFrameLen = inBuff[index + 9] + 12;

        if(index + curFrameLen > inBuffLen)
        {
            LogD("Incomplete frame! Exiting...");
            return false;
        }

        FrameStruct* tmpFrameStruct = new FrameStruct[1];
        if (!tmpFrameStruct)
        {
            LogD("New FrameStruct failed!");
            return false;
        }

        memset(tmpFrameStruct, 0, sizeof(FrameStruct));
        for(int j = 0; j < curFrameLen; j++)
        {
            tmpFrameStruct->Framedata[j] =  inBuff[index + j];
        }

        tmpFrameStruct->FrameLen = curFrameLen;
        tmpFrameStruct->next = *Frames;
        *Frames=tmpFrameStruct;
        index += curFrameLen;
    } while(index < inBuffLen -1);

    LogD("SeperateRevData() done!");
    return true;
}

PROPARSE_CMD_TYPE CProParse::ParseSingleFrame(FrameStruct* Frames)
{
    BYTE* ptr = Frames->Framedata;
    PROPARSE_CMD_TYPE cmdType = CT_ERROR;
    if (*(ptr+Frames->FrameLen - 1) != 0x16)
    {
        LogD("Bad end of frame!");
        return cmdType;
    }

    if(GetCheckSum(ptr, Frames->FrameLen-2) != *(ptr+Frames->FrameLen-2))
    {
        LogD("CS is not matched!");
        return cmdType;
    }

    for(int j = 0; j < Frames->FrameLen; j++)
    {
        if ((j > 9) && (j < (10 + Frames->Framedata[9])))
        {
            Frames->Framedata[j] -= 0x33;
        }
    }
    ptr += 7;

    cmdType = (PROPARSE_CMD_TYPE)(*(++ptr) & 0x1f);
    LogD("ParseSingleFrame cmdType is %d", cmdType);
/*
    switch ((*(++ptr) & 0x1f))
    {
        case CT_READ_PARAM:
            {
                cmdType = CT_READ_PARAM;
            }
            break;
        case CT_WRITE_PARAM:
            {
                cmdType = CT_WRITE_PARAM;
            }
            break;
        default:
            {
                LogD("Function code can't be identified!");
                cmdType = CT_ERROR;
            }
            break;
    }
*/
    LogD("ParseSingleFrame done!");
    return cmdType;
}

int CProParse::FormSndFrame(BYTE* SndBuff, BYTE* OrgBuff, int OrgLen, BYTE FuncCode)
{
    if (OrgLen != 0)
    {
        for (int j = 0; j < OrgLen; j++)
        {
            SndBuff[10 + j] = OrgBuff[j];
            SndBuff[10 + j] += 0x33;
        }
    }

    int i = 1;
    SndBuff[0] = 0x68;
    SndBuff[7] = 0x68;
    SndBuff[i++] = 0;//zone code
    SndBuff[i++] = 0;
    SndBuff[i++] = 0;
    SndBuff[i++] = 0;//terminal address
    SndBuff[i++] = 0;
    SndBuff[i++] = 0;
    SndBuff[++i] = FuncCode;
    SndBuff[++i] = OrgLen;
    i += OrgLen;
    i++;
    SndBuff[i] = GetCheckSum(SndBuff, i);
    SndBuff[++i] = 0x16;

    char temp[500] = {0};
    if (( i + 1 ) < 400)
    {
        sprintf(temp, "FormSndFrame: frame data len=%d, data:", i + 1);
        for (int j=0; j<(i+1); j++)
        {
            sprintf(temp, "%s %02x", temp, SndBuff[j]);
        }

        LogD(temp);
    }

    return (i + 1);
}

int CProParse::GetCheckSum(const BYTE *buf, const int len)
{
    int CSum = 0;

    for(int i = 0; i < len; i++)
    {
        CSum += *(buf + i);
    }

    return (int)CSum & 0xff;
}

bool CProParse::HandleFrameByCmd(PROPARSE_CMD_TYPE cmd, FrameStruct* tmpFrame)
{
    if (!tmpFrame)
    {
        return false;
    }

    if (m_MapFunc.find(cmd) == m_MapFunc.end())
    {
        LogE("CProParse::HandleFrameByCmd can not find handler");
        return false;
    }

    LogD("CProParse::HandleFrameByCmd");
    FrameHandler function = m_MapFunc[cmd];
    return (*(function))(tmpFrame);
}
