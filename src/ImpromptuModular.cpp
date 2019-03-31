//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


Plugin *pluginInstance;


void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelTact);
	p->addModel(modelTact1);
	p->addModel(modelTwelveKey);
	p->addModel(modelClocked);
	p->addModel(modelClockedExpander);
	p->addModel(modelFoundry);
	p->addModel(modelFoundryExpander);
	p->addModel(modelGateSeq64);
	p->addModel(modelGateSeq64Expander);
	p->addModel(modelPhraseSeq16);
	p->addModel(modelPhraseSeq32);
	p->addModel(modelPhraseSeqExpander);
	p->addModel(modelWriteSeq32);
	p->addModel(modelWriteSeq64);
	p->addModel(modelBigButtonSeq);
	p->addModel(modelBigButtonSeq2);
	p->addModel(modelFourView);
	p->addModel(modelSemiModularSynth);
	p->addModel(modelBlankPanel);
}


// Screws

// nothing



// Ports

// nothing



// Buttons and switches


CKSSH::CKSSH() {
	fb->removeChild(sw);
	
	TransformWidget *tw = new TransformWidget();
	tw->addChild(sw);
	fb->addChild(tw);

	Vec center = sw->box.getCenter();
	tw->translate(center);
	tw->rotate(M_PI/2.0f);
	tw->translate(Vec(center.y, sw->box.size.x).neg());
	
	tw->box.size = sw->box.size.flip();
	box.size = tw->box.size;
}


LEDBezelBig::LEDBezelBig() {
	momentary = true;
	float ratio = 2.13f;
	addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/LEDBezel.svg")));
	sw->box.size = sw->box.size.mult(ratio);
	fb->removeChild(sw);
	tw = new TransformWidget();
	tw->addChild(sw);
	tw->scale(Vec(ratio, ratio));
	tw->box.size = sw->box.size; 
	fb->addChild(tw);
	box.size = sw->box.size; 
}



// Knobs

// nothing



// Lights

// nothing



// Other widgets

// Invisible key

void InvisibleKeySmall::onButton(const ButtonEvent &e) {
	if (e.action == GLFW_PRESS && paramQuantity) {
		paramQuantity->maxValue = 1.0f;
	}
	Switch::onButton(e);
}
void InvisibleKeySmall::onDoubleClick(const DoubleClickEvent &e) {
	if (paramQuantity) {
		paramQuantity->maxValue = 2.0f;
	}
	Switch::onDoubleClick(e);
}


// Tactile pad

IMTactile::IMTactile() {
	box.size = Vec(padWidth, padHeight);
}

void IMTactile::onDragStart(const DragStartEvent &e) {
	if (paramQuantity) {
		dragValue = paramQuantity->getValue();
		dragY = APP->scene->rack->mousePos.y;
	}
	e.consume(this);// Must consume to set the widget as dragged
}

void IMTactile::onDragMove(const DragMoveEvent &e) {
	if (paramQuantity) {
		float rangeValue = paramQuantity->getMaxValue() - paramQuantity->getMinValue();// infinite not supported (not relevant)
		float newDragY = APP->scene->rack->mousePos.y;
		float delta = -(newDragY - dragY) * rangeValue / box.size.y;
		dragY = newDragY;
		dragValue += delta;
		float dragValueClamped = clampSafe(dragValue, paramQuantity->getMinValue(), paramQuantity->getMaxValue());
		paramQuantity->setValue(dragValueClamped);
	}
	e.consume(this);
}

void IMTactile::onButton(const ButtonEvent &e) {
	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && paramQuantity) {
		float val = rescale(e.pos.y, box.size.y, 0.0f, paramQuantity->getMinValue(), paramQuantity->getMaxValue());
		paramQuantity->setValue(val);
	}
	ParamWidget::onButton(e);
}

void IMTactile::reset() {
	if (paramQuantity) {
		paramQuantity->reset();
	}
}

void IMTactile::randomize() {
	if (paramQuantity) {
		float value = math::rescale(random::uniform(), 0.f, 1.f, paramQuantity->getMinValue(), paramQuantity->getMaxValue());
		paramQuantity->setValue(value);
	}
}



// Other objects

bool Trigger::process(float in) {
	switch (state) {
		case LOW:
			if (in >= 1.0f) {
				state = HIGH;
				return true;
			}
			break;
		case HIGH:
			if (in <= 0.1f) {
				state = LOW;
			}
			break;
		default:
			if (in >= 1.0f) {
				state = HIGH;
			}
			else if (in <= 0.1f) {
				state = LOW;
			}
			break;
	}
	return false;
}	

bool HoldDetect::process(float paramValue) {
	bool ret = false;
	if (modeHoldDetect > 0l) {
		if (paramValue < 0.5f)
			modeHoldDetect = 0l;
		else {
			if (modeHoldDetect == 1l) {
				ret = true;
			}
			modeHoldDetect--;
		}
	}
	return ret;
}



// Other functions

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

