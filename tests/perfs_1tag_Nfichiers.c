#define _GNU_SOURCE 1 /* must stay at the very top */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

/*
 * Ce programme mesures les performances de manipulation d'un seul tag sur plein de fichiers.
 * Il doit être lancé DANS le répertoire racine du montage virtuel avec plein de fichiers existants.
 *
 * Il mesure le temps pour réaliser les différentes opérations sur ces fichiers,
 * pour 1, puis 2, puis ... 2^N ... jusqu'au nombre total de fichiers existants.
 *
 * Il a besoin du support de mkdir, link, readdir, getattr, rename, unlink et rmdir.
 * mkdir/rmdir pourront être enlevés si l'implémentation les crée à la volée.
 */

#define TAG1 "perfs_tmp_tag1"
#define TAG2 "perfs_tmp_tag2"

int main()
{
  unsigned nfiles, i;
  char **filenames;
  struct dirent *dirent;
  DIR *dir;
  struct timeval tv1, tv2;
  unsigned long usecs;
  int err;

  dir = opendir(".");

  nfiles = 0;
  while ((dirent = readdir(dir)) != NULL) {
    struct stat stbuf;
    err = stat(dirent->d_name, &stbuf);
    if (err < 0)
      continue;
    if (S_ISREG(stbuf.st_mode))
      nfiles++;
  }
  printf("Found %u files to tag\n", nfiles);

  filenames = malloc(nfiles*sizeof(*filenames));
  if (!filenames)
    exit(EXIT_FAILURE);

  rewinddir(dir);
  i = 0;
  while ((dirent = readdir(dir)) != NULL) {
    struct stat stbuf;
    err = stat(dirent->d_name, &stbuf);
    if (err < 0)
      continue;
    if (S_ISREG(stbuf.st_mode))
      filenames[i++] = strdup(dirent->d_name);
  }

  closedir(dir);

  err = mkdir(TAG1, S_IRWXU);
  if (err < 0) {
    perror("mkdir");
    exit(EXIT_FAILURE);
  }
  err = mkdir(TAG2, S_IRWXU);
  if (err < 0) {
    perror("mkdir");
    exit(EXIT_FAILURE);
  }

  gettimeofday(&tv1, NULL);
  for(i=0; i<nfiles; i++) {
    char *linkname;
    err = asprintf(&linkname, TAG1 "/%s", filenames[i]);
    assert(err >= 0);
    err = link(filenames[i], linkname);
    if (err < 0) {
      perror("link");
      exit(EXIT_FAILURE);
    }
    free(linkname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for tagging %u files\n", usecs, i+1);
    }
  }
  gettimeofday(&tv2, NULL);
  usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
  printf("%lu usecs for tagging all %u files\n", usecs, nfiles);

  gettimeofday(&tv1, NULL);
  dir = opendir(TAG1);
  assert(dir);
  while ((dirent = readdir(dir)) != NULL) {
    struct stat stbuf;
    err = stat(dirent->d_name, &stbuf);
    if (err < 0)
      perror("stat");
  }
  gettimeofday(&tv2, NULL);
  usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
  printf("%lu usecs for listing and stat'ing all %u files\n", usecs, nfiles);

  gettimeofday(&tv1, NULL);
  for(i=0; i<nfiles; i++) {
    char *oldlinkname, *newlinkname;
    err = asprintf(&oldlinkname, TAG1 "/%s", filenames[(i+nfiles/4)%nfiles /* out of order */]);
    assert(err >= 0);
    err = asprintf(&newlinkname, TAG2 "/%s", filenames[(i+nfiles/4)%nfiles /* out of order */]);
    assert(err >= 0);
    err = rename(oldlinkname, newlinkname);
    if (err < 0) {
      perror("rename");
      exit(EXIT_FAILURE);
    }
    free(oldlinkname);
    free(newlinkname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for replacing tag %u files\n", usecs, i+1);
    }
  }
  gettimeofday(&tv2, NULL);
  usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
  printf("%lu usecs for tagging all %u files\n", usecs, nfiles);

  gettimeofday(&tv1, NULL);
  for(i=0; i<nfiles; i++) {
    char *linkname;
    err = asprintf(&linkname, TAG2 "/%s", filenames[(i+nfiles/2)%nfiles /* out of order */]);
    assert(err >= 0);
    err = unlink(linkname);
    if (err < 0) {
      perror("unlink");
      exit(EXIT_FAILURE);
    }
    free(linkname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for untagging %u files\n", usecs, i+1);
    }
  }
  gettimeofday(&tv2, NULL);
  usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
  printf("%lu usecs for untagging all %u files\n", usecs, nfiles);

  err = rmdir(TAG1);
  if (err < 0) {
    perror("rmdir");
    exit(EXIT_FAILURE);
  }
  err = rmdir(TAG2);
  if (err < 0) {
    perror("rmdir");
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
