/*
 * File: file.c :)
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2000 - 2004 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Directory scanning is no longer streamed, but it gets sorted instead!
 * Directory entries on top, files next.
 * Not forked anymore; pthread handled.
 * With new HTML layout.
 */
#include <mgdconfig.h>
#ifdef ENABLE_PTHREADS

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>           /* for tolower */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <math.h>            /* for rint */
#include <errno.h>           /* for errno */

#include "url.h"
#include "io.h"
#include "../misc.h"
#include "../list.h"
#include "../web.h"
#include "interface.h"

#define DEBUG_LEVEL 5
#include "../debug.h"


typedef struct _mSpiderDir {
   gint FD_Write, FD_Read;
   char *dirname;
   DIR *dir;
   gboolean scanned;   /* Flag: Have we scanned it? */
   gint parent_dir;    /* Flag: Have we shown parent directory? */
   char **dlist;       /* List of subdirectories (for sorting) */
   gint dlist_size;
   gint dlist_max;
   gint dlist_idx;
   char **flist;       /* List of files (for sorting) */
   gint flist_size;
   gint flist_max;
   gint flist_idx;

   pthread_t th1;      /* This transfer's thread id. */
} mSpiderDir;

typedef struct {
   gint FD_Write,   /* Where we write */
        FD_Read;    /* Where our clients read */
   gint FD;         /* Our local-file descriptor */
   char *FileName;
   glong FileSize;

   pthread_t th1;      /* This transfer's thread id. */
} mSpiderFile;



/*
 * Local data
 */


/*
 * Forward references
 */
static const char *File_content_type(const char *filename);
static gint File_get_file(const gchar *FileName);
static gint File_get_dir(const gchar *DirName);
static GString *File_dir2html(mSpiderDir *Ddir);
static void File_not_found_msg(mSpiderWeb *web, const char *filename, int fd);
static gchar *File_html_escape(const gchar *str);

/*
 * Close a file descriptor, but handling EINTR
 */
static void File_close(int fd)
{
   gint st;
   while ((st = close(fd)) < 0 && errno == EINTR);
}

/*
 * Escape unsafe characters as html entities.
 * Return value: NULL if there's no need to escape, New string otherwise.
 */
static gchar *File_html_escape(const gchar *str)
{
   static const char *unsafe_chars = "&<>\"'";
   static const char *unsafe_rep[] =
     { "&amp;", "&lt;", "&gt;", "&quot;", "&#39;" };

   gchar *p;
   GString *gstr;
   gint i;

   if ((p = strpbrk(str, unsafe_chars))) {
      gstr = g_string_sized_new(64);

      for (i = 0; str[i]; ++i) {
         if ((p = strchr(unsafe_chars, str[i])))
            g_string_append(gstr, unsafe_rep[p - unsafe_chars]);
         else
            g_string_append_c(gstr, str[i]);
      }
      p = gstr->str;
      g_string_free(gstr, FALSE);
   }
   return p;
}

/*
 * Allocate a mSpiderFile structure, and set working values in it.
 */
static mSpiderFile *File_mspiderfile_new(const char *filename)
{
   gint fds[2], fd;
   struct stat sb;
   mSpiderFile *Dfile;
#ifdef _NOUNIX_
   if ( (fd = open(filename, O_RDONLY,440)) < 0 || pipe(fds) )
#else
   if ( (fd = open(filename, O_RDONLY)) < 0 || pipe(fds) )
#endif 
      return NULL;

   Dfile = g_new(mSpiderFile, 1);
   Dfile->FD_Read  = fds[0];
   Dfile->FD_Write = fds[1];
   Dfile->FD = fd;
   Dfile->FileName = g_strdup(filename);
   Dfile->FileSize = fstat(fd, &sb) ? -1 : (glong) sb.st_size;

   return Dfile;
}

/*
 * Deallocate a mSpiderFile structure.
 */
static void File_mspiderfile_free(mSpiderFile *Dfile)
{
   g_free(Dfile->FileName);
   g_free(Dfile);
}

/*
 * Allocate a mSpiderDir structure, and set safe values in it.
 */
