#ifndef _LOG_EXPORT_H_
#define _LOG_EXPORT_H

enum LOG_LEVEL {
	LEVEL_DEBUG = 0,
	LEVEL_WARNING = 1,
	LEVEL_ERROR = 2,
	LEVEL_FATAL = 3,
};

#ifdef __cplusplus
extern "C" {
#endif

void SetLogLevel(LOG_LEVEL minLevel);
void Log(LOG_LEVEL level, const char *strFileName, int nLine,
		 const char *strFormat, ...);

#define LogD(strFormat, ...) \
    Log(LEVEL_DEBUG, __FILE__, __LINE__, strFormat, ## __VA_ARGS__)

#define LogW(strFormat, ...) \
    Log(LEVEL_WARNING, __FILE__, __LINE__, strFormat, ## __VA_ARGS__)

#define LogE(strFormat, ...) \
    Log(LEVEL_ERROR, __FILE__, __LINE__, strFormat, ## __VA_ARGS__)

#define LogF(strFormat, ...) \
    Log(LEVEL_FATAL, __FILE__, __LINE__, strFormat, ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif /*_LOG_EXPORT_H_*/
