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

int main(void)
{
    //1.open device 
    const int fd = open("/dev/video0",O_RDWR); 
    if(fd < 0) perror("open video device failed!");

    //2.get support format
    struct v4l2_fmtdesc v4l2_fmtdesc;
    v4l2_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(fd,VIDIOC_ENUM_FMT,&v4l2_fmtdesc);
    if (ret < 0)
    {
        perror("e1.can't get support format!");
    }
    printf("index=%d\n",v4l2_fmtdesc.index);
    printf("flags=%d\n",v4l2_fmtdesc.flags);
    printf("description=%s\n",v4l2_fmtdesc.description);
    unsigned char *pixel = (unsigned char *)&v4l2_fmtdesc.pixelformat;
    printf("pixelformat=%s\n",pixel);
    printf("reserved=%d\n",v4l2_fmtdesc.reserved[0]);

    //3.set support format
    struct v4l2_format v4l2_format;
    v4l2_format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_format.fmt.pix.width=640;
    v4l2_format.fmt.pix.height=480;
    v4l2_format.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;

    ret = ioctl(fd,VIDIOC_S_FMT,&v4l2_format);
    if (ret < 0)
    {
        perror("e2.set format failed!\n");
    }
    memset(&v4l2_format,0,sizeof(v4l2_format)); 
    v4l2_format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd,VIDIOC_G_FMT,&v4l2_format);
    if (ret < 0)
    {
        perror("e3.get format failed!");
    }
    if(v4l2_format.fmt.pix.width==640 && v4l2_format.fmt.pix.height==480 && 
       v4l2_format.fmt.pix.pixelformat==V4L2_PIX_FMT_MJPEG)
    {
        printf("r1.set format succeed!\n");
    }
    else
    {
        printf("e4.set format failed!\n");
    }

    //4.require quence memory
    struct v4l2_requestbuffers v4l2_reqbuffers;
    v4l2_reqbuffers.count=4; v4l2_reqbuffers.memory=V4L2_MEMORY_MMAP;
    v4l2_reqbuffers.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd,VIDIOC_REQBUFS,&v4l2_reqbuffers);
    if (ret < 0)
    {
        perror("e5.require buffers failed!\n");
    }

    //5.map  quence memory
    //VIDIOC_QUERYBUF  VIDIOC_QBUF
    struct v4l2_buffer v4l2_mapbuffer;
    v4l2_mapbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    unsigned char *mptr[4];
    for(int i=0;i<4;++i){
        v4l2_mapbuffer.index=i;
        printf("%d\n",i);
        ret = ioctl(fd,VIDIOC_QBUF,&v4l2_mapbuffer);
        printf("%d\n",ret);
        if (ret < 0) {
            perror("e6.query kernel buffers failed!\n");
        }
        mptr[i]=(unsigned char *)mmap(NULL,v4l2_mapbuffer.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,v4l2_mapbuffer.m.offset);
        ret = ioctl(fd,VIDIOC_QBUF,&v4l2_mapbuffer);
        if (ret < 0) {
            perror("e7.map kernel buffer failed!\n");
        }
    }

    

    //9.close device 
    close(fd);

    return 0;
}


