/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <time.h>




#include <include/linux/videodev2.h>
#include <include/linux/dmw96ciu_common.h>
#include "include/video/dmw96osdm_common.h"
#include "include/video/dwfb_utils_common.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

typedef enum {
	FORMAT_PLANAR,
	FORMAT_SEMIPLANAR,
} plane_org;

typedef enum {
	FORMAT_NV12,
	FORMAT_NV16,
	FORMAT_NV21,
	FORMAT_NV61,
} image_fmt;

struct buffer {
        void *                  start;
        size_t                  length;
};

static char *           dev_name        = NULL;
static char *			sensor_regs_file_name = NULL;
static io_method	io		= IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;
static struct v4l2_format fmt;
static plane_org pl_org				= FORMAT_PLANAR;
static image_fmt format				= FORMAT_NV12;
static unsigned int width = 352 , hight = 288;
static unsigned int frame_per_second = 0;
static unsigned int rotate = 0;
/*osdm settings*/
unsigned char* osdm_frame_buf = NULL;
static int              fd_osdm_video	= -1;
struct fb_fix_screeninfo fb_fix;
static char*		osdm_dev_name	= NULL;

static FILE*				fd_y			= NULL;
static FILE*				fd_uv			= NULL;
static FILE*				fd_regs			= NULL;
const static char*		file_y		="out.y";
const static char*		file_uv		="out.uv";
const static char*		file_yuv		="out.yuv";

static unsigned int calibration_enable = 0;
static unsigned int count = 30;
static unsigned int count_shot = 0;
static unsigned int online_getset_enb = 0;
#define QXGA_SIZE 2048*1536
#define FRAME_SIZE (QXGA_SIZE*2+4)
unsigned static char frame[FRAME_SIZE];

void uninit_device (void);
void init_device (void);

