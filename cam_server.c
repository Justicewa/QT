#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#define	REQBUFS_COUNT	4

struct cam_buf {
	void *start;
	size_t length;
};

struct v4l2_requestbuffers reqbufs;
struct cam_buf bufs[REQBUFS_COUNT];

int camera_init(char *devpath, unsigned int *width, unsigned int *height, unsigned int *size, unsigned int *ismjpeg)
{
	int i;
	int fd = -1;;
	int ret;
	struct v4l2_buffer vbuf;
	struct v4l2_format format;
	struct v4l2_capability capability;
	/*open 打开设备文件*/
	if((fd = open(devpath, O_RDWR)) == -1){
		perror("open:");	
		return -1;		
	}
	/*ioctl 查看支持的驱动*/
	ret = ioctl(fd, VIDIOC_QUERYCAP, &capability);
	if (ret == -1) {
		perror("camera->init");
		return -1;
	}
	/*判断设备是否支持视频采集*/
	if(!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "camera->init: device can not support V4L2_CAP_VIDEO_CAPTURE\n");
		close(fd);
		return -1;
	}
	/*判断设备是否支持视频流采集*/
	if(!(capability.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "camera->init: device can not support V4L2_CAP_STREAMING\n");
		close(fd);
		return -1;
	}
	/*设置捕获的视频格式 MYJPEG*/

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = *width;
	format.fmt.pix.height = *height;
	format.fmt.pix.field = V4L2_FIELD_ANY;
	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if(ret == -1) {
		perror("camera init");
		return -1;
	} else {
		*ismjpeg = 0;
		fprintf(stdout, "camera->init: picture format is mjpeg\n");
		goto get_fmt;
	}
	/*设置捕获的视频格式 YUYV*/
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;			//永远都是这个类型
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;		//设置采集图片的格式
	format.fmt.pix.width = *width;
	format.fmt.pix.height = *height;
	format.fmt.pix.field = V4L2_FIELD_ANY;				//设置图片一行一行的采集
	ret = ioctl(fd, VIDIOC_S_FMT, &format);				//ioctl	是设置生效
	if(ret == -1)
		perror("camera init");
	else {
		fprintf(stdout, "camera->init: picture format is yuyv\n");
		*ismjpeg = 1;
		goto get_fmt;
	}

get_fmt:
	ret = ioctl(fd, VIDIOC_G_FMT, &format);
	if (ret == -1) {
		perror("camera init");
		return -1;
	}
	/*向驱动申请缓存*/
	memset(&reqbufs, 0, sizeof(struct v4l2_requestbuffers));
	reqbufs.count	= REQBUFS_COUNT;					//缓存区个数
	reqbufs.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbufs.memory	= V4L2_MEMORY_MMAP;					//设置操作申请缓存的方式:映射 MMAP
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);			
	if (ret == -1) {	
		perror("camera init");
		close(fd);
		return -1;
	}
	/*循环映射并入队*/
	for (i = 0; i < reqbufs.count; i++){
		/*真正获取缓存的地址大小*/
		memset(&vbuf, 0, sizeof(struct v4l2_buffer));
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.index = i;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &vbuf);
		if (ret == -1) {
			perror("camera init");
			close(fd);
			return -1;
		}
		/*映射缓存到用户空间,通过mmap讲内核的缓存地址映射到用户空间,并切和文件描述符fd相关联*/
		bufs[i].length = vbuf.length;
		bufs[i].start = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, vbuf.m.offset);
		if (bufs[i].start == MAP_FAILED) {
			perror("camera init");
			close(fd);
			return -1;
		}
		/*每次映射都会入队,放入缓冲队列*/
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_QBUF, &vbuf);
		if (ret == -1) {
			perror("camera init");
			close(fd);
			return -1;
		}
	}
	/*返回真正设置成功的宽.高.大小*/
	*width = format.fmt.pix.width;
	*height = format.fmt.pix.height;
	*size = bufs[0].length;

	return fd;
}

int camera_start(int fd)
{
	int ret;
	
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/*ioctl控制摄像头开始采集*/
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret == -1) {
		perror("camera->start");
		return -1;
	}
	fprintf(stdout, "camera->start: start capture\n");

	return 0;
}

int camera_dqbuf(int fd, void **buf, unsigned int *size, unsigned int *index)
{
	int ret;
	fd_set fds;
	struct timeval timeout;
	struct v4l2_buffer vbuf;

	while (1) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		ret = select(fd + 1, &fds, NULL, NULL, &timeout);	//使用select机制,保证fd有图片的时候才能出对
		if (ret == -1) {
			perror("camera->dbytesusedqbuf");
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (ret == 0) {
			fprintf(stderr, "camera->dqbuf: dequeue buffer timeout\n");
			return -1;
		} else {
			vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			vbuf.memory = V4L2_MEMORY_MMAP;
			ret = ioctl(fd, VIDIOC_DQBUF, &vbuf);	//出队,也就是从用户空间取出图片
			if (ret == -1) {
				perror("camera->dqbuf");
				return -1;
			}
			*buf = bufs[vbuf.index].start;
			*size = vbuf.bytesused;
			*index = vbuf.index;

			return 0;
		}
	}
}

