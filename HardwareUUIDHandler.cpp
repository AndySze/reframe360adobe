#include "HardwareUUIDHandler.h"


#ifndef DARWIN

#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>

// we just need this for purposes of unique machine id. 
// So any one or two mac's is fine.
uint16_t hashMacAddress(PIP_ADAPTER_INFO info)
{
	uint16_t hash = 0;
	for (uint32_t i = 0; i < info->AddressLength; i++)
	{
		hash += (info->Address[i] << ((i & 1) * 8));
	}
	return hash;
}

void getMacHash(uint16_t& mac1, uint16_t& mac2)
{
	IP_ADAPTER_INFO AdapterInfo[32];
	DWORD dwBufLen = sizeof(AdapterInfo);

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
		return; // no adapters.

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	mac1 = hashMacAddress(pAdapterInfo);
	if (pAdapterInfo->Next)
		mac2 = hashMacAddress(pAdapterInfo->Next);

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2)
	{
		uint16_t tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

uint16_t getVolumeHash()
{
	DWORD serialNum = 0;

	// Determine if this volume uses an NTFS file system.
	GetVolumeInformation("c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
	uint16_t hash = (uint16_t)((serialNum + (serialNum >> 16)) & 0xFFFF);

	return hash;
}

uint16_t getCpuHash()
{
	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	uint16_t hash = 0;
	uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
	for (uint32_t i = 0; i < 8; i++)
		hash += ptr[i];

	return hash;
}

const char* getMachineName()
{
	static char computerName[1024];
	DWORD size = 1024;
	GetComputerName(computerName, &size);
	return &(computerName[0]);
}

#else

#include "machine_id.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef DARWIN
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#else //!DARWIN
#include <linux/if.h>
#include <linux/sockios.h>
#endif //!DARWIN

#include <sys/resource.h>
#include <sys/utsname.h>

//---------------------------------get MAC addresses ---------------------------------
// we just need this for purposes of unique machine id. So any one or two 
// mac's is fine.
uint16_t hashMacAddress(uint8_t* mac)
{
	uint16_t hash = 0;

	for (uint32_t i = 0; i < 6; i++)
	{
		hash += (mac[i] << ((i & 1) * 8));
	}
	return hash;
}

void getMacHash(uint16_t& mac1, uint16_t& mac2)
{
	mac1 = 0;
	mac2 = 0;

#ifdef DARWIN

	struct ifaddrs* ifaphead;
	if (getifaddrs(&ifaphead) != 0)
		return;

	// iterate over the net interfaces
	bool foundMac1 = false;
	struct ifaddrs* ifap;
	for (ifap = ifaphead; ifap; ifap = ifap->ifa_next)
	{
		struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
		if (sdl && (sdl->sdl_family == AF_LINK) && (sdl->sdl_type == IFT_ETHER))
		{
			if (!foundMac1)
			{
				foundMac1 = true;
				mac1 = hashMacAddress((uint8_t*)(LLADDR(sdl))); //sdl->sdl_data) + 
				sdl->sdl_nlen) );
			}
			else {
				mac2 = hashMacAddress((uint8_t*)(LLADDR(sdl))); //sdl->sdl_data) + 
				sdl->sdl_nlen) );
				break;
			}
		}
	}

	freeifaddrs(ifaphead);

#else // !DARWIN

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) return;

	// enumerate all IP addresses of the system
	struct ifconf conf;
	char ifconfbuf[128 * sizeof(struct ifreq)];
	memset(ifconfbuf, 0, sizeof(ifconfbuf));
	conf.ifc_buf = ifconfbuf;
	conf.ifc_len = sizeof(ifconfbuf);
	if (ioctl(sock, SIOCGIFCONF, &conf))
	{
		assert(0);
		return;
	}

	// get MAC address
	bool foundMac1 = false;
	struct ifreq* ifr;
	for (ifr = conf.ifc_req; (s8*)ifr < (s8*)conf.ifc_req + conf.ifc_len; ifr++)
	{
		if (ifr->ifr_addr.sa_data == (ifr + 1)->ifr_addr.sa_data)
			continue;  // duplicate, skip it

		if (ioctl(sock, SIOCGIFFLAGS, ifr))
			continue;  // failed to get flags, skip it
		if (ioctl(sock, SIOCGIFHWADDR, ifr) == 0)
		{
			if (!foundMac1)
			{
				foundMac1 = true;
				mac1 = hashMacAddress((uint8_t*)&(ifr->ifr_addr.sa_data));
			}
			else {
				mac2 = hashMacAddress((uint8_t*)&(ifr->ifr_addr.sa_data));
				break;
			}
		}
	}

	close(sock);

