#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1050
    #include <AudioUnit/AUNTComponent.h>
#endif

#include "defines.h"

#define Uint32 unsigned int

//---------------------------------------SOUNDCARD-VARS--------------
int play_active=0;				// Audio thread active
AudioUnit outputAudioUnit;
static int mixing_frequency = 44100;

//---------------------------------------SID-VARS--------------------
word play_addr;
byte play_speed;

//---------------------------------------SID-ENGINE-FUCTIONS
extern int cpuJSR(word npc, byte na);
extern void synth_render (word *buffer, dword len);
extern unsigned char memory[65536];

int is_little_endian(void)
{
        static const unsigned long a = 1;

        return *(const unsigned char *)&a;

}

void SetError(char *pszError)
{
	printf("%s", pszError);
	exit(-1);
}

int callbackActive = 0;

static signed short buffer[882*10];
static int nBufferLength = 0;

/* The CoreAudio callback */
static OSStatus     audioCallback (void                             *inRefCon, 
                                    AudioUnitRenderActionFlags      *inActionFlags,
                                    const AudioTimeStamp            *inTimeStamp, 
                                    UInt32                          inBusNumber,
                                    UInt32                          inNumberFrames,
                                    AudioBufferList                 *ioData)
{
    UInt32 remaining;
    signed short *ptr;
	unsigned int nSamplesPerCall;
	
	// Mark a flag that we are now entering the audiocallback
	// (so noone is trying to close the audio right now)
	
	callbackActive = 1;
	
	// Find out how many samples we have to calculate
    //remaining = ioData->mDataByteSize/sizeof(signed short);
    remaining = ioData->mBuffers[0].mDataByteSize / sizeof(signed short);
    //ptr = ioData->mData;
    ptr = ioData->mBuffers[0].mData;
	
	// If we're not playing anything, then mute
	if (!play_active)
	{
		memset(ptr, 0, remaining*sizeof(signed short));
	}
	else // Otherwise calculate music
	{
		while (nBufferLength < remaining)
		{
			cpuJSR(play_addr, 0);
			
			int nRefreshCIA = (int)(20000*(memory[0xdc04]|(memory[0xdc05]<<8))/0x4c00);
			
			if ((nRefreshCIA == 0) || (play_speed == 0)) nRefreshCIA = 20000;
			nSamplesPerCall = mixing_frequency*nRefreshCIA/1000000;
			
			synth_render((word*)&buffer[nBufferLength], nSamplesPerCall);
			nBufferLength += nSamplesPerCall;
		}
		
		memcpy(ptr, buffer, remaining*sizeof(signed short));
		nBufferLength -= remaining;
		memmove(buffer, &buffer[remaining], nBufferLength*sizeof(signed short));
	}

	// We've finished the audio callback
	callbackActive = 0;
        
    return 0;
}

void Core_CloseAudio(void)
{
    OSStatus result;
    struct AURenderCallbackStruct callback;

    /* stop processing the audio unit */
    result = AudioOutputUnitStop (outputAudioUnit);
    if (result != noErr) {
        SetError("Core_CloseAudio: AudioOutputUnitStop");
        return;
    }
	
	// Wait until no callback is active;
	while (callbackActive);

    /* Remove the input callback */
    callback.inputProc = 0;
    callback.inputProcRefCon = 0;
    result = AudioUnitSetProperty (outputAudioUnit, 
                        kAudioUnitProperty_SetRenderCallback, 
                        kAudioUnitScope_Input, 
                        0,
                        &callback, 
                        sizeof(callback));
    if (result != noErr) {
        SetError("Core_CloseAudio: AudioUnitSetProperty (kAudioUnitProperty_SetInputCallback)");
        return;
    }

    result = CloseComponent(outputAudioUnit);
    if (result != noErr) {
        SetError("Core_CloseAudio: CloseComponent");
        return;
    }
    
}

#define CHECK_RESULT(msg) \
    if (result != noErr) { \
        perror(msg); \
        return -1; \
    }


int Core_OpenAudio(void)
{
    OSStatus result = noErr;
    Component comp;
    ComponentDescription desc;
    struct AURenderCallbackStruct callback;
    AudioStreamBasicDescription requestedDesc;

    /* Setup a AudioStreamBasicDescription with the requested format */
    requestedDesc.mFormatID = kAudioFormatLinearPCM;
    requestedDesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    requestedDesc.mChannelsPerFrame = 1; //spec->channels;
    requestedDesc.mSampleRate = mixing_frequency; //spec->freq;
    
    requestedDesc.mBitsPerChannel = 16; //spec->format & 0xFF;
    //if (spec->format & 0x8000)
        requestedDesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
    //if (spec->format & 0x1000)
        requestedDesc.mFormatFlags |= (is_little_endian())?0:kLinearPCMFormatFlagIsBigEndian;

    requestedDesc.mFramesPerPacket = 1;
    requestedDesc.mBytesPerFrame = requestedDesc.mBitsPerChannel * requestedDesc.mChannelsPerFrame / 8;
    requestedDesc.mBytesPerPacket = requestedDesc.mBytesPerFrame * requestedDesc.mFramesPerPacket;


    /* Locate the default output audio unit */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    comp = FindNextComponent (NULL, &desc);
    if (comp == NULL) {
        SetError ("Failed to start CoreAudio: FindNextComponent returned NULL");
        return -1;
    }
    
    /* Open & initialize the default output audio unit */
    result = OpenAComponent (comp, &outputAudioUnit);
    CHECK_RESULT("OpenAComponent")

    result = AudioUnitInitialize (outputAudioUnit);
    CHECK_RESULT("AudioUnitInitialize")
                
    /* Set the input format of the audio unit. */
    result = AudioUnitSetProperty (outputAudioUnit,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &requestedDesc,
                               sizeof (requestedDesc));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_StreamFormat)")

    /* Set the audio callback */
    callback.inputProc = audioCallback;
    callback.inputProcRefCon = NULL; // this
    result = AudioUnitSetProperty (outputAudioUnit, 
                        kAudioUnitProperty_SetRenderCallback, 
                        kAudioUnitScope_Input, 
                        0,
                        &callback, 
                        sizeof(callback));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_SetInputCallback)")
    
    /* Allocate a sample buffer */
    //bufferOffset = bufferSize = this->spec.size;
    //buffer = malloc(bufferSize);

    /* Finally, start processing of the audio unit */
    result = AudioOutputUnitStart (outputAudioUnit);
    CHECK_RESULT("AudioOutputUnitStart")    
    
    /* We're running! */
    return(1);
}


void stop_playing(void)
{
	if (play_active)
	{
		play_active=0;
		Core_CloseAudio();
	}
}

void soundcard_init(void)
{
	Core_OpenAudio();
}

void start_playing(word nplay_addr, byte nplay_speed)
{
	play_addr = nplay_addr;
	play_speed= nplay_speed;
		
	play_active = -1;
}
