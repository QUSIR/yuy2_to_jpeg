#include"encode_jpeg.h"
static   int      fdencode_jpeg;

struct buffer
{
    void * start;
    unsigned int length;
} * buffers;

BOOL init_device(int width,int height)
{
    int i;
    int ret = 0;
    enum v4l2_buf_type type;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt,fmtack;
    //opendev
    struct   v4l2_capability   cap;

    if ((fdencode_jpeg = open(FILE_VIDEO, O_RDWR)) == -1)
    {
        l_error("Error opening V4L interface\n");
        return (FALSE);
    }

    //query cap
    if (ioctl(fdencode_jpeg, VIDIOC_QUERYCAP, &cap) == -1)
    {
        l_error("Error opening device %s: unable to query device.\n",FILE_VIDEO);
        return (FALSE);
    }
    else
    {


         if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
         {
            l_error("Device %s: supports capture.\n",FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        {
            l_error("Device %s: supports streaming.\n",FILE_VIDEO);
        }
    }

    //emu all support fmt
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while(ioctl(fdencode_jpeg,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        l_error("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;
    }

    //set fmt
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if(ioctl(fdencode_jpeg, VIDIOC_S_FMT, &fmt) == -1)
    {
        l_error("Unable to set format\n");
        return FALSE;
    }
    if(ioctl(fdencode_jpeg, VIDIOC_G_FMT, &fmt) == -1)
    {
        l_error("Unable to get format\n");
        return FALSE;
    }


    return TRUE;
}

BOOL getv4l2_grab(void)
{
    unsigned int n_buffers=0;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    struct v4l2_requestbuffers req;

    req.count=1;
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory=V4L2_MEMORY_MMAP;
    if(ioctl(fdencode_jpeg,VIDIOC_REQBUFS,&req)==-1)
    {
        l_error("request for buffers error\n");
        return(FALSE);
    }

    //mmap for buffers
    buffers = malloc(req.count*sizeof (*buffers));
    if (!buffers)
    {
        l_error ("Out of memory\n");
        return(FALSE);
    }


    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;
    //query buffers
    if (ioctl (fdencode_jpeg, VIDIOC_QUERYBUF, &buf) == -1)
    {
        l_error("query buffer error\n");
        return(FALSE);
    }

    buffers[n_buffers].length = buf.length;
    //map
    buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ |PROT_WRITE, MAP_SHARED, fdencode_jpeg, buf.m.offset);
    if (buffers[n_buffers].start == MAP_FAILED)
    {
        l_error("buffer map error\n");
        return(FALSE);
    }

    for (n_buffers = 0; n_buffers < req.count; n_buffers++)
    {
        buf.index = n_buffers;
        ioctl(fdencode_jpeg, VIDIOC_QBUF, &buf);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (fdencode_jpeg, VIDIOC_STREAMON, &type);

    ioctl(fdencode_jpeg, VIDIOC_DQBUF, &buf);


    return(TRUE);
}


int yuyv_2_rgb888(char *frame_buffer,int width,int height)
{
    int           i,j;
    unsigned char y1,y2,u,v;
    int r1,g1,b1,r2,g2,b2;
    char * pointer;

    pointer = buffers[0].start;

    for(i=0;i<height;i++)
    {
        for(j=0;j<width/2;j++)//每次取4个字节，也就是两个像素点，转换rgb，6个字节，还是两个像素点
        {
            y1 = *( pointer + (i*width/2+j)*4);
            u  = *( pointer + (i*width/2+j)*4 + 1);
            y2 = *( pointer + (i*width/2+j)*4 + 2);
            v  = *( pointer + (i*width/2+j)*4 + 3);

            r1 = y1 + 1.042*(v-128);
            g1 = y1 - 0.34414*(u-128) - 0.71414*(v-128);
            b1 = y1 + 1.772*(u-128);

            r2 = y2 + 1.042*(v-128);
            g2 = y2 - 0.34414*(u-128) - 0.71414*(v-128);
            b2 = y2 + 1.772*(u-128);

            if(r1>255)
            r1 = 255;
            else if(r1<0)
            r1 = 0;

            if(b1>255)
            b1 = 255;
            else if(b1<0)
            b1 = 0;

            if(g1>255)
            g1 = 255;
            else if(g1<0)
            g1 = 0;

            if(r2>255)
            r2 = 255;
            else if(r2<0)
            r2 = 0;

            if(b2>255)
            b2 = 255;
            else if(b2<0)
            b2 = 0;

            if(g2>255)
            g2 = 255;
            else if(g2<0)
            g2 = 0;

            *(frame_buffer + (i*width/2+j)*6    ) = (unsigned char)b1;
            *(frame_buffer + (i*width/2+j)*6 + 1) = (unsigned char)g1;
            *(frame_buffer + (i*width/2+j)*6 + 2) = (unsigned char)r1;
            *(frame_buffer + (i*width/2+j)*6 + 3) = (unsigned char)b2;
            *(frame_buffer + (i*width/2+j)*6 + 4) = (unsigned char)g2;
            *(frame_buffer + (i*width/2+j)*6 + 5) = (unsigned char)r2;
        }
    }

}

BOOL encode_jpeg(char *lpbuf,int width,int height,char *filename)
{
    struct jpeg_compress_struct cinfo ;
    struct jpeg_error_mgr jerr ;
    JSAMPROW  row_pointer[1] ;
    int row_stride ;
    char *buf=NULL ;
    int x ;

    FILE *fptr_jpg = fopen (filename,"wb");//注意这里为什么用fopen而不用open
    if(fptr_jpg==NULL)
    {
        l_error("Encoder:open file failed!/n") ;
        return FALSE;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fptr_jpg);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);


    jpeg_set_quality(&cinfo, 80,TRUE);


    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * 3;
    buf=malloc(row_stride) ;
    row_pointer[0] = buf;
    while (cinfo.next_scanline < height)
    {
        for (x = 0; x < row_stride; x+=3)
        {

            buf[x]   = lpbuf[x];
            buf[x+1] = lpbuf[x+1];
            buf[x+2] = lpbuf[x+2];

        }
        jpeg_write_scanlines (&cinfo, row_pointer, 1);//critical
        lpbuf += row_stride;
    }

    jpeg_finish_compress(&cinfo);
    fclose(fptr_jpg);
    jpeg_destroy_compress(&cinfo);
    free(buf) ;
    return TRUE ;

}

void close_device(void)
{

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fdencode_jpeg,VIDIOC_STREAMOFF,&type);

    if(munmap(buffers[0].start,buffers[0].length)<0){
        l_error("v4l_capture munmap error\n");
    }


    close(fdencode_jpeg);

}

int get_photo(int size,int filename){

    int g_width = 0;
    int g_height = 0;

    if(size==2){
        g_width = 320;
        g_height = 240;

    }else{
        g_width = 640;
        g_height = 480;
    }

    unsigned char frame_buffer[g_width*g_height*3];

    char tmpfile[512]={'\0'};
    snprintf(tmpfile,512,"%s/%d.jpg",PHOTO_FOLDER,filename);

    if(!init_device(g_width,g_height)) l_error("init_v4l error \n");
    if(!getv4l2_grab()) l_error("getv4l2_grab error");
    yuyv_2_rgb888(frame_buffer,g_width,g_height);
    encode_jpeg(frame_buffer,g_width,g_height,tmpfile);


    close_device();

    return(TRUE);
}

