//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************

#ifndef IMPROMPU_MODULAR_HPP
#define IMPROMPU_MODULAR_HPP


#include "rack.hpp"
#include "IMWidgets.hpp"

using namespace rack;


extern Plugin *pluginInstance;


// All modules that are part of pluginInstance go here
extern Model *modelTact;
extern Model *modelTact1;
extern Model *modelTwelveKey;
extern Model *modelClocked;
extern Model *modelClockedExpander;
extern Model *modelFoundry;
extern Model *modelFoundryExpander;
extern Model *modelGateSeq64;
extern Model *modelGateSeq64Expander;
extern Model *modelPhraseSeq16;
extern Model *modelPhraseSeq32;
extern Model *modelPhraseSeqExpander;
extern Model *modelWriteSeq32;
extern Model *modelWriteSeq64;
extern Model *modelBigButtonSeq;
extern Model *modelBigButtonSeq2;
extern Model *modelFourView;
extern Model *modelSemiModularSynth;
extern Model *modelBlankPanel;


// General constants
static const bool clockIgnoreOnRun = false;
static const bool retrigGatesOnReset = true;
static constexpr float clockIgnoreOnResetDuration = 0.001f;// disable clock on powerup and reset for 1 ms (so that the first step plays)
static const int displayAlpha = 23;
static const std::string lightPanelID = "Classic";
static const std::string darkPanelID = "Dark-valor";
static const unsigned int displayRefreshStepSkips = 256;
static const unsigned int userInputsStepSkipMask = 0xF;// sub interval of displayRefreshStepSkips, since inputs should be more responsive than lights
// above value should make it such that inputs are sampled > 1kHz so as to not miss 1ms triggers
static const unsigned int expanderRefreshStepSkips = 64;


// Component offset constants

static const int hOffsetCKSS = 5;
static const int vOffsetCKSS = 2;
static const int vOffsetCKSSThree = -2;
static const int hOffsetCKSSH = 2;
static const int vOffsetCKSSH = 5;
static const int offsetCKD6 = -1;//does both h and v
static const int offsetCKD6b = 0;//does both h and v
static const int vOffsetDisplay = -2;
static const int offsetIMBigKnob = -6;//does both h and v
static const int offsetIMSmallKnob = 0;//does both h and v
static const int offsetMediumLight = 9;
static const float offsetLEDbutton = 3.0f;//does both h and v
static const float offsetLEDbuttonLight = 4.4f;//does both h and v
static const int offsetTL1105 = 4;//does both h and v
static const int offsetLEDbezel = 1;//does both h and v
static const float offsetLEDbezelLight = 2.2f;//does both h and v
static const float offsetLEDbezelBig = -11;//does both h and v
static const int offsetTrimpot = 3;//does both h and v

static const float blurRadiusRatio = 0.06f;



// Variations on existing knobs, lights, etc



// Screws

struct IMScrew : DynamicSVGScrew {
	IMScrew() {
		addSVGalt(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/comp/ScrewSilver.svg")));
	}
};



// Ports

struct IMPort : DynamicSVGPort {
	IMPort() {
		//addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/PJ301M.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/PJ301M.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/PJ301M.svg"));
		shadow->blurRadius = 1.0f;
		// shadow->opacity = 0.8;
	}
};



// Buttons and switches

struct CKSSNoRandom : CKSS {
	void randomize() override {}
};

struct CKSSH : CKSS {
	CKSSH();
};
struct CKSSHNoRandom : CKSSH {
	void randomize() override {}
};

struct CKSSThreeInv : app::SvgSwitch {
	CKSSThreeInv() {
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/CKSSThree_2.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/CKSSThree_1.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/CKSSThree_0.svg")));
	}
};

struct CKSSThreeInvNoRandom : CKSSThreeInv {
	void randomize() override {}
};

struct IMBigPushButton : DynamicSVGSwitch {
	IMBigPushButton() {
		momentary = true;
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/CKD6b_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/CKD6b_1.svg")));
		addFrameAlt0(asset::plugin(pluginInstance, "res/dark/comp/CKD6b_0.svg"));
		addFrameAlt1(asset::plugin(pluginInstance, "res/dark/comp/CKD6b_1.svg"));	
		shadow->blurRadius = 1.0f;
	}
};

struct IMPushButton : DynamicSVGSwitch {
	IMPushButton() {
		momentary = true;
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/TL1105_0.svg")));
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/TL1105_1.svg")));
		addFrameAlt0(asset::plugin(pluginInstance, "res/dark/comp/TL1105_0.svg"));
		addFrameAlt1(asset::plugin(pluginInstance, "res/dark/comp/TL1105_1.svg"));	
	}
};


struct LEDBezelBig : SvgSwitch {
	TransformWidget *tw;
	LEDBezelBig();
};



// Knobs

struct IMKnob : DynamicSVGKnob {
	IMKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};

struct IMBigKnob : IMKnob {
	IMBigKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/BlackKnobLargeWithMark.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/BlackKnobLargeWithMark.svg"));
		addFrameEffect(asset::plugin(pluginInstance, "res/dark/comp/BlackKnobLargeWithMarkEffects.svg"));
		shadow->blurRadius = box.size.y * blurRadiusRatio;
		// shadow->opacity = 0.1;
	}
	void randomize() override {}
};
struct IMBigSnapKnob : IMBigKnob {
	IMBigSnapKnob() {
		snap = true;
	}
};

