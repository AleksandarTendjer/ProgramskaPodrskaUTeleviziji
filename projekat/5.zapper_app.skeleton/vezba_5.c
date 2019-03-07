#include "remote_controller.h"
#include "stream_controller.h"
//#include "graphics.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

/* macro function for error checking */
#define ERRORCHECK(x)                                                       \
{                                                                           \
if (x != 0)                                                                 \
 {                                                                          \
    textColor(1,1,0);                                                       \
    printf(" Error!\n File: %s \t Line: <%d>\n", __FILE__, __LINE__);       \
    textColor(0,7,0);                                                       \
    return -1;                                                              \
 }                                                                          \
}

static void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value);
static pthread_cond_t deinitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t deinitMutex = PTHREAD_MUTEX_INITIALIZER;
static ChannelInfo channelInfo;
int8_t currChannel=0;
void readConf(FILE *);
input_struct* inputData;
int main(int argc, char *argv[])
{
	FILE *fptr;
	//creating a string for a config fiole name 
	int v = strlen(argv[1]); // for allocating memroy
    	char *str = (char *)malloc(v);

    	strcat(str, argv[1]);
	//openning a config file 
	 fptr = fopen(str,"r");
	  inputData = (input_struct*) malloc(sizeof(input_struct));
  		
		readConf(fptr);
		currChannel=inputData->programNumber;
    /* initialize remote controller module */
    ERRORCHECK(remoteControllerInit());
    
    /* register remote controller callback */
    ERRORCHECK(registerRemoteControllerCallback(remoteControllerCallback));
  
    /* initialize stream controller module */
    ERRORCHECK(streamControllerInit(inputData));
	

    /* wait for a EXIT remote controller key press event */
    pthread_mutex_lock(&deinitMutex);
	if (ETIMEDOUT == pthread_cond_wait(&deinitCond, &deinitMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&deinitMutex);
    
    /* unregister remote controller callback */
    ERRORCHECK(unregisterRemoteControllerCallback(remoteControllerCallback));

    /* deinitialize remote controller module */
    ERRORCHECK(remoteControllerDeinit());

    /* deinitialize stream controller module */
    ERRORCHECK(streamControllerDeinit());
    free(inputData);
	free(str);
  	fclose(fptr);
    return 0;
}

void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value)
{
    switch(code)
	{
		case KEYCODE_INFO:
            printf("\nInfo pressed\n");          
            if (getChannelInfo(&channelInfo) == SC_NO_ERROR)
            {
                printf("\n********************* Channel info *********************\n");
                printf("Program number: %d\n", channelInfo.programNumber);
                printf("Audio pid: %d\n", channelInfo.audioPid);
                printf("Video pid: %d\n", channelInfo.videoPid);
                printf("**********************************************************\n");
            }
			onInfoPressed();
			break;
		case KEYCODE_P_PLUS:
			printf("\nCH+ pressed\n");
    		channelUp();
			currChannel++;
			break;
		case KEYCODE_P_MINUS:
		    printf("\nCH- pressed\n");
    		channelDown();
			currChannel--;
			break;
		case KEYCODE_EXIT:
			printf("\nExit pressed\n");
            pthread_mutex_lock(&deinitMutex);
		    pthread_cond_signal(&deinitCond);
		    pthread_mutex_unlock(&deinitMutex);
			break;
		//channel number pressed
		case 2:
			printf("Changing to Channel 1!\n");
			if(currChannel!=1)
			{
				
				switchChannel(1);
			}
			currChannel=1;			
			break;
		case 3:
			printf("Changing to Channel 2!\n");
			if(currChannel!=2)
			{
				
				switchChannel(2);
			}	
			currChannel=2;
			break;
		case 4:
			printf("Changing to Channel 3!\n");
			
			if(currChannel!=3)
			{
				
				switchChannel(3);
			}
			
			currChannel=3;
			break;
		case 5:
			printf("Changing to Channel 4!\n");
			
			if(currChannel!=4)
			{
				
				switchChannel(4);
			}
			printf("current Channel: %d\n",currChannel);
			currChannel=4;		
			break;
		case 6:
			printf("Changing to Channel 5!\n");
			
			if(currChannel!=5)
			{
				
				switchChannel(5);
			}
			
			currChannel=5;
			break;
		case 7:
			printf("Changing to Channel 6!\n");
			if(currChannel!=6)
			{
				
				switchChannel(6);
			}
			
			currChannel=6;
			break;
		case 8:
			printf("Changing to Channel 7!\n");
			if(currChannel!=7)
			{
				switchChannel(7);
			}
			
			currChannel=7;
			break;
		case VOLUME_UP:
			printf("Volume up!\n");
			volumeUp();	
			onVolumePressed();
			break;
		case VOLUME_DOWN:
			printf("Volume down!\n");
			volumeDown();
			onVolumePressed();
			break;
		case MUTE:
			printf("Muted!\n");
			muteVolume();
			onVolumePressed();
			break;
		default: /*No defined key pressed*/
			printf("\nPress P+, P-, info or exit! \n\n");
	}
}

void readConf(FILE *fp)
{	
	//char ch = getc(fp);
	fscanf(fp,"%d %d %s %d %d %d %d %d",&inputData->freq,&inputData->bandwith,inputData->module,&inputData->audioPID,&inputData->videoPID,&inputData->audioType,&inputData->videoType,&inputData->programNumber);
	
	
} 


