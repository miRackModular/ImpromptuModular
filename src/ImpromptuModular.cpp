//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

	p->addModel(modelTact);
	p->addModel(modelTact1);
	p->addModel(modelTwelveKey);
	p->addModel(modelClocked);
	p->addModel(modelFoundry);
	p->addModel(modelGateSeq64);
	p->addModel(modelPhraseSeq16);
	p->addModel(modelPhraseSeq32);
	p->addModel(modelWriteSeq32);
	p->addModel(modelWriteSeq64);
	p->addModel(modelBigButtonSeq);
	p->addModel(modelBigButtonSeq2);
	p->addModel(modelFourView);
	p->addModel(modelSemiModularSynth);
	// p->addModel(modelBlankPanel);
}


void IMBigPushButtonWithRClick::onMouseDown(EventMouseDown &e)  {
	if (windowIsShiftPressed()) {// if right button (see events.hpp)
		maxValue = 2.0f;
		// Simulate MomentarySwitch::onDragStart() since not called for right clicks:
		setValue(maxValue);
		EventAction eAction;
		onAction(eAction);
	}
	else 
		maxValue = 1.0f;
	//ParamWidget::onMouseDown(e);// don't want the reset() that is called in ParamWidget::onMouseDown(), so implement rest of that function here:
	e.consumed = true;
	e.target = this;
}
void IMBigPushButtonWithRClick::onMouseUp(EventMouseUp &e) {
	ParamWidget::onMouseUp(e);
}		


LEDBezelBig::LEDBezelBig() {
	float ratio = 2.13f;
	addFrame(SVG::load(assetGlobal("res/ComponentLibrary/LEDBezel.svg")));
	sw->box.size = sw->box.size.mult(ratio);
	box.size = sw->box.size;
	tw = new TransformWidget();
	removeChild(sw);
	tw->addChild(sw);
	
	addChild(tw);
	
	tw->box.size = sw->box.size; 
	tw->scale(Vec(ratio, ratio));
}


void InvisibleKeySmall::onMouseDown(EventMouseDown &e) {
	if (windowIsShiftPressed()) {// if right button (see events.hpp)
		maxValue = 2.0f;
		// Simulate MomentarySwitch::onDragStart() since not called for right clicks:
		setValue(maxValue);
		EventAction eAction;
		onAction(eAction);
	}
	else 
		maxValue = 1.0f;
	//ParamWidget::onMouseDown(e);// don't want the reset() that is called in ParamWidget::onMouseDown(), so implement rest of that function here:
	e.consumed = true;
	e.target = this;
}
void InvisibleKeySmall::onMouseUp(EventMouseUp &e) {
	ParamWidget::onMouseUp(e);
}



void LEDButtonWithRClick::onMouseDown(EventMouseDown &e)  {
	if (windowIsShiftPressed()) {// if right button (see events.hpp)
		maxValue = 2.0f;
		// Simulate MomentarySwitch::onDragStart() since not called for right clicks:
		setValue(maxValue);
		EventAction eAction;
		onAction(eAction);
	}
	else 
		maxValue = 1.0f;
	//ParamWidget::onMouseDown(e);// don't want the reset() that is called in ParamWidget::onMouseDown(), so implement rest of that function here:
	e.consumed = true;
	e.target = this;
}
void LEDButtonWithRClick::onMouseUp(EventMouseUp &e) {
	ParamWidget::onMouseUp(e);
}		





NVGcolor prepareDisplay(NVGcontext *vg, Rect *box, int fontSize) {
	NVGcolor backgroundColor = nvgRGB(0x38, 0x38, 0x38); 
	NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
	nvgBeginPath(vg);
	nvgRoundedRect(vg, 0.0, 0.0, box->size.x, box->size.y, 5.0);
	nvgFillColor(vg, backgroundColor);
	nvgFill(vg);
	nvgStrokeWidth(vg, 1.0);
	nvgStrokeColor(vg, borderColor);
	nvgStroke(vg);
	nvgFontSize(vg, fontSize);
	NVGcolor textColor = nvgRGB(0xaf, 0xd2, 0x2c);
	return textColor;
}

void printNote(float cvVal, char* text, bool sharp) {// text must be at least 4 chars long (three displayed chars plus end of string)
	static const char noteLettersSharp[12] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	static const char noteLettersFlat [12] = {'C', 'D', 'D', 'E', 'E', 'F', 'G', 'G', 'A', 'A', 'B', 'B'};
	static const char isBlackKey      [12] = { 0,   1,   0,   1,   0,   0,   1,   0,   1,   0,   1,   0 };

	float cvValOffset = cvVal + 10.0f;// to properly handle negative note voltages
	int indexNote =  clamp( (int)((cvValOffset-floor(cvValOffset)) * 12.0f + 0.5f),  0,  11);
	
	// note letter
	text[0] = sharp ? noteLettersSharp[indexNote] : noteLettersFlat[indexNote];
	
	// octave number
	int octave = (int) roundf(floorf(cvVal)+4.0f);
	if (octave < 0 || octave > 9)
		text[1] = (octave > 9) ? ':' : '_';
	else
		text[1] = (char) ( 0x30 + octave);
	
	// sharp/flat
	text[2] = ' ';
	if (isBlackKey[indexNote] == 1)
		text[2] = (sharp ? '\"' : 'b' );
	
	text[3] = 0;
}

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

