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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>
#include <signal.h>
#include <assert.h>
static struct fb_var_screeninfo vi;
struct fb_fix_screeninfo fi;

static void dumpinfo(struct fb_fix_screeninfo *fi, struct fb_var_screeninfo *vi);
static int fd;
static void *bits;
static int sockfd;
static z_stream strm;

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
static int cleanup(){
  printf("Cleaning up");
  deflateEnd(&strm);
  close(sockfd);
  munmap(bits,fi.smem_len);
  close(fd);
  exit(0);
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
  cleanup();
}

int main(int argc, char **argv){
  signal(SIGINT, sighandler);
  // Open capture device
  if(open_framebuffer()<0){
    perror("Failed to open capture device");
    return -1;
  };
  // Open client socket
  int port,n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  if(argc < 1){
    fprintf(stderr, "usage: %s hostname port\n",argv[0]);
    return 0;
  }
  port = 5566;// atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0 );
  if(sockfd < 0){
    perror("Error, opening socket\n");
    exit(-1);
  }
  
  server = gethostbyname("192.168.1.6");
  if(server == NULL){
    perror("Error, no such host\n");
    exit(-1);
  }
  // Configure destination
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *) server -> h_addr,
      (char *) &serv_addr.sin_addr.s_addr,
      server -> h_length);
  serv_addr.sin_port = htons(port);
  // Connect
  if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
    perror("Connection refused");
    exit(-1);
  }
  printf("Connection estabilshed");
  // Initialize compression stream
  #define CHUNK 16384
  int ret,flush;
  unsigned have;
  unsigned char out[CHUNK];
  // Allocate deflate state
  
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, 2);
  if(ret != Z_OK){
    perror("Failed to initialize compressor");
    exit(-1);
  }

  GGLSurface surf;
  get_framebuffer(&surf);
  while(!stopstream){
    #define FRAME_t (surf.width * surf.height * 4)
    do{
      strm.avail_in = FRAME_t;
      strm.next_in = (unsigned char*)bits;
      do{
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = deflate(&strm,flush);
        have = CHUNK - strm.avail_out;
        // Send data over the wire.
        n = write(sockfd, out, have );
        if(n < 0){
          perror("Error writing to socket");
          stopstream=1;
        }
      }while(strm.avail_out == 0);
      assert(strm.avail_in == 0);
    } while(flush != Z_FINISH);
    assert(ret == Z_STREAM_END);
  }
  cleanup();
  return 0;
}

