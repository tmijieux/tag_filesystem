#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>

#include "./util.h"
#include "./path.h"

DIR *realdir;
char *realdirpath;
char *tagdbpath;

char *path_realpath(const char *user_path)
{
    char *realpath, *filename;
    /* remove directory names before the actual filename
     * so that grepped file '/foo/bar/toto' becomes 'toto'
     */
    filename = basenamedup(user_path);

    /* prepend dirpath since the daemon runs in '/' */
    realpath = append_dir(realdirpath, filename);
    free(filename);
    return realpath;
}

struct path *path_create(const char *user_path, bool is_tag)
{
    struct path *path = calloc(sizeof *path, 1);

    path->realpath = path_realpath(user_path);
    path->virtpath = strdup(user_path);
    path->virtdirpath = dirnamedup(user_path);
    path->filename = basenamedup(user_path);
    if (is_tag)
        compute_selected_tags(path->virtpath, &path->selected_tags);
    else
        compute_selected_tags(path->virtdirpath, &path->selected_tags);

    return path;
}

void path_delete(struct path *path)
{
    free((char*) path->realpath);
    free((char*) path->virtpath);
    free((char*) path->virtdirpath);
    free((char*) path->filename);
    ht_free(path->selected_tags);
    free(path);
}


void path_free(struct path *path)
{
    free((char*) path->realpath);
    free((char*) path->virtpath);
}

void path_set(struct path *p, const char *virtdirpath,
              const char *filename, struct hash_table *selected_tags)
{
    p->virtdirpath = virtdirpath;
    p->filename = filename;
    p->realpath = append_dir(realdirpath, filename);
    p->virtpath = append_dir(virtdirpath, filename);
    p->selected_tags = selected_tags;
}
