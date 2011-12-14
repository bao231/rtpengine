#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>

#include "xt_MEDIAPROXY.h"

#include "aux.h"
#include "kernel.h"




#if 1
#define PREFIX "/proc/mediaproxy"
#else
#define PREFIX "/tmp/mediaproxy"
#endif





int kernel_create_table(unsigned int id) {
	char str[64];
	int fd;
	int i;

	fd = open(PREFIX "/control", O_WRONLY | O_TRUNC);
	if (fd == -1)
		return -1;
	sprintf(str, "add %u\n", id);
	i = write(fd, str, strlen(str));
	if (i == -1)
		goto fail;
	close(fd);

	return 0;

fail:
	close(fd);
	return -1;
}


int kernel_open_table(unsigned int id) {
	char str[64];
	int fd;
	struct mediaproxy_message msg;
	int i;

	sprintf(str, PREFIX "/%u/control", id);
	fd = open(str, O_WRONLY | O_TRUNC);
	if (fd == -1)
		return -1;

	ZERO(msg);
	msg.cmd = MMG_NOOP;
	i = write(fd, &msg, sizeof(msg));
	if (i <= 0)
		goto fail;

	return fd;

fail:
	close(fd);
	return -1;
}


static void addr_copy(struct mp_address *mp, struct ip_port *ap) {
	mp->family = ap->family;
	mp->port = ap->port;
	switch (mp->family) {
		case AF_INET:
			mp->ipv4 = ap->ipv4;
			break;
		case AF_INET6:
			memcpy(mp->ipv6, ap->ipv6, 16);
			break;
		default:
			/* XXX panic */
			break;
	}
}


int kernel_add_stream(int fd, struct kernel_stream *info, int update) {
	struct mediaproxy_message msg;

	ZERO(msg);
	msg.cmd = update ? MMG_UPDATE : MMG_ADD;
	msg.target.target_port = info->local_port;
	addr_copy(&msg.target.src_addr, &info->src);
	addr_copy(&msg.target.dst_addr, &info->dest);
	addr_copy(&msg.target.mirror_addr, &info->mirror);
	msg.target.tos = info->tos;

	return write(fd, &msg, sizeof(msg)) <= 0 ? -1 : 0;
}


int kernel_del_stream(int fd, u_int16_t p) {
	struct mediaproxy_message msg;

	ZERO(msg);
	msg.cmd = MMG_DEL;
	msg.target.target_port = p;

	return write(fd, &msg, sizeof(msg)) <= 0 ? -1 : 0;
}


GList *kernel_list(unsigned int id) {
	char str[64];
	int fd;
	struct mediaproxy_list_entry *buf;
	GList *li = NULL;
	int ret;

	sprintf(str, PREFIX "/%u/blist", id);
	fd = open(str, O_RDONLY);
	if (fd == -1)
		return NULL;


	for (;;) {
		buf = g_slice_alloc(sizeof(*buf));
		ret = read(fd, buf, sizeof(*buf));
		if (ret != sizeof(*buf))
			break;
		li = g_list_prepend(li, buf);
	}

	g_slice_free1(sizeof(*buf), buf);
	close(fd);

	return li;
}