static void
errno_exit                      (const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
        int r;

        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

int to_semiplanar(void)
{
	FILE *y_in;
	FILE *uv_in;
	FILE *yuv_out;
	int s;
	int i = 0;
	char input;
	char buf[1024];
	
    y_in = fopen(file_y, "rb");
	uv_in = fopen(file_uv, "rb");
	yuv_out = fopen(file_yuv, "wb");

	if ( y_in == NULL || uv_in == NULL || yuv_out == NULL) {
		printf("Error: can't open file\n");
		return -1;
	}

	// copy y_in to yuv_out
	while (!feof(y_in)) {
		s = fread(buf, 1, sizeof(buf), y_in);
		fwrite(buf, 1, s, yuv_out);
	}

	// copy y_in to yuv_out
	while (!feof(uv_in)) {
		s = fread(buf, 1, sizeof(buf), uv_in);
		fwrite(buf, 1, s, yuv_out);
	}


	fclose(y_in);
	fclose(uv_in);
	fclose(yuv_out);

	return 0;
}




int to_planar(void)
{
	FILE *y_in;
	FILE *uv_in;
	FILE *yuv_out;
	int s;
	int i = 0;
	char input;
	char buf[1024];
	
    y_in = fopen(file_y, "rb");
	uv_in = fopen(file_uv, "rb");
	yuv_out = fopen(file_yuv, "wb");

	if ( y_in == NULL || uv_in == NULL || yuv_out == NULL) {
		printf("Error: can't open file\n");
		return -1;
	}


	// copy y_in to yuv_out
	while (!feof(y_in)) {
		s = fread(buf, 1, sizeof(buf), y_in);
		fwrite(buf, 1, s, yuv_out);
	}

	fclose(y_in);

	// copy only u from uv_in
	while(!feof(uv_in)) {
		if (i%2 == 0)	{
			s = fread(&input, 1, 1, uv_in);
			if (s == 1) 
				fwrite(&input, 1, 1, yuv_out);
		}
		else {
			fread(&input, 1, 1, uv_in);
		}
		
		i++;
	}

	fseek( uv_in , 0, SEEK_SET);
	i = 0;
	
	while(!feof(uv_in)) {
		if (i%2 == 1)	{
			s = fread(&input, 1, 1, uv_in);
			if (s == 1) 
				fwrite(&input, 1, 1, yuv_out);
		}
		else {
			fread(&input, 1, 1, uv_in);
		}

		i++;
	}


	fclose(uv_in);
	fclose(yuv_out);

	return 0;

}

int osdm_set(int fd , unsigned int IOCTL_TYPE ,void* val)
{
	if (!val || fd == -1)
		return -1;

	if (fd == 0)
		return -1;

	if (ioctl(fd, IOCTL_TYPE, val) < 0) {
		perror("ioctl ");
		return -1;
	}

	return 0;
}

static void calibration (void)
{
	struct v4l2_dbg_register reg;
	unsigned char wb_r, wb_g, wb_b, awb;
	unsigned char cmd;
	
	if ((count & 0x3) == 0)
	{
		reg.reg = 0x332b;
		if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_G_REGISTER");
		awb = reg.val;

		reg.reg = 0x33a7;
		if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_G_REGISTER");
		wb_r = reg.val;
		
		reg.reg = 0x33a8;
		if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_G_REGISTER");
		wb_g = reg.val;

		reg.reg = 0x33a9;
		if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_G_REGISTER");
		wb_b = reg.val;
		
		printf ("Frame %d Current WB Setting: %s\tR 0x%02x | G 0x%02x | B 0x%02x\n", count, ((awb & 0x08)==0x08)? "Manual" : "Auto", wb_r, wb_g, wb_b);
		printf ("R | Add Red           r | Reduce Red\n"
				"G | Add Green         g | Reduce Green\n"
				"B | Add Blue          b | Reduce Blue\n"
				"A or a to toggle between auto and manual white balance setting\n"
				"K or k to skip setting in this frame\n"
				"");
		cmd = getchar ( ); getchar ( );
		switch (cmd)
		{
			case 'R':	if (wb_r < 0xff) wb_r++;	else printf ("Max\n");	break;
			case 'r':	if (wb_r > 0) wb_r--;		else printf ("Min\n");	break;
			case 'G':	if (wb_g < 0xff) wb_g++;	else printf ("Max\n");	break;
			case 'g':	if (wb_g > 0) wb_g--;		else printf ("Min\n");	break;
			case 'B':	if (wb_b < 0xff) wb_b++;	else printf ("Max\n");	break;
			case 'b':	if (wb_b > 0) wb_b--;		else printf ("Min\n");	break;
			case 'A':
			case 'a':
				reg.reg = 0x332b;
				if (awb & 0x08)
				{
					reg.val = 0x00;
					if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
						errno_exit ("VIDIOC_DBG_S_REGISTER");
				}
				else
				{
					reg.val = 0x08;
					if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
						errno_exit ("VIDIOC_DBG_S_REGISTER");
				}
				break;
				
			case 'k':
			case 'K':	printf ("Skip Setting in this frame\n");			break;
			default:	printf ("Error: Incorrect Key\n");break;
		}

	/*	reg.reg = 0x332b;
		reg.val = 0x08;
		if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_S_REGISTER");
	*/
		reg.reg = 0x33a7;
		reg.val = wb_r;
		if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_S_REGISTER");
		
		reg.reg = 0x33a8;
		reg.val = wb_g;
		if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_S_REGISTER");

		reg.reg = 0x33a9;
		reg.val = wb_b;
		if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_S_REGISTER");
	}
}

static void getset(void)
{
	unsigned int address;
	unsigned char value;
	struct v4l2_dbg_register reg;
	unsigned char cmd;
	static unsigned int count_n = 0;

	if( (count_n > 0) && count_n--)
		return;

	printf("\nPlease enter: 's' <reg addr> Set register\n");
	printf("	      'g' <reg addr> Get register\n");
	printf("	      'c' Continue some frames\n");
	printf("	      'f' Continue forever\n");
	printf("	      Enter for skip to next frame\n");

	cmd = getchar ( );
	switch (cmd)
	{
		case 'g':
			printf("\nPlease enter register address to get:");
			scanf("%x",&address);

			reg.reg = address;

			if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
						  errno_exit ("VIDIOC_DBG_G_REGISTER");

			printf("Register 0x%x=0x%x\n",reg.val,reg.reg);
			break;
		case 's':
			printf("\nPlease enter register address to set:");
			scanf("%x",&address);
	
			reg.reg = address;
	
			if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
						  errno_exit ("VIDIOC_DBG_G_REGISTER");
	
			printf("Register 0x%x=0x%x\n",reg.val,reg.reg);
	
			printf("Set new value:");
			scanf("%x",&value);
		
			reg.val = value;
			
			if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
					errno_exit ("VIDIOC_DBG_S_REGISTER");
		
			printf("Succes set register 0x%x=0x%x\n",reg.val,reg.reg);

			break;
		case 'f':
			online_getset_enb = 0;
			break;
		case 'c':
			printf("\nPlease enter number of frames to run:\n");
			scanf("%d",&count_n);	
			break;
		default:
			return;
			
	}
}

static void
process_image                   (const void *           p)
{
	
	static int i = 0;
	struct fb_var_screeninfo fb_var;
	memset (&fb_var, 0  , sizeof (fb_var) );

	if (!p) {
		printf("Capture application invalid input buffer\n");
		return;
	}

	if (calibration_enable)
		calibration ();

	if(online_getset_enb) {
		getset();
	}

    unsigned int image_size = (fmt.fmt.pix.width * fmt.fmt.pix.height * 6) / ((fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV16 || fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV61) ? 3 : 4);    

	
    if ( (buffers[0].length > 0) && (p != NULL) && (image_size > 0) && (count == 18) )
        memcpy(frame , p , image_size + 4 ); /*we copy 4 more byte in case of aligment*/


	if ( (fd_osdm_video != -1) && (image_size > 0))
	{
		if( (image_size + 4) > fb_fix.smem_len){
			printf("Error size of OSDM input buffer %d is too small for camera\n",image_size);
			exit(1);
		}

		/*Delivering to the OSDM the virtual adderss of the CIU buffer OSDM will do the virtual to physical translation*/
		fb_var.reserved[0] = PAN_OSDM_ZERO_COPY_MAGIC;
		fb_var.reserved[1] = (unsigned int)p;
		
		if ( osdm_set(fd_osdm_video, FBIOPAN_OSDM_DISPLAY, &fb_var) < 0)
			printf("Error OSDM mix \n");
	}

    fputc ('.', stdout);
    fflush (stdout);
	

	/*
	if (++i == 30)
		sleep(1);
	*/
	
}

static int
read_frame			(void)
{
        struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
    		if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("read");
			}
		}

    		process_image (buffers[0].start);

		break;

	case IO_METHOD_MMAP:
		CLEAR (buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

    	if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

					case EIO:
						/* Could ignore EIO, see spec. */

						/* fall through */
					default:
						errno_exit ("VIDIOC_DQBUF");
			}
		}

        assert (buf.index < n_buffers);

		//printf("buf.index = %d\n",buf.index);
        process_image (buffers[buf.index].start);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR (buf);

    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
			    && buf.length == buffers[i].length)
				break;

		assert (i < n_buffers);

    		process_image ((void *) buf.m.userptr);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;
	}

	return 1;
}

