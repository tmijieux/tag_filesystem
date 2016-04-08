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
 * Ce programme mesures les performances de manipulation de plein de tags sur un seul fichier.
 * Il doit être lancé DANS le répertoire racine du montage virtuel avec au moins un fichier existant.
 *
 * Il mesure le temps pour réaliser les différentes opérations sur ce fichier,
 * pour 1, puis 2 tag, puis ... 2^N ... jusqu'a TAG_MAX.
 *
 * Il a besoin du support de mkdir, link, unlink et rmdir.
 * mkdir/rmdir pourront être enlevés si l'implémentation les crée à la volée.
 *
 * Lorsque l'opération permettant de lister les tags d'un fichier sera implémentée,
 * elle devra être mesurée selon le même principe.
 */

#define TAGPREFIX "perfs_tmp_tag_"

#define TAG_MAX (64*1024)

/* up to 16^6 = 16 million tags actually supported in tag names */
#define TAG_PRINTF_FORMAT "%c%c%c%c%c%c"
#define TAG_PRINTF_ARGS(n) (((n)>>20)&15)+'a', (((n)>>16)&15)+'a', (((n)>>12)&15)+'a', (((n)>>8)&15)+'a', (((n)>>4)&15)+'a', (((n)>>0)&15)+'a'

int main()
{
  char *filename = NULL;
  struct dirent *dirent;
  DIR *dir;
  unsigned i;
  struct timeval tv1, tv2;
  unsigned long usecs;
  int err;

  dir = opendir(".");

  while ((dirent = readdir(dir)) != NULL) {
    struct stat stbuf;
    err = stat(dirent->d_name, &stbuf);
    if (err < 0)
      continue;
    if (S_ISREG(stbuf.st_mode)) {
      filename = strdup(dirent->d_name);
      break;
    }
  }
  closedir(dir);

  if (!filename) {
    printf("couldn't find any file to tag\n");
    exit(EXIT_FAILURE);
  }
  printf("Will use file %s\n", filename);

  gettimeofday(&tv1, NULL);
  for(i=0; i<TAG_MAX; i++) {
    char *tagname;
    err = asprintf(&tagname, TAGPREFIX TAG_PRINTF_FORMAT, TAG_PRINTF_ARGS(i));
    assert(err >= 0);
    err = mkdir(tagname, S_IRWXU);
    if (err < 0) {
      perror("mkdir");
      exit(EXIT_FAILURE);
    }
    free(tagname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for creating %u tags\n", usecs, i+1);
    }
  }

  gettimeofday(&tv1, NULL);
  for(i=0; i<TAG_MAX; i++) {
    char *linkname;
    err = asprintf(&linkname, TAGPREFIX TAG_PRINTF_FORMAT "/%s", TAG_PRINTF_ARGS((i+TAG_MAX/2)%TAG_MAX /* out of order */), filename);
    assert(err >= 0);
    err = link(filename, linkname);
    if (err < 0) {
      perror("link");
      exit(EXIT_FAILURE);
    }
    free(linkname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for adding %u tags to that file\n", usecs, i+1);
    }
  }

  /* FIXME performance du listage de tags du fichier selon l'API décidée par l'équipe */
  printf("listage des tags du fichier pas implémenté\n");

  gettimeofday(&tv1, NULL);
  for(i=0; i<TAG_MAX; i++) {
    char *linkname;
    err = asprintf(&linkname, TAGPREFIX TAG_PRINTF_FORMAT "/%s", TAG_PRINTF_ARGS((i+TAG_MAX/4)%TAG_MAX /* out of order */), filename);
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
      printf("%lu usecs for removing %u tags to that file\n", usecs, i+1);
    }
  }

  gettimeofday(&tv1, NULL);
  for(i=0; i<TAG_MAX; i++) {
    char *tagname;
    err = asprintf(&tagname, TAGPREFIX TAG_PRINTF_FORMAT, TAG_PRINTF_ARGS((i+TAG_MAX/4*3)%TAG_MAX /* out of order */));
    assert(err >= 0);
    err = rmdir(tagname);
    if (err < 0) {
      perror("rmdir");
      exit(EXIT_FAILURE);
    }
    free(tagname);
    if (!(i & (i+1))) { /* i+1 power of two */
      gettimeofday(&tv2, NULL);
      usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      printf("%lu usecs for destroying %u tags\n", usecs, i+1);
    }
  }

  return EXIT_SUCCESS;
}
