
#include "stdafx.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <Ws2tcpip.h>
#include "windivert.h"

#define MAXBUF  0xFFFF
#pragma comment(lib, "WinDivert.lib")
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

HANDLE handle;
WINDIVERT_ADDRESS addr; // Packet address
PWINDIVERT_IPHDR ip_header;
PWINDIVERT_UDPHDR udp_header;
char packet[MAXBUF];
UINT packetLen;
UINT16 dst_port = 80;


string readParam(const char* in) {
	string dstAddrLang = "udp.DstAddr=";
	string dstPortLang = "udp.DstPort=";
	string dstPortMatch = "";
	struct sockaddr_in sa;
	char* strdupe = _strdup(in);

	if (strstr(strdupe, "->") != NULL) {
		dstPortMatch = strtok(strdupe, "->");
		dst_port = atoi(strtok(NULL, "->"));
		return dstPortLang + dstPortMatch;
	}
	else if (inet_pton(AF_INET, strdupe, &(sa.sin_addr)) == 1) {
		return dstAddrLang + string(strdupe);
	}
	return "";
}

int main(int argc, char* argv[]) {

	// TODO: return filter policy here from param
	string truncPolic = "";

	int i = 0;
	while (i < argc) {
		char* token = _strdup(argv[i]);
		truncPolic += readParam(token) + ((i == argc - 1) || (i == 0) ? "" : " and ");
		i++;
	}

	printf("%s", truncPolic.c_str());
	handle = WinDivertOpen(truncPolic.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);   // Open some filter
	if (handle == INVALID_HANDLE_VALUE)
	{
		// Handle error
		printf("\n!\n");
		exit(1);
	}

	// Main capture-modify-inject loop:
	while (TRUE)
	{
		if (!WinDivertRecv(handle, packet, sizeof(packet), &addr, &packetLen))
		{
			// Handle recv error
			continue;
		}

		// Modify packet.
		WinDivertHelperParsePacket(packet, packetLen, &ip_header, NULL, NULL, NULL, NULL, &udp_header, NULL, NULL);

		udp_header->DstPort = dst_port;

		WinDivertHelperCalcChecksums(packet, packetLen, &addr, 0);
		if (!WinDivertSend(handle, packet, packetLen, &addr, NULL))
		{
			// Handle send error
			continue;
		}
	}
}
