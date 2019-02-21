//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//***********************************************************************************************

#include "dsp/digital.hpp"

using namespace rack;

class StepAttributesGS {
	unsigned short attributes;
	
	public:
	
	static const unsigned short ATT_MSK_PROB = 0xFF;
	static const unsigned short ATT_MSK_GATEP = 0x100;
	static const unsigned short ATT_MSK_GATE = 0x200;
	static const unsigned short ATT_MSK_GATEMODE = 0x1C00, gateModeShift = 10;// 3 bits

	static const unsigned short ATT_MSK_INITSTATE =  50;
	
	inline void init() {attributes = ATT_MSK_INITSTATE;}
	inline void randomize() {attributes = ( (randomu32() % 101) | (randomu32() & (ATT_MSK_GATEP | ATT_MSK_GATE | ATT_MSK_GATEMODE)) );}
		
	inline bool getGate() {return (attributes & ATT_MSK_GATE) != 0;}
	inline bool getGateP() {return (attributes & ATT_MSK_GATEP) != 0;}
	inline int getGatePVal() {return attributes & ATT_MSK_PROB;}
	inline int getGateMode() {return (attributes & ATT_MSK_GATEMODE) >> gateModeShift;}
	inline unsigned short getAttribute() {return attributes;}

	inline void setGate(bool gateState) {attributes &= ~ATT_MSK_GATE; if (gateState) attributes |= ATT_MSK_GATE;}
	inline void setGateP(bool gatePState) {attributes &= ~ATT_MSK_GATEP; if (gatePState) attributes |= ATT_MSK_GATEP;}
	inline void setGatePVal(int pVal) {attributes &= ~ATT_MSK_PROB; attributes |= (pVal & ATT_MSK_PROB);}
	inline void setGateMode(int gateMode) {attributes &= ~ATT_MSK_GATEMODE; attributes |= (gateMode << gateModeShift);}
	inline void setAttribute(unsigned short _attributes) {attributes = _attributes;}

	inline void toggleGate() {attributes ^= ATT_MSK_GATE;}
};// class StepAttributesGS 


