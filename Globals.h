// Globals.h

// Included from every module in the project, used for precompiled headers





// STL headers:
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <cassert>
#include <algorithm>

// Windows SDK headers:
#include <Windows.h>
#include <tchar.h>





#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

#define FORMATSTRING(X,Y)

#undef max
#undef min

#define ASSERT assert




typedef std::string AString;
typedef int32_t Int32;
typedef uint8_t Byte;




#include "StringUtils.h"




#define LOG(fmt, ...) OutputDebugStringA(Printf("%s(%d): %s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str())
#define UNUSED(X) ((void)X)




