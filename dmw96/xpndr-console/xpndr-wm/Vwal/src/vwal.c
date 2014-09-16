/*
 *  Copyright (C) 2001-2011 DSP GROUP, INC. All Rights Reserved
 *
 * Portions of this code are based on hostap's drver_nl80211
 * 	Driver interaction with Linux nl80211/cfg80211
 * 	Copyright (c) 2002-2010, Jouni Malinen <j@w1.fi>
 * 	Copyright (c) 2003-2004, Instant802 Networks, Inc.
 * 	Copyright (c) 2005-2006, Devicescape Software, Inc.
 * 	Copyright (c) 2007, Johannes Berg <johannes@sipsolutions.net>
 * 	Copyright (c) 2009-2010, Atheros Communications
 *
 * 	This program is free software; you can redistribute it and/or modify it
 * 	under the terms of the GNU General Public License version 2 as
 * 	published by the Free Software Foundation.
 *
 * 	Alternatively, this software may be distributed under the terms of BSD
 * 	license.
 */

#include <errno.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "nl80211_copy.h"
#include "WifiApi.h"

static struct vwal_context {
	struct nl_cb *nl_cb;
	struct nl_handle *nl_handle;
	struct nl_cache *nl_cache;
	struct genl_family *nl80211;
	unsigned int wiphy;
	int ifindex;
	char ifname[IFNAMSIZ];
	int ioctl_sock;
	int loaded;
} g_ctx[1] = {
	[0] = {
		.nl_handle = NULL,
		.nl_cache = NULL,
		.nl80211 = NULL,
		.loaded = 0,
	}
};

#ifdef ANDROID
/*
 * The structure to exchange data for ioctl.
 * This structure is the same as 'struct ifreq', but (re)defined for
 * convenience...
 * Do I need to remind you about structure size (32 octets) ?
 */
struct ifreq_andr
{
 union
 {
 char ifrn_name[IFNAMSIZ];
 } ifr_ifrn;

 union {
  short ifru_flags;
 } ifr_ifru;
};

# define ifr_name	ifr_ifrn.ifrn_name	/* interface name 	*/
# define ifr_flags	ifr_ifru.ifru_flags	/* flags		*/

#endif

#ifndef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */

static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_handle *h)
{
	nl_handle_destroy(h);
}

static inline int __genl_ctrl_alloc_cache(struct nl_handle *h, struct nl_cache **cache)
{
	struct nl_cache *tmp = genl_ctrl_alloc_cache(h);
	if (!tmp)
		return -ENOMEM;
	*cache = tmp;
	return 0;
}
#define genl_ctrl_alloc_cache __genl_ctrl_alloc_cache
#endif /* CONFIG_LIBNL20 */          

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int send_and_recv_msgs(struct vwal_context *ctx, struct nl_msg *msg,
			 int (*valid_handler)(struct nl_msg *, void *),
			 void *valid_data)
{
	struct nl_cb *cb;
	int err = -ENOMEM;

	cb = nl_cb_clone(ctx->nl_cb);
	if (!cb)
		goto error;

	err = nl_send_auto_complete(ctx->nl_handle, msg);
	if (err < 0)
		goto error;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	if (valid_handler)
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
			  valid_handler, valid_data);

	while (err > 0)
		nl_recvmsgs(ctx->nl_handle, cb);
error:
	nl_cb_put(cb);
	nlmsg_free(msg);
	return err;
}


static int testmode_reply_handler(struct nl_msg *msg, void *arg)
{
	wamHdr2_t *wamMsg = arg;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_TESTDATA]) {
		/* TODO use nla_parse_nested */
		wamHdr2_t *ret_wamMsg = (wamHdr2_t *)(nla_data(tb[NL80211_ATTR_TESTDATA]) + NLA_HDRLEN);
		wamMsg->status = ret_wamMsg->status;
		wamMsg->num = ret_wamMsg->num;
	}

	return NL_SKIP;
}

