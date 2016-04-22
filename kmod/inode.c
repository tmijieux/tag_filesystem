#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/parser.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "./tfs_i.h"
#define TFS_SUPER_MAGIC 0xdeadbeef04fa5c9d
#define TAG_FILENAME ".tagfs"
typedef int fuse_req_t;

typdef struct FILE {
	struct file*f;
} FILE;

MODULE_AUTHOR("Thomas Mijieux <thomas.mijieux@hotmail.fr>");
MODULE_DESCRIPTION("a basic tag-driven pseudo filesystem");
MODULE_LICENSE("GPL");

static struct kmem_cache *tfs_inode_cachep;
static const char *realdirpath = NULL;

static void tfs_inode_init_once(void *inode)
{
	inode_init_once(inode);
}

int tfs_init_inode_mem_cache(void)
{
	tfs_inode_cachep = kmem_cache_create(
		"tfs_inode", sizeof(struct tfs_inode), 0,
		SLAB_HWCACHE_ALIGN|SLAB_ACCOUNT, tfs_inode_init_once);
	if (!tfs_inode_cachep)
		return -ENOMEM;
	return 0;
}

static struct inode *tfs_alloc_inode(struct super_block *sb)
{
	struct inode *inode;
	struct tfs_inode *ti;
	
	inode = kmem_cache_alloc(tfs_inode_cachep, GFP_KERNEL);
	if (!inode)
		return NULL;
	
	ti = get_tfs_inode(inode);
	ti->is_directory = 0;
	ti->is_tagfile = 0;
	ti->filename = NULL;
	ti->tags = NULL;
	ti->ntag = 0;
	
	return inode;
}

static void tfs_i_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
	kmem_cache_free(tfs_inode_cachep, inode);
}

static void tfs_destroy_inode(struct inode *inode)
{
	struct tfs_inode *ti = get_tfs_inode(inode);
	kfree(ti->tags);
	call_rcu(&inode->i_rcu, &tfs_i_callback);
}

static const struct super_operations tfs_super_operations = {
	.alloc_inode    = tfs_alloc_inode,
	.destroy_inode  = tfs_destroy_inode,
	.evict_inode	= NULL,
	.write_inode	= NULL,
	.drop_inode	= NULL,
	.remount_fs	= NULL,
	.put_super	= NULL,
	.umount_begin	= NULL,
	.statfs		= NULL,
	.show_options	= NULL,
};

struct dirbuf {
	char *p;
	size_t size;
};

static unsigned long next_gen_number(void)
{
	static unsigned long number = 1;
	if (number == 0)
		++number;
	return number++;
}

static void dirbuf_add(
	fuse_req_t req, struct dirbuf *b, const char *name, ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
	b->p = (char *) realloc(b->p, b->size);
	memset(&stbuf, 0, sizeof stbuf);
	stbuf.st_ino = ino;
	/*
	fuse_add_direntry(
		req, b->p + oldsize, b->size - oldsize, name, &stbuf, b->size);
	*/
}

static int reply_buf_limited(
	fuse_req_t req, const char *buf, size_t bufsize, off_t off, size_t maxsize)
{
	/*
	if (off < bufsize) {
		return fuse_reply_buf(
			req, buf + off, min(bufsize - off, maxsize));
	} else {
		return fuse_reply_buf(req, NULL, 0);
	}
	*/
}

static int file_matches_tags(
	struct file *f, struct hash_table *selected_tags)
{
	/*
	bool match = true;
	// viva el gcc: 
	void check_tags(const char *name, void *tag, void *match_) {
		bool *match = match_;
		struct tag *t = tag;
		if (t == INVALID_TAG || !ht_has_entry(f->tags, t->value))
			*match = false;
	}
	print_debug("selected tags size: %d\n", ht_entry_count(selected_tags));
	ht_for_each(selected_tags, &check_tags, &match);
	return match;
	*/
	return 0;
}

static void tfs_inode_forget(
	fuse_req_t req, ino_t number, unsigned long nlookup)
{
	struct tag_inode *ino;
	print_log("forget ino = %lu, nlookup = %lu\n", number, nlookup);
	ino = inode_get(number);
	if (ino != NULL) {

		ino->nlookup -= nlookup;
		inode_delete_if_noref(ino);
	}
	fuse_reply_none(req);
}

