/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*********************************************************************
 *
 * Converts BMP files to Rockbox bitmap format
 *
 * 1999-05-03 Linus Nielsen Feltzing
 *
 **********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define debugf printf

#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif

struct Fileheader
{
  unsigned short Type;          /* signature - 'BM' */
  unsigned  long Size;          /* file size in bytes */
  unsigned short Reserved1;     /* 0 */
  unsigned short Reserved2;     /* 0 */
  unsigned long  OffBits;       /* offset to bitmap */
  unsigned long  StructSize;    /* size of this struct (40) */
  unsigned long  Width;         /* bmap width in pixels */
  unsigned long  Height;        /* bmap height in pixels */
  unsigned short Planes;        /* num planes - always 1 */
  unsigned short BitCount;      /* bits per pixel */
  unsigned long  Compression;   /* compression flag */
  unsigned long  SizeImage;     /* image size in bytes */
  long           XPelsPerMeter; /* horz resolution */
  long           YPelsPerMeter; /* vert resolution */
  unsigned long  ClrUsed;       /* 0 -> color table size */
  unsigned long  ClrImportant;  /* important color count */
} STRUCT_PACKED;

struct RGBQUAD
{
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved;
} STRUCT_PACKED;

#ifdef LITTLE_ENDIAN
#define readshort(x) x
#define readlong(x) x
#else

#define readshort(x) (((x&0xff00)>>8)|((x&0x00ff)<<8))
#define readlong(x) (((x&0xff000000)>>24)| \
                     ((x&0x00ff0000)>>8) | \
                     ((x&0x0000ff00)<<8) | \
                     ((x&0x000000ff)<<24))
#endif

/*********************************************************************
 * read_bmp_file()
 *
 * Reads a 8bit BMP file and puts the data in a 1-pixel-per-byte
 * array. Returns 0 on success.
 *
 *********************************************************************/
int read_bmp_file(char* filename,
                  int *get_width,  /* in pixels */
                  int *get_height, /* in pixels */
                  char **bitmap)
{
   struct Fileheader fh;
   struct RGBQUAD palette[2]; /* two colors only */

   unsigned int bitmap_width, bitmap_height;

   long PaddedWidth;
   int background;
   int fd = open(filename, O_RDONLY);
   long size;
   unsigned int row, col;
   int l;
   unsigned char *bmp;
   int width;

   if(fd == -1)
   {
      debugf("error - can't open '%s'\n", filename);
      return 1;
   }
   else
   {
     if(read(fd, &fh, sizeof(struct Fileheader)) !=
        sizeof(struct Fileheader))
      {
	debugf("error - can't Read Fileheader Stucture\n");
        close(fd);
        return 2;
      }

      /* Exit if not monochrome */
      if(readshort(fh.BitCount) > 8)
      {
	debugf("error - Bitmap must be less than 8, got %d\n",
               readshort(fh.BitCount));
        close(fd);
        return 2;
      }

      /* Exit if too wide */
      if(readlong(fh.Width) > 112)
      {
	debugf("error - Bitmap is too wide (%d pixels, max is 112)\n",
               readlong(fh.Width));
        close(fd);
        return 3;
      }

      /* Exit if too high */
      if(readlong(fh.Height) > 64)
      {
	debugf("error - Bitmap is too high (%d pixels, max is 64)\n",
               readlong(fh.Height));
        close(fd);
	return 4;
      }

      for(l=0;l < 2;l++)
      {
	if(read(fd, &palette[l],sizeof(struct RGBQUAD)) !=
           sizeof(struct RGBQUAD))
	{
          debugf("error - Can't read bitmap's color palette\n");
          close(fd);
          return 5;
	}
      }
      /* pass the other palettes */
      lseek(fd, 254*sizeof(struct RGBQUAD), SEEK_CUR);

      /* Try to guess the foreground and background colors.
         We assume that the foreground color is the darkest. */
      if(((int)palette[0].rgbRed +
          (int)palette[0].rgbGreen +
          (int)palette[0].rgbBlue) >
          ((int)palette[1].rgbRed +
          (int)palette[1].rgbGreen +
          (int)palette[1].rgbBlue))
      {
         background = 0;
      }
      else
      {
         background = 1;
      }

      width = readlong(fh.Width);
      PaddedWidth = ((width+3)&(~0x3));
      size = PaddedWidth*readlong(fh.Height);

      bmp = (unsigned char *)malloc(size);
      *bitmap = (unsigned char *)malloc(size);

      if(bmp == NULL)
      {
          debugf("error - Out of memory\n");
          close(fd);
          return 6;
      }
      else
      {
          if(read(fd, (unsigned char*)bmp,(long)size) != size) {
              debugf("error - Can't read image\n");
              close(fd);
              return 7;
          }
      }

      bitmap_height = readlong(fh.Height);
      bitmap_width = readlong(fh.Width);

      *get_width = bitmap_width;
      *get_height = bitmap_height;

      /* Now convert the bitmap into an array with 1 byte per pixel,
         exactly the size of the image */

      for(row = 0;row < bitmap_height;row++) {
	for(col = 0;col < bitmap_width;col++) {
          if(bmp[(bitmap_height-1 -row) * PaddedWidth + col]) {
            (*bitmap)[ (row/8) * bitmap_width + col ] &= ~ (1<<(row&7));
          }
          else {
            (*bitmap)[ (row/8) * bitmap_width + col ] |= 1<<(row&7);
          }
	}
      }

      free(bmp);

   }
   close(fd);
   return 0; /* success */
}

