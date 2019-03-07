#include "stream_controller.h"
#include "graphics.h"

static PatTable *patTable;
static PmtTable *pmtTable;
static SdtTable* sdtTable;
static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

static uint32_t playerHandle = 0;
static uint32_t sourceHandle = 0;
static uint32_t streamHandleA = 0;
static uint32_t streamHandleV = 0;
static uint32_t filterHandle = 0;
static uint8_t threadExit = 0;
static bool changeChannel = false;
static bool changeVolume=false;
static int16_t programNumber = 0;
ChannelInfo currentChannel;
static bool isInitialized = false;
int16_t volume=0;
static int8_t previousVolume=0;
static bool mute=false;
 bool teletextExists=false;
static struct timespec lockStatusWaitTime;
static struct timeval now;
struct sigevent signalEvent;
struct sigevent signalEventVolume;
 timer_t timerId;
 timer_t timerIdVolume;
static pthread_t scThread;
struct itimerspec timerSpec;
struct itimerspec timerSpecOld;
struct itimerspec timerSpecVolume;
struct itimerspec timerSpecOldVolume;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
DFBSurfaceDescription surfaceDesc;

char serviceName[7][100];
uint8_t serviceType[7];

bool flagCH=false;
//
static int8_t firstPassVideo=0;
static int8_t firstPassAudio=0;
static tStreamType audioStreamType;
static tStreamType videoStreamType;
static int16_t inputVideoPID=0;
static int16_t inputAudioPID=0;

static pthread_cond_t demuxCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t demuxMutex = PTHREAD_MUTEX_INITIALIZER;
static void* streamControllerTask(input_struct* );

static t_Module moduleConvertFun(char*);
static tStreamType streamConvertFun(int8_t);

StreamControllerError streamControllerInit(input_struct* inputStruct)
{
	
	printf("streamControllerInit\n");
    DFBCHECK(DirectFBInit(NULL,NULL));
    /* fetch the DirectFB interface */
	DFBCHECK(DirectFBCreate(&dfbInterface));
    
    /* tell the DirectFB to take the full screen for this application */
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	

    if (pthread_create(&scThread, NULL, &streamControllerTask, inputStruct))
    {
        printf("Error creating input event task!\n");
        return SC_THREAD_ERROR;
    }
	//set the starting channel from config
	programNumber=inputStruct->programNumber-1;
	//setting the value for using the audio and video PIDS from config in StartChannelFunction
	firstPassVideo=1;
	firstPassAudio=1;
	inputVideoPID=inputStruct->videoPID;
	inputAudioPID=inputStruct->audioPID;
	//stream type
	videoStreamType=streamConvertFun(inputStruct->videoType);
	audioStreamType=streamConvertFun(inputStruct->audioType);
	
	
    return SC_NO_ERROR;
}

