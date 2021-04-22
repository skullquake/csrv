#include"./res.h"
#if !defined(_WIN32)||defined(LEADING_UNDERSCORES)
#undef RSRPFX
#define RSRPFX(a) const char*a=_##a
#else
#define RSRPFX(a) a
#endif
#ifdef __cplusplus
extern "C"{
#endif
#if !defined(_WIN32)||defined(LEADING_UNDERSCORES)
RSRPFX(binary_src_res_version_json_size);
RSRPFX(binary_src_res_version_json_start);
RSRPFX(binary_src_res_version_json_end);
#endif
#ifdef __cplusplus
}
#endif
