
#include <sqlite3.h>

#include "initializer.h"
#include "./sys.h"
#include "./util.h"
#include "./schema.h"
#include "./db.h"

static sqlite3 *tag_db;

static int db_query(
    sqlite3 *db, void *callback, void *context, const char *format, ...)
{
    int ret;
    char *errmsg, *query;
    va_list ap;

    va_start(ap, format);
    if (vasprintf(&query, format, ap) < 0) {
        perror("vasprintf");
        exit(EXIT_FAILURE);
    }
    ret = sqlite3_exec(db, query, callback, context, &errmsg);
    va_end(ap);

    if (NULL != errmsg) {
        print_error("%s\nquery: %s\n", errmsg, query);
        sqlite3_free(errmsg);
        free(query);
        exit(EXIT_FAILURE);
    }
    free(query);
    return ret;
}

INITIALIZER(init_db)
{
    sqlite3_open(":memory:", &tag_db);
    db_query(tag_db, NULL, NULL, (const char*)_schema_sql_data);
}
        
#define DB_BIND_TEXT(STMT, NAME, TEXT)                                  \
    if (sqlite3_bind_text(                                              \
        STMT,                                                           \
        sqlite3_bind_parameter_index(STMT, ":"NAME),                    \
        TEXT,                                                           \
        -1,                                                             \
        NULL                                                            \
    ) != SQLITE_OK) {print_error("error binding text: %s\n",            \
                                 sqlite3_errmsg(tag_db));exit(1);}

#define DB_BIND_INT(STMT, NAME, TEXT)                                   \
    if(sqlite3_bind_int(                                                \
        STMT,                                                           \
        sqlite3_bind_parameter_index(STMT, ":"NAME),                    \
        TEXT                                                            \
    ) != SQLITE_OK) {print_error("error binding int: %s\n",              \
                                sqlite3_errmsg(tag_db));exit(1);}

int db_add_file(const char *name)
{
    int ret;
    struct sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        tag_db,
        "insert into file(f_name) VALUES (:name)",
        -1, &stmt, NULL
    );
    if (ret == SQLITE_ERROR)
        print_error("%s\n", sqlite3_errmsg(tag_db));
    
    DB_BIND_TEXT(stmt, "name", name);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

int db_add_tag(const char *name)
{
    int ret;
    struct sqlite3_stmt *stmt;
    sqlite3_prepare_v2(
        tag_db,
        "insert into tag(t_name) VALUES (:name)",
        -1, &stmt, NULL
    );
    DB_BIND_TEXT(stmt, "name", name);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}
 
int db_list_all_tags(void *buf, fuse_fill_dir_t filler)
{
    int ret;
    struct sqlite3_stmt *stmt;
    sqlite3_prepare_v2(tag_db, "select t_name from tag", -1, &stmt, NULL);

    do {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ERROR || ret == SQLITE_MISUSE) {
            print_error("%s\n", sqlite3_errmsg(tag_db));
            break;
        }
        if (ret != SQLITE_ROW)
            break;
        char *tagname;
        tagname = (char*)  sqlite3_column_text(stmt, 0);
        if (!tagname)
            print_error("%s\n", sqlite3_errmsg(tag_db));

        filler(buf, tagname, NULL, 0);
    } while (ret == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

static char *tag_list_string(struct hash_table *selected_tags)
{
    char *string = strdup("");
    void append_string(const char *n, void *t, void *c)
    {
        char *str;
        if (asprintf(&str, "%s %s", string, n) < 0)
            perror("asprintf");
        free(string);
        string = str;
    }
    ht_for_each(selected_tags, append_string, NULL);
    return string;
}

static char *build_selected_file_query(
    struct hash_table *selected_tags, int use_id)
{
    char *query, *tlist, *tmp;
    tlist = tag_list_string(selected_tags);
    if (asprintf(&query, "select t_id from tag where t_name in (%s)", tlist) < 0)
        perror("asprintf");
    free(tlist);
    tmp = query;
    if (asprintf(&query , "select distinct f_id from tag_file where t_id in (%s)",
                 tmp) < 0)
        perror("asprintf");
    free(tmp);

    if (!use_id) {
        tmp = query;
        if (asprintf(&query,
                     "select f_name from file where f_id in (%s)", tmp) < 0)
            perror("asprintf");
        free(tmp);
    }
    return query;
}

char *build_remaining_tags_query(char *selected_files_query)
{
    char *query, *tmp;

    if (asprintf(&query, "select distinct t_id from tag_file where f_id in (%s)",
                 selected_files_query) < 0)
        perror("asprintf");
    tmp = query;
    if (asprintf(&query, "select t_name from tag where t_id in (%s)",
                 tmp) < 0)
        perror("asprintf");
    
    return query;
}

int db_list_remaining_tags(
    struct hash_table *selected_tags, void *buf, fuse_fill_dir_t filler)
{
    char *query, *query2;
    query = build_selected_file_query(selected_tags, 1);
    query2 = build_remaining_tags_query(query);
    free(query);
    
    int ret;
    struct sqlite3_stmt *stmt;
    sqlite3_prepare_v2(tag_db, query2, -1, &stmt, NULL);

    do {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ERROR || ret == SQLITE_MISUSE) {
            print_error("%s\n", sqlite3_errmsg(tag_db));
            break;
        }
        if (ret != SQLITE_ROW)
            break;
        char *tagname;
        tagname = (char*)  sqlite3_column_text(stmt, 0);
        if (!tagname)
            print_error("%s\n", sqlite3_errmsg(tag_db));
        filler(buf, tagname, NULL, 0);
    } while (ret == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

int db_list_selected_files(
    struct hash_table *selected_tags, void *buf, fuse_fill_dir_t filler)
{
    int ret;
    struct sqlite3_stmt *stmt;
    sqlite3_prepare_v2(tag_db, "select * from tag", -1, &stmt, NULL);

    do {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ERROR || ret == SQLITE_MISUSE) {
            print_error("%s\n", sqlite3_errmsg(tag_db));
            break;
        }
        char *tagname;
        tagname = (char*)  sqlite3_column_text(stmt, 2);
        filler(buf, tagname, NULL, 0);
    } while (ret == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

int db_tag_file(int t_id, int f_id)
{
    int ret;
    struct sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        tag_db,
        "insert into tag_file(t_id, f_id) VALUES (:tid, :fid)",
        -1, &stmt, NULL
    );
    if (ret == SQLITE_ERROR)
        print_error("%s\n", sqlite3_errmsg(tag_db));
    
    DB_BIND_INT(stmt, "tid", t_id);
    DB_BIND_INT(stmt, "fid", f_id);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}
