/*
* =====================================================================================
*
*       Filename:  log.h
*
*    Description:
*
*        Version:  1.0
*        Created:  08/13/2014 02:06:07 AM
*       Revision:  none
*       Compiler:  gcc
*
*         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
*        Company:  Mediatek
*
* =====================================================================================
*/

#ifndef _LIVE_WEBCAM_LOG_H
#define _LIVE_WEBCAM_LOG_H

/*
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
*/

#include <stdio.h>
#include <string.h>

#define LOGD(...) LOG(LOG_LEVEL_D, __VA_ARGS__)

#define LOGW(...) LOG(LOG_LEVEL_W, __VA_ARGS__)

#define LOGE(...) LOG(LOG_LEVEL_E, __VA_ARGS__)

#define LOGI(...) LOG(LOG_LEVEL_I, __VA_ARGS__)

#ifdef WIN32
#define __MY_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __MY_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define LOG_MAX_BUFFER_SIZE 4096

#ifdef ENABLE_ANDROID_LOG
#include <android/log.h>
#define LOG(level, x...) do { if (LOG_LEVEL_V == level) break; \
	char buf[LOG_MAX_BUFFER_SIZE]; \
	sprintf(buf, x); \
	__android_log_print(ANDROID_LOG_ERROR, "MY_LOG_TAG", \
	"%s | %s:%i", buf, __MY_FILE__, __LINE__); \
} while (0)
#if 0
#elif defined(_WIN32)
#include <iostream>
#define LOG(level, ...) do { if (LOG_LEVEL_V == level) break; \
	char buf[LOG_MAX_BUFFER_SIZE]; \
	sprintf_s(buf, LOG_MAX_BUFFER_SIZE, __VA_ARGS__); \
	std::cout << buf << std::endl; \
	std::cout.flush(); \
} while (0)
#endif
#else
#define LOG(level, ...) do { if (LOG_LEVEL_V == level) break; \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, " | %s %d", __MY_FILE__, __LINE__); \
	fprintf(stderr, "\n"); fflush(stderr); \
} while (0)
#endif

#define LOG_LEVEL_V 2
#define LOG_LEVEL_D 3
#define LOG_LEVEL_I 4
#define LOG_LEVEL_W 5
#define LOG_LEVEL_E 6

#endif
