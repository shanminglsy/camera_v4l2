#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
  
  
#include <assert.h>  
#include <getopt.h>             
#include <fcntl.h>              
#include <unistd.h>  
#include <errno.h>  
#include <malloc.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
  
  
  
  
#include <asm/types.h>          
#include <linux/videodev2.h>  
  
  
#define CAMERA_DEVICE "/dev/video0"  
  
  
#define CAPTURE_FILE "frame_yuyv_new.jpg"  
#define CAPTURE_RGB_FILE "frame_rgb_new.bmp"  
#define CAPTURE_show_FILE "a.bmp"  
  
  
#define VIDEO_WIDTH 640  
#define VIDEO_HEIGHT 480  
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV  
#define BUFFER_COUNT 4  
  
  
typedef struct VideoBuffer {  
    void   *start;  
    size_t  length;  
} VideoBuffer;  
   
#pragma pack(1)  
typedef struct BITMAPFILEHEADER  
{  
  unsigned short bfType;
  unsigned long bfSize; 
  unsigned short bfReserved1;
  unsigned short bfReserved2;  
  unsigned long bfOffBits; 
} BITMAPFILEHEADER;  
#pragma pack()  
  
  
typedef struct BITMAPINFOHEADER  
{  
  unsigned long biSize; 
  unsigned long biWidth; 
  unsigned long biHeight;  
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned long biCompression;  
  unsigned long biSizeImage;
  unsigned long biXPelsPerMeter; 
  unsigned long biYPelsPerMeter; 
  unsigned long biClrUsed;
  unsigned long biClrImportant;
} BITMAPINFOHEADER;  
  
  
  
  
VideoBuffer framebuf[BUFFER_COUNT];  
int fd;  
struct v4l2_capability cap;  
struct v4l2_fmtdesc fmtdesc;  
struct v4l2_format fmt;  
struct v4l2_requestbuffers reqbuf;  
struct v4l2_buffer buf;  
unsigned char *starter;  
unsigned char *newBuf;  
struct BITMAPFILEHEADER bfh;  
struct BITMAPINFOHEADER bih;  
  
  
void create_bmp_header()  
{  
  bfh.bfType = (unsigned short)0x4D42;  
  bfh.bfSize = (unsigned long)(14 + 40 + VIDEO_WIDTH * VIDEO_HEIGHT*3);  
  bfh.bfReserved1 = 0;  
  bfh.bfReserved2 = 0;  
  bfh.bfOffBits = (unsigned long)(14 + 40);  
  
  
  bih.biBitCount = 24;  
  bih.biWidth = VIDEO_WIDTH;  
  bih.biHeight = VIDEO_HEIGHT;  
  bih.biSizeImage = VIDEO_WIDTH * VIDEO_HEIGHT * 3;  
  bih.biClrImportant = 0;  
  bih.biClrUsed = 0;  
  bih.biCompression = 0;  
  bih.biPlanes = 1;  
  bih.biSize = 40;//sizeof(bih);  
  bih.biXPelsPerMeter = 0x00000ec4;  
  bih.biYPelsPerMeter = 0x00000ec4;  
}  
  
  
int open_device()  
{  
    int fd;  
    fd = open(CAMERA_DEVICE, O_RDWR, 0);//  
    if (fd < 0) {  
        printf("Open %s failed\n", CAMERA_DEVICE);  
        return -1;  
    }  
    return fd;  
}  
  
  
void get_capability()  
{  
  
    int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);  
    if (ret < 0) {  
        printf("VIDIOC_QUERYCAP failed (%d)\n", ret);  
        return;  
    }  
    // Print capability infomations  
    printf("------------VIDIOC_QUERYCAP-----------\n");  
    printf("Capability Informations:\n");  
    printf(" driver: %s\n", cap.driver);  
    printf(" card: %s\n", cap.card);  
    printf(" bus_info: %s\n", cap.bus_info);  
    printf(" version: %08X\n", cap.version);  
    printf(" capabilities: %08X\n\n", cap.capabilities);  
    return;  
}  
  
  
void get_format()  
{   
    int ret;  
    fmtdesc.index=0;  
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    ret=ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);  
    while (ret != 0)  
    {  
        fmtdesc.index++;  
        ret=ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);  
    }  
    printf("--------VIDIOC_ENUM_FMT---------\n");  
    printf("get the format what the device support\n{ pixelformat = ''%c%c%c%c'', description = ''%s'' }\n",fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF, (fmtdesc.pixelformat >> 16) & 0xFF,(fmtdesc.pixelformat >> 24) & 0xFF, fmtdesc.description);  
      
    return;  
}  
  
  
int set_format()  
{  
  
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    fmt.fmt.pix.width       = VIDEO_WIDTH;  
    fmt.fmt.pix.height      = VIDEO_HEIGHT;  
    fmt.fmt.pix.pixelformat = fmtdesc.pixelformat;//V4L2_PIX_FMT_YUYV;  
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;  
    int ret = ioctl(fd, VIDIOC_S_FMT, &fmt);  
    if (ret < 0) {  
        printf("VIDIOC_S_FMT failed (%d)\n", ret);  
        return;  
    }  
  
  
  
    // Print Stream Format  
    printf("------------VIDIOC_S_FMT---------------\n");  
    printf("Stream Format Informations:\n");  
    printf(" type: %d\n", fmt.type);  
    printf(" width: %d\n", fmt.fmt.pix.width);  
    printf(" height: %d\n", fmt.fmt.pix.height);  
  
  
    char fmtstr[8];  
    memset(fmtstr, 0, 8);    
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);  
    printf(" pixelformat: %s\n", fmtstr);  
    printf(" field: %d\n", fmt.fmt.pix.field);  
    printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);  
    printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);  
    printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);  
    printf(" priv: %d\n", fmt.fmt.pix.priv);  
    printf(" raw_date: %s\n", fmt.fmt.raw_data);  
    return;  
}  
  
  
void request_buf()  
{  

    reqbuf.count = BUFFER_COUNT;  
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    reqbuf.memory = V4L2_MEMORY_MMAP;  
      
    int ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);  
    if(ret < 0) {  
        printf("VIDIOC_REQBUFS failed (%d)\n", ret);  
        return;  
    }  
    printf("the buffer has been assigned successfully!\n");  
    return;  
}  
  
  
void query_map_qbuf()  
{  
    int i,ret;  
    for (i = 0; i < reqbuf.count; i++)  
    {  
        buf.index = i;  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
        ret = ioctl(fd , VIDIOC_QUERYBUF, &buf);
        if(ret < 0) {  
            printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);  
            return;  
        }  
    
        framebuf[i].length = buf.length; 
        framebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);  
        if (framebuf[i].start == MAP_FAILED) {  
            printf("mmap (%d) failed: %s\n", i, strerror(errno));  
            return;  
        }  
      
        // Queen buffer  
 
        ret = ioctl(fd , VIDIOC_QBUF, &buf);  
        if (ret < 0) {  
            printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);  
            return;  
        }   
        printf("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuf[i].start, framebuf[i].length);  
    }
    return;  
}  
  
  
  
  
void yuyv2rgb()  
{  
    unsigned char YUYV[4],RGB[6];  
    int j,k,i;     
    unsigned int location=0;  
    j=0;  
    for(i=0;i < framebuf[buf.index].length;i+=4)  
    {  
        YUYV[0]=starter[i];//Y0  
        YUYV[1]=starter[i+1];//U  
        YUYV[2]=starter[i+2];//Y1  
        YUYV[3]=starter[i+3];//V  
        if(YUYV[0]<1)  
        {  
            RGB[0]=0;  
            RGB[1]=0;  
            RGB[2]=0;  
        }  
        else  
        {  
            RGB[0]=YUYV[0]+1.772*(YUYV[1]-128);//b  
            RGB[1]=YUYV[0]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[3]-128);//g  
            RGB[2]=YUYV[0]+1.402*(YUYV[3]-128);//r  
        }  
        if(YUYV[2]<0)  
        {  
            RGB[3]=0;  
            RGB[4]=0;  
            RGB[5]=0;  
        }  
        else  
        {  
            RGB[3]=YUYV[2]+1.772*(YUYV[1]-128);//b  
            RGB[4]=YUYV[2]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[3]-128);//g  
            RGB[5]=YUYV[2]+1.402*(YUYV[3]-128);//r  
  
  
        }  
  
  
        for(k=0;k<6;k++)  
        {  
            if(RGB[k]<0)  
                RGB[k]=0;  
            if(RGB[k]>255)  
                RGB[k]=255;  
        }  
    
        if(j%(VIDEO_WIDTH*3)==0)
        {  
            location=(VIDEO_HEIGHT-j/(VIDEO_WIDTH*3))*(VIDEO_WIDTH*3);  
        }  
        bcopy(RGB,newBuf+location+(j%(VIDEO_WIDTH*3)),sizeof(RGB));  
  
  
        j+=6;         
    }  
    return;  
}  
  
  
void move_noise()  
{ 
    int i,j,k,temp[3],temp1[3];  
    unsigned char BGR[13*3];  
    unsigned int sq,sq1,loc,loc1;  
    int h=VIDEO_HEIGHT,w=VIDEO_WIDTH;  
    for(i=2;i<h-2;i++)  
    {  
        for(j=2;j<w-2;j++)  
        {  
            memcpy(BGR,newBuf+(i-1)*w*3+3*(j-1),9);  
            memcpy(BGR+9,newBuf+i*w*3+3*(j-1),9);  
            memcpy(BGR+18,newBuf+(i+1)*w*3+3*(j-1),9);  
            memcpy(BGR+27,newBuf+(i-2)*w*3+3*j,3);  
            memcpy(BGR+30,newBuf+(i+2)*w*3+3*j,3);  
            memcpy(BGR+33,newBuf+i*w*3+3*(j-2),3);  
            memcpy(BGR+36,newBuf+i*w*3+3*(j+2),3);  
  
  
            memset(temp,0,4*3);  
              
            for(k=0;k<9;k++)  
            {  
                temp[0]+=BGR[k*3];  
                temp[1]+=BGR[k*3+1];  
                temp[2]+=BGR[k*3+2];  
            }  
            temp1[0]=temp[0];  
            temp1[1]=temp[1];  
            temp1[2]=temp[2];  
            for(k=9;k<13;k++)  
            {  
                temp1[0]+=BGR[k*3];  
                temp1[1]+=BGR[k*3+1];  
                temp1[2]+=BGR[k*3+2];  
            }  
            for(k=0;k<3;k++)  
            {  
                temp[k]/=9;  
                temp1[k]/=13;  
            }  
            sq=0xffffffff;loc=0;  
            sq1=0xffffffff;loc1=0;  
            unsigned int a;           
            for(k=0;k<9;k++)  
            {  
                a=abs(temp[0]-BGR[k*3])+abs(temp[1]-BGR[k*3+1])+abs(temp[2]-BGR[k*3+2]);  
                if(a<sq)  
                {  
                    sq=a;  
                    loc=k;  
                }  
            }  
            for(k=0;k<13;k++)  
            {  
                a=abs(temp1[0]-BGR[k*3])+abs(temp1[1]-BGR[k*3+1])+abs(temp1[2]-BGR[k*3+2]);  
                if(a<sq1)  
                {  
                    sq1=a;  
                    loc1=k;  
                }  
            }  
              
            newBuf[i*w*3+3*j]=(unsigned char)((BGR[3*loc]+BGR[3*loc1])/2);  
            newBuf[i*w*3+3*j+1]=(unsigned char)((BGR[3*loc+1]+BGR[3*loc1+1])/2);  
            newBuf[i*w*3+3*j+2]=(unsigned char)((BGR[3*loc+2]+BGR[3*loc1+2])/2);   
            temp[0]=(BGR[3*loc]+BGR[3*loc1])/2;  
            temp[1]=(BGR[3*loc+1]+BGR[3*loc1+1])/2;  
            temp[2]=(BGR[3*loc+2]+BGR[3*loc1+2])/2;  
            sq=abs(temp[0]-BGR[loc*3])+abs(temp[1]-BGR[loc*3+1])+abs(temp[2]-BGR[loc*3+2]);  
            sq1=abs(temp[0]-BGR[loc1*3])+abs(temp[1]-BGR[loc1*3+1])+abs(temp[2]-BGR[loc1*3+2]);  
            if(sq1<sq) loc=loc1;  
            newBuf[i*w*3+3*j]=BGR[3*loc];  
            newBuf[i*w*3+3*j+1]=BGR[3*loc+1];  
            newBuf[i*w*3+3*j+2]=BGR[3*loc+2];  
        }  
    }  
    return;  
}  
  
  
void yuyv2rgb1()  
{  
    unsigned char YUYV[3],RGB[3];  
    memset(YUYV,0,3);  
    int j,k,i;     
    unsigned int location=0;  
    j=0;  
    for(i=0;i < framebuf[buf.index].length;i+=2)  
    {  
        YUYV[0]=starter[i];//Y0  
        if(i%4==0)  
            YUYV[1]=starter[i+1];//U  
        //YUYV[2]=starter[i+2];//Y1  
        if(i%4==2)  
            YUYV[2]=starter[i+1];//V  
        if(YUYV[0]<1)  
        {  
            RGB[0]=0;  
            RGB[1]=0;  
            RGB[2]=0;  
        }  
        else  
        {  
            RGB[0]=YUYV[0]+1.772*(YUYV[1]-128);//b  
            RGB[1]=YUYV[0]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[2]-128);//g  
            RGB[2]=YUYV[0]+1.402*(YUYV[2]-128);//r  
        }  
  
  
        for(k=0;k<3;k++)  
        {  
            if(RGB[k]<0)  
                RGB[k]=0;  
            if(RGB[k]>255)  
                RGB[k]=255;  
        }  
  
        if(j%(VIDEO_WIDTH*3)==0)
        {  
            location=(VIDEO_HEIGHT-j/(VIDEO_WIDTH*3))*(VIDEO_WIDTH*3);  
        }  
        bcopy(RGB,newBuf+location+(j%(VIDEO_WIDTH*3)),sizeof(RGB));  
  
  
        j+=3;         
    }  
    return;  
}  
  
  
void store_yuyv()  
{  
    FILE *fp = fopen(CAPTURE_FILE, "wb");  
    if (fp < 0) {  
        printf("open frame data file failed\n");  
        return;  
    }  
    fwrite(framebuf[buf.index].start, 1, buf.length, fp);  
    fclose(fp);  
    printf("Capture one frame saved in %s\n", CAPTURE_FILE);  
    return;  
}  
  
  
  
  
void store_bmp(int n_len)  
{  
    FILE *fp1 = fopen(CAPTURE_RGB_FILE, "wb");  
    if (fp1 < 0) {  
        printf("open frame data file failed\n");  
        return;  
    }  
    fwrite(&bfh,sizeof(bfh),1,fp1);  
    fwrite(&bih,sizeof(bih),1,fp1);  
    fwrite(newBuf, 1, n_len, fp1);  
    fclose(fp1);  
    printf("Change one frame saved in %s\n", CAPTURE_RGB_FILE);  
    return;  
}  
  
  
int main()  
{  
    int i, ret;  
  
    fd=open_device();  
    get_capability();  
    //struct v4l2_fmtdesc fmtdesc;  
    memset(&fmtdesc,0,sizeof(fmtdesc));  
    get_format();  
      
    //struct v4l2_format fmt;   
    memset(&fmt, 0, sizeof(fmt));
    set_format();  
      

    //struct v4l2_requestbuffers reqbuf;  
    request_buf();  
  
    //struct v4l2_buffer buf;  
    query_map_qbuf();  
  
  

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    ret = ioctl(fd, VIDIOC_STREAMON, &type);  
    if (ret < 0) {  
        printf("VIDIOC_STREAMON failed (%d)\n", ret);  
        return ret;  
    }  
  
  
    // Get frame  
    ret = ioctl(fd, VIDIOC_DQBUF, &buf); 
    if (ret < 0) {  
        printf("VIDIOC_DQBUF failed (%d)\n", ret);  
        return ret;  
    }  
  
  
    // Process the frame 
    store_yuyv();  
      
  
    printf("********************************************\n");  
    int n_len;  
    n_len=framebuf[buf.index].length*3/2;  
    newBuf=calloc((unsigned int)n_len,sizeof(unsigned char));  
    
    if(!newBuf)  
    {  
        printf("cannot assign the memory !\n");  
        exit(0);  
    }  
  
  
    printf("the information about the new buffer:\n start Address:0x%x,length=%d\n\n",(unsigned int)newBuf,n_len);  
  
  
    printf("----------------------------------\n");  
      
    //YUYV to RGB  
    starter=(unsigned char *)framebuf[buf.index].start;  
    yuyv2rgb(); 
    move_noise();  
    //yuyv2rgb1();  
    create_bmp_header();  
      
    store_bmp(n_len);  
      
      
    // Re-queen buffer  
    ret = ioctl(fd, VIDIOC_QBUF, &buf);  
    if (ret < 0) {  
        printf("VIDIOC_QBUF failed (%d)\n", ret);  
        return ret;  
    }  
    printf("re-queen buffer end\n");  
    // Release the resource  
 
    for (i=0; i< 4; i++)  
    {  
      
        munmap(framebuf[i].start, framebuf[i].length);  
    }  
    //free(starter);  
    printf("free starter end\n");  
    //free(newBuf);  
    printf("free newBuf end\n");  
    close(fd);  
  
  
      
    printf("Camera test Done.\n");  
    return 0;  
}  