static void stream_off(enum v4l2_buf_type type)
{
	if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		printf("Invalid stream type\n");
		return;
	}

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
			errno_exit ("VIDIOC_STREAMOFF");

}


static void stream_on(enum v4l2_buf_type type)
{
	if (type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		printf("Invalid stream type\n");
		return;
	}

	if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");

}

static void
start_capturing                 (void)
{
        unsigned int i;
        enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR (buf);

			buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = i;
			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
						errno_exit ("VIDIOC_QBUF");
		}
		
		
		stream_on(V4L2_BUF_TYPE_VIDEO_CAPTURE);

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

			CLEAR (buf);

			buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr	= (unsigned long) buffers[i].start;
			buf.length      = buffers[i].length;

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    		errno_exit ("VIDIOC_QBUF");
		}

		stream_on(V4L2_BUF_TYPE_VIDEO_CAPTURE);

		break;
	}
}



static void
stop_capturing                  (void)
{
        enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		stream_off(V4L2_BUF_TYPE_VIDEO_CAPTURE);

		break;
	}
}

static void
mainloop                        (void)
{
	

		


        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO (&fds);
                        FD_SET (fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 4;
                        tv.tv_usec = 0;

                        r = select (fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                errno_exit ("select");
                        }

                        if (0 == r) {
                                fprintf (stderr, "select timeout\n");
                                exit (EXIT_FAILURE);
                        }

						if (count_shot != 0) {

							if (count == count_shot) {
								printf("\nstopping . . .\n");
								stop_capturing();
								uninit_device();
								//sleep(3);
								//while(1){}

								printf("\nreconfiguring . . .\n");
								
								fmt.fmt.pix.width = 400;
								fmt.fmt.pix.height = 300;
								//rotate = 90;

								init_device();  
								printf("\nstarting . . .\n");
								start_capturing();
								break;
							}
							
						}
												
			
						if (read_frame ())
                    		break;
						/* EAGAIN - continue select loop. */
						
                }
        }
}


