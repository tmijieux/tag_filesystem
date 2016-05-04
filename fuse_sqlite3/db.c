
#include <sqlite3.h>

#include "initializer.h"
#include "./sys.h"
#include "./util.h"
#include "./schema.h"
#include "./db.h"
#include "./tag.h"

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
    if (sqlite3_bind_int(                                                \
        STMT,                                                           \
        sqlite3_bind_parameter_index(STMT, ":"NAME),                    \
        TEXT                                                            \
    ) != SQLITE_OK) {print_error("error binding int: %s\n",             \
                                 sqlite3_errmsg(tag_db));exit(1);}

static int db_add_(const char *name, int id,
                   const char *table, const char *prefix)
{
    int ret;
    struct sqlite3_stmt *stmt;
    char *req = xasprintf(
        "INSERT INTO %s(%s_id, %s_name) VALUES(:id, :name)",
        table, prefix, prefix);
    
    ret = sqlite3_prepare_v2(tag_db, req,  -1, &stmt, NULL);
    free(req);
    
    if (ret == SQLITE_ERROR)
        print_error("%s\n", sqlite3_errmsg(tag_db));
    
    DB_BIND_TEXT(stmt, "name", name);
    DB_BIND_INT(stmt, "id", id);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

int db_add_file(const char *name, int id)
{
    return db_add_(name, id, "file", "f");
}

int db_add_tag(const char *name, int id)
{
    return db_add_(name, id, "tag", "t");
}
 
int db_list_all_tags(void *buf, fuse_fill_dir_t filler)
{
    int ret;
    struct sqlite3_stmt *stmt;
    sqlite3_prepare_v2(tag_db, "SELECT t_name FROM tag", -1, &stmt, NULL);

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
    bool first = true;
    void append_string(const char *n, void *t, void *c)
    {
        char *tmp;
        tmp = xasprintf("%s%s'%s'", string, first?"":", ", n);
        free(string);
        string = tmp;
        first = false;
    }
    ht_for_each(selected_tags, append_string, NULL);
    return string;
}

static char *tag_id_list_string(struct hash_table *selected_tags)
{
    char *string = strdup("");
    bool first = true;
    void append_string(const char *n, void *t_, void *c)
    {
        char *tmp;
        struct tag *t = t_;
        tmp = xasprintf("%s%s%d", string, first?"":", ", t->id);
        free(string);
        string = tmp;
        first = false;
    }
    ht_for_each(selected_tags, append_string, NULL);
    return string;
}

static char *build_selected_file_query(
    char *tlist, struct hash_table *selected_tags, int use_id)
{
    char *query;
    query = xasprintf(
        "SELECT distinct f_id FROM tag_file WHERE t_id IN "
        "("
        "    SELECT t_id FROM tag WHERE t_name in (%s)"
        ")", tlist);
    
    if (!use_id) {
        char *tmp = query;
        query = xasprintf(
            "SELECT f_name FROM file WHERE f_id in (%s)", tmp);
        free(tmp);
    }
    return query;
}

char *build_remaining_tags_query(char *tlist, char *selected_files_query)
{
    return xasprintf(
        "SELECT t_name FROM ("
        "SELECT t_name FROM tag WHERE t_id IN"
        "("
        "    SELECT DISTINCT t_id FROM tag_file WHERE f_id IN"
        "    ("
        "        %s"
        "    )"
        ")"
        ") WHERE t_name NOT IN (%s)", selected_files_query, tlist);
}

bool db_file_match_tags(int fid, struct hash_table *selected_tags)
{
    char *tlist = tag_id_list_string(selected_tags);
    char *query = xasprintf(
        "SELECT COUNT(t_id) FROM tag_file WHERE f_id = %d AND t_id IN (%s)",
        fid, tlist);

    free(tlist);
    int ret;
    struct sqlite3_stmt *stmt;
    print_debug("%s\n", query);
    sqlite3_prepare_v2(tag_db, query, -1, &stmt, NULL);
    free(query);
    
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ERROR || ret == SQLITE_MISUSE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
        return false;
    }
    if (ret != SQLITE_ROW)
        return false;
    char *count;
    count = (char*)  sqlite3_column_text(stmt, 0);
    if (!count)
        print_error("%s\n", sqlite3_errmsg(tag_db));

    if (atoi(count) != ht_entry_count(selected_tags))
        return false;
    return true;
}

int db_list_remaining_tags(
    struct hash_table *selected_tags, void *buf, fuse_fill_dir_t filler)
{
    char *query, *query2;
    char *tlist = tag_list_string(selected_tags);
    query = build_selected_file_query(tlist, selected_tags, 1);
    query2 = build_remaining_tags_query(tlist, query);
    print_debug("%s\n", query2);
    free(query);
    free(tlist);
    
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
    sqlite3_prepare_v2(tag_db, "SELECT * FROM tag", -1, &stmt, NULL);

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
        "INSERT INTO tag_file(t_id, f_id) VALUES (:tid, :fid)",
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

int db_untag_file(int t_id, int f_id)
{
    int ret;
    struct sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        tag_db,
        "DELETE FROM tag_file WHERE t_id = :tid AND f_id = :fid",
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


static int db_remove_tag_file(int t_id)
{
    int ret;
    struct sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        tag_db,
        "DELETE FROM tag_file WHERE t_id = :tid",
        -1, &stmt, NULL
    );
    if (ret == SQLITE_ERROR)
        print_error("%s\n", sqlite3_errmsg(tag_db));
    DB_BIND_INT(stmt, "tid", t_id);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}

int db_remove_tag(int t_id)
{
    db_remove_tag_file(t_id);
    int ret;
    struct sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(
        tag_db,
        "DELETE FROM tag WHERE t_id = :tid",
        -1, &stmt, NULL
    );
    if (ret == SQLITE_ERROR)
        print_error("%s\n", sqlite3_errmsg(tag_db));
    DB_BIND_INT(stmt, "tid", t_id);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        print_error("%s\n", sqlite3_errmsg(tag_db));
    }
    sqlite3_finalize(stmt);
    return ret != SQLITE_DONE ? -1 : 0;
}