static int nl80211_testmode_send_and_recv(struct vwal_context *ctx, uint8_t cmd, void *data, uint8_t *num)
{
	struct nl_msg *msg;
	struct nl_data *nl_data;
	wamHdr2_t wamMsg = {
		.version = WIFIAPIVERSION,
		.cmd = cmd,
		.num = *num,
		.data = data,
		.status = WIFI_FAIL,
	};

	msg = nlmsg_alloc();
	if (!msg)
		goto error;

	nl_data = nl_data_alloc(&wamMsg, sizeof(wamMsg));

	genlmsg_put(msg, 0, 0, genl_family_get_id(ctx->nl80211), 0, 0,
		    NL80211_CMD_TESTMODE, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, ctx->wiphy);
	NLA_PUT(msg, NL80211_ATTR_TESTDATA, nl_data_get_size(nl_data), nl_data_get(nl_data));

	if (send_and_recv_msgs(ctx, msg, testmode_reply_handler, &wamMsg) == 0) {
		if (wamMsg.status != WIFI_SUCCESS)
			goto error;

		*num =  wamMsg.num;
		return wamMsg.status;
	}

nla_put_failure:
	nlmsg_free(msg);
error:
	*num = 0;
	return wamMsg.status;
}

uint32 WifiQosAPI(void *device, WIFI_STREAM * stream)
{
	/* v2 does not support QosAPI */
	return WIFI_INVALID_PARAMS;
}

uint32 Wifi80211APISendPkt(void *device, TX_WIFI_PACKET_DESC * wifiPacket)
{
	struct vwal_context *ctx = g_ctx;
	uint8_t num = 1;

	if (!ctx->loaded)
		return WIFI_80211_DEVICE_NOT_READY;

	if (!wifiPacket)
		return WIFI_INVALID_PARAMS;

	return nl80211_testmode_send_and_recv(ctx, WIFI_CMD_80211_TX,
			wifiPacket, &num);
}

uint32 Wifi80211APIReceivePkt(void *device, RX_WIFI_PACKET_DESC rxDesc[],
			      uint8 * numRxDesc)
{
	struct vwal_context *ctx = g_ctx;

	if (!ctx->loaded)
		return WIFI_80211_DEVICE_NOT_READY;

	if (!rxDesc || !numRxDesc || *numRxDesc == 0)
		return WIFI_INVALID_PARAMS;

	return nl80211_testmode_send_and_recv(ctx, WIFI_CMD_80211_RX,
			rxDesc, numRxDesc);
}

static int nl80211_set_freq(struct vwal_context *ctx, int freq, int ht_enabled)
{
	struct nl_msg *msg;
	int ret;

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	genlmsg_put(msg, 0, 0, genl_family_get_id(ctx->nl80211), 0, 0,
		    NL80211_CMD_SET_WIPHY, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex);
	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
	if (ht_enabled) {
		NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE,
				NL80211_CHAN_HT20);
	}

	ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
	if (ret == 0)
		return 0;
	fprintf(stderr, "nl80211: Failed to set channel (freq=%d): "
		   "%d (%s)", freq, ret, strerror(-ret));
nla_put_failure:
	return -1;
}

static uint32_t compat_MIBOID_CURRENTCHANNELFREQUENCY(struct vwal_context *ctx, mibObjectContainer_t *moc)
{
	uint32_t ret;
	uint32_t freq;

	if (moc->operation == WIFI_MIB_API_OPERATION_WRITE) {
		freq = *(uint32_t*)moc->mibObject;
		/* TODO DWF should always use HT channel type regardless of
		 * NL80211 type? */
		if (nl80211_set_freq(ctx, freq, 0) < 0)
			ret = WIFI_FAIL;
		else
			ret = WIFI_SUCCESS;
	} else {
		FILE *file = fopen("/sys/kernel/debug/ieee80211/phy0/frequency", "r");
		if (!file) {
			ret = WIFI_80211_DEVICE_NOT_READY;
			goto error;
		}

		if (fscanf(file, "%u", &freq) == 1)
			ret = WIFI_SUCCESS;
		else
			ret = WIFI_80211_DEVICE_NOT_READY;

		*(uint32_t*)moc->mibObject = freq;

		fclose(file);
	}
error:
	return ret;
}