static unsigned int  
video_format_and_packing(void)
{
	if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV16){ 
		return OSDM_YUV422_SEMI_PLANAR_V1_U1_V0_U0;
	}
	if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12){
		return OSDM_YUV420_MPEG2_SEMI_PLANAR_V1_U1_V0_U0; 
	}
	if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21){
		return OSDM_YUV420_MPEG1_JPEG_SEMI_PLANAR_U1_V1_U0_V0;
	}
	if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV61) {
		return OSDM_YUV422_SEMI_PLANAR_U1_V1_U0_V0;
	}
	else {
		printf("Can't configure OSDM invalid type pixelformat 0x%x\n", fmt.fmt.pix.pixelformat);
		return 0;
	}

	return 0;
}

static void close_osdm (void)
{

	if (osdm_dev_name && (fd_osdm_video != -1))
		if (-1 == close (fd_osdm_video))
			errno_exit ("close");

	fd_osdm_video = -1;

}

static void open_osdm (void)
{
	if (fd_osdm_video != -1) {
		printf("OSDM device is already open\n");
		return;
	}

	fd_osdm_video = open(osdm_dev_name, O_RDWR);
	if (-1 == fd_osdm_video) {
			fprintf (stderr, "Cannot open '%s': %d, %s\n",
					 osdm_dev_name, errno, strerror (errno));
			exit (EXIT_FAILURE);
	}
}

static int 
uninit_osdm(void)
{
	if ((fd_osdm_video != -1) && osdm_frame_buf)
		munmap(osdm_frame_buf,fb_fix.smem_len);

	close_osdm();

	return 0;
}

static int
init_osdm (void)
{
	unsigned int osdm_format;
	struct fb_var_screeninfo dim;
    struct osdm_position pos;
	int	enb =  1;

	memset(&dim,0,sizeof(struct fb_var_screeninfo));
	memset(&pos,0,sizeof(struct osdm_position));
	memset(&fb_fix,0, sizeof(struct fb_fix_screeninfo));

	if (fd_osdm_video != -1)
	{
		if (osdm_set(fd_osdm_video, FBIOGET_FSCREENINFO, &fb_fix) < 0)
			perror("ioctl FBIOGET_FSCREENINFO");
		
		osdm_frame_buf = mmap(NULL, fb_fix.smem_len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd_osdm_video, 0);
	
		if (osdm_frame_buf == NULL) {
			  printf("Fail map fbdevice %s physic address 0x%lx \n",osdm_dev_name, fb_fix.smem_start);
			  goto uninit_osdm_;
			  
		} else {
			/*printf("Success map fbdevice %s physic address 0x%lx virtual address %p len=%dKbyte\n",osdm_dev_name,fb_fix.smem_start , osdm_frame_buf , fb_fix.smem_len >> 10);*/
		}
	
		memset((void*)osdm_frame_buf,0,fb_fix.smem_len);
	

	osdm_format = video_format_and_packing();

	if (!osdm_format)
		goto uninit_osdm_;
	
	if ( osdm_set(fd_osdm_video, FBIOSET_OSDM_CONFIG, &osdm_format) < 0 )
		goto uninit_osdm_;

	if ( osdm_set(fd_osdm_video, FBIOSET_OSDM_POS, &pos) < 0 )
		goto uninit_osdm_;

	if (rotate == 90 || rotate == 270 || rotate == 180) {
		if (fmt.fmt.pix.width % 64 || fmt.fmt.pix.height % 64)
			printf("Image size not divided by 64, hence image might looks improler on OSDM\n");
		dim.xres = fmt.fmt.pix.width & ~0x3f;
		dim.yres = fmt.fmt.pix.height & ~0x3f;
	}
	else {
		dim.xres = fmt.fmt.pix.width;
		dim.yres = fmt.fmt.pix.height;
	}
	
	dim.rotate = rotate;

	if ( osdm_set(fd_osdm_video, FBIOPUT_OSDM_VSCREENINFO, &dim) < 0)
		goto uninit_osdm_;


	if ( osdm_set(fd_osdm_video, FBIOSET_OSDM_PLANE_ENB, &enb) < 0 )
		goto uninit_osdm_;

	}


	return 0;

uninit_osdm_:
	printf("Fail to set OSDM\n");
	uninit_osdm();

	return -1;
}


