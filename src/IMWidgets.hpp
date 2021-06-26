//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from Valley Rack Free by Dale Johnson
//See ./LICENSE.txt for all licenses
//***********************************************************************************************

#ifndef IM_WIDGETS_HPP
#define IM_WIDGETS_HPP


#include "rack.hpp"
#include "window.hpp"

using namespace rack;



// Dynamic SVGScrew

template <class TWidget>
TWidget* createDynamicScrew(Vec pos, int* mode) {
	return Widget::create<ScrewSilver>(pos);
}


// Dynamic SVGPanel

struct PanelBorderWidget : TransparentWidget { // from SVGPanel.cpp
	int** expWidth = nullptr;
	void draw(NVGcontext *vg) override;
};




// ******** Dynamic Ports ********

// General Dynamic Port creation
template <class TDynamicPort>
Port* createDynamicPort(Vec pos, Port::PortType type, Module *module, int portId, int* mode) {
	return Port::create<PJ301MPort>(pos, type, module, portId);
}

template <class TDynamicPort>
Port* createDynamicPortCentered(Vec pos, Port::PortType type, Module *module, int portId, int* mode) {
	Port *port = Port::create<PJ301MPort>(pos, type, module, portId);
	port->box.pos = port->box.pos.minus(port->box.size.div(2)); // centering
	return port;
}




// ******** Dynamic Params ********

// General Dynamic Param creation
template <class TDynamicParam>
TDynamicParam* createDynamicParam(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue,
                                               int* mode) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId, minValue, maxValue, defaultValue);
	return dynParam;
}
template <class TDynamicParam>
TDynamicParam* createDynamicParamCentered(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue,
                                               int* mode) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId, minValue, maxValue, defaultValue);
	dynParam->box.pos = dynParam->box.pos.minus(dynParam->box.size.div(2));// centering
	return dynParam;
}

// Dynamic SVGSwitch (see SVGSwitch in app.hpp and SVGSwitch.cpp)
typedef SVGSwitch DynamicSVGSwitch;

// Dynamic SVGKnob (see SVGKnob in app.hpp and SVGKnob.cpp)
typedef SVGKnob DynamicSVGKnob;


// General Dynamic Param creation version two with float* instead of one int*
template <class TDynamicParam>
TDynamicParam* createDynamicParam2(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue,
                                               Param* widerParam, float* paramReadRequest) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId, minValue, maxValue, defaultValue);
	dynParam->widerParam = widerParam;
	dynParam->paramReadRequest = paramReadRequest;
	return dynParam;
}

// Dynamic Tactile pad (see Knob in app.hpp and Knob.cpp, and see SVGSlider in SVGSlider.cpp and app.hpp)
struct DynamicIMTactile : ParamWidget, FramebufferWidget {
	Param* widerParam;// > 0.5f = true
	float* paramReadRequest;
	float oldWider;
	float dragY;
	float dragValue;
	bool snap;
	static const int padWidth = 45;
	static const int padHeight = 200;
	static const int padInterSpace = 18;
	static const int padWidthWide = padWidth * 2 + padInterSpace;
	
	DynamicIMTactile();
	void step() override;
	void onDragStart(EventDragStart &e) override;
	void onDragMove(EventDragMove &e) override;	
	void onMouseDown(EventMouseDown &e) override;
};


#endif