static int psmode_reply_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!attrs[NL80211_ATTR_PS_STATE])
		return NL_SKIP;

	switch (*(uint32_t *) nla_data(attrs[NL80211_ATTR_PS_STATE])) {
	case NL80211_PS_ENABLED:
		*(uint32*)arg = NL80211_PS_ENABLED;
		break;
	case NL80211_PS_DISABLED:
		*(uint32*)arg = NL80211_PS_DISABLED;
		break;
	default:
		*(uint32*)arg = NL80211_PS_DISABLED;
		 fprintf(stderr, "nl80211 warning: Failed to GET Power state");
		break;
	}

	return NL_OK;
}

static uint32_t compat_MIBOID_POWERMGMT_MODE(struct vwal_context *ctx, mibObjectContainer_t *moc)
{
	uint32_t psMode;
    struct nl_msg *msg;
    int32 ret;
	struct nl_data *nl_data;

    msg = nlmsg_alloc();
	if (!msg)
		return WIFI_FAIL;

	if (moc->operation == WIFI_MIB_API_OPERATION_WRITE) {
		psMode = *(uint32_t*)moc->mibObject;

        genlmsg_put(msg, 0, 0, genl_family_get_id(ctx->nl80211), 0, 0,
		    NL80211_CMD_SET_POWER_SAVE, 0);

        NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex);
        NLA_PUT_U32(msg, NL80211_ATTR_PS_STATE, psMode);

        ret = send_and_recv_msgs(ctx, msg, NULL, NULL);
    	if (ret != 0)
        {
            fprintf(stderr, "nl80211: Failed to SET Power state (psMode=%d): "
                   "%ld (%s)", psMode, ret, strerror(-ret));
            goto nla_put_failure;
        }
    } else {
		genlmsg_put(msg, 0, 0, genl_family_get_id(ctx->nl80211), 0, 0,
		    NL80211_CMD_GET_POWER_SAVE, 0);

        NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex);

        ret = send_and_recv_msgs(ctx, msg, psmode_reply_handler, moc->mibObject);
    	if (ret != 0)
        {
            fprintf(stderr, "nl80211: Failed to GET Power state: "
                   "%ld (%s)", ret, strerror(-ret));
            goto nla_put_failure;
        }
	}

    return WIFI_SUCCESS;

nla_put_failure:
	nlmsg_free(msg);
	return WIFI_FAIL;
}


static uint32_t compat_MIBOID_POWERMGMT_UAPSDCONFIG(struct vwal_context *ctx, mibObjectContainer_t *moc)
{
	int32 			ret = WIFI_80211_DEVICE_NOT_READY;
	unsigned int	uapsd_queues;

	if (moc->operation == WIFI_MIB_API_OPERATION_WRITE)
	{
		FILE *file = fopen("/sys/kernel/debug/ieee80211/phy0/uapsd_queues", "w");
		if (!file) 
			goto error;
	
		uapsd_queues = (*(unsigned int*)moc->mibObject);
		
		/* Only Enable or Disable action supported, when enable, set to 0xf */
		if ( uapsd_queues)
			uapsd_queues = 0xf;
		
		if (fprintf(file, "0x%x", uapsd_queues) > 0)
			ret = WIFI_SUCCESS;
		else
			ret = WIFI_80211_DEVICE_NOT_READY;
	
		fclose(file);

	}else{
		FILE *file = fopen("/sys/kernel/debug/ieee80211/phy0/uapsd_queues", "r");
		if (!file) 
			goto error;
	
		if (fscanf(file, "%x", &uapsd_queues) == 1)
			ret = WIFI_SUCCESS;
		else
			ret = WIFI_80211_DEVICE_NOT_READY;
	
		*(unsigned int*)moc->mibObject = uapsd_queues & 0x1;
	printf("moc->mibObject = %d\n", *(unsigned int*)moc->mibObject);
		fclose(file);
	}

	return ret;

error:
	return ret;
}

