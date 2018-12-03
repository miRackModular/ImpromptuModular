//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//***********************************************************************************************


#include "PhraseSeqUtil.hpp"


int moveIndex(int index, int indexNext, int numSteps) {
	if (indexNext < 0)
		index = numSteps - 1;
	else
	{
		if (indexNext - index >= 0) { // if moving right or same place
			if (indexNext >= numSteps)
				index = 0;
			else
				index = indexNext;
		}
		else { // moving left 
			if (indexNext >= numSteps)
				index = numSteps - 1;
			else
				index = indexNext;
		}
	}
	return index;
}

bool moveIndexRunMode(int* index, int numSteps, int runMode, unsigned long* history) {// some of this code if from PS32EX)
	int reps = 1;
	// assert((reps * numSteps) <= 0xFFF); // for BRN and RND run modes, history is not a span count but a step count
	
	bool crossBoundary = false;
	
	switch (runMode) {
	
		// history 0x0000 is reserved for reset
		
		case MODE_REV :// reverse; history base is 0x2000
			if ((*history) < 0x2001 || (*history) > 0x2FFF)
				(*history) = 0x2000 + reps;
			(*index)--;
			if ((*index) < 0) {
				(*index) = numSteps - 1;
				(*history)--;
				if ((*history) <= 0x2000)
					crossBoundary = true;
			}
		break;
		
		case MODE_PPG :// forward-reverse; history base is 0x3000
			if ((*history) < 0x3001 || (*history) > 0x3FFF) // even means going forward, odd means going reverse
				(*history) = 0x3000 + reps * 2;
			if (((*history) & 0x1) == 0) {// even so forward phase
				(*index)++;
				if ((*index) >= numSteps) {
					(*index) = numSteps - 1 ;
					(*history)--;
				}
			}
			else {// odd so reverse phase
				(*index)--;
				if ((*index) < 0) {
					(*index) = 0;
					(*history)--;
					if ((*history) <= 0x3000)
						crossBoundary = true;
				}
			}
		break;

		case MODE_PEN :// forward-reverse; history base is 0x4000
			if ((*history) < 0x4001 || (*history) > 0x4FFF) // even means going forward, odd means going reverse
				(*history) = 0x4000 + reps * 2;
			if (((*history) & 0x1) == 0) {// even so forward phase
				(*index)++;
				if ((*index) >= numSteps) {
					(*index) = numSteps - 2;
					(*history)--;
					if ((*index) < 1) {// if back at 0 after turnaround, then no reverse phase needed
						(*index) = 0;
						(*history)--;
						if ((*history) <= 0x4000)
							crossBoundary = true;
					}
				}
			}
			else {// odd so reverse phase
				(*index)--;
				if ((*index) < 1) {
					(*index) = 0;
					(*history)--;
					if ((*history) <= 0x4000)
						crossBoundary = true;
				}
			}
		break;
		
		case MODE_BRN :// brownian random; history base is 0x5000
			if ((*history) < 0x5001 || (*history) > 0x5FFF) 
				(*history) = 0x5000 + numSteps * reps;
			(*index) += (randomu32() % 3) - 1;
			if ((*index) >= numSteps) {
				(*index) = 0;
			}
			if ((*index) < 0) {
				(*index) = numSteps - 1;
			}
			(*history)--;
			if ((*history) <= 0x5000) {
				crossBoundary = true;
			}
		break;
		
		case MODE_RND :// random; history base is 0x6000
		case MODE_RN2 :
			if ((*history) < 0x6001 || (*history) > 0x6FFF) 
				(*history) = 0x6000 + numSteps * reps;
			(*index) = (randomu32() % numSteps) ;
			(*history)--;
			if ((*history) <= 0x6000) {
				crossBoundary = true;
			}
		break;
		
		//case MODE_FW2 :// forward twice
		//case MODE_FW3 :// forward three times
		//case MODE_FW4 :// forward four times
		default :// MODE_FWD  forward; history base is 0x1000
			if (runMode == MODE_FW2) reps++;
			else if (runMode == MODE_FW3) reps += 2;
			else if (runMode == MODE_FW4) reps += 3;
			if ((*history) < 0x1001 || (*history) > 0x1FFF)
				(*history) = 0x1000 + reps;
			(*index)++;
			if ((*index) >= numSteps) {
				(*index) = 0;
				(*history)--;
				if ((*history) <= 0x1000)
					crossBoundary = true;
			}
	}

	return crossBoundary;
}
/*
bool moveIndexRunMode(int* index, int numSteps, int runMode, int* history) {		
	bool crossBoundary = false;
	int numRuns;// for FWx
	
	switch (runMode) {
	
		case MODE_REV :// reverse; history base is 1000 (not needed)
			(*history) = 1000;
			(*index)--;
			if ((*index) < 0) {
				(*index) = numSteps - 1;
				crossBoundary = true;
			}
		break;
		

		case MODE_PPG :// forward-reverse; history base is 2000
			if ((*history) != 2000 && (*history) != 2001) // 2000 means going forward, 2001 means going reverse
				(*history) = 2000;
			if ((*history) == 2000) {// forward phase
				(*index)++;
				if ((*index) >= numSteps) {
					(*index) = numSteps - 1 ;
					(*history) = 2001;
				}
			}
			else {// it is 2001; reverse phase
				(*index)--;
				if ((*index) < 0) {
					(*index) = 0;
					(*history) = 2000;
					crossBoundary = true;
				}
			}
		break;

		case MODE_PEN :// forward-reverse; history base is 6000
			if ((*history) != 6000 && (*history) != 6001) // 6000 means going forward, 6001 means going reverse
				(*history) = 6000;
			if ((*history) == 6000) {// forward phase
				(*index)++;
				if ((*index) >= numSteps) {
					(*index) = numSteps - 2;
					if ((*index) < 1) {// if back at 0 after turnaround, then no reverse phase needed
						crossBoundary = true;
						(*index) = 0;
					}
					else
						(*history) = 6001;
				}
			}
			else {// it is 6001; reverse phase
				(*index)--;
				if ((*index) < 1) {
					(*index) = 0;
					(*history) = 6000;
					crossBoundary = true;
				}
			}
		break;
		
		case MODE_BRN :// brownian random; history base is 3000
			if ( (*history) < 3000 || ((*history) > (3000 + numSteps)) ) 
				(*history) = 3000 + numSteps;
			(*index) += (randomu32() % 3) - 1;
			if ((*index) >= numSteps) {
				(*index) = 0;
			}
			if ((*index) < 0) {
				(*index) = numSteps - 1;
			}
			(*history)--;
			if ((*history) <= 3000) {
				(*history) = 3000 + numSteps;
				crossBoundary = true;
			}
		break;
		
		case MODE_RND :// random; history base is 4000
		case MODE_RN2 :
			if ( (*history) < 4000 || ((*history) > (4000 + numSteps)) ) 
				(*history) = 4000 + numSteps;
			(*index) = (randomu32() % numSteps) ;
			(*history)--;
			if ((*history) <= 4000) {
				(*history) = 4000 + numSteps;
				crossBoundary = true;
			}
		break;
		
		case MODE_FW2 :// forward twice
		case MODE_FW3 :// forward three times
		case MODE_FW4 :// forward four times
			numRuns = 5002 + runMode - MODE_FW2;
			if ( (*history) < 5000 || (*history) >= numRuns ) // 5000 means first pass, 5001 means 2nd pass, etc...
				(*history) = 5000;
			(*index)++;
			if ((*index) >= numSteps) {
				(*index) = 0;
				(*history)++;
				if ((*history) >= numRuns) {
					(*history) = 5000;
					crossBoundary = true;
				}				
			}
		break;

		default :// MODE_FWD  forward; history base is 0 (not needed)
			(*history) = 0;
			(*index)++;
			if ((*index) >= numSteps) {
				(*index) = 0;
				crossBoundary = true;
			}
	}

	return crossBoundary;
}
*/

int keyIndexToGateMode(int keyIndex, int pulsesPerStep) {
	int ret = keyIndex;
	
	if (keyIndex == 1 || keyIndex == 3 || keyIndex == 6 || keyIndex == 8 || keyIndex == 10) {// black keys
		if ((pulsesPerStep % 6) != 0)
			ret = -1;
	}
	else if (keyIndex == 4 || keyIndex == 7 || keyIndex == 9) {// 75%, DUO, DU2 
		if ((pulsesPerStep % 4) != 0)
			ret = -1;
	}
	else if (keyIndex == 2) {// 50%
		if ((pulsesPerStep % 2) != 0)
			ret = -1;
	}
	else if (keyIndex == 0) {// 25%
		if (pulsesPerStep != 1 && (pulsesPerStep % 4) != 0)
			ret = -1;
	}
	//else always good: 5 (full) and 11 (trig)
	
	return ret;
}

/*CHANGE LOG

0.6.12:
revert PPG and add the new one as a run mode called PND (Pendulum); fix PS, SMS, GS toJson/fromJson to adjust old patches
fix PPG run mode, so that it is a true PPG (ex: 1,2,3,2,1,2... instead of 1,2,3,3,2,1,1,2...)

*/