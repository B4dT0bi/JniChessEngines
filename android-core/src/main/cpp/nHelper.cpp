
#include <jni.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * Class:     de_badtobi_jnichessengines_androidCore_AndroidOsHelper
 * Method:    ensureExecutable
 * Signature: (Ljava/lang/String;)Z
 */
extern "C" JNIEXPORT jboolean JNICALL Java_de_badtobi_jnichessengines_androidCore_AndroidOsHelper_ensureExecutable
  (JNIEnv *env, jclass, jstring jFile) {
    const char* file = (*env).GetStringUTFChars(jFile, NULL);
    if (!file)
        return (jboolean) false;
    bool result = chmod(file, 0744) == 0;
    (*env).ReleaseStringUTFChars(jFile, file);
    return (jboolean) result;
}
