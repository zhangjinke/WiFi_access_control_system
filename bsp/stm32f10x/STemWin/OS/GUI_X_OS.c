#include "GUI.h"
    
/* RT Thread include files */
#include "rtthread.h"
    
/*********************************************************************
*
* Global data
*/
static rt_sem_t DispSem;  	//显示的信号量
static rt_sem_t EventSem;  

//static rt_sem_t	KeySem;  	//按键信号量
//static int		KeyPressed;
//static char		KeyIsInited;
/*********************************************************************
*
* Timing:
* GUI_X_GetTime()
* GUI_X_Delay(int)

Some timing dependent routines require a GetTime
and delay function. Default time unit (tick), normally is
1 ms.
*/

int GUI_X_GetTime(void)
{
	return ((int) rt_tick_get());
}

void GUI_X_Delay(int ms)
{
	rt_thread_delay(rt_tick_from_millisecond(ms));
}

/*********************************************************************
*
* GUI_X_Init()
*
* Note:
* GUI_X_Init() is called from GUI_Init is a possibility to init
* some hardware which needs to be up and running before the GUI.
* If not required, leave this routine blank.
*/

void GUI_X_Init(void) {
}


/*********************************************************************
*
* GUI_X_ExecIdle
*
* Note:
* Called if WM is in idle state
*/

void GUI_X_ExecIdle(void)
{
	GUI_X_Delay(1);
}

/*********************************************************************
*
* Multitasking:
*
* GUI_X_InitOS()
* GUI_X_GetTaskId()
* GUI_X_Lock()
* GUI_X_Unlock()
*
* Note:
* The following routines are required only if emWin is used in a
* true multi task environment, which means you have more than one
* thread using the emWin API.
* In this case the
* #define GUI_OS 1
* needs to be in GUIConf.h
*/

/* Init OS */
void GUI_X_InitOS(void)
{ 
	DispSem = rt_sem_create("Disp_SEM",1,RT_IPC_FLAG_FIFO);
	EventSem = rt_sem_create("Event_SEM",0,RT_IPC_FLAG_FIFO);
}

void GUI_X_Unlock(void)
{ 
	rt_sem_release(DispSem);
}

void GUI_X_Lock(void)
{
	rt_sem_take(DispSem,RT_WAITING_FOREVER);
}

/* Get Task handle */
U32 GUI_X_GetTaskId(void) 
{
	return 0;
}


void GUI_X_WaitEvent (void) 
{
	rt_sem_take(EventSem,RT_WAITING_FOREVER);
}


void GUI_X_SignalEvent (void) 
{
	rt_sem_release(EventSem);
}

/*********************************************************************
*
* Logging: OS dependent

Note:
Logging is used in higher debug levels only. The typical target
build does not use logging and does therefor not require any of
the logging routines below. For a release build without logging
the routines below may be eliminated to save some space.
(If the linker is not function aware and eliminates unreferenced
functions automatically)

*/

void GUI_X_Log (const char *s) { }
void GUI_X_Warn (const char *s) { }
void GUI_X_ErrorOut(const char *s) { }

/*************************** End of file ****************************/
