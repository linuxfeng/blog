#include <android/log.h>

/* logcat日志各个级别*/
/*ANDROID_LOG_UNKNOWN*/
//ANDROID_LOG_DEFAULT 
//ANDROID_LOG_VERBOSE
//ANDROID_LOG_DEBUG
//ANDROID_LOG_INFO
//ANDROID_LOG_WARN
//ANDROID_LOG_ERROR
//ANDROID_LOG_FATAL
/*ANDROID_LOG_SILENT*/


#define  LOG_TAG    "test_exec"
/* 这里只定义三个，可以把上面的所有都用宏定义进行使用*/
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
