
#ifndef _SSLIB_H
#define _SSLIB_H

#define MAX_QUEUED_CHUNKS		(180 +1)
#define SS_NUM_CHANNELS			16

struct SSChunk
{
	int16_t *buffer;
	int length;
	
	signed char *bytebuffer;			// same as bytebuffer but in BYTES
	int bytelength;						// TOTAL length in BYTES
	
	// current read position. this is within bytebuffer and is in BYTES.
	int bytepos;
	
	int userdata;						// user data to be sent to FinishedCallback when finished
};


struct SSChannel
{
	SSChunk chunks[MAX_QUEUED_CHUNKS];
	int head, tail;
	
	int volume;
	char reserved;						// if 1, can only be played on explicitly, not by passing -1
	
	int FinishedChunkUserdata[MAX_QUEUED_CHUNKS];
	int nFinishedChunks;

	int loops_left = 0; // if -1 loop until abort

    bool doLoop = false;
	
	void (*FinishedCB)(int channel, int chunkid);
};

int SSPlayChunk(int c, SSChunk *inChunk, int userdata, int loops, void(*FinishedCB)(int, int));
int SSEnqueueChunk(int c, SSChunk *inChunk, int userdata, int loops, void(*FinishedCB)(int, int));
void SSAbortChannel(int c);
char SSInit(void);
void SSClose(void);
void SSReserveChannel(int c);

#endif
