#ifndef DATA_H
#define DATA_H



#ifndef PKG_DATA_DIR
#define PKG_DATA_DIR "."
#endif

#ifndef PKG_CONFIG_DIR
#define PKG_CONFIG_DIR "."
#endif


FILE *rz_open_config(const char *path, const char *mode);
FILE *rz_open_resource(const char *path, const char *mode);
char *rz_prefix_path(const char *prefix, const char *path);

#endif //DATA_H
