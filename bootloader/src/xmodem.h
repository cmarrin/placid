#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*InByteFunc)(unsigned short);
typedef void (*OutByteFunc)(int c);
typedef void (*MemCpyFunc)(unsigned char *dst, unsigned char *src, int count);
int xmodemReceive(unsigned int dest, int destsz);
#ifdef __cplusplus
}
#endif
