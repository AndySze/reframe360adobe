#include "HardwareUUIDHandler.h"


#ifdef _WIN32

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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <assert.h>

#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>

const char* getMachineName()
{
    static struct utsname u;
    
    if ( uname( &u ) < 0 )
    {
        assert(0);
        return "unknown";
    }
    
    return u.nodename;
}


//---------------------------------get MAC addresses ------------------------------------unsigned short-unsigned short----------
// we just need this for purposes of unique machine id. So any one or two mac's is fine.
unsigned short hashMacAddress( unsigned char* mac )
{
    unsigned short hash = 0;
    
    for ( unsigned int i = 0; i < 6; i++ )
    {
        hash += ( mac[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

void getMacHash( unsigned short& mac1, unsigned short& mac2 )
{
    mac1 = 0;
    mac2 = 0;
    
    struct ifaddrs* ifaphead;
    if ( getifaddrs( &ifaphead ) != 0 )
        return;
    
    // iterate over the net interfaces
    bool foundMac1 = false;
    struct ifaddrs* ifap;
    for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next )
    {
        struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
        if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER ))
        {
            if ( !foundMac1 )
            {
                foundMac1 = true;
                mac1 = hashMacAddress( (unsigned char*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
            } else {
                mac2 = hashMacAddress( (unsigned char*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
                break;
            }
        }
    }
    
    freeifaddrs( ifaphead );
    
    // sort the mac addresses. We don't want to invalidate
    // both macs if they just change order.
    if ( mac1 > mac2 )
    {
        unsigned short tmp = mac2;
        mac2 = mac1;
        mac1 = tmp;
    }
}

unsigned short getVolumeHash()
{
    // we don't have a 'volume serial number' like on windows. Lets hash the system name instead.
    unsigned char* sysname = (unsigned char*)getMachineName();
    unsigned short hash = 0;
    
    for ( unsigned int i = 0; sysname[i]; i++ )
        hash += ( sysname[i] << (( i & 1 ) * 8 ));
    
    return hash;
}

#include <mach-o/arch.h>
unsigned short getCpuHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    unsigned short val = 0;
    val += (unsigned short)info->cputype;
    val += (unsigned short)info->cpusubtype;
    return val;
}

int main()
{
    
    printf("Machine: %s\n", getMachineName());
    printf("CPU: %d\n", getCpuHash());
    printf("Volume: %d\n", getVolumeHash());
    return 0;
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
