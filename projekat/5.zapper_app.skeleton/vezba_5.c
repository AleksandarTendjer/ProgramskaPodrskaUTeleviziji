#include "remote_controller.h"
#include "stream_controller.h"

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

void readConf(FILE *);
input_struct* inputData;

int main(int argc, char *argv[])
{
	FILE *fptr;
	//creating a string for a config fiole name 
	int v = strlen(argv[1]); // for allocating memroy
    	char *str = (char *)malloc(v);

	printf("alocirao\n");
    	strcat(str, argv[1]);
	//openning a config file 
	 fptr = fopen(str,"r");
	  inputData = (input_struct*) malloc(sizeof(input_struct));
  		
		readConf(fptr);
	
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
			break;
		case KEYCODE_P_PLUS:
			printf("\nCH+ pressed\n");
            		channelUp();
			break;
		case KEYCODE_P_MINUS:
		    printf("\nCH- pressed\n");
            		channelDown();
			break;
		case KEYCODE_EXIT:
			printf("\nExit pressed\n");
            pthread_mutex_lock(&deinitMutex);
		    pthread_cond_signal(&deinitCond);
		    pthread_mutex_unlock(&deinitMutex);
			break;
		default:
			printf("\nPress P+, P-, info or exit! \n\n");
	}
}

void readConf(FILE *fp)
{	
	//char ch = getc(fp);
	fscanf(fp,"%d %d %s %d %d %d %d %d",&inputData->freq,&inputData->bandwith,inputData->module,&inputData->audioPID,&inputData->videoPID,&inputData->audioType,&inputData->videoType,&inputData->programNumber);
	
	/*while(1)
	{
	//printf("%d",inputData->audioPID);
	printf("uspeo \n");
	}*/
	/*int8_t lineCount=0;
	int8_t len=0;
	char line[10];
	while(ch!=EOF)
	{
		if(lineCount!=0)
			ch=getc(fp);

		while(ch!='\n')
		{
			line[len]=ch;
			len++;
			ch=getc(fp);
		}
		//len++;
		line[len]='\0';
		lineCount++;

		switch(lineCount)
		{
			case 1:
				inputData->freq=atoi(line);
				printf("%d \n",inputData->freq);
				line[0]='\0';
				len=0;
				break;
			case 2:
				inputData->bandwith=atoi(line);
				printf("%d \n",inputData->bandwith);
				line[0]='\0';
				len=0;
				break;
			case 3:
				strcpy(inputData->module,line);
				printf("%s \n",inputData->module);
				line[0]='\0';
				len=0;
				break;
			case 4:
				inputData->audioPID=atoi(line);
				printf("%d \n",inputData->audioPID);
				line[0]='\0';
				len=0;
				break;
			case 5:
				inputData->videoPID=atoi(line);
				printf("%d \n",inputData->videoPID);
				line[0]='\0';
				len=0;
				break;
			case 6:
				inputData->audioType=atoi(line);
				
				line[0]='\0';
				len=0;
				break;
			case 7:
				inputData->videoType=atoi(line);
				line[0]='\0';
				len=0;
				break;
			case 8:
				inputData->programNumber=atoi(line);
				printf("%d \n",inputData->programNumber);
				line[0]='\0';
				len=0;
				return 0;
				
		}
	} */
} 