void
uninit_device                   (void)
{
        unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free (buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap (buffers[i].start, buffers[i].length))
				errno_exit ("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free (buffers[i].start);
		break;
	}
	uninit_osdm();

	free (buffers);
}

static void
init_read			(unsigned int		buffer_size)
{
        buffers = calloc (1, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);

	if (!buffers[0].start) {
    		fprintf (stderr, "Out of memory\n");
            	exit (EXIT_FAILURE);
	}
}

static void
init_mmap			(void)
{
	int tmp[10];

	struct v4l2_requestbuffers req;

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf (stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit (EXIT_FAILURE);
        }

        buffers = calloc (req.count, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit ("VIDIOC_QUERYBUF");
				
                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap (NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

				if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit ("mmap");

				if (-1 == xioctl (fd, VIDIOC_G_PHYSICAL, &buf))
                        errno_exit ("VIDIOC_G_PHYSICAL");

				//printf("buf[%d]=0x%x\n",n_buffers,buf.reserved);

        }
}

static void
init_userp			(unsigned int		buffer_size)
{
	struct v4l2_requestbuffers req;
        unsigned int page_size;

        page_size = getpagesize ();
        buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc (4, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                                     buffer_size);

                if (!buffers[n_buffers].start) {
    			fprintf (stderr, "Out of memory\n");
            		exit (EXIT_FAILURE);
		}
        }
}

static void
configure_device(void)
{
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control ctrl;
	struct v4l2_fmtdesc fmtdesc;
	CLEAR(queryctrl);
	CLEAR(ctrl);
	CLEAR(fmtdesc);

/*
	queryctrl.id = V4L2_CID_CONTRAST;
	
	
	if (-1 == xioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_QUERYCTRL");
			}
	}
	printf("name=%s mini=0x%x max=0x%x\n", queryctrl.name , queryctrl.minimum , queryctrl.maximum);
	
    ctrl.id = V4L2_CID_CONTRAST;
    if (-1 == xioctl (fd, VIDIOC_G_CTRL, &ctrl)) {
		
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_G_CTRL");
			}
			
	}
	printf("value=0x%x\n", ctrl.value);

	ctrl.value = queryctrl.maximum;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}

	ctrl.id = V4L2_CID_BRIGHTNESS;
	 ctrl.value = 6;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}

	ctrl.id = V4L2_CID_PRIVATE_BASE;
	 ctrl.value = 2;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}

	
	ctrl.id = V4L2_CID_HFLIP;
	ctrl.value = 0;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}
	*/

	/*
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmtdesc.index = 2;


	if (-1 == xioctl (fd, VIDIOC_ENUM_FMT , &fmtdesc)) {
		if (EINVAL == errno) {
				fprintf (stderr, "%s is no V4L2 device\n",
						 dev_name);
				exit (EXIT_FAILURE);
		} else {
				errno_exit ("VIDIOC_S_CTRL");
		}
	}

	printf("description = %s index= %d\n",fmtdesc.description , fmtdesc.index);
	*/


	ctrl.id = V4L2_CID_HFLIP;
	ctrl.value = 1;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}

	ctrl.id = V4L2_CID_VFLIP;
	ctrl.value = 1;

	if (-1 == xioctl (fd, VIDIOC_S_CTRL, &ctrl)) {
			if (EINVAL == errno) {
					fprintf (stderr, "%s is no V4L2 device\n",
							 dev_name);
					exit (EXIT_FAILURE);
			} else {
					errno_exit ("VIDIOC_S_CTRL");
			}
	}
	 
}