/*********************************************************************
** generate_c_source()
**
** Outputs a C source code with the bitmap in an array, accompanied by
** some #define's
**********************************************************************/
void generate_c_source(char *id, int width, int height, unsigned char *bitmap)
{
   FILE *f;
   unsigned int i, a, eline;

   f = stdout;

   if(!id || !id[0])
     id = "bitmap";

   fprintf(f,
           "#define BMPHEIGHT_%s %d"
           "\n#define BMPWIDTH_%s %d"
           "\nconst unsigned char %s[] = {\n",
           id, height, id, width, id );

   for(i=0, eline=0; i< height; i+=8, eline++) {
       for (a=0; a<width; a++) fprintf(f, "0x%02x, ", bitmap[eline*width + a]);
       fprintf(f, "\n");
   }
   

   fprintf(f, "\n};\n");
}


/*********************************************************************
** generate_ascii()
**
** Outputs an ascii picture of the bitmap
**********************************************************************/
void generate_ascii(int width, int height, unsigned char *bitmap)
{
    FILE *f;
    unsigned int i, eline;

    f = stdout;

    /* for screen output debugging */
    for(i=0, eline=0; i< height; i+=8, eline++) {
        unsigned int x, y;
        for(y=0; y<8 && (i+y < height); y++) {
            for(x=0; x < width; x++) {
                
                if(bitmap[eline*width + x] & (1<<y)) {
                    fprintf(f, "*");
                }
                else
                    fprintf(f, " ");
            }
            fprintf(f, "\n");
        }
    }
}

void print_usage(void)
{
   printf("Usage: %s [-i <id>] [-a] <bitmap file>\n"
          "\t-i <id>     Bitmap name (default is filename without extension)\n"
          "\t-a          Show ascii picture of bitmap\n",
          APPLICATION_NAME);
   printf("build date: " __DATE__ "\n\n");
}

int main(int argc, char **argv)
{
   char *bmp_filename = NULL;
   char *id = NULL;
   int i;
   int height, width;
   int ascii = false;
   char* bitmap = NULL;
   
      
   for(i = 1;i < argc;i++)
   {
      if(argv[i][0] == '-')
      {
	switch(argv[i][1])
	{
	case 'i':   /* ID */
	   if(argv[i][2])
	   {
	      id = &argv[i][2];
	   }
	   else if(argc > i+1)
	   {
	      id = argv[i+1];
	      i++;
	   }
	   else
	   {
	      print_usage();
	      exit(1);
	   }
	   break;

	case 'a':   /* Assembly */
	   ascii = true;
	   break;

	default:
	   print_usage();
	   exit(1);
	   break;
	}
      }
      else
      {
	if(!bmp_filename)
	{
	   bmp_filename = argv[i];
	}
	else
	{
	   print_usage();
	   exit(1);
	}
      }
   }

   if (!bmp_filename)
   {
      print_usage();
      exit(1);
   }

   if (!id)
   {
      id = strdup(bmp_filename);
      for (i = 0; id[i]; i++) if (id[i] == '.') id[i] = '\0';
   }

   if (read_bmp_file(bmp_filename, &width, &height, &bitmap))
       return 0;
   
   if (ascii)
       generate_ascii(width, height, bitmap);
   else
       generate_c_source(id, width, height, bitmap);
   
   return 0;
}