StreamControllerError streamControllerDeinit()
{
    if (!isInitialized) 
    {
        printf("\n%s : ERROR streamControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    threadExit = 1;
    if (pthread_join(scThread, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return SC_THREAD_ERROR;
    }
    
    /* free demux filter */  
    Demux_Free_Filter(playerHandle, filterHandle);

	/* remove audio stream */
	Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
    
    /* remove video stream */
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
    
    /* close player source */
    Player_Source_Close(playerHandle, sourceHandle);
    
    /* deinitialize player */
    Player_Deinit(playerHandle);
    
    /* deinitialize tuner device */
    Tuner_Deinit();
    
    /* free allocated memory */  
    free(patTable);
    free(pmtTable);
    free(sdtTable);
    /* set isInitialized flag */
    isInitialized = false;

    return SC_NO_ERROR;
}
StreamControllerError switchChannel(uint8_t number)
{
	programNumber=number-1;	
	changeChannel=true;
	return SC_NO_ERROR;
}
/***************************VOLUME FUNCTIONS*************************************/
//volume up
StreamControllerError volumeUp()
{
	if(volume<10)
	{
		volume++;
		changeVolume=true;
		graphicVolume(volume);
	}

	
	return SC_NO_ERROR;
		
}
StreamControllerError volumeDown()
{
	if(volume!=0)
	{
		volume--;
		changeVolume=true;
		graphicVolume(volume);
	}
	return SC_NO_ERROR;
		
}
StreamControllerError muteVolume()
{
	if(mute==false)
	{
		previousVolume=volume;
		printf("muted!\n");
		volume=0;
		mute=true;
		graphicVolume(volume);
	}
	else
	{
		volume=previousVolume;
		printf("unmuted!\n");
		mute=false;
		graphicVolume(volume);
	}
	changeVolume=true;
	
	return SC_NO_ERROR;
		
}
void setVolume()
{
	uint32_t ret;

	uint32_t tdpVolume=volume*107374182;
	if(Player_Volume_Set(playerHandle, tdpVolume))
	{
		printf("Volume can't be set\n");
	}
	
	memset(&timerSpecVolume,0,sizeof(timerSpecVolume));
    timerSpecVolume.it_value.tv_sec = 3;
    
    /* set the new timer specs */
    ret = timer_settime(timerIdVolume,0,&timerSpecVolume,&timerSpecOldVolume);
    if(ret == -1)
    {
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}
StreamControllerError channelUp()
{   
	
    if (programNumber >= patTable->serviceInfoCount - 2)
    {
        programNumber = 0;
	printf("vece od moguceg %d \n",programNumber);
    } 
    else
    {
        programNumber++;
    }

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError channelDown()
{
    if (programNumber <= 0)
    {
        programNumber = patTable->serviceInfoCount - 2;
	printf("manje od nula  %d \n",programNumber);
    } 
    else
    {
        programNumber--;
	printf("ovde %d \n",programNumber);
    }
   
    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError getChannelInfo(ChannelInfo* channelInfo)
{
    if (channelInfo == NULL)
    {
        printf("\n Error wrong parameter\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    channelInfo->programNumber = currentChannel.programNumber;
    channelInfo->audioPid = currentChannel.audioPid;
    channelInfo->videoPid = currentChannel.videoPid;
    
    return SC_NO_ERROR;
}

/* Sets filter to receive current channel PMT table
 * Parses current channel PMT table when it arrives
 * Creates streams with current channel audio and video pids
 */
void startChannel(int32_t channelNumber)
{
     int32_t ret;
    
    /* create timer */
    signalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEvent.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
    signalEvent.sigev_value.sival_ptr = NULL;//&currentChannel; /* thread arguments */
    signalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEvent,
                       /*where to store the ID of the newly created timer*/&timerId);
    if(ret == -1){
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return;
    }
    
     /* set the function for clearing screen */

    /* create timer */
    signalEventVolume.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEventVolume.sigev_notify_function = wipeScreenVolume; /* function to be called when timer runs out */
    signalEventVolume.sigev_value.sival_ptr = NULL; /* thread arguments */
    signalEventVolume.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEventVolume,
                       /*where to store the ID of the newly created timer*/&timerIdVolume);
    if(ret == -1){
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return;
    }
    /* free PAT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);
    
    /* set demux filter for receive PMT table of program */
    if(Demux_Set_Filter(playerHandle, patTable->patServiceInfoArray[channelNumber + 1].pid, 0x02, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        	return;
	}
    
    /* wait for a PMT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
	}
	pthread_mutex_unlock(&demuxMutex);
	

    /* get audio and video pids */
    int16_t audioPid = -1;
    int16_t videoPid = -1;
    uint8_t i = 0;
	teletextExists=false;
    for (i = 0; i < pmtTable->elementaryInfoCount; i++)
    {
        if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x1) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x2) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x1b))
  && (videoPid == -1))
        {
			/*check if the stream is started first time,use the config PID*/
			if(firstPassVideo==1)
			{
				if(inputVideoPID==pmtTable->pmtElementaryInfoArray[i].elementaryPid)
				{
            				videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
					printf("Video Pid:%d\n",videoPid);
					
				}
				else
				{
					videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
					printf("Input video PID is incorect.Please check the channel PID!\n");
					printf("input video pid %d \n",inputVideoPID);
					printf("Video Pid: %d\n",videoPid);
				}
				
				firstPassVideo=0;
			}
				else/*not the first pass*/
			{
				videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
				printf("Video Pid: %d\n",videoPid);
			}       
		} 
        else if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x3) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x4))
            && (audioPid == -1))
        {
			/*check if the stream is started first time,use the config PID*/
			if(firstPassAudio==1)
			{
				if(inputAudioPID== pmtTable->pmtElementaryInfoArray[i].elementaryPid)
				{
            				audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
					printf("Audio Pid:%d\n",audioPid);
		
				}
				else
				{
					audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
					printf("Input audio PID is incorect.Please check the channel PID!\n");
					printf("input audio PID %d \n",inputAudioPID);
					printf("Audio Pid: %d\n",audioPid );
				}
				
				firstPassAudio=0;
			}
				else /*not the first pass*/
			{
				audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
				printf("Audio Pid: %d\n",audioPid);
			}       
            
        }
	if(teletextExists==false)
		if(pmtTable->pmtElementaryInfoArray[i].streamType == 0x06)
        {
        	teletextExists = true;
        }
	}
  	/*no video PID, its a radio service*/
	 if (audioPid != -1 && videoPid != -1){
    	flagCH=true;
    	
    }
	
	if (audioPid != -1)
    {   
        /* remove previos audio stream */
        if (streamHandleA != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
            streamHandleA = 0;
        }

	    /* create audio stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, audioPid, audioStreamType, &streamHandleA))
        {
            printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }
    if (videoPid != -1) 
    {
        /* remove previous video stream */
        if (streamHandleV != 0)
        {
		    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
        }

        /* create video stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, videoPid,videoStreamType , &streamHandleV))
        {
            printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }
	else
	{
		flagCH=false;
	/* remove previous video stream */
		if (streamHandleV != 0)
			{
					Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
				streamHandleV = 0;
			
			}
	}

    
	//creating a thread for graphics function
     memset(&timerSpec,0,sizeof(timerSpec));
    timerSpec.it_value.tv_sec = 3;

    if (pthread_create(&scThread, NULL, &graphics,NULL))
    {
        printf("Error creating input event task!\n");
    }
    /* set the new timer specs */
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
    {
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
    /* store current channel info */
    currentChannel.programNumber = channelNumber + 1;
    currentChannel.audioPid = audioPid;
    currentChannel.videoPid = videoPid;

/***********************************************************************************************/
	/* free PMT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);
    
    	 /* set demux filter for receive SDT table of program */
    if(Demux_Set_Filter(playerHandle, 0x11, 0x42, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return;
	}
    
    /* wait for a SDT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
	}
	pthread_mutex_unlock(&demuxMutex);
/**********************************************************************************************/

	 printf("ovde sam \n");
		
}

void* streamControllerTask(input_struct* inputStruct)
{
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    /* allocate memory for PAT table section */
    patTable=(PatTable*)malloc(sizeof(PatTable));
    if(patTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(patTable, 0x0, sizeof(PatTable));

    /* allocate memory for PMT table section */
    pmtTable=(PmtTable*)malloc(sizeof(PmtTable));
    if(pmtTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(pmtTable, 0x0, sizeof(PmtTable));

	    /* allocate memory for SDT table section */
    sdtTable=(SdtTable*)malloc(sizeof(SdtTable));
    if(sdtTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(sdtTable, 0x0, sizeof(SdtTable));
       
    /* initialize tuner device */
    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
	free(sdtTable);
        return (void*) SC_ERROR;
    }
    
    /* register tuner status callback */
    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}
    
    /* lock to frequency */
	
    if(!Tuner_Lock_To_Frequency(inputStruct->freq, inputStruct->bandwith,moduleConvertFun(inputStruct->module)/* DVB_T(t_Module)inputStruct->module*/))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,inputStruct->freq);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,inputStruct->freq);
        free(patTable);
        free(pmtTable);
	free(sdtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    
    /* wait for tuner to lock */
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n",__FUNCTION__);
        free(patTable);
        free(pmtTable);
	free(sdtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    pthread_mutex_unlock(&statusMutex);
   
    /* initialize player */
    if(Player_Init(&playerHandle))
    {
		printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
		free(patTable);
        free(pmtTable);
		free(sdtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	
	/* open source */
	if(Player_Source_Open(playerHandle, &sourceHandle))
    {
		printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
		free(patTable);
    	free(pmtTable);
		free(sdtTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;	
	}

	/* set PAT pid and tableID to demultiplexer */
	if(Demux_Set_Filter(playerHandle, 0x00, 0x00, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
	}
	
	/* register section filter callback */
    if(Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
    {
		printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
	}

    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s:ERROR Lock timeout exceeded!\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
		free(sdtTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	pthread_mutex_unlock(&demuxMutex);
    
    /* start current channel */
    startChannel(programNumber);
    
    /* set isInitialized flag */
    isInitialized = true;

    while(!threadExit)
    {
        if (changeChannel)
        {
            changeChannel = false;
            startChannel(programNumber);
        }
		if(changeVolume)
		{
			changeVolume=false;
			setVolume();
		}
    }
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
	int j=0;	
    uint8_t tableId = *buffer; 
	printf("table ID: %d \n",tableId); 
    if(tableId==0x00)
    {
        //printf("\n%s -----PAT TABLE ARRIVED-----\n",__FUNCTION__);
        
        if(parsePatTable(buffer,patTable)==TABLES_PARSE_OK)
        {
            //printPatTable(patTable);
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
            
        }
    } 
    else if (tableId==0x02)
    {
        //printf("\n%s -----PMT TABLE ARRIVED-----\n",__FUNCTION__);
        
        if(parsePmtTable(buffer,pmtTable)==TABLES_PARSE_OK)
        {
            //printPmtTable(pmtTable);
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
        }
    }
		/*check if SDT table has arrived*/
	else if (tableId==0x42)
	{
		printf("\n%s -----SDT TABLE ARRIVED-----\n",__FUNCTION__);
        /*parse SDT table*/
        if(parseSdtTable(buffer,sdtTable)==TABLES_PARSE_OK)
        {
            //printPatTable(patTable);
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
            
        }
		
	for(j=0;j<7;j++)
	{
		strcpy(serviceName[j],sdtTable->sdtElementaryInfoArray[j].descriptor.serviceName);
		printf("serfviceName:%s\n",serviceName[j]);
		serviceType[j]=sdtTable->sdtElementaryInfoArray[j].descriptor.serviceType;
		printf("service Type:%d\n",serviceType[j]);
	}	
	
	}
    return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
    if(status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n%s -----TUNER LOCKED-----\n",__FUNCTION__);
    }
    else
    {
        printf("\n%s -----TUNER NOT LOCKED-----\n",__FUNCTION__);
    }
    return 0;
}

t_Module moduleConvertFun(char* string)
{
	if(strcmp(string,"DVB_T")==0)
	{
		return DVB_T;
	}else if(strcmp(string,"DVB_T2")==0)
	{
		return DVB_T2;
	}else
	{
		printf("error in input  module Standard!\n");
	}
}

tStreamType streamConvertFun(int8_t streamType)
{
	if(streamType==10)
	{
		return AUDIO_TYPE_MPEG_AUDIO;
	}
	else if(streamType==11)
	{
		return AUDIO_TYPE_MP3;
	}
	else if(streamType==43)
	{
		return VIDEO_TYPE_MPEG1;
	}
	else if(streamType==42)
	{
		return VIDEO_TYPE_MPEG2;
	}
	else if(streamType==39)
	{
		return VIDEO_TYPE_H264;
	}
	else if(streamType==2)
	{
		return AUDIO_TYPE_DOLBY_PLUS;
	}
	else if(streamType==3)
	{
		return AUDIO_TYPE_DOLBY_TRUE_HD;
	}
	else
	{
		printf("error in input stream type!\n");
	}
}

void onInfoPressed()
{
	printf("onInfoPressed\n");
	int32_t ret;
	timer_gettime(timerId, &timerSpec);
	
	if(timerSpec.it_value.tv_sec < 3 && timerSpec.it_value.tv_sec > 0)
   	{
		memset(&timerSpec,0,sizeof(timerSpec));
		timerSpec.it_value.tv_sec = 3;
		
		/* set the new timer specs */
		ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
		if(ret == -1)
		{
		    printf("Error setting timer in %s!\n", __FUNCTION__);
		}
	}
	else
	{
		graphics();	
	
		memset(&timerSpec,0,sizeof(timerSpec));
		timerSpec.it_value.tv_sec = 3;
		
		/* set the new timer specs */
		ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
		if(ret == -1)
		{
		    printf("Error setting timer in %s!\n", __FUNCTION__);
		}			
	}	
}

void onVolumePressed()
{
	int32_t ret;
	timer_gettime(timerIdVolume, &timerSpecVolume);
	/*check if the timer tame has exceeded */
	if(timerSpecVolume.it_value.tv_sec < 3 && timerSpecVolume.it_value.tv_sec > 0)
   	{
		memset(&timerSpecVolume,0,sizeof(timerSpecVolume));
		timerSpecVolume.it_value.tv_sec = 3;
		
		/* set the new timer specs */
		ret = timer_settime(timerIdVolume,0,&timerSpecVolume,&timerSpecOldVolume);
		if(ret == -1)
		{
		    printf("Error setting timer in %s!\n", __FUNCTION__);
		}
	}
	/*timer time has exceeded*/
	else
	{
		graphicVolume(volume);	
	
		memset(&timerSpecVolume,0,sizeof(timerSpecVolume));
		timerSpecVolume.it_value.tv_sec = 3;
		
		/* set the new timer specs */
		ret = timer_settime(timerIdVolume,0,&timerSpecVolume,&timerSpecOldVolume);
		if(ret == -1)
		{
		    printf("Error setting timer in %s!\n", __FUNCTION__);
		}			
	}	
}