static int getattr_intra(struct stat *stbuf, struct hash_table *selected_tags,
			 const char *realpath, const char *filename)
{
	int res;
	struct file *f = file_get_or_create(filename);

	/* try to stat the actual file */
	if ((res = stat(realpath, stbuf)) < 0 || S_ISDIR(stbuf->st_mode)) {
		LOG("error ::%s :: %s\n", realpath, strerror(errno));
		/* if the file doesn't exist, check if it's a tag
		   directory and stat the main directory instead */
		if (tag_exist(filename) && !ht_has_entry(selected_tags, filename)) {
			res = stat(realdirpath, stbuf);
		} else {
			res = -ENOENT;
		}
	} else {
		/* if the file exist check that it have the selected tags*/
		if (ht_entry_count(selected_tags) > 0) {
			if (!file_matches_tags(f, selected_tags)) {
				res = -ENOENT;
			}
		} else if (!strcmp(filename, TAG_FILENAME)) {
			/* tag file is special and have "almost infinite" size */
			stbuf->st_size = (off_t) LONG_MAX;
		}
	}
	return res;
}

static void tfs_inode_lookup(
	fuse_req_t req, ino_t parentnum, const char *name)
{
	struct stat st = {0};
	struct fuse_entry_param e = {0};
	struct tag_inode *parent, *ino;

	print_log("lookup: %s - parent ino: %d\n", name, parentnum);
	parent = inode_get(parentnum);
	if (!parent) {
		print_debug("lookup BUG?\n");
		fuse_reply_err(req, EINVAL);
		return;
	}

	int res;
	char *realpath = tag_realpath(name);
	res = stat(realpath, &st);
	free(realpath);
	if (res < 0 || S_ISDIR(st.st_mode)) { 
		if (tag_exist(name) && !ht_has_entry(parent->selected_tags, name)) {
			res = stat(realdirpath, &st);
			// TODO get or create the good inode
			ino = create_directory_inode(parent, name);
		} else {
			fuse_reply_err(req, ENOENT);
			return;
		}
	} else {
		struct file *f;
		f = file_get_or_create(name);
		ino = f->ino;
		st.st_ino = ino->number;
		f->ino->st = st;
	
		if (parentnum != 1) {
			if (!file_matches_tags(f, parent->selected_tags)) {
				fuse_reply_err(req, ENOENT);
				return;
			} else if (!strcmp(name, TAG_FILENAME)) {
				st.st_size = (off_t) LONG_MAX;
			}
		}
	}

	st.st_ino = ino->number;
	e.ino = ino->number;
	e.generation = next_gen_number();
	e.attr = st;
	e.attr_timeout = 1.0;
	e.entry_timeout = 1.0;
	++ ino->nlookup;

	fuse_reply_entry(req, &e);
	print_log("lookup OK %s is %lu\n", name, ino->number);
}

static void tfs_inode_open(
	fuse_req_t req, ino_t number, struct fuse_file_info *fi)
{
	struct tag_inode *ino = inode_get(number);
	print_log("open ino = %lu\n", number);

	if (ino != NULL) {
		int fd;
		struct file *f;

		f = file_get_or_create(ino->name);
		fd = file_open(f, fi->flags);
		fi->fh = (uint64_t) fd;
	
		if (fd >= 0) {
			++ ino->nlookup;
			fuse_reply_open(req, fi);
			return;
		}
	}
	fuse_reply_err(req, errno);
}

