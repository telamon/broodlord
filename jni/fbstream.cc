#include <stdlib.h>
#include <unistd.h>
#include "fbstream.h"
//#include <android/log.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include "pixelflinger.h"

static struct fb_var_screeninfo vi;
struct fb_fix_screeninfo fi;

static void dumpinfo(struct fb_fix_screeninfo *fi, struct fb_var_screeninfo *vi);
static int fd;
static void *bits;

static int open_framebuffer(){
    printf("Opening fb device");
    fd = open("/dev/graphics/fb0", O_RDWR);
    if(fd < 0) {
        perror("cannot open fb0");
        return -1;
    }

    if(ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }

    if(ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        return -1;
    }
    dumpinfo(&fi, &vi);
    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        return -1;
    }
    return 0;
};
static int close_framebuffer(){
  printf("Closing fb device");
  munmap(bits,fi.smem_len);
  close(fd);
}
static int get_framebuffer(GGLSurface *fb){	
    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length / (vi.bits_per_pixel >> 3);
    fb->data = (GGLubyte*) bits;
    fb->format = GGL_PIXEL_FORMAT_RGB_565;
    fb++;
    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length / (vi.bits_per_pixel >> 3);
    fb->data =  ((GGLubyte*) (((unsigned) bits) + vi.yres * vi.xres * 2));
    fb->format = GGL_PIXEL_FORMAT_RGB_565;
    return fd;
}


static void dumpinfo(struct fb_fix_screeninfo *fi, struct fb_var_screeninfo *vi)
{
    fprintf(stderr,"vi.xres = %d\n", vi->xres);
    fprintf(stderr,"vi.yres = %d\n", vi->yres);
    fprintf(stderr,"vi.xresv = %d\n", vi->xres_virtual);
    fprintf(stderr,"vi.yresv = %d\n", vi->yres_virtual);
    fprintf(stderr,"vi.xoff = %d\n", vi->xoffset);
    fprintf(stderr,"vi.yoff = %d\n", vi->yoffset);
    fprintf(stderr, "vi.bits_per_pixel = %d\n", vi->bits_per_pixel);
    fprintf(stderr, "fi.line_length = %d\n", fi->line_length);
}
bool stopstream = 0;
void sighandler(int sig){
  stopstream = 1;
}
int main(int argc, char **argv){
  printf("...\n");
  if(!open_framebuffer());
  while(!stopstream){
    GGLSurface surf;
    get_framebuffer(&surf);
  }
  close_framebuffer();
  return 0;
}

