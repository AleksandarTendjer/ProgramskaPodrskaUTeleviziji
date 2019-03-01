#ifndef __STREAM_CONTROLLER_H__
#define __STREAM_CONTROLLER_H__

#include <stdio.h>
#include "tables.h"
#include "tdp_api.h"
#include "tables.h"
#include "pthread.h"
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>


#define DESIRED_FREQUENCY 818000000	        /* Tune frequency in Hz */
#define BANDWIDTH 8  
  				        /* Bandwidth in Mhz */
typedef struct input{
	int freq;
	int8_t bandwith;
	char module[8];
	//initial channel data
	int16_t audioPID;
	int16_t videoPID;
	int8_t audioType;
	int8_t videoType;
	int8_t programNumber;
}input_struct;

/**
 * @brief Structure that defines stream controller error
 */
typedef enum _StreamControllerError
{
    SC_NO_ERROR = 0,
    SC_ERROR,
    SC_THREAD_ERROR
}StreamControllerError;

/**
 * @brief Structure that defines channel info
 */
typedef struct _ChannelInfo
{
    int16_t programNumber;
    int16_t audioPid;
    int16_t videoPid;
}ChannelInfo;

extern ChannelInfo currentChannel;
extern bool flagCH;

extern struct itimerspec timerSpec;
extern struct itimerspec timerSpecOld;
extern struct itimerspec timerSpecVolume;
extern struct itimerspec timerSpecOldVolume;
extern timer_t timerId;
extern timer_t timerIdVolume;
/**
 * @brief Initializes stream controller module
 *
 * @return stream controller error code
 */
StreamControllerError streamControllerInit(input_struct*);

/**
 * @brief Deinitializes stream controller module
 *
 * @return stream controller error code
 */
StreamControllerError streamControllerDeinit();

/**
 * @brief Channel up
 *
 * @return stream controller error
 */
StreamControllerError channelUp();

/**
 * @brief Channel down
 *
 * @return stream controller error
 */
StreamControllerError channelDown();

/**
 * @brief Returns current channel info
 *
 * @param [out] channelInfo - channel info structure with current channel info
 * @return stream controller error code
 */
StreamControllerError getChannelInfo(ChannelInfo* channelInfo);
/**
 * @brief starts new channel
 *
 * @param [out] channelNumber-the channel that will be played
 * @return no return
 */
void startChannel(int32_t channelNumber);
/**
 * @brief increments the volume  
 *
 * @param [out]  
 * @return stream error or no error
 */
StreamControllerError volumeUp();
/**
 * @brief decrements the volume  
 *
 * @param [out]  
 * @return stream error or no error
 */
StreamControllerError volumeDown();
/**
 * @brief sets the volume on 0 
 *
 * @param [out] 
 * @return stream error or no error
 */
StreamControllerError muteVolume();
/**
 * @brief callse the tdp api voluem set function
 *
 * @param [out] 
 * @return
 */
void setVolume();
void onInfoPressed();
void onVolumePressed();

#endif /* __STREAM_CONTROLLER_H__ */