int camera_eqbuf(int fd, unsigned int index)
{
	int ret;
	struct v4l2_buffer vbuf;

	vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf.memory = V4L2_MEMORY_MMAP;
	vbuf.index = index;
	ret = ioctl(fd, VIDIOC_QBUF, &vbuf);		//入队
	if (ret == -1) {
		perror("camera->eqbuf");
		return -1;
	}

	return 0;
}

int camera_stop(int fd)
{
	int ret;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret == -1) {
		perror("camera->stop");
		return -1;
	}
	fprintf(stdout, "camera->stop: stop capture\n");

	return 0;
}

int camera_exit(int fd)
{
	int i;
	int ret;
	struct v4l2_buffer vbuf;
	vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf.memory = V4L2_MEMORY_MMAP;
	for (i = 0; i < reqbufs.count; i++) {
		ret = ioctl(fd, VIDIOC_DQBUF, &vbuf);
		if (ret == -1)
			break;
	}
	for (i = 0; i < reqbufs.count; i++)
		munmap(bufs[i].start, bufs[i].length);
	fprintf(stdout, "camera->exit: camera exit\n");
	return close(fd);
}

int main()
{
	
	int ser_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(ser_sock < 0)
        {
                perror("socket");
                return 1;
        }

        struct sockaddr_in addr={0}, peer = {0};
	int peer_len = sizeof(peer);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);
        addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        if(bind(ser_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
                perror("bind");
                return 2;
        }

        int listen_sock = listen(ser_sock, 5);
        if(listen_sock < 0)
        {
                perror("listen");
                return 3;
        }


	int i;
	int fd;
	int ret;
	unsigned int width;
	unsigned int height;
	unsigned int size;
	unsigned int index;
	unsigned int ismjpeg;

	width = 320;
	height = 240;
	fd = camera_init("/dev/video0", &width, &height, &size, &ismjpeg);
	if (fd == -1)
		return -1;
	ret = camera_start(fd);
	if (ret == -1)
		return -1;
	// 采集几张图片丢弃 
	char *jpeg_ptr = NULL;
	for (i = 0; i < 8; i++) {
		ret = camera_dqbuf(fd, (void **)&jpeg_ptr, &size, &index);
		if (ret == -1)
			exit(EXIT_FAILURE);

		ret = camera_eqbuf(fd, index);
		if (ret == -1)
			exit(EXIT_FAILURE);
	}
	signal(SIGPIPE, SIG_IGN);
	fprintf(stdout, "init camera success\n");
	while(1)
	{
		printf("wating!\n");
		char buf[1024];
		int clientfd = accept(ser_sock, (struct sockaddr*)&peer, &peer_len);
		if(clientfd < 0)
		{
			perror("accept");
			return 4;
		}
		else
			printf("video connected with ip: %s  and port: %d\n", inet_ntop(AF_INET,&peer.sin_addr, buf, 1024), ntohs(peer.sin_port));



		/* 循环采集图片 */
		char sizes[10]={0};
		while (1) {
			ret = camera_dqbuf(fd, (void **)&jpeg_ptr, &size, &index);
			if (ret == -1)
			{
				perror("dqbuf");
				return -1;
			}

		//	char sizes[10] = {0};
			memset(sizes,0,10);
			sprintf(sizes, "%d", size);
			printf("size:%d,%s\r\n",size,sizes);
			int ret = write(clientfd, sizes, 10); //发送照片大小
			if(-1 == ret || 0 == ret)
			{
				printf("write size error\r\n");
				close(clientfd);
				ret = camera_eqbuf(fd, index);
				break;
			}
					printf("write size: %d\n", ret);
			ret = write(clientfd, jpeg_ptr, size);//发送照片内容
			if(-1 == ret || 0 == ret)
			{
				printf("write-------error,size=%d\r\n",size);
				close(clientfd);
				ret = camera_eqbuf(fd, index);
				break;
			}
            printf("----ret:%d\r\n",ret);
			ret = camera_eqbuf(fd, index);
			if (ret == -1)
			{
				perror("eqbuf");
				return -1;
			}
	//		usleep(100000);
		}
		printf("ended !\n");

	}
		ret = camera_stop(fd);
		if (ret == -1)
			return -1;

		ret = camera_exit(fd);
		if (ret == -1)
			return -1;
}