static void tfs_inode_opendir(
	fuse_req_t req, ino_t number, struct fuse_file_info *fi)
{
	struct tag_inode *ino;
	// TODO file descriptor for directory that recall the parent directory inode
	//  --> enable us to get to right parent inode in readdir for the '..' entry
    
	print_log("opendir ino = %lu\n", number);
	ino = inode_get(number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	if (!ino->is_directory) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	fi->fh = (uint64_t) ino;
	fuse_reply_open(req, fi);
}

static void tfs_inode_releasedir(
	fuse_req_t req, ino_t number, struct fuse_file_info *fi)
{
	fuse_reply_err(req, 0);
}

static void tfs_inode_release(
	fuse_req_t req, ino_t ino, struct fuse_file_info *fi)
{
	int fd = (int) fi->fh;
	fuse_reply_err(req, -file_close(fd));
}

/* get attributes */
static void tfs_inode_getattr(
	fuse_req_t req, ino_t number, struct fuse_file_info *fi)
{
	int res;
	struct stat st = {0};
	struct tag_inode *ino;

	print_log("getattr ino = %lu\n", number);
	ino = inode_get(number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	if (ino->is_directory)
		res = stat(realdirpath, &st);
	else
		res = stat(ino->realpath, &st);
	st.st_ino = ino->number;
	if (res < 0)
		fuse_reply_err(req, errno);
	else {
		if (ino->is_tagfile)
			st.st_size = LONG_MAX;
		fuse_reply_attr(req, &st, 1.0);
	}
	print_log("getattr returning '%s'; ino = %lu\n", strerror(-res), st.st_ino);
}

static void tfs_inode_unlink(
	fuse_req_t req, ino_t parentnum, const char *name)
{
	print_log("rmdir ino = %lu; name = %s\n", parentnum, name);
	if (parentnum == 1) {
		fuse_reply_err(req, EPERM);
		return;
	}

	struct file *f;
	f = file_get(name);
	if (!f) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	struct tag_inode *parent = inode_get(parentnum);
	if (!parent->is_directory
	    || ht_entry_count(parent->selected_tags) > 1) {
		fuse_reply_err(req, EPERM);
		return;
	}
	struct tag *t = tag_get(parent->name);
	if (t == INVALID_TAG) {
		fuse_reply_err(req, EIO);
		return;
	}
    
	untag_file(t, f);
	fuse_reply_err(req, 0);
}

static void tfs_inode_rmdir(
	fuse_req_t req, ino_t parentnum, const char *name)
{
	print_log("rmdir ino = %lu; name = %s\n", parentnum, name);

	if (parentnum != 1) {
		fuse_reply_err(req, EPERM);
		return;
	}
	// parent = inode_get(parentnum);
	struct tag *t = tag_get(name);
	if (!t) {
		if (file_get(name) != NULL)
			fuse_reply_err(req, ENOTDIR);
		else
			fuse_reply_err(req, ENOENT);
		return;
	}
	tag_remove(t);
	fuse_reply_err(req, 0);
}

static void tfs_inode_mkdir(
	fuse_req_t req, ino_t parentnum, const char *name, mode_t mode)
{
	if (parentnum != 1) {
		fuse_reply_err(req, EPERM);
		return;
	}
    
	struct tag *t;
	t = tag_get(name);
	if (INVALID_TAG != t) {
		fuse_reply_err(req, EEXIST);
		return;
	}

	struct tag_inode *ino;
	struct fuse_entry_param e = {0};
	struct stat st = {0};
	
	tag_get_or_create(name);
	ino = create_directory_inode(inode_get(1), name);

	stat(realdirpath, &st);
	st.st_ino = ino->number;
	e.ino = st.st_ino;
	e.generation = next_gen_number();
	e.attr = st;
	e.attr_timeout = 1.0;
	e.entry_timeout = 1.0;
	fuse_reply_entry(req, &e);
}

static void readdir_list_files(
	fuse_req_t req, struct dirbuf *b, struct hash_table *selected_tags)
{
	struct dirent *dirent;

	rewinddir(realdir);
	while ((dirent = readdir(realdir)) != NULL) {
		int res;
		struct stat st;
		// only list files, don't list directories
		if (dirent->d_type == DT_DIR)
			continue;

		// only files whose contents matches tags in path
		char *realpath = append_dir(realdirpath, dirent->d_name);
		res = getattr_intra(&st, selected_tags, realpath, dirent->d_name);
		free(realpath);
		if (res < 0)
			continue;
		dirbuf_add(req, b, dirent->d_name, st.st_ino);
	}
}

static void readdir_list_tags_mode1(
	fuse_req_t req, struct dirbuf *b, struct hash_table *selected_tags)
{
	// afficher les tags pas encore selectionné

	struct list *tagl = tag_list();
	unsigned s = list_size(tagl);

	for (int i = 1; i <= s; ++i) {
		struct tag *t = list_get(tagl, i);
		if (!ht_has_entry(selected_tags, t->value)) {
			dirbuf_add(req, b, t->value, 2);
		}
	}
}

static struct hash_table *readdir_get_selected_files(
	struct hash_table *selected_tags)
{
	struct hash_table *selected_files;
	selected_files = ht_create(0, NULL);
	// 1 get all file for selected tags:
	void each_tag(const char *n, void *tag, void *arg)
	{
		void each_file(const char *n, void *file, void *arg)
		{
			struct file *f = file;
			ht_add_unique_entry(selected_files, f->name, f);
		}
		struct tag *t = tag;
		ht_for_each(t->files, &each_file, NULL);
	}
	ht_for_each(selected_tags, &each_tag, NULL);

	// remove file not matching all the tags
	void each_file(const char *n, void *file, void *arg)
	{
		void each_tag(const char *n, void *tag, void *file)
		{
			struct file *f = file;
			struct tag *t = tag;
			if (!ht_has_entry(f->tags, t->value)) {
				ht_remove_entry(selected_files, f->name);
				print_debug("removing file %s from selected_files\n", f->name);
			}
		}
		ht_for_each(selected_tags, &each_tag, file);
	}
	ht_for_each(selected_files, &each_file, NULL);

	return selected_files;
}

static void readdir_fill_remaining_tags(
	struct tag_inode *parent, struct hash_table *selected_tags,
	struct hash_table *selected_files, fuse_req_t req, struct dirbuf *b)
{
	struct hash_table *remaining_tags;
	remaining_tags = ht_create(0, NULL);
	void each_file(const char *n, void *file, void *arg)
	{
		void each_tag(const char *n, void *tag, void *arg)
		{
			print_debug("tag %s processed\n",
				    ((struct tag*)tag)->value);
			struct tag *t = tag;
			if (!ht_has_entry(selected_tags, t->value) &&
			    !ht_has_entry(remaining_tags, t->value))
			{
				struct tag_inode *ino;
				ino = create_directory_inode(parent, t->value);
				print_debug("tag %s on le garde ino = %lu\n",
					    ((struct tag*)tag)->value, ino->number);
		
				dirbuf_add(req, b, t->value, ino->number);
				ht_add_unique_entry(remaining_tags, t->value, t);
			}
		}
		struct file *f = file;
		ht_for_each(f->tags, &each_tag, NULL);
	}
	ht_for_each(selected_files, &each_file, NULL);
	ht_free(remaining_tags);
}

static void readdir_list_tags_mode2(
	fuse_req_t req, struct tag_inode *parent, struct dirbuf *b,
	struct hash_table *selected_tags)
{
	print_log("readdir mode2\n");
	// recuperer les tags possibles pour les fichiers correspondant au
	// tags selectionnés

	// 1 etape: recuperer les fichiers correspondant au tag selectionnés:
	struct hash_table *selected_files;
	selected_files = readdir_get_selected_files(selected_tags);
	print_debug("sizeoftable = %lu\n", ht_entry_count(selected_files));

    
	// 2 eme etape: afficher tous les tags non selectionnés parmi les tags de
	// ces fichiers:
	readdir_fill_remaining_tags(
		parent, selected_tags, selected_files, req, b);
	ht_free(selected_files);
	// peut etre faudrait il le cacher au lieu de le liberer ici
}


/* list files within directory */
// TODO use opendir and fuse_file_info to remember the inode
static void tfs_inode_readdir(
	fuse_req_t req, ino_t number,
	size_t size, off_t off, struct fuse_file_info *fi)
{
	struct tag_inode *ino;
	ino = (struct tag_inode *)fi->fh;

	print_log("readdir ino = %lu\n", number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	if (!ino->is_directory) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	struct dirbuf b;
	memset(&b, 0, sizeof(b));
	dirbuf_add(req, &b, ".", ino->number);
	dirbuf_add(req, &b, "..", 1);
    
	if (number != 1)
		readdir_list_tags_mode2(req, ino, &b, ino->selected_tags);
	else
		readdir_list_tags_mode1(req, &b, ino->selected_tags);
	readdir_list_files(req, &b, ino->selected_tags);

	reply_buf_limited(req, b.p, b.size, off, size);
	free(b.p);
}

static int tfs_read_tag_file(char *buffer, size_t len, off_t off)
{
	int res = 0;
	FILE *tagfile = tmpfile();
	tag_db_dump(tagfile);
	rewind(tagfile);
	res = fseek(tagfile, off, SEEK_SET);
	if (res < 0) {
		res = -errno;
		goto out;
	}
	res = fread(buffer, 1, len, tagfile);
	if (res < 0) {
		res = -errno;
	} else if (res < len) {
		print_debug("res = %d ;; len = %d\n", res, len);
		for (int i = res; i < len; ++i) {
			buffer[i] = 0;
		}
	}
out:
	fclose(tagfile);
	if (res < 0)
		print_log("read_tag returning '%s'\n", strerror(-res));
	else
		print_log("read_tag returning success (read %d)\n", res);
	return res;
}

/* read the content of the file */
static void tfs_inode_read(
	fuse_req_t req, ino_t number, size_t size, off_t off,
	struct fuse_file_info *fi)
{
	int res = 0;
	struct tag_inode *ino;
	int fd = (int) fi->fh;

	ino = inode_get(number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	char *buffer = malloc(size);
	if (ino->is_tagfile) {
		res = read_tag_file(buffer, size, off);
	} else {
		print_log("read '%s' for %ld bytes starting at offset %ld\n",
			  ino->name, size, off);
		res = file_read(fd, buffer, size, off);
	}
	if (res < 0) {
		print_log("read returning '%s'\n", strerror(-res));
		fuse_reply_err(req, -res);
	} else {
		reply_buf_limited(req, buffer, size, off, size);
		print_log("read returning success (read %d)\n", res);
	}
	free(buffer);
}

static void tfs_inode_write(
	fuse_req_t req, ino_t number, const char *buf,
	size_t size, off_t off, struct fuse_file_info *fi)
{
	struct tag_inode *ino = NULL;
	int fd = (int) fi->fh;

	ino = inode_get(number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	if (ino->is_tagfile) {
		fuse_reply_err(req, EPERM);
		return;
	}
    
	int res = file_write(fd, buf, size, off);
	if (res < 0) {
		fuse_reply_err(req, -res);
		return;
	}
	fuse_reply_write(req, res);
    
	if (res < 0)
		print_log("write returning '%s'\n", strerror(-res));
	else
		print_log("write returning success (wrote %d)\n", res);
}

static void tfs_inode_mknod(fuse_req_t req, ino_t parent,
			    const char *name, mode_t mode, dev_t rdev)
{
	if (parent != 1) {
		fuse_reply_err(req, EPERM);
		return;
	}
	struct tag *t = tag_get(name);
	if (t != INVALID_TAG) {
		fuse_reply_err(req, EEXIST);
		return;
	}

	char *rname = tag_realpath(name);
	int res = mknod(rname, mode, rdev);
	free(rname);
	if (res < 0) {
		fuse_reply_err(req, errno);
		return;
	}

	struct file *f;
	struct fuse_entry_param e = {0};
	f = file_get_or_create(name);

	stat(f->realpath, &f->ino->st);
	f->ino->st.st_ino = f->ino->number;
     
	e.ino = f->ino->number;
	e.generation = next_gen_number();
	e.attr = f->ino->st;
	e.attr_timeout = 1.0;
	e.entry_timeout = 1.0;

	fuse_reply_entry(req, &e);
}

static void tfs_inode_setattr(
	fuse_req_t req, ino_t number, struct stat *attr,
	int to_set, struct fuse_file_info *fi)
{
	struct file *f;
	struct tag_inode *ino = inode_get(number);
	if (!ino || ino->is_directory) {
		fuse_reply_err(req, EPERM);
		return;
	}
	f = file_get_or_create(ino->name);
    
	if (to_set & FUSE_SET_ATTR_MODE) {
		chmod(f->realpath, attr->st_mode);
		f->ino->st.st_mode = attr->st_mode;
	}
	if (to_set & FUSE_SET_ATTR_ATIME) {
		f->ino->st.st_atim = attr->st_atim;
	}
	if (to_set & FUSE_SET_ATTR_MTIME) {
		f->ino->st.st_mtim = attr->st_mtim;
	}
	if (to_set & FUSE_SET_ATTR_ATIME_NOW) {
		f->ino->st.st_atim.tv_nsec = UTIME_NOW;
	}
	if (to_set & FUSE_SET_ATTR_MTIME_NOW) {
		f->ino->st.st_mtim.tv_nsec = UTIME_NOW;
	}
	struct timespec times[2];
	times[0] = f->ino->st.st_atim;
	times[1] = f->ino->st.st_mtim;
	utimensat(AT_FDCWD, f->realpath, times, 0);

	fuse_reply_attr(req, &f->ino->st, 1.0);
}

static int tfs_inode_truncate(const char *user_path, off_t length)
{
	int res;
	char *realpath = tag_realpath(user_path);
	res = truncate(realpath, length);
	free(realpath);
	return res;
}

static int tfs_inode_chmod(const char *user_path, mode_t mode)
{
	int res;
	char *realpath = tag_realpath(user_path);
	res = chmod(realpath, mode);
	free(realpath);
	return res;
}

static int tfs_inode_chown(const char *user_path, uid_t user, gid_t group)
{
	int res;
	char *realpath = tag_realpath(user_path);
	res = chown(realpath, user, group);
	free(realpath);
	return res;
}

static int tfs_inode_utime(const char *user_path, struct utimbuf *times)
{
	int res;
	char *realpath = tag_realpath(user_path);
	res = utime(realpath, times);
	free(realpath);
	return res;
}

static void tfs_inode_link(
	fuse_req_t req, ino_t number, ino_t newparent,
	const char *newname)
{
	struct tag_inode *parent = inode_get(newparent);
	struct tag_inode *fileino = inode_get(number);

	if (fileino->is_directory || strcmp(newname, fileino->name) != 0) {
		print_debug("oldname = %s, newname = %s\n",
			    fileino->name, newname);
		fuse_reply_err(req, EPERM);
		return;
	}

	if (!parent->is_directory) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}
    
	struct tag *t;
	struct file *f;
	struct fuse_entry_param e = {0};

	e.ino = fileino->number;
	e.generation = next_gen_number();
	e.attr = fileino->st;
	e.attr_timeout = 1.0;
	e.entry_timeout = 1.0;

	f = file_get_or_create(fileino->name);
	t = tag_get(parent->name);
	
	tag_file(t, f);
	fuse_reply_entry(req, &e);
}

static void tfs_inode_rename(
	fuse_req_t req, ino_t parent, const char *name,
	ino_t newparent, const char *newname)
{
	if (parent == newparent) {
		fuse_reply_err(req, EPERM);
		return;
	}
	struct tag_inode *op, *np;
	op = inode_get(parent);
	np = inode_get(newparent);

	struct file *f = file_get(name);
	if (op == NULL || np == NULL || f == NULL) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	if (op->number == 1 || np->number == 1 ||
	    op->ntag > 1 || np->ntag > 1) {
		fuse_reply_err(req, EPERM);
		return;
	}
    
	for (int i = 0; i < op->ntag; ++i) {
		struct tag *t = tag_get(op->tag[i]);
		untag_file(t, f);
	}

	for (int i = 0; i < np->ntag; ++i) {
		struct tag *t = tag_get(np->tag[i]);
		tag_file(t, f);
	}

	fuse_reply_err(req, 0);
}

static void tfs_ioctl_read_tags(
	fuse_req_t req, struct file *f, const struct tag_ioctl *data)
{
	char *str;
	int len;

	if (NULL == f) {
		fuse_reply_err(req, ENOENT);
		return ;
	}
        
	struct tag_ioctl *ndata = malloc(sizeof*data);
	memcpy(ndata, data, sizeof*data);
    
	str = file_get_tags_string(f, &len);
	print_debug("file tag string : %s\n", str);    
    
	strncpy(ndata->buf, str, BUFSIZE);
	ndata->buf[BUFSIZE] = '\0';

	print_debug("copied data : %s\n", ndata->buf);
	if (len > BUFSIZE) {
		ndata->size = BUFSIZE;
	} else {
		ndata->size = len;
	}
    
	free(str);
	fuse_reply_ioctl(req, 0, ndata, sizeof*ndata);
	free(ndata);
}

static void tfs_inode_ioctl(
	fuse_req_t req, ino_t number, int cmd, void *arg,
	struct fuse_file_info *fi, unsigned flags,
	const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{
	struct tag_inode *ino;
	struct file *f;
    
	(void) flags;
    
	ino = inode_get(number);
	if (!ino) {
		fuse_reply_err(req, ENOENT);
		return;
	}
    
	if (ino->is_tagfile || ino->is_directory)
		fuse_reply_err(req, EINVAL);
    
	if (flags & FUSE_IOCTL_COMPAT)
		fuse_reply_err(req, ENOSYS);
    
	switch (cmd) {
	case TAGIOC_READ_TAGS:
	    f = file_get_or_create(ino->name);
	    ioctl_read_tags(req, f, in_buf);
	    return;
	    break;
	}
	fuse_reply_err(req, EINVAL);
}

static void tag_init(void *userdata, struct fuse_conn_info *conn)
{
	inode_init();
	realdir_set_root(userdata);
	realdir_add_files();
	realdir_parse_tags();
}

static void tag_exit(void *user_data)
{
	char *newtagfile;
	int len = asprintf(&newtagfile, "%s.new", tagdbpath);
	if (len < 0) {
		perror("asprintf");
		//exit(EXIT_FAILURE);
	}
	FILE *f = fopen(newtagfile, "w");
	if (NULL != f) {
		tag_db_dump(f);
	}
	LOG("%s\n", newtagfile);
	free(newtagfile);
}

static const struct inode_operations tfs_inode_operations = {
	.lookup = tfs_inode_lookup,
	.get_link = NULL,
	.permission = NULL,
	.get_acl = NULL,
	.readlink = NULL,
	.create = NULL,
	.link = tfs_inode_link,
	.unlink = tfs_inode_unlink,
	.symlink = NULL,
	.mkdir = tfs_inode_mkdir,
	.rmdir = tfs_inode_rmdir,
	.mknod = tfs_inode_mknod,
	.rename = tfs_inode_rename,
	.rename2 = tfs_inode_rename,
	.setattr = tfs_inode_setattr,
	.getattr = tfs_inode_getattr,
	.setxattr = NULL,
	.getxattr = NULL,
	.listxattr = NULL,
	.removexattr = NULL,
	.fiemap = NULL,
	.update_time = NULL,
	.atomic_open = NULL,
	.tmpfile = NULL,
	.set_acl = NULL,
};


static int tfs_parse_mount_options(const char *options)
{
	return 0;
}

static int tfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct tree_descr empty_descr = {""};
	return simple_fill_super(sb, TFS_SUPER_MAGIC, &empty_descr);
}

static struct dentry *tfs_mount(
	struct file_system_type *fs_type, int flags,
	const char *dev_name, void *raw_data)
{
	int err;
	printk(KERN_INFO "mounting %s\n", dev_name);
	realdirpath = kstrdup_const(dev_name, GFP_KERNEL);

	err = tfs_parse_mount_options(raw_data);
	if (err)
		return ERR_PTR(err);

	return mount_nodev(fs_type, flags, raw_data, tfs_fill_super);
}

static struct file_system_type tfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "tfs",
	.mount = tfs_mount,
	.kill_sb = kill_litter_super,
};
MODULE_ALIAS_FS("tfs");

static int __init tfs_init(void)
{
	int err;
	err = tfs_init_inode_mem_cache();
	if (err)
		goto out0;
	
	err = register_filesystem(&tfs_fs_type);
	if (err)
		goto out1;
	
	printk(KERN_INFO "tag filesystem 'tfs' starting.\n");
	return 0;

out1:
	kmem_cache_destroy(tfs_inode_cachep);
out0:
	return err;
}
module_init(tfs_init);

static void __exit tfs_exit(void)
{
	unregister_filesystem(&tfs_fs_type);
	printk(KERN_INFO "tag filesystem 'tfs' exiting.\n");
}
module_exit(tfs_exit);
