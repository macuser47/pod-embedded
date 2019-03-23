/***
 *  Filename: states.c
 *
 *  Summary: All of the in-depth functionality of each state. Each state has its
 *  own actions and transitions and this is where all of the states actions are
 *  defined. It also has a number of helper functions to simplify the states
 *  actions.
 *
 */

/* Includes */
#include <stdio.h>
#include <stdbool.h>

#include "state_machine.h"
#include "data.h"


/* Defines */

/* Pressure sensor acceptable limits (in PSI) */
#define PS1_BOTTOM_LIMIT        1000
#define PS1_TOP_LIMIT           3000
#define PS2_BOTTOM_LIMIT        1000
#define PS2_TOP_LIMIT           3000
#define PS3_BOTTOM_LIMIT        1000
#define PS3_TOP_LIMIT           3000
#define PS4_BOTTOM_LIMIT        0
#define PS4_TOP_LIMIT           20
/**/

#define MAX_BATT_TEMP			60	/* Degrees Celcius */
#define MAX_STOPPED_ACCEL		0.3 /* Limit for qualifying the pod as stopped in m/s */

/* In mV */
#define MAX_CELL_VOLTAGE        4200    
#define MIN_CELL_VOLTAGE        3000

#define TUBE_LENGTH             3800    /*in ft //FIXME I could be wrong on this on haha */

#define MAX_RUN_TIME            30  /* in seconds, TODO get the real number */

/* Imports/Externs */

extern stateMachine_t stateMachine;
extern data_t *data;
static bool checkPrimPressures(void);
static bool checkStopped(void);

extern stateTransition_t *findTransition(state_t *currState, char *name);

/***
 * checkPrimPressures - Compares the readings from the pressure sensors on
 * the primary brakes agaainst expected values. Applicable only for pre-braking
 * pressures
 *
 * RETURNS: true if everything is ok, false if there is an issue
 */

static bool checkPrimPressures(void) {
    bool noProblem = true;
    if (data->pressure->ps1 < PS1_BOTTOM_LIMIT || data->pressure->ps1 > PS1_TOP_LIMIT) {
        fprintf(stderr, "Tank pressure failing\n");
        noProblem = false;
    }
    if (data->pressure->ps2 < PS2_BOTTOM_LIMIT || data->pressure->ps2 > PS2_TOP_LIMIT) {
        fprintf(stderr, "Line pressure failing\n");
        noProblem = false;
    }
    if (data->pressure->ps3 < PS3_BOTTOM_LIMIT || data->pressure->ps3 > PS3_TOP_LIMIT) {
        fprintf(stderr, "Line pressure failing\n");
        noProblem = false;
    }
    if (data->pressure->ps4 < PS4_BOTTOM_LIMIT || data->pressure->ps4 > PS4_TOP_LIMIT) {
        fprintf(stderr, "Line pressure failing\n");
        noProblem = false;
    }

    return noProblem;
}

/***
 * checkStopped - checks a variety of values to make sure the pod is stopped.
 *
 * RETURNS: true if stopped, false if moving
 */

static bool checkStopped(void) {
	return data->motion->accel < MAX_STOPPED_ACCEL && 
        (time() - data->timers->lastRetro) > 15;
}


static bool checkBattTemp(void) {
    return data->bms->highTemp < MAX_BATT_TEMP; 
}
/***
 * Actions for all the states.
 * They perform transition and error condition
 * checking and perform the functionality of the state: e.g. brake if in
 * braking.
 *
 */

stateTransition_t * powerOnAction() {
    return findTransition(stateMachine.currState, IDLE_NAME);
}

stateTransition_t * idleAction() {
    // First check for nominal values?
    
    if(!checkPrimPressures() || !checkStopped()){
        STATE_ERROR();
    }

    if(data->flags->readyPump){
        return findTransition(stateMachine.currState, READY_FOR_PUMPDOWN_NAME);
    }

    return NULL;
}

stateTransition_t * readyForPumpdownAction() {
    // First check for nominal values?

    if (!checkPrimPressures() || !checkBattTemp()) {
	return findTransition(stateMachine.currState, PRE_RUN_FAULT_NAME);
    }

    if (data->flags->pumpDown){
        return findTransition(stateMachine.currState, PUMPDOWN_NAME);
    }

    return NULL;
}

stateTransition_t * pumpdownAction() {
    // First check for nominal values?

    if(data->flags->readyCommand){
	return findTransition(stateMachine.currState, READY_NAME);
    }

    return NULL;
}

stateTransition_t * readyForLaunchAction() {
    // First check for nominal values?
    
    if(data->flags->propulse){
        return findTransition(stateMachine.currState, PROPULSION_NAME);
    }

    return NULL;
}

stateTransition_t * propulsionAction() {
    if(data->flags->emergencyBrake){
	// If it's an emergency, shold we trigger braking first then go through the formalities of doing a state transition?
        return findTransition(stateMachine.currState, BRAKING_NAME);
    }

    // Check for nominal values?
    

    if (data->timers->global > MAX_RUN_TIME

    return NULL;
}

stateTransition_t * brakingAction() {
    /* transition to post run */
    if (!checkBattTemp()) 
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);
    if (checkStopped()) {
        return findTransition(stateMachine.currState, POST_RUN_NAME);
    }
    return NULL;
}

stateTransition_t * stoppedAction() {
    if (!checkBattTemp()) 
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);
    if (data->bms->cellMaxVoltage > MAX_CELL_VOLTAGE || data->cellMinVoltage < MIN_CELL_VOLTAGE)
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);


	return NULL;
}

stateTransition_t * crawlAction() {
    if (!checkBattTemp())
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);
    if (data->bms->cellMaxVoltage > MAX_CELL_VOLTAGE || data->cellMinVoltage < MIN_CELL_VOLTAGE)
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);
    if (data->pressures->pv < 13)
        return findTransition(stateMachine.currState, RUN_FAULT_NAME);

    if (data->motion->position >= (TUBE_LENGTH - 100))
        return findTransition(stateMachine.currState, BRAKING_NAME);
    
	return NULL;
}

stateTransition_t * postRunAction() {
	if (!checkBattTemp()) {
        return findTransition(stateMachine.currState, POST_FAULT_NAME);
    }
    
    if (data->timers->timeInState >= 30 && data->bms->packVoltage > 0) {
        return findTransition(stateMachine.currState, POST_FAULT_NAME);
    }

    if (data->bms->packVoltage == 0 && data->ps1 < 30) {
        return findTransition(stateMachine.currState, SAFE_TO_APPROACH_NAME);
    }

    return NULL;
}

stateTransition_t * safeToApproachAction() {
	return NULL;
}

stateTransition_t * secondaryBrakingAction() {
	return NULL;
}

stateTransition_t * preFaultAction() {
	return NULL;
}

stateTransition_t * runFaultAction() {
	return NULL;
}

stateTransition_t * postFaultAction() {
	return NULL;
}