static int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
{
#ifdef ANDROID
	struct ifreq_andr ifr;
#else
	struct ifreq ifr;
#endif

	if (sock < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		fprintf(stderr, "Could not read interface %s flags: %s",
			   ifname, strerror(errno));
		return -1;
	}

	if (dev_up) {
		if (ifr.ifr_flags & IFF_UP)
			return 0;
		ifr.ifr_flags |= IFF_UP;
	} else {
		if (!(ifr.ifr_flags & IFF_UP))
			return 0;
		ifr.ifr_flags &= ~IFF_UP;
	}

	if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
		fprintf(stderr, "Could not set interface %s flags: %s",
			   ifname, strerror(errno));
		return -1;
	}

	return 0;
}

static uint32_t compat_MIBOID_DSPGCOMMAND(struct vwal_context *ctx, mibObjectContainer_t *moc)
{
	uint32_t ret = 0;

	if (moc->operation != WIFI_MIB_API_OPERATION_WRITE)
		return WIFI_MIB_READ_NOT_ALLOWED;

	switch (*(uint32_t*)moc->mibObject) {
	case WIFI_MIB_API_COMMAND_START:
		if (linux_set_iface_flags(ctx->ioctl_sock, ctx->ifname, 1) == 0)
			ret = WIFI_SUCCESS;
		break;
	case WIFI_MIB_API_COMMAND_STOP:
		if (linux_set_iface_flags(ctx->ioctl_sock, ctx->ifname, 0) == 0)
			ret = WIFI_SUCCESS;
		break;
	default:
		ret = WIFI_INVALID_PARAMS;
		break;
	}

	return ret;
}

static int linux_get_iface_state(int sock, const char *ifname)
{
#ifdef ANDROID
	struct ifreq_andr ifr;
#else
	struct ifreq ifr;
#endif

	if (sock < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		fprintf(stderr, "Could not read interface %s flags: %s",
			   ifname, strerror(errno));
		return -1;
	}

	return !!(ifr.ifr_flags & IFF_UP);
}

static uint32_t compat_MIBOID_CONNECTIONSTATE(struct vwal_context *ctx, mibObjectContainer_t *moc)
{
	int ret = WIFI_FAIL;

	if (moc->operation != WIFI_MIB_API_OPERATION_READ)
		return WIFI_MIB_WRITE_NOT_ALLOWED;

	ret = linux_get_iface_state(ctx->ioctl_sock, ctx->ifname);
	switch (ret) {
	case 0:
		*(uint32_t*)moc->mibObject = WIFI_MIB_API_CONNECTION_STATE_STOPPED;
		ret = WIFI_SUCCESS;	
		break;
	case 1:
		*(uint32_t*)moc->mibObject = WIFI_MIB_API_CONNECTION_STATE_STARTED;
		ret = WIFI_SUCCESS;	
		break;
	default:
		ret = WIFI_80211_DEVICE_NOT_READY;	
		break;
	}

	return ret;
}

uint32 WifiMibAPI(void *device, mibObjectContainer_t *moc, uint32 len)
{
	struct vwal_context *ctx = g_ctx;
	uint32 i;
	uint32 ret = 0;
	/* we only support reading/writing one MIBOID at a time */
	uint8_t num = 1;

	if (!ctx->loaded)
		return WIFI_80211_DEVICE_NOT_READY;

	if (!moc || len == 0)
		return WIFI_INVALID_PARAMS;

	for (i = 0; i < len; ++i) {
		switch (moc[i].mibObjectID) {
		case MIBOID_DSPGCOMMAND:
			ret = compat_MIBOID_DSPGCOMMAND(ctx, &moc[i]);
			break;
		case MIBOID_CURRENTCHANNELFREQUENCY:
			ret = compat_MIBOID_CURRENTCHANNELFREQUENCY(ctx, &moc[i]);
			break;
		case MIBOID_CONNECTIONSTATE:
			ret = compat_MIBOID_CONNECTIONSTATE(ctx, &moc[i]);
			break;
		case MIBOID_POWERMGMT_MODE:
            ret = compat_MIBOID_POWERMGMT_MODE(ctx, &moc[i]);
            break;
		case MIBOID_POWERMGMT_UAPSDCONFIG:
			ret = compat_MIBOID_POWERMGMT_UAPSDCONFIG(ctx, &moc[i]);
			break;
		default:
			ret = nl80211_testmode_send_and_recv(ctx, WIFI_CMD_MIB, &moc[i], &num);
			break;
		}

		if (WIFI_SUCCESS != ret)
			break;
	}

	return ret; 
}

