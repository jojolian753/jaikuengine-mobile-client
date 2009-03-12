#include <zlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

void childhandler(int data) 
{
	int status;
	pid_t pid;
	for(;;) {
		pid=waitpid(-1, &status, WNOHANG);
		if (pid<=0) break;
	}
}

void set_child_handler()
{
	signal(SIGCHLD, &childhandler);
}

int init_z_streams(
	z_stream*	in_z_stream, 
	z_stream* out_z_stream)
{
	int ret;

	ret=Z_OK;
	in_z_stream->zalloc = Z_NULL;
	in_z_stream->zfree = Z_NULL;
	in_z_stream->opaque = Z_NULL;
	ret = inflateInit(in_z_stream);
	if (ret != Z_OK) {
		fprintf(stderr, "error in inflateInit %d\n", ret);
		return -1;
	}
		
	out_z_stream->zalloc = Z_NULL;
	out_z_stream->zfree = Z_NULL;
	out_z_stream->opaque = Z_NULL;
	ret = deflateInit(out_z_stream, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK) {
		fprintf(stderr, "error out deflateInit %d\n", ret);
		inflateEnd(in_z_stream);
		return -1;
	}

	return 0;
}
	
int write_buf(int fd, char *buf, int len)
{
	size_t write_bytes;

	while(len > 0) {
		write_bytes=write(fd, 
			buf, len);
		if (write_bytes == -1) {
			perror("error in write");
			return -1;
		}
		len -= write_bytes;
		buf += write_bytes;
	}
	return 0;
}

int handle_buffer(int from_fd, int to_fd,
	int* is_first, int* is_z, int do_compress,
	z_stream*	in_z_stream, 
	z_stream* out_z_stream)
{
	size_t read_bytes;
	char read_buf[8192];
	char z_buf[8192];
	int err;

	read_bytes=read(from_fd, read_buf, 8192);
	if (read_bytes == -1) {
		perror("error in read");
		goto error;
	}
	if (read_bytes == 0) {
		/* EOF */
		return -1;
	}

	if (*is_first) {
		*is_first=0;
		if (read_buf[0]!='<') {
			if (init_z_streams(in_z_stream, out_z_stream)==-1) {
				return -1;
			}
			*is_z=1;
			fprintf(stderr, "doing compressed stream\n");
		}
	}

	if (! *is_z ) {
		return write_buf(to_fd, read_buf, read_bytes);
	} else {
		if (do_compress) {
			/*write(2, read_buf, read_bytes);*/
			out_z_stream->avail_in = read_bytes;
			out_z_stream->next_in = read_buf;
			do {
				out_z_stream->avail_out = 8192;
				out_z_stream->next_out = z_buf;
				err = deflate(out_z_stream, Z_SYNC_FLUSH);
				if (err==Z_NEED_DICT || err==Z_DATA_ERROR || err==Z_MEM_ERROR) {
					goto error;
				}
				if (write_buf(to_fd, z_buf, 8192-out_z_stream->avail_out)==-1) {
					return -1;
				}
			} while ( out_z_stream->avail_out== 0);
		} else {
			in_z_stream->avail_in = read_bytes;
			in_z_stream->next_in = read_buf;
			do {
				in_z_stream->avail_out = 8192;
				in_z_stream->next_out = z_buf;
				err = inflate(in_z_stream, Z_SYNC_FLUSH);
				if (err==Z_NEED_DICT || err==Z_DATA_ERROR || err==Z_MEM_ERROR) {
					goto error;
				}
				/*write(2, z_buf, 8192-in_z_stream->avail_out);*/
				if (write_buf(to_fd, z_buf, 8192-in_z_stream->avail_out)==-1) {
					return -1;
				}
			} while(in_z_stream->avail_out==0);
		}
	}

		
	return 0;
error:
	return -1;
}

