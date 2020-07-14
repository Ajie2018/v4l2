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

unsigned int lcd_h=1366;
unsigned int lcd_w=768;


int read_jpeg_file(const unsigned char *jpegDate,const unsigned char *rgbDate,int size);
int lcd_show_rgb(unsigned int *lcdptr,unsigned char *rgbData,int w,int h);

int main(void)
{
    //1.open device 
    const int fd = open("/dev/video0",O_RDWR); 
    if(fd < 0) perror("open video device failed!");

    //2.get support format
    struct v4l2_fmtdesc v4l2_fmtdesc;
    v4l2_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret,num=0;
    while(1){
        v4l2_fmtdesc.index=num;
        ret = ioctl(fd,VIDIOC_ENUM_FMT,&v4l2_fmtdesc);
        if (ret < 0)
        {
            printf("%d:can't get support format!\n",__LINE__);
            break;
        }
        printf("index=%d\n",v4l2_fmtdesc.index);
        printf("flags=%d\n",v4l2_fmtdesc.flags);
        printf("description=%s\n",v4l2_fmtdesc.description);
        unsigned char *pixel = (unsigned char *)&v4l2_fmtdesc.pixelformat;
        printf("pixelformat=%s\n",pixel);
        printf("reserved=%d\n",v4l2_fmtdesc.reserved[0]);
        num++;
    }

    //3.set sample format
    struct v4l2_format v4l2_format;
    v4l2_format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_format.fmt.pix.width=lcd_w;
    v4l2_format.fmt.pix.height=lcd_h;
    v4l2_format.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;

    ret = ioctl(fd,VIDIOC_S_FMT,&v4l2_format);
    if (ret < 0)
    {
        printf("errline%d:set format failed!\n",__LINE__);
    }
    memset(&v4l2_format,0,sizeof(v4l2_format)); 
    v4l2_format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd,VIDIOC_G_FMT,&v4l2_format);
    if (ret < 0)
    {
        printf("errline%d:get format failed!\n",__LINE__);
    }
    if(v4l2_format.fmt.pix.width==lcd_w && v4l2_format.fmt.pix.height==lcd_h && 
       v4l2_format.fmt.pix.pixelformat==V4L2_PIX_FMT_MJPEG)
    {
        printf("set format succeed!\n");
    }
    else
    {
        printf("errline%d:set format failed!\n",__LINE__);
    }

    //4.require quence memory
    struct v4l2_requestbuffers v4l2_reqbuffers;
    v4l2_reqbuffers.count=4;
    v4l2_reqbuffers.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffers.memory=V4L2_MEMORY_MMAP;

    ret = ioctl(fd,VIDIOC_REQBUFS,&v4l2_reqbuffers);
    if (ret < 0)
    {
        printf("errline%d:require buffers failed!\n",__LINE__);
    }

    //5.map  quence memory
    struct v4l2_buffer v4l2_mapbuffer;
    v4l2_mapbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    unsigned char *mptr[4];
    unsigned int mplen[4];
    for(int i=0;i<4;++i){
        v4l2_mapbuffer.index=i;
        ret = ioctl(fd,VIDIOC_QUERYBUF,&v4l2_mapbuffer);
        if (ret < 0) {
            printf("errline%d:query kernel buffers failed!\n",__LINE__);
        }
        mptr[i]=(unsigned char *)mmap(NULL,v4l2_mapbuffer.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,v4l2_mapbuffer.m.offset);
        mplen[i]=v4l2_mapbuffer.length;
        ret = ioctl(fd,VIDIOC_QBUF,&v4l2_mapbuffer);
        if (ret < 0) {
            printf("errline%d:map kernel buffers failed!\n",__LINE__);
        }
    }

    //6.start sample pictures
    int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd,VIDIOC_STREAMON,&type);
    if (ret < 0) {
        printf("errline%d:streamon failed!\n",__LINE__);
    }

    unsigned char rgbData[1366*768*3];
    
    int lcdfd = open("/dev/fb0",O_RDWR);
    if (lcdfd < 0) printf("errline%d:open lcd failed!\n",__LINE__);
    unsigned int *lcdptr = (unsigned int *)mmap(NULL,lcd_w*lcd_h*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcdfd,0);
    struct fb_var_screeninfo binfo;
    ret = ioctl(lcdfd,FBIOGET_VSCREENINFO,&binfo);
    lcd_w = binfo.xres;
    lcd_h = binfo.yres;

    while(1){
        //7.get datas from queuce space
        struct v4l2_buffer v4l2_readbuffer;
        v4l2_readbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd,VIDIOC_DQBUF,&v4l2_readbuffer);
        if (ret < 0) {
            printf("errline%d:extract data failed!\n",__LINE__);
        }
        
        //display on screan
        read_jpeg_file(mptr[v4l2_readbuffer.index],rgbData,v4l2_readbuffer.length);
        lcd_show_rgb(lcdptr,rgbData,lcd_w,lcd_h);

        ret = ioctl(fd,VIDIOC_QBUF,&v4l2_readbuffer);
        if (ret < 0) {
            printf("errline%d:return quence failed!\n",__LINE__);
        }
    }
    //8 stop sample pictures
    ret = ioctl(fd,VIDIOC_STREAMOFF,&type);
    if (ret < 0) {
        printf("errline%d:streamoff failed!\n",__LINE__);
    }

    for(int i=0;i<4;++i){
        munmap(mptr[i],mplen[i]);
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