static mSpiderDir *File_mspiderdir_new(char *dirname)
{
   DIR *dir;
   gint fds[2];
   mSpiderDir *Ddir;
   if ( !(dir = opendir(dirname)) || pipe(fds) )
      return NULL;

   Ddir = g_new(mSpiderDir, 1);
   Ddir->dir = dir;
   Ddir->scanned = FALSE;
   Ddir->parent_dir = FALSE;
   Ddir->dirname = g_strdup(dirname);
   Ddir->FD_Read  = fds[0];
   Ddir->FD_Write = fds[1];
   Ddir->dlist = NULL;
   Ddir->dlist_size = 0;
   Ddir->dlist_max = 256;
   Ddir->dlist_idx = 0;
   Ddir->flist = NULL;
   Ddir->flist_size = 0;
   Ddir->flist_max = 256;
   Ddir->flist_idx = 0;

   return Ddir;
}

/*
 * Deallocate a mSpiderDir structure.
 */
static void File_mspiderdir_free(mSpiderDir *Ddir)
{
   g_free(Ddir->dirname);
   g_free(Ddir);
}

/*
 * Read a local file, and send it through a pipe.
 * (This function runs on its own thread)
 */
static void *File_transfer_file(void *data)
{
#define LBUF 16*1024

   char buf[LBUF];
   mSpiderFile *Dfile = data;
   ssize_t nbytes;
   const gchar *ct;

   /* Set this thread to detached state */
   pthread_detach(Dfile->th1);

   /* Content type info: as we may misdetect a lot of files,
    * every unknown type is rendered as "text/plain".
    * todo: a better approach could be to detect&reject those types we know
    * for sure we don't handle (as gzip, bzip, ELF, etc)
    */
   ct = File_content_type(Dfile->FileName);
   if (!strcmp(ct, "application/octet-stream"))
      ct = "text/plain";

   /* Send content type info */
   g_snprintf(buf, LBUF, "Content-Type: %s\n", ct);
   write(Dfile->FD_Write, buf, strlen(buf));

   /* Send File Size info */
   if (Dfile->FileSize != -1) {
      g_snprintf(buf, LBUF, "Content-length: %ld\n", Dfile->FileSize);
      write(Dfile->FD_Write, buf, strlen(buf));
   }
   /* Send end-of-header */
   strcpy(buf, "\n");
   write(Dfile->FD_Write, buf, strlen(buf));


   /* Append raw file contents */
   while ( (nbytes = read(Dfile->FD, buf, LBUF)) != 0 ) {
      write(Dfile->FD_Write, buf, nbytes);
   }

   File_close(Dfile->FD);
   File_close(Dfile->FD_Write);
   File_mspiderfile_free(Dfile);
   return NULL;
}

/*
 * Read a local directory, translate it to html, and send it through a pipe.
 * (This function runs on its own thread)
 */
static void *File_transfer_dir(void *data)
{
   char *s1, *s2, *s3, *Hdirname, *Cdirname, *HCdirname;
   GString *gstr, *gs;
   mSpiderDir *Ddir = data;
   FILE *F_Write;

   /* Set this thread to detached state */
   pthread_detach(Ddir->th1);

   /* With a stream, the buffering speeds up the transfer */
   F_Write = fdopen(Ddir->FD_Write, "w");

   gstr = g_string_sized_new(128);
   /* Send MIME content/type info */
   g_string_sprintf(gstr, "Content-Type: %s\n\n",
                    File_content_type("dir.html"));
   fwrite(gstr->str, gstr->len, 1, F_Write);

   /* Send page title */
   Cdirname =
      (s1 = a_Misc_escape_chars(Ddir->dirname, "%#:' ")) ? s1 : Ddir->dirname;
   HCdirname = (s2 = File_html_escape(Cdirname)) ? s2 : Cdirname;
   Hdirname = (s3 = File_html_escape(Ddir->dirname)) ? s3 : Ddir->dirname;
   g_string_sprintf(gstr, "<HTML>\n<HEAD>\n <BASE href='%s%s'>\n"
                    " <TITLE>%s%s</TITLE>\n</HEAD>\n",
                    "file:", HCdirname,
                    "file:", Hdirname);
   fwrite(gstr->str, gstr->len, 1, F_Write);
   g_string_sprintf(gstr, "<BODY><H1>%s %s</H1>\n<pre>\n",
                    "Directory listing of", Hdirname);
   fwrite(gstr->str, gstr->len, 1, F_Write);
   g_free(s3);
   g_free(s2);
   g_free(s1);

   /* Append formatted directory contents */
   while ( (gs = File_dir2html(Ddir)) ){
      fwrite(gs->str, gs->len, 1, F_Write);
      g_string_free(gs, TRUE);
   }

   /* Close open HTML tags */
   g_string_sprintf(gstr, "\n</pre></BODY></HTML>\n");
   fwrite(gstr->str, gstr->len, 1, F_Write);

   fclose(F_Write);
   File_close(Ddir->FD_Write);
   closedir(Ddir->dir);
   Ddir->dir = NULL;
   File_mspiderdir_free(Ddir);
   g_string_free(gstr, TRUE);
   return NULL;
}