void
init_device                     (void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
		struct v4l2_streamparm streamparm;
		struct v4l2_frmivalenum frmi; 
		CLEAR(streamparm);
        //struct v4l2_format fmt;
	
		unsigned int min;

        if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf (stderr, "%s is no video capture device\n",
                         dev_name);
                exit (EXIT_FAILURE);
        }

        
	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf (stderr, "%s does not support read i/o\n",
				 dev_name);
			exit (EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n",
				 dev_name);
			exit (EXIT_FAILURE);
		}

		break;
	}


        /* Select video input, video standard and tune here. */


	CLEAR (cropcap);

		#if 0 
		TODO FIXME wehn crop will be supported
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; // reset to default 

				/*
				crop.c.height = 1000;
				crop.c.width = 256;
				crop.c.top = 64;
				crop.c.left = 200;
				*/
				
				/*
				crop.c.height = 128;
				crop.c.width = 128;
				crop.c.top = 64;
				crop.c.left = 64;
				*/

                if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                // Cropping not supported. 
                                break;
                        default:
                                // Errors ignored. 
                                break;
                        }
                }
        } else {	
                // Errors ignored. 
        }
		#endif


		fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

		/*
		printf("pix.width=%d pix.height=%d pix.bytesperline=%d pix.sizeimage=%d byte \n",
			   fmt.fmt.pix.width,
			   fmt.fmt.pix.height,
			   fmt.fmt.pix.bytesperline,
			   fmt.fmt.pix.sizeimage);
		*/

        if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
                errno_exit ("VIDIOC_S_FMT");

		//if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt))
		//        errno_exit ("VIDIOC_G_FMT");


		//fmt.fmt.pix.width = 160;
		//fmt.fmt.pix.height = 120;

		//fmt.fmt.pix.width = 320;
		//fmt.fmt.pix.height = 240;

		//fmt.fmt.pix.width = 80;
		//fmt.fmt.pix.height = 64;

        /* Note VIDIOC_S_FMT may change width and height. */

		/*
		printf("pix.width=%d pix.height=%d pix.bytesperline=%d pix.sizeimage=%d byte \n",
			   fmt.fmt.pix.width,
			   fmt.fmt.pix.height,
			   fmt.fmt.pix.bytesperline,
			   fmt.fmt.pix.sizeimage);
		*/
		
		frmi.pixel_format = fmt.fmt.pix.pixelformat;
			
		frmi.index = frame_per_second;
		frmi.width = fmt.fmt.pix.width;
		frmi.height = fmt.fmt.pix.height;
		
		if (-1 == xioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmi))
				errno_exit ("VIDIOC_ENUM_FRAMEINTERVALS");
		 
		
		streamparm.parm.capture.timeperframe.denominator = frmi.discrete.denominator;
		streamparm.parm.capture.timeperframe.numerator = frmi.discrete.numerator;
		streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_S_PARM, &streamparm))
				errno_exit ("VIDIOC_S_PARM");



	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read (fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap ();
		break;

	case IO_METHOD_USERPTR:
		init_userp (fmt.fmt.pix.sizeimage);
		break;
	}
	if (osdm_dev_name != NULL && fd_osdm_video == -1)
		open_osdm();

	if (fd_osdm_video != -1)
		init_osdm();

}

int parse_input_file(void)
{
	FILE *fr;            /* declare the file pointer */
	unsigned int i2c_addr, address , mask, regval;
	unsigned char value;
	char line[80];
	struct v4l2_dbg_register reg;
	
	if (!fd_regs)
		return -1;
	
	while(fgets(line, 80, fd_regs) != NULL)
	{
		i2c_addr = 0 ; address = 0 ; value = 0; mask = 0;
		/* get a line, up to 80 chars from fr.  done if NULL */
		sscanf (line, "%2x %4x %2x %2x", &i2c_addr, &address , &value, &mask);

		reg.reg = address;
		reg.val = value;

		if (mask) {
			 
			  if (-1 == xioctl (fd, VIDIOC_DBG_G_REGISTER, &reg))
				  errno_exit ("VIDIOC_DBG_G_REGISTER");
			  
			  reg.val &= ~(unsigned char)mask;
			  value &= mask;
			  value |= reg.val;
			  reg.val = value;
		}
		
		if (-1 == xioctl (fd, VIDIOC_DBG_S_REGISTER, &reg))
			errno_exit ("VIDIOC_DBG_S_REGISTER");

	}

	return 0;
} 

