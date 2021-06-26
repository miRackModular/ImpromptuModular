//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé 
//
//Based on code from Valley Rack Free by Dale Johnson
//See ./LICENSE.txt for all licenses
//***********************************************************************************************


#include "IMWidgets.hpp"



// Dynamic SVGScrew



// Dynamic SVGPanel



// Dynamic SVGPort



// Dynamic SVGSwitch


// Dynamic SVGKnob



// Dynamic IMTactile

DynamicIMTactile::DynamicIMTactile() {
	snap = false;
	smooth = false;// must be false or else DynamicIMTactile::changeValue() call from module will crash Rack
	widerParam = nullptr;
	paramReadRequest = nullptr;
	oldWider = -1.0f;
	box.size = Vec(padWidth, padHeight);
}

void DynamicIMTactile::step() {
   if (widerParam) {
	   float wider = widerParam->value;
	   if(wider != oldWider) {
	        if (wider > 0.5f) {
				box.size = Vec(padWidthWide, padHeight);
			}
			else {
				box.size = Vec(padWidth, padHeight);
			}
	        oldWider = wider;
	    }
    }	
	if (paramReadRequest != nullptr) {
		float readVal = *paramReadRequest;
		if (readVal != -10.0f) {
			setValue(readVal);
			*paramReadRequest = -10.0f;
		}
	}
	FramebufferWidget::step();
}

void DynamicIMTactile::onDragStart(EventDragStart &e) {
	dragValue = value;
	dragY = gRackWidget->lastMousePos.y;
}

void DynamicIMTactile::onDragMove(EventDragMove &e) {
	float rangeValue = maxValue - minValue;// infinite not supported (not relevant)
	float newDragY = gRackWidget->lastMousePos.y;
	float delta = -(newDragY - dragY) * rangeValue / box.size.y;
	dragY = newDragY;
	dragValue += delta;
	float dragValueClamped = clamp2(dragValue, minValue, maxValue);
	if (snap)
		dragValueClamped = roundf(dragValueClamped);
	setValue(dragValueClamped);
}

void DynamicIMTactile::onMouseDown(EventMouseDown &e) {
	float val = rescale(e.pos.y, box.size.y, 0.0f , minValue, maxValue);
	if (snap)
		val = roundf(val);
	setValue(val);
	ParamWidget::onMouseDown(e);
}

