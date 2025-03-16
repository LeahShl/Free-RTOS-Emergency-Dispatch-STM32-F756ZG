/*
 * edconfig.h
 *
 *  Created on: Mar 14, 2025
 *      Author: leah
 */

#ifndef SRC_EDCONFIG_H_
#define SRC_EDCONFIG_H_

/**
 * Emergency events
 */
#define CODE_PLC 1 // Police event code
#define CODE_AMB 2 // Ambulance event code
#define CODE_FIR 3 // Fire event code

/**
 * Number of resources
 */
#define N_PCR 3 // Police cab #
#define N_AMB 4 // Ambulance #
#define N_FTR 2 // Fire truck #

/**
 * Time consts
 */
#define MIN_DELAY 1000 // Min amount of miliseconds for event generation
#define MAX_DELAY 5000 // Max amount of miliseconds for event generation
#define HNDL_DELAY 3000 // Event handling time in ms

/**
 * Priorities
 */
#define PR_GEN 32 // Priority of event generator
#define PR_DIS 32 // Priority of dispatcher
#define PR_PLC 24 // Priority of police
#define PR_AMB 24 // Priority of ambulance
#define PR_FIR 24 // Priority of fire department
#define PR_LOG 16 // Priority of logger

/**
 * Log events
 */
#define LOG_LEN 256 // Log message length
#define LOG_CREATE 1 // Event created
#define LOG_SENT 2 // Event sent
#define LOG_PLC_RCV 3 // Police received event
#define LOG_AMB_RCV 4 // Ambulance received event
#define LOG_FIR_RCV 5 // Fire department received event
#define LOG_PLC_HNDL 6 // Police handled event
#define LOG_AMB_HNDL 7 // Ambulance handled event
#define LOG_FIR_HNDL 8 // Fire department handled event
#define LOG_ERR 9 // Error


/**
 * Misc.
 */
#define QUEUE_SIZE 10 // Size of queue


#endif /* SRC_EDCONFIG_H_ */