static void
dump_device						(void)
{
	unsigned int offset = 0;
	offset = DMW96CIU_GET_CHROMA_OFFSET(fmt.fmt.pix.width , fmt.fmt.pix.height);
	unsigned int luma_size = fmt.fmt.pix.width * fmt.fmt.pix.height;
	unsigned int chroma_size = ( (fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV16) || (fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV61)) ? luma_size : luma_size/2;
	//printf ("width= %d high = %d \n",fmt.fmt.pix.width,fmt.fmt.pix.height);
	//printf ("luma size= %d chroma size = %d \n",luma_size ,chroma_size);
	//printf ("offset = %d \n", offset);

	if( fwrite( frame , 1 , luma_size , fd_y) == 0) {
			printf("Write Luma error!\n");
			exit(1);
	}

	//printf("offset = %d \n", offset);


	if( fwrite( &frame[offset] , 1 , chroma_size , fd_uv) == 0) {
		printf("Write Chroma error!\n");
		exit(1);
	}


	if (-1 == fclose(fd_y))
		errno_exit("close");

	if (-1 == fclose(fd_uv))
		errno_exit("close");

	fd_y = NULL;
	fd_uv = NULL;

	if (pl_org == FORMAT_PLANAR)
	{
		to_planar();
		printf("\nImage is planar format\n");
		printf("\nyay -s %dx%d -f %d /opt/arm/target/out.yuv\n", fmt.fmt.pix.width , fmt.fmt.pix.height , (fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV16 || fmt.fmt.pix.pixelformat ==  V4L2_PIX_FMT_NV61 ) ? 2 : 1 );
	}
	else{
		to_semiplanar(); 
		printf("\nImage is semi planar format\n");
	}

}

static void
close_device                    (void)
{
		
		if (-1 == close (fd))
			errno_exit ("close");

		fd = -1;

		close_osdm();
		if (fd_regs)
			if (-1 == fclose(fd_regs))
				errno_exit("close");
		
		fd_regs = NULL;


}


static void
open_device                     (void)
{
        struct stat st; 

        if (-1 == stat (dev_name, &st)) {
                fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }

        if (!S_ISCHR (st.st_mode)) {
                fprintf (stderr, "%s is no device\n", dev_name);
                exit (EXIT_FAILURE);
        }

        fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }

		fd_y = fopen(file_y, "wb");
        if( fd_y == 0) {
			 fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         file_y, errno, strerror (errno));
             exit (EXIT_FAILURE);

		}

		fd_uv = fopen(file_uv, "wb");
        if( fd_uv == 0) {
			 fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         file_uv, errno, strerror (errno));
             exit (EXIT_FAILURE);

		}

		if (osdm_dev_name != NULL)
		{
				if (-1 == stat (osdm_dev_name, &st)) {
					fprintf (stderr, "Cannot identify '%s': %d, %s\n",
							 osdm_dev_name, errno, strerror (errno));
					exit (EXIT_FAILURE);
				}
		
				if (!S_ISCHR (st.st_mode)) {
						fprintf (stderr, "%s is no device\n", osdm_dev_name);
						exit (EXIT_FAILURE);
				}

				open_osdm();
		}
		
		if (sensor_regs_file_name)
		{
			fd_regs = fopen (sensor_regs_file_name, "rt");
	
			if (fd_regs == NULL) {
					fprintf (stderr, "Cannot open '%s': %d, %s\n",
							 sensor_regs_file_name, errno, strerror (errno));
					exit (EXIT_FAILURE);
			}
	
		}

}

