#include "graphics.h"
//#include "flag.h"

static IDirectFBSurface *primary = NULL;

DFBSurfaceDescription surfaceDesc;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;
IDirectFBSurface *logoSurface = NULL;
int32_t logoHeight, logoWidth;
IDirectFBImageProvider *provider;
//
 IDirectFB *dfbInterface;
 //ChannelInfo currentChannel;

 bool teletextExists;
//


static DFBRegion flipRegion;

static int16_t flagVolume = 0;
bool teletextExists;
char name[1000];

static char* images[11] = {"volume_0.png", "volume_1.png", "volume_2.png", "volume_3.png", "volume_4.png", "volume_5.png", "volume_6.png", "volume_7.png", "volume_8.png", "volume_9.png","volume_10.png"}; 


void wipeScreen(union sigval signalArg)
{ 
	printf("WipeScreen banner\n");
	int32_t ret;
	//signalArg.ptr
    if(currentChannel.videoPid!=-1)
    {
    	printf("WipeScreen banner (Stream)\n");
		// clear screen
		
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
		DFBCHECK(primary->FillRectangle(primary, 0, 4*screenHeight/5, screenWidth, screenHeight/5));
		
		flipRegion.x1 = 0;
		flipRegion.y1 = 4*screenHeight/5;
		flipRegion.x2 = screenWidth;
		flipRegion.y2 = screenHeight;
		
		DFBCHECK(primary->Flip(primary, &flipRegion, 0));
    }
    else
    {
    	printf("WipeScreen banner(RADIO)\n");
    	//funkcijazatestiranje();
    	DFBCHECK(primary->SetColor(primary, 0x0f, 0x0f, 0x0f, 0xff));
		DFBCHECK(primary->FillRectangle(primary, 0, 4*screenHeight/5, screenWidth, screenHeight/5));
		
		flipRegion.x1 = 0;
		flipRegion.y1 = 4*screenHeight/5;
		flipRegion.x2 = screenWidth;
		flipRegion.y2 = screenHeight;
		
		DFBCHECK(primary->Flip(primary, &flipRegion, 0));
    }
     
    // stop the timer
    memset(&timerSpec,0,sizeof(timerSpec));
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1)
    {
        printf("Error setting timer in %s!\n", __FUNCTION__);
    } 
}

void wipeScreenVolume(union sigval signalArga)
{
	printf("WipeScreen Volume\n");
	int32_t ret;
	if(flagVolume ==1)
	{
		if(currentChannel.videoPid!=-1)
		{
			DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
			DFBCHECK(primary->FillRectangle(primary, screenWidth-240, 40, logoWidth, logoHeight));
		}
		else
		{
			DFBCHECK(primary->SetColor(primary, 0x0f, 0x0f, 0x0f, 0xff));
			DFBCHECK(primary->FillRectangle(primary, screenWidth-240, 40, logoWidth+5, logoHeight+5));	
		}
	
		flipRegion.x1 = screenWidth-240;
		flipRegion.y1 = 40;
		flipRegion.x2 = screenWidth-240+logoWidth;
		flipRegion.y2 = 40+logoHeight;
		
		DFBCHECK(primary->Flip(primary,&flipRegion,0));
		
		memset(&timerSpecVolume,0,sizeof(timerSpecVolume));
		ret = timer_settime(timerIdVolume,0,&timerSpecVolume,&timerSpecOldVolume);
		if(ret == -1)
		{
		    printf("Error setting timer in %s!\n", __FUNCTION__);
		} 
    }
}

