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
void drawKeycode(int32_t keycode);
 void wipeScreen(union sigval signalArg);
void drawInfo(); 