#endif // !DARWIN

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2)
	{
		uint16_t tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

uint16_t getVolumeHash()
{
	// we don't have a 'volume serial number' like on windows. 
	// Lets hash the system name instead.
	uint8_t* sysname = (uint8_t*)getMachineName();
	uint16_t hash = 0;

	for (uint32_t i = 0; sysname[i]; i++)
		hash += (sysname[i] << ((i & 1) * 8));

	return hash;
}

#ifdef DARWIN   
#include <mach-o/arch.h>
uint16_t getCpuHash()
{
	const NXArchInfo* info = NXGetLocalArchInfo();
	uint16_t val = 0;
	val += (uint16_t)info->cputype;
	val += (uint16_t)info->cpusubtype;
	return val;
}

#else // !DARWIN

static void getCpuid(uint32_t* p, uint32_t ax)
{
	__asm __volatile
	("movl %%ebx, %%esi\n\t"
		"cpuid\n\t"
		"xchgl %%ebx, %%esi"
		: "=a" (p[0]), "=S" (p[1]),
		"=c" (p[2]), "=d" (p[3])
		: "0" (ax)
	);
}

uint16_t getCpuHash()
{
	uint32_t cpuinfo[4] = { 0, 0, 0, 0 };
	getCpuid(cpuinfo, 0);
	uint16_t hash = 0;
	uint32_t* ptr = (&cpuinfo[0]);
	for (uint32_t i = 0; i < 4; i++)
		hash += (ptr[i] & 0xFFFF) + (ptr[i] >> 16);

	return hash;
}
#endif // !DARWIN

const char* getMachineName()
{
	static struct utsname u;

	if (uname(&u) < 0)
	{
		assert(0);
		return "unknown";
	}

	return u.nodename;
}

#endif

uint16_t mask[5] = { 0x4e25, 0xf4a1, 0x5437, 0xab41, 0x0000 };

static void smear(uint16_t* id)
{
	for (uint32_t i = 0; i < 5; i++)
		for (uint32_t j = i; j < 5; j++)
			if (i != j)
				id[i] ^= id[j];

	for (uint32_t i = 0; i < 5; i++)
		id[i] ^= mask[i];
}

static void unsmear(uint16_t* id)
{
	for (uint32_t i = 0; i < 5; i++)
		id[i] ^= mask[i];

	for (uint32_t i = 0; i < 5; i++)
		for (uint32_t j = 0; j < i; j++)
			if (i != j)
				id[4 - i] ^= id[4 - j];
}

static uint16_t* computeSystemUniqueId()
{
	static uint16_t id[5];
	static bool computed = false;

	if (computed) return id;

	// produce a number that uniquely identifies this system.
	id[0] = getCpuHash();
	id[1] = getVolumeHash();
	getMacHash(id[2], id[3]);

	// fifth block is some checkdigits
	id[4] = 0;
	for (uint32_t i = 0; i < 4; i++)
		id[4] += id[i];

	smear(id);

	computed = true;
	return id;
}

std::string getSystemUniqueId()
{
	// get the name of the computer
	std::string buf;
	buf.append(getMachineName());

	uint16_t* id = computeSystemUniqueId();
	for (uint32_t i = 0; i < 5; i++)
	{
		char num[16];
		snprintf(num, 16, "%x", id[i]);
		buf.append("-");
		switch (strlen(num))
		{
		case 1: buf.append("000"); break;
		case 2: buf.append("00");  break;
		case 3: buf.append("0");   break;
		}
		buf.append(num);
	}

	char p[512];
	int len = buf.copy(p, 512, 0);
	p[len] = 0;
	char* p_ptr = p;
	while (*p_ptr) { *p_ptr = toupper(*p_ptr); p_ptr++; }

	return std::string(p);
}

static bool validate(std::string testIdString)
{
	// unpack the given string. parse failures return false.
	std::string testString = testIdString;
	char p[512];
	int len = testString.copy(p, 512, 0);
	p[len] = 0;
	char* p_ptr = &p[0];

	char* testName = strtok(p_ptr, "-");
	if (!testName) return false;

	uint16_t testId[5];
	for (uint32_t i = 0; i < 5; i++)
	{
		char* testNum = strtok(NULL, "-");
		if (!testNum) return false;
		testId[i] = (uint16_t)(strtol(testNum, NULL, 16));
	}
	unsmear(testId);

	// make sure this id is valid - by looking at the checkdigits
	uint16_t check = 0;
	for (uint32_t i = 0; i < 4; i++)
		check += testId[i];
	if (check != testId[4]) return false;

	// get the current system information
	uint16_t systemId[5];
	memcpy(systemId, computeSystemUniqueId(), sizeof(systemId));
	unsmear(systemId);

	// now start scoring the match
	uint32_t score = 0;

	for (uint32_t i = 0; i < 4; i++)
		if (testId[i] == systemId[i])
			score++;

	if (!strcmp(getMachineName(), testName))
		score++;

	// if we score 3 points or more then the id matches.
	return (score >= 3) ? true : false;
}