struct IMBigKnobInf : IMKnob {
	IMBigKnobInf() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/BlackKnobLarge.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/BlackKnobLarge.svg"));
		addFrameEffect(asset::plugin(pluginInstance, "res/dark/comp/BlackKnobLargeEffects.svg"));
		shadow->blurRadius = box.size.y * blurRadiusRatio;
		// shadow->opacity = 0.1;
		speed = 0.9f;				
	}
};

struct IMSmallKnob : IMKnob {
	IMSmallKnob() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/RoundSmallBlackKnob.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnob.svg"));
		addFrameEffect(asset::plugin(pluginInstance, "res/dark/comp/RoundSmallBlackKnobEffects.svg"));		
		shadow->blurRadius = box.size.y * blurRadiusRatio;
		// shadow->opacity = 0.1;
		// shadow->box.pos = Vec(0.0, box.size.y * 0.15);
	}
};
struct IMSmallKnobNoRandom : IMSmallKnob {
	void randomize() override {}
};
struct IMSmallSnapKnob : IMSmallKnob {
	IMSmallSnapKnob() {
		snap = true;
	}
};

struct IMMediumKnobInf : IMKnob {
	IMMediumKnobInf() {
		addFrameAll(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/comp/RoundMediumBlackKnobNoMark.svg")));
		addFrameAlt(asset::plugin(pluginInstance, "res/dark/comp/RoundMediumBlackKnobNoMark.svg"));
		addFrameEffect(asset::plugin(pluginInstance, "res/dark/comp/RoundMediumBlackKnobNoMarkEffects.svg"));
		shadow->blurRadius = box.size.y * blurRadiusRatio;
		// shadow->opacity = 0.1;
		// shadow->box.pos = Vec(0.0, box.size.y * 0.15);
		speed = 0.9f;				
	}
};

struct IMFivePosSmallKnob : IMSmallSnapKnob {
	IMFivePosSmallKnob() {
		speed = 1.6f;
		minAngle = -0.5*M_PI;
		maxAngle = 0.5*M_PI;
	}
	void randomize() override {}
};

struct IMSixPosBigKnob : IMBigSnapKnob {
	IMSixPosBigKnob() {
		speed = 1.3f;
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
	void randomize() override {}
};



// Lights

struct OrangeLight : GrayModuleLightWidget {
	OrangeLight() {
		addBaseColor(SCHEME_ORANGE);
	}
};
struct GreenRedWhiteLight : GrayModuleLightWidget {
	GreenRedWhiteLight() {
		addBaseColor(SCHEME_GREEN);
		addBaseColor(SCHEME_RED);
		addBaseColor(SCHEME_WHITE);
	}
};

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0f, 6.0f));
	}
};


template <typename BASE>
struct GiantLight : BASE {
	GiantLight() {
		this->box.size = mm2px(Vec(19.0f, 19.0f));
	}
};
template <typename BASE>
struct GiantLight2 : BASE {
	GiantLight2() {
		this->box.size = mm2px(Vec(12.8f, 12.8f));
	}
};



// Other widgets

struct InvisibleKey : Switch {
	InvisibleKey() {
		momentary = true;
		box.size = Vec(34, 72);
	}
};

struct InvisibleKeySmall : Switch {
	InvisibleKeySmall() {
		momentary = true;
		box.size = Vec(23, 38);
	}
	void onButton(const event::Button &e) override;
	void onDoubleClick(const event::DoubleClick &e) override;
};

struct IMTactile : ParamWidget {
	// Note: double-click initialize doesn't work in this setup because onDragMove() gets calls after onDoubleClick()
	float dragY;
	float dragValue;
	static const int padWidth = 45;
	static const int padHeight = 200;
	
	IMTactile();
	void onDragStart(const event::DragStart &e) override;
	void onDragMove(const event::DragMove &e) override;
	void onButton(const event::Button &e) override;
	void reset() override;
	void randomize() override;
};


// Other objects

struct Trigger : dsp::SchmittTrigger {
	// implements a 0.1V - 1.0V SchmittTrigger (see include/dsp/digital.hpp) instead of 
	//   calling SchmittTriggerInstance.process(math::rescale(in, 0.1f, 1.f, 0.f, 1.f))
	bool process(float in);
};	

struct HoldDetect {
	long modeHoldDetect;// 0 when not detecting, downward counter when detecting
	
	void reset() {
		modeHoldDetect = 0l;
	}
	
	void start(long startValue) {
		modeHoldDetect = startValue;
	}

	bool process(float paramValue);
};



// Other functions

inline bool calcWarningFlash(long count, long countInit) {
	if ( (count > (countInit * 2l / 4l) && count < (countInit * 3l / 4l)) || (count < (countInit * 1l / 4l)) )
		return false;
	return true;
}	

NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize);

void printNote(float cvVal, char* text, bool sharp);

int moveIndex(int index, int indexNext, int numSteps);


#endif