/*
 * Return 1 if the extension matches that of the filename.
 */
static gint File_ext(const char *filename, const char *ext)
{
   char *e;

   if ( !(e = strrchr(filename, '.')) )
      return 0;
   return (g_strcasecmp(ext, ++e) == 0);
}

/*
 * Based on the extension, return the content_type for the file.
 * (if there's no extension, analize the data and try to figure it out)
 */
static const char *File_content_type(const char *filename)
{
   gint fd;
   gchar buf[256];
   const gchar *ct;
   ssize_t buf_size;

   ct = "text/plain";

   if (File_ext(filename, "gif")) {
      ct = "image/gif";
   } else if (File_ext(filename, "jpg") || File_ext(filename, "jpeg")) {
      ct = "image/jpeg";
   } else if (File_ext(filename, "png")) {
      ct = "image/png";
   } else if (File_ext(filename, "html") || File_ext(filename, "htm") ||
              File_ext(filename, "shtml")) {
      ct = "text/html";
   }else if (File_ext(filename, "bmp")) {
      ct = "image/x-ms-bmp";
   } else {
      /* everything failed, let's analize the data... */
#ifdef _NOUNIX_
      if ((fd = open(filename, O_RDONLY,440)) >= 0) {
#else
      if ((fd = open(filename, O_RDONLY)) >= 0) {
#endif
         if ((buf_size = read(fd, buf, 256)) > 12 ) {
            a_Misc_get_content_type_from_data(buf, buf_size, &ct);
         }
         File_close(fd);
      }
   }

   return ct;
}

/*
 * Create a new file connection for 'Url', and asynchronously
 * feed the bytes that come back to the cache.
 * ( Data1 = Requested Url; Data2 = Web structure )
 */
static void File_get(ChainLink *Info, void *Data1, void *Data2)
{
   const gchar *path;
   gchar *filename;
   gint fd;
   struct stat sb;
   const mSpiderUrl *Url = Data1;
   mSpiderWeb *web = Data2;

   path = URL_PATH_(Url);
   if (!path || !strcmp(path, ".") || !strcmp(path, "./"))
      /* if there's no path in the URL, show current directory */
      filename = g_get_current_dir();
   else if (!strcmp(path, "~") || !strcmp(path, "~/"))
      filename = g_strdup(g_get_home_dir());
   else
   {
      filename = a_Url_parse_hex_path(Url);
   }

   if ( stat(filename, &sb) != 0 ) {
      /* stat failed, prepare a file-not-found error. */
      fd = -2;
   } else if (S_ISDIR(sb.st_mode)) {
      /* set up for reading directory */
      fd = File_get_dir(filename);
   } else {
      /* set up for reading a file */
      fd = File_get_file(filename);
   }

   if ( fd < 0 ) {
      File_not_found_msg(web, filename, fd);
   } else {
      /* Tell the cache to start the receiving CCC */
      a_Chain_fcb(OpSend, Info, &fd, NULL);
      /* End the requesting CCC */
      a_Chain_fcb(OpEnd, Info, NULL, NULL);
   }

   g_free(filename);
   return;
}

/*
 * Create a pipe connection for URL 'url', which is a directory,
 * and feed an ordered html listing through it.
 * (The feeding function runs on its own thread)
 */
static gint File_get_dir(const gchar *DirName)
{
   GString *g_dirname;
   mSpiderDir *Ddir;

   /* Let's make sure this directory url has a trailing slash */
   g_dirname = g_string_new(DirName);
   if ( g_dirname->str[g_dirname->len - 1] != '/' )
      g_string_append(g_dirname, "/");

   /* Let's get a structure ready for transfer */
   Ddir = File_mspiderdir_new(g_dirname->str);
   g_string_free(g_dirname, TRUE);
   if ( Ddir ) {
      gint fd = Ddir->FD_Read;
      pthread_create(&Ddir->th1, NULL, File_transfer_dir, Ddir);
      return fd;
   } else
      return -1;
}

/*
 * Create a pipe connection for URL 'url', which is a file,
 * send the MIME content/type through it and then send the file itself.
 * (The feeding function runs on its own thread)
 */
static gint File_get_file(const gchar *FileName)
{
   mSpiderFile *Dfile;
   int ret;

   /* Create a control structure for this transfer */
   Dfile = File_mspiderfile_new(FileName);
   if ( Dfile ) {
      gint fd = Dfile->FD_Read;
      ret = pthread_create(&Dfile->th1, NULL, File_transfer_file, Dfile);
      if (ret != 0)
      {
          printf("pthread_create is failure \n");
          return -1; 
      }
      return fd;
   } else
      return -1;
}

/*
 * Return a HTML-line from file info.
 */
static GString *
 File_info2html(struct stat *SPtr, const char *fname,
                const char *name, const char *dirname, const char *date)
{
   static char *dots = ".. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. ..";
   GString *gstr;
   gint size, ndots;
   char *sizeunits;

#define MAXNAMESIZE 30
   char namebuf[MAXNAMESIZE + 1];
   const char *ref, *Cparent, *HCparent, *Cref, *HCref, *Hname;
   const gchar *cont, *filecont;
   gchar *s1, *s2, *s3;

   if (!name)
      return NULL;

   gstr = g_string_sized_new(128);

   if (strcmp(name, "..") == 0) {
      if ( strcmp(dirname, "/") != 0 ){        /* Not the root dir */
         char *parent = g_strdup(dirname);
         char *p;
         /* cut trailing '/' */
         parent[strlen(parent) - 1] = '\0';
         /* make 'parent' have the parent dir path */
         if ( (p = strrchr(parent, '/')) )
            *(p + 1) = '\0';
         else { /* in case name == ".." */
            g_free(parent);
            parent = g_get_current_dir();
            if ( (p = strrchr(parent, '/')) )
              *p = '\0'; /* already cut trailing '/' */
            if ( (p = strrchr(parent, '/')) )
              *(p + 1) = '\0';
         }

         Cparent = (s1 = a_Misc_escape_chars(parent, "%#:' ")) ? s1: parent;
         HCparent = (s2 = File_html_escape(Cparent)) ? s2 : Cparent;
         g_string_sprintf(gstr, "<a href='file:%s'>%s</a>\n\n\n",
                          HCparent, "Parent directory");
         g_free(s2);
         g_free(s1);
         g_free(parent);
      } else
         g_string_sprintf(gstr, "<br>\n");
      return gstr;
   }

   if (SPtr->st_size <= 9999) {
      size = SPtr->st_size;
      sizeunits = "bytes";
   } else if (SPtr->st_size / 1024 <= 9999) {
      size = (int)(SPtr->st_size / 1024.0 + 0.5);
      sizeunits = "Kb";
   } else {
      size = (int)(SPtr->st_size / 1048576.0 + 0.5);
      sizeunits = "Mb";
   }
   /* we could note if it's a symlink... */
   if S_ISDIR (SPtr->st_mode) {
      cont = "application/directory";
      filecont = "Directory";
   } else if (SPtr->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      cont = "application/executable";
      filecont = "Executable";
   } else {
      filecont = cont = File_content_type(fname);
      if (!strcmp(filecont, "application/octet-stream"))
         filecont = "unknown";
   }

   ref = name;

   if (strlen(name) > MAXNAMESIZE) {
      memcpy(namebuf, name, MAXNAMESIZE - 3);
      strcpy(namebuf + (MAXNAMESIZE - 3), "...");
      name = namebuf;
   }
   ndots = MAXNAMESIZE - strlen(name);
   ndots = MAX(ndots, 0);

   /* escape problematic filenames */
   Cref = (s1 = a_Misc_escape_chars(ref, "%#:' ")) ? s1 : ref;
   HCref = (s2 = File_html_escape(Cref)) ? s2 : Cref;
   Hname = (s3 = File_html_escape(name)) ? s3 : name;
   g_string_sprintf(gstr,
                    "%s<a href='%s'>%s</a> %s%10s %5d %-5s %s\n",
                    S_ISDIR (SPtr->st_mode) ? ">" : " ", HCref, Hname,
                    dots+50-ndots, filecont, size, sizeunits, date);
   g_free(s3);
   g_free(s2);
   g_free(s1);

   return gstr;
}

/*
 * Given a timestamp, create a date string and place it in 'datestr'
 */
static void File_get_datestr(time_t *mtime, char *datestr)
{
   time_t currenttime;
   char *ds;

   time(&currenttime);
   ds = ctime(mtime);
   if (currenttime - *mtime > 15811200) {
      /* over about 6 months old */
      sprintf(datestr, "%6.6s  %4.4s", ds + 4, ds + 20);
   } else {
      /* less than about 6 months old */
      sprintf(datestr, "%6.6s %5.5s", ds + 4, ds + 11);
   }
}

/*
 * Compare two strings
 * This function is used for sorting directories
 */
static gint File_comp(const void *a, const void *b)
{
   return strcmp(*(char **)a, *(char **)b);
}

/*
 * Read directory entries to a list, sort them alphabetically, and return
 * formatted HTML, line by line, one per entry.
 * When there're no more entries, return NULL
 */
static GString *File_dir2html(mSpiderDir *Ddir)
{
   struct dirent *de;
   struct stat sb;
   char *name;
   GString *gs;
   gint *index;

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
   char fname[MAXPATHLEN + 1];
   char datebuf[64];

   if ( !Ddir->scanned ) {
      /* Lets scan every name and sort them */
      while ((de = readdir(Ddir->dir)) != 0) {
         if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;              /* skip "." and ".." */
         sprintf(fname, "%s/%s", Ddir->dirname, de->d_name);

         if (stat(fname, &sb) == -1)
            continue;              /* ignore files we can't stat */

         if ( S_ISDIR(sb.st_mode) ) {
            a_List_add(Ddir->dlist, Ddir->dlist_size, Ddir->dlist_max);
            Ddir->dlist[Ddir->dlist_size] = g_strdup(de->d_name);
            ++Ddir->dlist_size;
         } else {
            a_List_add(Ddir->flist, Ddir->flist_size, Ddir->flist_max);
            Ddir->flist[Ddir->flist_size] = g_strdup(de->d_name);
            ++Ddir->flist_size;
         }
      }
      /* sort the entries */
      qsort(Ddir->dlist, Ddir->dlist_size, sizeof(char *), File_comp);
      qsort(Ddir->flist, Ddir->flist_size, sizeof(char *), File_comp);

      /* Directory scanning done */
      Ddir->scanned = TRUE;
   }

   if ( !Ddir->parent_dir ) {
      name = g_strdup("..");
      index = &Ddir->parent_dir;
   } else if ( Ddir->dlist_idx < Ddir->dlist_size ) {
      /* We still have directory entries */
      name = Ddir->dlist[Ddir->dlist_idx];
      index = &Ddir->dlist_idx;
   } else if ( Ddir->flist_idx < Ddir->flist_size ) {
      /* We still have file entries */
      name = Ddir->flist[Ddir->flist_idx];
      index = &Ddir->flist_idx;
   } else {
      g_free(Ddir->flist);
      g_free(Ddir->dlist);
      return NULL;
   }

   sprintf(fname, "%s/%s", Ddir->dirname, name);
   stat(fname, &sb);
   File_get_datestr(&sb.st_mtime, datebuf);
   gs = File_info2html(&sb, fname, name, Ddir->dirname, datebuf);
   g_free(name);
   ++(*index);
   return gs;
}

/*
 * Give a file-not-found error (an HTML page).
 * Return Value: -1
 */
static void File_not_found_msg(mSpiderWeb *web, const char *filename, int fd)
{
   if ( web->flags & WEB_RootUrl ){
      a_Interface_msg(web->dd, "ERROR: Can't find %s %s",
                         (fd == -2) ? "" : "file", filename);
fprintf (stderr, "file nt found!\n");
   }else
      DEBUG_MSG(4, "Warning: Can't find <%s>\n", filename);
}

/*
 * CCC function for the FILE module
 */
void a_File_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                void *Data1, void *Data2)
{
   a_Chain_debug_msg("a_File_ccc", Op, Branch, Dir);

   if ( Branch == 1 ) {
      /* Start file method */
      if (Dir == BCK) {
         switch (Op) {
         case OpStart:
            /* (Data1 = Url;  Data2 = Web) */
            File_get(Info, Data1, Data2);
            break;
         }
      }
   }
}
#endif /* ENABLE_PTHREADS */
