#include "log.h"

void SetLogLevel(LOG_LEVEL minLevel)
{
	CLog::GetInstance()->SetLogLevel(minLevel);
}

void Log(LOG_LEVEL level, const char *strFileName, int nLine,
	 const char *strFormat, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, strFormat);
	CLog::GetInstance()->Log(level, strFileName, nLine, strFormat, arg_ptr);
	va_end(arg_ptr);
}

CLog *CLog::m_pInstance = NULL;

CLog::CLog()
{
	m_LogLevel = LEVEL_DEBUG;
}

CLog::~CLog()
{

}

CLog *CLog::GetInstance()
{
	if (NULL == m_pInstance) {
		m_pInstance = new CLog();
	}

	return m_pInstance;

}

bool CLog::ReleaseInstance()
{
	if (m_pInstance) {
		delete m_pInstance;
		m_pInstance = NULL;
	}

	return true;
}

void CLog::SetLogLevel(LOG_LEVEL minLevel)
{
	m_LogLevel = minLevel;
}

void CLog::Log(LOG_LEVEL level, const char *strFileName, int nLine,
	       const char *strFormat, va_list arg_ptr)
{
	if (level < m_LogLevel) {
		return;
	}

	if (NULL == strFormat) {
		return;
	}

	char strInfo[512] = { 0 };
	vsprintf(strInfo, strFormat, arg_ptr);
	char strOutPut[512] = { 0 };
	sprintf(strOutPut, "[%s::%d] %s", strFileName, nLine, strInfo);
	printf("%s\n", strOutPut);
}