void child(int incoming_fd, int connect_port)
{
	int connect_socket, err, max_fd;
	struct sockaddr_in connect_addr; 
	fd_set rfds, efds;
	struct timeval tv;
	int is_first, is_z;
	z_stream in_z_stream; 
	z_stream out_z_stream;

	is_first=1; is_z=0;

	connect_socket=socket(PF_INET, SOCK_STREAM, 0);
	if (connect_socket < 0) {
		perror("cannot create socket");
		exit(1);
	}
	
	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = htons(connect_port);
	connect_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* fprintf(stderr, "connecting to %d", connect_port); */
	err=connect(connect_socket, (struct sockaddr *)&connect_addr,
		sizeof(connect_addr));
	if (err<0) {
		perror("cannot connect to localhost on port");
		close(connect_socket);
		exit(2);
	}
	fprintf(stderr, "\n");
	
	max_fd=incoming_fd;
	if (connect_socket>max_fd) max_fd=connect_socket;
	++max_fd;
	
	for(;;) {
		FD_ZERO(&rfds);
		FD_SET(incoming_fd, &rfds);
		FD_SET(connect_socket, &rfds);
		FD_ZERO(&efds);
		FD_SET(incoming_fd, &efds);
		FD_SET(connect_socket, &efds);
		tv.tv_sec = 60*60;
		tv.tv_usec = 0;
		err=select(max_fd, &rfds, NULL, &efds, &tv);
		if (err == -1) {
			perror("error in select");
			goto error;
		}
		if (FD_ISSET(incoming_fd, &efds) ||
				FD_ISSET(connect_socket, &efds) ) {
			goto error;
		}
		if (FD_ISSET(incoming_fd, &rfds)) {
			/*fprintf(stderr, "read from incoming\n");*/
			if (handle_buffer(incoming_fd, connect_socket,
				&is_first, &is_z, 0, &in_z_stream, &out_z_stream)==-1) goto error;
		}
		if (FD_ISSET(connect_socket, &rfds)) {
			/* fprintf(stderr, "read from outgoing\n");*/
			if (handle_buffer(connect_socket, incoming_fd,
				&is_first, &is_z, 1, &in_z_stream, &out_z_stream)==-1) goto error;
		}
	}
error:
	if (is_z) {
		inflateEnd(&in_z_stream);
		deflateEnd(&out_z_stream);
	}
	fprintf(stderr, "closing connections\n");
	close(incoming_fd);
	close(connect_socket);
	exit(1);
}

int listenloop(int incoming_port, int connect_port)
{
	int listen_socket, incoming_socket;
	int err;
	struct sockaddr_in listen_addr;
	struct sockaddr_in incoming_addr;
	int yes; socklen_t socklen;
	pid_t child_pid;
	
	listen_socket=socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket < 0) {
		perror("cannot create listen socket on port");
		return 3;
	}
	yes=1;
	if (setsockopt(listen_socket,
			SOL_SOCKET,SO_REUSEADDR,
			&yes,sizeof(int)) == -1) {
		close(listen_socket);
		perror("cannot set socket reuse option");
		return 4;
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(incoming_port);
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	err=bind(listen_socket, (struct sockaddr *)&listen_addr, 
		sizeof(listen_addr));
	if (err < 0) {
		close(listen_socket);
		perror("cannot bind socket");
		return 5;
	}
	
	err=listen(listen_socket, 10);
	if (err<0) {
		close(listen_socket);
		perror("cannot listen on socket");
		return 6;
	}
	fprintf(stderr, "listening on port %d\n", incoming_port);
	for(;;) {
		socklen=sizeof(incoming_addr);
		incoming_socket=accept(listen_socket, 
			(struct sockaddr *)&incoming_addr,
			&socklen);
		if (incoming_socket<0) {
			if (errno==EINTR) break;
			perror("error in accept()");
			close(listen_socket);
			return 7;
		}
		fprintf(stderr, "incoming connection\n");
		child_pid=fork();
		if (child_pid < 0) {
			perror("cannot fork");
			return 8;
		}
		if (child_pid==0) {
			/* child
			fprintf(stderr, "child\n"); */
			close(listen_socket);
			child(incoming_socket, connect_port);
			return 0;
		} else {
			/* parent
			fprintf(stderr, "parent\n"); */
			close(incoming_socket);
		}
	}
	return 0;
}

void usage()
{
	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "zproxy listen_on_port connect_to_port\n");
}

int main(int argc, char** argv) {
	int listen_on_port, connect_to_port;
	if (argc<3) {
		usage();
		exit(1);
	}
	listen_on_port=atoi(argv[1]);
	connect_to_port=atoi(argv[2]);
	if (! listen_on_port || ! connect_to_port ) {
		fprintf(stderr, "ports must be "
			"numeric and non-zero\n\n");
		usage();
		exit(2);
	}

	set_child_handler();
	return listenloop(listen_on_port, connect_to_port);
}
