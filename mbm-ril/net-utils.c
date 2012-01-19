/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/route.h>
#include <linux/wireless.h>

#define LOG_TAG "mbm-netutils"
#include <cutils/log.h>
#include <cutils/properties.h>

static int ifc_ctl_sock = -1;

static const char *ipaddr_to_string(in_addr_t addr)
{
    struct in_addr in_addr;

    in_addr.s_addr = addr;
    return inet_ntoa(in_addr);
}

int ifc_init(void)
{
    if (ifc_ctl_sock == -1) {
	ifc_ctl_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ifc_ctl_sock < 0)
	    LOGE("%s() socket() failed: %s", __func__, strerror(errno));
    }
    return ifc_ctl_sock < 0 ? -1 : 0;
}

void ifc_close(void)
{
    if (ifc_ctl_sock != -1) {
	(void) close(ifc_ctl_sock);
	ifc_ctl_sock = -1;
    }
}

static void ifc_init_ifr(const char *name, struct ifreq *ifr)
{
    memset(ifr, 0, sizeof(struct ifreq));
    strncpy(ifr->ifr_name, name, IFNAMSIZ);
    ifr->ifr_name[IFNAMSIZ - 1] = 0;
}

static int ifc_set_flags(const char *name, unsigned set, unsigned clr)
{
    struct ifreq ifr;
    ifc_init_ifr(name, &ifr);

    if (ioctl(ifc_ctl_sock, SIOCGIFFLAGS, &ifr) < 0)
	return -1;
    ifr.ifr_flags = (ifr.ifr_flags & (~clr)) | set;
    return ioctl(ifc_ctl_sock, SIOCSIFFLAGS, &ifr);
}

int ifc_up(const char *name)
{
    return ifc_set_flags(name, IFF_UP | IFF_NOARP, 0);
}

int ifc_down(const char *name)
{
    return ifc_set_flags(name, 0, IFF_UP);
}

static void init_sockaddr_in(struct sockaddr *sa, in_addr_t addr)
{
    struct sockaddr_in *sin = (struct sockaddr_in *) sa;
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = addr;
}

int ifc_set_addr(const char *name, in_addr_t addr)
{
    struct ifreq ifr;

    ifc_init_ifr(name, &ifr);
    init_sockaddr_in(&ifr.ifr_addr, addr);

    return ioctl(ifc_ctl_sock, SIOCSIFADDR, &ifr);
}

int ifc_set_mask(const char *name, in_addr_t mask)
{
    struct ifreq ifr;

    ifc_init_ifr(name, &ifr);
    init_sockaddr_in(&ifr.ifr_addr, mask);

    return ioctl(ifc_ctl_sock, SIOCSIFNETMASK, &ifr);
}

int ifc_configure(const char *ifname,
        in_addr_t address,
        in_addr_t gateway)
{
    in_addr_t netmask = ~0;
    (void) gateway;

    ifc_init();

    if (ifc_up(ifname)) {
	LOGE("%s() Failed to turn on interface %s: %s", __func__,
	     ifname,
	     strerror(errno));
	ifc_close();
	return -1;
    }
    if (ifc_set_addr(ifname, address)) {
	LOGE("%s() Failed to set ipaddr %s: %s", __func__,
	     ipaddr_to_string(address), strerror(errno));
	ifc_down(ifname);
	ifc_close();
	return -1;
    }
    if (ifc_set_mask(ifname, netmask)) {
	LOGE("%s() failed to set netmask %s: %s", __func__,
	     ipaddr_to_string(netmask), strerror(errno));
	ifc_down(ifname);
	ifc_close();
	return -1;
    }

    ifc_close();

    return 0;
}
