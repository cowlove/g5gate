#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>

using namespace std;

bool nline(ifstream &i, int *addr, int *dlen, int *b)  {
	string line;
	if (getline(i, line)) {
		*addr = -1;
		*dlen = 0;
		sscanf(line.c_str(), " can0 %x [%d] %x %x %x %x %x %x %x %x",
		addr, dlen, b, b+1, b+2, b+3, b+4, b+5, b+6, b+7);
		return true;
	}
	return false;
}



class PacketComparer {
	int size, addr;
	int *increasing;
	int *lastpkt;
	int firstbyte;
public:
	PacketComparer(int a, int fb, int sz) : size(sz), addr(a), firstbyte(fb) { 
		increasing = new int[sz]; 
		lastpkt = new int[sz]; 
		bzero(increasing, sizeof(increasing[0]) * size);
		bzero(lastpkt, sizeof(lastpkt[0]) * size);
	}

	void add(ifstream &i, int a, int dlen, int *b) { 
		if (a != addr || b[0] != firstbyte) {
			return;
		}
		int n = dlen;
		while(n < size) {
			int bytes;
			if (!nline(i, &a, &bytes, b + n))
				return;
			n += bytes;
		}
		for (n = 0; n < size; n++)  {
			if (b[n] > lastpkt[n]) {
				increasing[n]++;
			} else if (b[n] < lastpkt[n]) {
				if (increasing[n] > 0) 
					printf ("%08d %03d 0x%x-%x\n", increasing[n], n, addr, firstbyte);
				increasing[n] = 0;
			} 
			
		}
		memcpy(lastpkt, b, size * sizeof(b[0]));
	}
};	

int main(int argc, char **argv) { 
    ifstream i(argv[1], ios_base::in);
	int addr, dlen, b[256];
	
	PacketComparer pc[] = {
	PacketComparer(0x18882100, 0xc1, 60),
	PacketComparer(0x18882100, 0xdd, 44),
	PacketComparer(0x18882100, 0xdc, 30),
	PacketComparer(0x18882000, 0x01, 8),
	PacketComparer(0x188c2100, 0xc1, 60),
	PacketComparer(0x188c2100, 0xdd, 44),
	PacketComparer(0x188c2100, 0xdc, 30),
	PacketComparer(0x188c2000, 0x01, 8),
	};
		
	while(nline(i, &addr, &dlen, b)) {
		for (int n = 0; n < (int)(sizeof(pc) / sizeof(pc[0])); n++) {
			pc[n].add(i, addr, dlen, b);
		}
	}		
}
