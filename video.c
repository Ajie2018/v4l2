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
    v4l2_format.fmt.pix.width=640;
    v4l2_format.fmt.pix.height=480;
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
    if(v4l2_format.fmt.pix.width==640 && v4l2_format.fmt.pix.height==480 && 
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
        printf("errline%d:require buffers failed!\n");
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

    //7.get datas from queuce space
    struct v4l2_buffer v4l2_readbuffer;
    v4l2_readbuffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd,VIDIOC_DQBUF,&v4l2_readbuffer);
    if (ret < 0) {
        printf("errline%d:extract data failed!\n",__LINE__);
    }
    
    FILE *fp=fopen("picture.jpg","w+");
    fwrite(mptr[v4l2_readbuffer.index],v4l2_readbuffer.length,1,fp);
    fclose(fp);

    ret = ioctl(fd,VIDIOC_QBUF,&v4l2_readbuffer);
    if (ret < 0) {
        printf("errline%d:return quence failed!\n",__LINE__);
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