static void
usage                           (FILE *                 fp,
                                 int                    argc,
                                 char **                argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-d | --device		Video device name [/dev/video]\n"
				 "-f | --format		0 - 420 (NV12)		1 - 422 (NV16)		2 - 420 (NV21)		3 - 422 (NV61)\n"
				 "-p | --organ		0 - planar	1 -	semiplanar\n"
				 "-s | --size		Width x Hight\n"
				 "-t | --fps		FPS enum (integer)\n"
				 "-i | --input 		The file\n"
				 "-e | --rotate		Select angle: 	90	180	 270\n"
				 "-l | --calibration	0 - without calibration	1 - with calibration\n"
				 "-c | --count		Number of frames to be captured\n"
				 "-x | --snap 		Frame number to snap shot (snap < count)\n"
				 "-o | --osdm 		OSDM support (as default open device main_video)\n"
				 "-g | --debug 		Get/Set register value for sensor\n"
                 "-h | --help		Print this message\n"
                 "-m | --mmap		Use memory mapped buffers\n"
                 "-r | --read		Use read() calls\n"
                 "-u | --userp		Use application allocated buffers\n"
                 "",
		 argv[0]);
}


static const char short_options [] = "d:f:p:s:t:i:e:c:l:o:p:ghmru";

static const struct option
long_options [] = {
        { "device",			required_argument,      NULL,           'd' },
		{ "format",			required_argument,		NULL,			'f' },
		{ "organ",			required_argument,		NULL,			'p' },
		{ "size",      		required_argument,		NULL,           's' },
		{ "fps",      		required_argument,		NULL,           't' },
		{ "input",     		required_argument,		NULL,           'i' },
		{ "rotate",     	required_argument,		NULL,           'e' },
		{ "count",     		required_argument,		NULL,           'c' },
        { "calibration",	required_argument,		NULL,			'l' },
		{ "osdm",     		required_argument,		NULL,           'o' },
		{ "snap",     		required_argument,		NULL,           'x' },
		{ "debug",    		no_argument,			NULL,           'g' },
        { "help",       	no_argument,            NULL,           'h' },
        { "mmap",       	no_argument,            NULL,           'm' },
        { "read",       	no_argument,            NULL,           'r' },
        { "userp",      	no_argument,            NULL,           'u' },
        { 0, 0, 0, 0 }
};

int
main                          (int                    argc,
                                 char **                argv)
{
        dev_name = "/dev/video";

        for (;;) {
                int index;
                int c;
                
                c = getopt_long (argc, argv,
                                 short_options, long_options,
                                 &index);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'f':
                        format = atoi(optarg);
			if (FORMAT_NV12 == format) {
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
			}
			else if (FORMAT_NV16 == format) {
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV16;
			}
			else if (FORMAT_NV21 == format) {
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
			}
			else if (FORMAT_NV61 == format) {
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV61;
			}
			else{
				fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
			}	
			break;

                case 'p':
                        pl_org = (atoi(optarg) == FORMAT_SEMIPLANAR) ? FORMAT_SEMIPLANAR : FORMAT_PLANAR;
						break;
	
				case 's':
						if (sscanf(optarg, "%dx%d", &width, &hight) != 2) {
							fprintf(stdout, "No geometry information provided by -s parameter.\n");
							return 1;
						}
						fmt.fmt.pix.width       =  width; 
						fmt.fmt.pix.height      =  hight;
						break;
				case 't':
						frame_per_second = atoi(optarg);
						break;
				case 'i':
						sensor_regs_file_name = optarg;
						break;
				case 'e':
						rotate = atoi(optarg);
						break;
				case 'l':
						printf ("Colour Calibration Mode\n");
						calibration_enable = atoi (optarg);
						break;
				case 'c':
						count = atoi(optarg);
						break;
				case 'x':
						count_shot = atoi(optarg);
						if (count_shot && count_shot >= count) {
							printf("One Shot frame is smaller then count\n",count_shot,count);
							return 1;
						}
						break;
				case 'o':
						if (optarg == NULL)
							osdm_dev_name = "/dev/main_video";
						else
							osdm_dev_name = optarg;
						break;
				case 'g':
						online_getset_enb = 1;
						break;
                case 'h':
                        usage (stdout, argc, argv);
                        exit (EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
						break;

                case 'r':
                        io = IO_METHOD_READ;
						break;

                case 'u':
                        io = IO_METHOD_USERPTR;
						break;

                default:
                        usage (stderr, argc, argv);
                        exit (EXIT_FAILURE);
                }
        }

        open_device ();

        init_device ();
		
		//configure_device();

        start_capturing ();

		parse_input_file();

        mainloop ();

        stop_capturing ();

        uninit_device ();

		dump_device();

        close_device ();

		exit (EXIT_SUCCESS);

        return 0;
}