uint32 VWAL_MibRead(void *device, uint16 id, void *buf)
{
	mibObjectContainer_t moc = {
		.mibObjectID = id,
		.operation = WIFI_MIB_API_OPERATION_READ,
		.mibObject = buf,
	};

	return WifiMibAPI(device, &moc, 1);
}

uint32 VWAL_MibWrite(void *device, uint16 id, void *buf)
{
	mibObjectContainer_t moc = {
		.mibObjectID = id,
		.operation = WIFI_MIB_API_OPERATION_WRITE,
		.mibObject = buf,
	};

	return WifiMibAPI(device, &moc, 1);
}

static int get_wiphy_info_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct vwal_context *ctx = arg;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_WIPHY]) {
		ctx->wiphy = nla_get_u32(tb[NL80211_ATTR_WIPHY]);
	} else {
		ctx->wiphy = 0xffffffff;
	}

	return NL_SKIP;
}

static int get_wiphy_info(struct vwal_context *ctx)
{
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	genlmsg_put(msg, 0, 0, genl_family_get_id(ctx->nl80211), 0, 0,
		    NL80211_CMD_GET_WIPHY, 0);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ctx->ifindex);
	if (send_and_recv_msgs(ctx, msg, get_wiphy_info_handler, ctx) == 0 &&
	    ctx->wiphy != 0xffffffff)
		return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -1;
}

int VWAL_Init()
{
	struct vwal_context *ctx = g_ctx;
	int ret;

	if (ctx->loaded)
		return 1;

	ctx->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (ctx->nl_cb == NULL)
		goto error1;

	ctx->nl_handle = nl_socket_alloc();
	if (ctx->nl_handle == NULL)
		goto error2;

	ret = genl_connect(ctx->nl_handle);
	if (ret)
		goto error3;

	ret = genl_ctrl_alloc_cache(ctx->nl_handle, &ctx->nl_cache);
	if (ret)
		goto error3;

	ctx->nl80211 = genl_ctrl_search_by_name(ctx->nl_cache, "nl80211");
	if (ctx->nl80211 == NULL)
		goto error4;

	ctx->ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (ctx->ioctl_sock < 0)
		goto error5;

	strncpy(ctx->ifname, "wlan0", sizeof(ctx->ifname) * sizeof(char));
	ctx->ifindex = if_nametoindex(ctx->ifname);

	ret = get_wiphy_info(ctx);
	if (ret)
		goto error6;

	ctx->loaded = 1;
	return 1;

error6:
	close(ctx->ioctl_sock);
error5:
	genl_family_put(ctx->nl80211);
error4:
	nl_cache_free(ctx->nl_cache);
error3:
	nl_socket_free(ctx->nl_handle);
error2:
	nl_cb_put(ctx->nl_cb);
error1:
	return 0;
}

void VWAL_Shutdown()
{
	struct vwal_context *ctx = g_ctx;

	close(ctx->ioctl_sock);
	genl_family_put(ctx->nl80211);
	nl_cache_free(ctx->nl_cache);
	nl_socket_free(ctx->nl_handle);
	nl_cb_put(ctx->nl_cb);

	memset(ctx, 0, sizeof(*ctx));
}

