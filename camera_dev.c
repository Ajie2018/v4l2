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
#include <poll.h>

int main(void){
    int ret;    //define variable of return
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
    format.fmt.pix.width=640;
    format.fmt.pix.height=480;
    format.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;
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
    struct v4l2_buffer readbuffer;
    readbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    readbuffer.memory=V4L2_MEMORY_MMAP;

    if(ioctl(fd,VIDIOC_DQBUF,&readbuffer)<0){
        printf("ERROR(%s):VIDIOC_DQBUF failed.\n",__func__);
        return -1;
    }

    readbuffer.index=0;
    if(ioctl(fd,VIDIOC_QBUF,&readbuffer)<0){
        printf("ERROR(%s):VIDIOC_QBUF failed.\n",__func__);
        return -1;
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


