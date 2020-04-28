#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>

// Takes output from candump -tz 

using namespace std;

string line;
bool nline(istream &i, int *addr, int *dlen, int *b, float *t)  {
	if (getline(i, line)) {
		*addr = -1;
		*dlen = 0;
		sscanf(line.c_str(), " (%f) can0 %x [%d] %x %x %x %x %x %x %x %x",
		t, addr, dlen, b, b+1, b+2, b+3, b+4, b+5, b+6, b+7);
		return true;
	}
	return false;
}

float getFloat(int *b) {
	int32_t x = b[0] + (b[1] << 8) + (b[2] << 16) +(b[3] << 24);
	float f = *((float *)(&x));
	return f;
}

int main(int argc, char **argv) { 
	istream &i = cin;
    ifstream f(argv[1], ios_base::in);
	int dlen;
	int b[256];
	int addr = -1, nextAddr;
	int plen = 0;
	float pktTime, nextTime;
	
	struct {
		float pitch, roll, ias, tas, palt, hdgBug;
	} stats = {0};
	
	
	while(nline(i, &nextAddr, &dlen, b + plen, &nextTime)) {		
		if (addr != nextAddr || plen > (int)sizeof(b) - 16) {
			printf("%08x (%010.4f) %03d ", addr, pktTime, plen);
			for (int n = 0; n < plen; n++)  {
				if (n > 0 && n % 4 == 0)
					printf(" ");
				printf("%02x", b[n]);
			}
			printf("\n");

			float p[32];
			for (int n = 0; n < (int)(sizeof(p)/sizeof(p[0])); n++) { 
				int32_t x  = b[n * 4] + (b[n * 4 + 1] << 8) + (b[n * 4 + 2] << 16) +(b[n * 4 + 3] << 24);
				p[n] = *((float *)(&x));
			} 

			if (1) {
				printf ("%08x (%010.4f) %02x%02x %03d FLT ", addr, pktTime, b[0], b[1], plen);
				for (int n = 1; n < plen/4; n++) {
					char b[9];
					snprintf(b, sizeof(b), "%+08.3f", p[n]);
					b[sizeof(b)-1] = 0; 
					printf("%s ", b);
				}
				printf("\n");
			}
			if (addr == 0x10882200 && b[1] == 0x65)  {
				if (b[2] == 05) stats.hdgBug = getFloat(&b[3]);
			}
			if (addr == 0x10882100 && b[0] == 0xdc && b[1] == 0x02)  {
				stats.roll = getFloat(&b[12]);
			}
			if (addr == 0x18882100 && b[0] == 0xdd && b[1] == 0x00)  {
				stats.pitch = getFloat(&b[20]);
			}			
			if (addr == 0x18882100 && b[0] == 0xdd && b[1] == 0x00)  {
				stats.pitch = getFloat(&b[20]);
			}
			if (addr == 0x18882100 && b[0] == 0xdd && b[1] == 0x0a)  {
				stats.ias = getFloat(&b[12]);
				stats.palt = getFloat(&b[24]);
			}
			
			for(int n = 0; n < dlen; n++)
				b[n] = b[n + plen];
				
			plen = 0;
			addr = nextAddr;
			pktTime = nextTime;
			
			static int count;
			if (++count % 30== 0)
				printf("hdg %f pitch %f roll %f ias %f palt %f\n", stats.hdgBug, stats.pitch, stats.roll, stats.ias, stats.palt);
		}
		plen += dlen;
	}		
}
