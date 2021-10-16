#ifndef hfac1635c9ef211eba845bf30f0db389c
#define hfac1635c9ef211eba845bf30f0db389c
/*
 * scheme of dealing with underscores/no underscores disparity between mingw and regular gcc for embedded resources
 */
#if !defined(_WIN32)||defined(LEADING_UNDERSCORES)
#define RSRPFX(a) extern char _##a[];extern const char*a
#else
#define RSRPFX(a) extern char a[]
#endif
#ifdef __cplusplus
extern "C"{
#endif
RSRPFX(binary_src_res_version_json_size);
RSRPFX(binary_src_res_version_json_start);
RSRPFX(binary_src_res_version_json_end);
#endif
#ifdef __cplusplus
}
#endif