void* graphics()
{	

	if(flagCH==false){
		printf("prosao flag\n");
		
		/*Set screen black*/ 
		DFBCHECK(primary->SetColor(primary,0x0f,0x0f,0x0f,0xff));
	  	DFBCHECK(primary->FillRectangle(primary,0,0,screenWidth,screenHeight));
	    /*Draw banner*/
        DFBCHECK(primary->SetColor(primary,0x00,0x00,0x4f,0xff));
     	DFBCHECK(primary->FillRectangle(primary,0,4*screenHeight/5,screenWidth,screenHeight/5));
		/*Set colour for text*/
        DFBCHECK(primary->SetColor(primary,0xff,0xff,0xff,0xff));	
        
        /* draw text */
		IDirectFBFont *fontInterface = NULL;
		DFBFontDescription fontDesc;
	
		/* specify the height of the font by raising the appropriate flag and setting the height value */
		fontDesc.flags = DFDESC_HEIGHT;
		fontDesc.height = 48;
	
		/* create the font and set the created font for primary surface text drawing */
		DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
		DFBCHECK(primary->SetFont(primary, fontInterface));  	                                       	
        DFBCHECK(primary->DrawString(primary, "RADIO", -1, 200, 100, DSTF_LEFT));
		
		if(teletextExists == false)
		{
			sprintf(name, "PN: %d    videoPID: %d    audioPID: %d    TTX: NO", currentChannel.programNumber, currentChannel.videoPid,  currentChannel.audioPid);
			DFBCHECK(primary->DrawString(primary,name , -1, 120, 7*screenHeight/8, DSTF_LEFT));		
		}
		else
		{
			sprintf(name, "PN: %d    videoPID: %d    audioPID: %d    TTX: YES", currentChannel.programNumber, currentChannel.videoPid,  currentChannel.audioPid);
			DFBCHECK(primary->DrawString(primary,name , -1, 120, 7*screenHeight/8, DSTF_LEFT));	
		}

		DFBCHECK(primary->Flip(primary,NULL,0));
	}
	else
	{
	
		/* create primary surface with double buffering enabled */
		surfaceDesc.flags = DSDESC_CAPS;
		surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
		DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
	   
		/* fetch the screen size */
		DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));

		/*Banner for stream*/
    	DFBCHECK(primary->SetColor(primary,0x00,0x00,0x4f,0xff));                                 
		DFBCHECK(primary->FillRectangle(primary,0,4*screenHeight/5,screenWidth,screenHeight/5));
           
        /*Set colour for text*/                         
        DFBCHECK(primary->SetColor(primary,0xff,0xff,0xff,0xff)); 
        
        /* draw text */
		IDirectFBFont *fontInterface = NULL;
		DFBFontDescription fontDesc;
	
		/* specify the height of the font by raising the appropriate flag and setting the height value */
		fontDesc.flags = DFDESC_HEIGHT;
		fontDesc.height = 48;
	
		/* create the font and set the created font for primary surface text drawing */
		DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
		DFBCHECK(primary->SetFont(primary, fontInterface));   
		
		          			                                                 
		if(teletextExists == false)
		{
			sprintf(name, "PN: %d    videoPID: %d    audioPID: %d    TTX: NO", currentChannel.programNumber, currentChannel.videoPid,  currentChannel.audioPid);
			DFBCHECK(primary->DrawString(primary,name , -1, 120, 7*screenHeight/8, DSTF_LEFT));		
		}
		else
		{
			sprintf(name, "PN: %d    videoPID: %d    audioPID: %d    TTX: YES", currentChannel.programNumber, currentChannel.videoPid,  currentChannel.audioPid);
			DFBCHECK(primary->DrawString(primary,name , -1, 120, 7*screenHeight/8, DSTF_LEFT));	
		}

		flipRegion.x1 = 0;
		flipRegion.y1 = 4*screenHeight/5;
		flipRegion.x2 = screenWidth;
		flipRegion.y2 = screenHeight;
		
		DFBCHECK(primary->Flip(primary, &flipRegion, 0));
		
	}
	
    return (void*)NO_ERROR;
} 

void graphicVolume(int8_t volumeNumber)
{
	printf("GraphicsVOLUMEE\n");
	/* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, images[volumeNumber], &provider));
    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));

    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	
	DFBCHECK(primary->Blit(primary,logoSurface,NULL,screenWidth-240,40));

    /* switch between the displayed and the work buffer (update the display) */
    
    flipRegion.x1 = screenWidth-240;
	flipRegion.y1 = 40;
	flipRegion.x2 = screenWidth-240+logoWidth;
	flipRegion.y2 = 40+logoHeight;
		
	DFBCHECK(primary->Flip(primary,&flipRegion,0));
	
	flagVolume=1;

	return;

}


