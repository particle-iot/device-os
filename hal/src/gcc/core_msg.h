
#ifdef __cplusplus
extern "C" {
#endif

extern void core_log(const char* msg, ...);

#define MSG(...) core_log(__VA_ARGS__)

#define NOT_IMPLEMENTED(x) MSG("*** method is not implemented: " #x)


#ifdef __cplusplus
}
#endif
