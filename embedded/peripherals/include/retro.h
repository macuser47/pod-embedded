#ifndef __RETRO_H__
#define __RETRO_H__

#define RETRO_1_PIN 	66		/* Temp pin, not the PCB one */
#define RETRO_2_PIN		67
#define RETRO_3_PIN		68

/* We map them, so they are indexed in a somewhat funky way */
#define RETRO_1			0
#define RETRO_2			1
#define RETRO_3			2

int initRetro(void);

#endif
