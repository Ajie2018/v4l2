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

int main(void){
    int ret;    //define variable of return
    //1.open device 
    const int fd = open("/dev/video1",O_RDWR); 
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
    
    //2.query device capability
    //2.1 list valid input
    struct v4l2_input input;
    input.index=0;
    while(!ioctl(fd,VIDIOC_ENUMINPUT,&input)){
        printf("input:%s\n",input.name);
        ++input.index;
    }
    //2.2 set valid input

    //9.close device 
    close(fd);

    return 0;
}


