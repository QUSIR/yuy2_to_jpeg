#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "Log.h"


typedef int BOOL;

#define  TRUE    1
#define  FALSE    0

#define FILE_VIDEO     "/dev/video1"

#define PHOTO_FOLDER "/sdcard/androidDoor"


int get_photo(int size,int filename);