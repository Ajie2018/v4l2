#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <jpeglib.h>
#include <poll.h>

unsigned int lcd_h=1366;
unsigned int lcd_w=768;
int read_jpeg_file(const unsigned char *jpegDate,const unsigned char *rgbDate,int size);
int lcd_show_rgb(unsigned int *lcdptr,unsigned char *rgbData,int w,int h);

int main(void){
    int ret;    //define variable of return
    unsigned char rgbData[1366*768*3];
    int lcdfd = open("/dev/fb0",O_RDWR);
    if (lcdfd < 0) printf("errline%d:open lcd failed!\n",__LINE__);
    struct fb_var_screeninfo binfo;
    ret = ioctl(lcdfd,FBIOGET_VSCREENINFO,&binfo);
    lcd_w = binfo.xres;
    lcd_h = binfo.yres;
    unsigned int *lcdptr = (unsigned int *)mmap(NULL,lcd_w*lcd_h*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
    //1.open device 
    const int fd = open("/dev/video0",O_RDWR); 
    if(fd < 0) perror("open video device failed!\n");

    //2.query device capability
    struct v4l2_capability cap;
    ret = ioctl(fd,VIDIOC_QUERYCAP,&cap);
    if(ret<0) perror("can't get capabilities!\n");
    else{
        printf("Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n",  \
                cap.driver,cap.card,cap.bus_info,(cap.version>>16)&0XFF, (cap.version>>8)&0XFF,cap.version&0XFF);
        if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)  printf("v4l2 dev support capture\n");  
        if(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)  printf("v4l2 dev support output\n");  
        if(cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)  printf("v4l2 dev support overlay\n");  
        if(cap.capabilities & V4L2_CAP_STREAMING)  printf("v4l2 dev support streaming\n");  
        if(cap.capabilities & V4L2_CAP_READWRITE)  printf("v4l2 dev support read write\n");  
    }
    
    //3.query device capability
    //3.1 list valid input
    struct v4l2_input input;
    input.index=0;
    while(!ioctl(fd,VIDIOC_ENUMINPUT,&input)){
        printf("input:%s\n",input.name);
        ++input.index;
    }
    //3.2 set valid input
    input.index=0;
    if(ioctl(fd,VIDIOC_S_INPUT,&input)<0){
        printf("ERROR(%s):VIDIOC_S_INPUT failed!\n",__func__);
        return -1;
    }

    //4.set imagine type
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index=0;
    while(!ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)){
        printf("fmt:%s\n",fmtdesc.description);
        fmtdesc.index++;
    }
    
    struct v4l2_format format;
    memset(&format,0,sizeof(struct v4l2_format));
    format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width=lcd_w;
    format.fmt.pix.height=lcd_h;
    format.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.field=V4L2_FIELD_ANY;

    if(ioctl(fd,VIDIOC_S_FMT,&format)){
        printf("ERROR(%s):VIDIOC_S_FMT failed!\n",__func__);
        return -1;
    }

    //5.1 ask kernel memory
    struct v4l2_requestbuffers reqbuffer;
    reqbuffer.count=4;
    reqbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.memory=V4L2_MEMORY_MMAP;

    if(ioctl(fd,VIDIOC_REQBUFS,&reqbuffer)<0){
        printf("ERROR(%s):VIDIOC_REQBUFS failed\n",__func__);
        return -1;
    }
    //5.2 map user buffer
    struct v4l2_buffer mapbuffer;
    void *mapaddr[4];
    unsigned int mapsize[4];

    for(int i=0;i<4;++i){
        memset(&mapbuffer,0,sizeof(struct v4l2_buffer));
        mapbuffer.index=i;
        mapbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapbuffer.memory=V4L2_MEMORY_MMAP;
        ret=ioctl(fd,VIDIOC_QUERYBUF,&mapbuffer);
        if(ret<0){
            printf("unable to query buffer.\n");
            return -1;
        }
        mapaddr[i]=mmap(NULL,     //start anywhere
                  mapbuffer.length,PROT_READ|PROT_WRITE,MAP_SHARED,
                  fd,mapbuffer.m.offset);
        mapsize[i]=mapbuffer.length;
    }
    
    //5.3 put all buffer into queue
    for(int i=0;i<4;++i){
        memset(&mapbuffer,0,sizeof(struct v4l2_buffer));
        mapbuffer.index=i;
        mapbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapbuffer.memory=V4L2_MEMORY_MMAP;
        ret=ioctl(fd,VIDIOC_QBUF,&mapbuffer);
        if(ret<0){
            printf("unable to queue buffer.\n");
            return -1;
        }
    }
    
    //6 start stream imagine
    enum v4l2_buf_type buftype=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd,VIDIOC_STREAMON,&buftype)<0){
        printf("ERROR(%s):VIDIOC_STREAMON failed.\n",__func__);
        return -1;
    }
    
    //7 start read data
     struct pollfd pollfd[1];
     pollfd[0].fd=fd;
     pollfd[0].events=POLLIN;    //waitting reading data 
     poll(pollfd,1,10000);

    while(1){
        struct v4l2_buffer readbuffer;
        readbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        readbuffer.memory=V4L2_MEMORY_MMAP;

        if(ioctl(fd,VIDIOC_DQBUF,&readbuffer)<0){
            printf("ERROR(%s):VIDIOC_DQBUF failed.\n",__func__);
            return -1;
        }

        //display on screan
        read_jpeg_file(mapaddr[readbuffer.index],rgbData,readbuffer.length);
        lcd_show_rgb(lcdptr,rgbData,lcd_w,lcd_h);

        if(ioctl(fd,VIDIOC_QBUF,&readbuffer)<0){
            printf("ERROR(%s):VIDIOC_QBUF failed.\n",__func__);
            return -1;
        }
    }
    
    //8.close stream imagine and release map
    if(ioctl(fd,VIDIOC_STREAMOFF,&buftype)<0){
        printf("ERROR(%s):VIDIOC_STREAMOFF failed.\n",__func__);
        return -1;
    }
    for(int i=0;i<4;++i){
        munmap(mapaddr[i],mapsize[i]);
    }
    
    //9.close device 
    close(fd);

    return 0;
}

int read_jpeg_file(const unsigned char *jpegData,const unsigned char *rgbData,int size){
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err=jpeg_std_error(&jerr);
    //creat jpeg_decompress object
    jpeg_create_decompress(&cinfo);
    //put in decompress data
    //jpeg_stdio_src(&cinfo,infile);
    jpeg_mem_src(&cinfo,jpegData,size);
    //get pic para
    jpeg_read_header(&cinfo,TRUE);
    //start decompress
    jpeg_start_decompress(&cinfo);
    //ask for memory to store a line of data
    int row_stride=cinfo.output_width * cinfo.output_components;
    unsigned char *buffer=malloc(row_stride);
    int i=0;
    while(cinfo.output_scanline < cinfo.output_height){
        jpeg_read_scanlines(&cinfo,&buffer,1);
        memcpy((void *)(rgbData+i*lcd_w*3),buffer,row_stride);
        ++i;
    }
    //finish decompress
    jpeg_finish_decompress(&cinfo);
    //release decompress object
    jpeg_destroy_decompress(&cinfo);
    return 0;
}

int lcd_show_rgb(unsigned int *lcdptr,unsigned char *rgbData,int w,int h){
    unsigned int *ptr = lcdptr;
    for (int i=0; i<h; ++i){
        for(int j=0; j<w; ++j){
            memcpy(ptr+j,rgbData+j*3,3);
        }
        ptr += lcd_w;
        rgbData += w*3;
    }
    return 0;
}
