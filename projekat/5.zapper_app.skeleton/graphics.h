#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#include <directfb.h>
#include <signal.h>
#include <time.h>
#include "stream_controller.h"
#define FRAME_THICKNESS 20
#define FONT_HEIGHT 150
#define EXIT_BUTTON_KEYCODE 102



/* helper macro for error checking */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}
 /**
 * @brief prints info banner
 *
 * @param [out] 
 * @return
 */
void* graphics(  );
/**
 * @brief clears the surface
 *
 * @param [out] signal timer structure
 * @return
 */

void wipeScreen(union sigval signalArg);

void wipeScreenVolume(union sigval signalArga);
/**
 * @brief graphics for volume bar presenting
 *
 * @param [out] the number of the volume that is going to be set
 * @return
 */
void graphicVolume(int8_t volumeNumber);

