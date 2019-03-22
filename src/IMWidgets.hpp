//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from Valley Rack Free by Dale Johnson
//See ./LICENSE.txt for all licenses
//***********************************************************************************************

#ifndef IM_WIDGETS_HPP
#define IM_WIDGETS_HPP


#include "rack.hpp"
//#include "window.hpp"

using namespace rack;



// Dynamic SVGScrew

// General Dynamic Screw creation
template <class TWidget>
TWidget* createDynamicScrew(Vec pos, int* mode) {
	TWidget *dynScrew = createWidget<TWidget>(pos);
	dynScrew->mode = mode;
	return dynScrew;
}

struct ScrewCircle : widget::TransparentWidget {
	float angle = 0.0f;
	float radius = 2.0f;
	ScrewCircle(float _angle);
	void draw(const DrawArgs &args) override;
};
struct DynamicSVGScrew : widget::FramebufferWidget {
    int* mode;
    int oldMode;
	// for random rotated screw used in primary mode
	widget::SvgWidget *sw;
	TransformWidget *tw;
	ScrewCircle *sc;
	// for fixed svg screw used in alternate mode
    widget::SvgWidget* swAlt;
	
    DynamicSVGScrew();
    void addSVGalt(std::shared_ptr<Svg> svg);
    void step() override;
};



// Dynamic SVGPanel

struct PanelBorderWidget : widget::TransparentWidget { // from app/SvgPanel.hpp
	void draw(const DrawArgs &args) override;
};

struct DynamicSVGPanel : widget::FramebufferWidget { // like app/SvgPanel.hpp but with dynmically assignable resizable panel
    int* mode;
    int oldMode;
    std::vector<std::shared_ptr<Svg>> panels;
    widget::SvgWidget* visiblePanel;
    PanelBorderWidget* border;
    DynamicSVGPanel();
    void addPanel(std::shared_ptr<Svg> svg);
    void dupPanel();
    void step() override;
};



// ******** Dynamic Ports ********

// General Dynamic Port creation
template <class TDynamicPort>
TDynamicPort* createDynamicPort(Vec pos, bool isInput, Module *module, int portId,
                                               int* mode) {
	TDynamicPort *dynPort = isInput ? 
		createInput<TDynamicPort>(pos, module, portId) :
		createOutput<TDynamicPort>(pos, module, portId);
	dynPort->mode = mode;
	return dynPort;
}
template <class TDynamicPort>
TDynamicPort* createDynamicPortCentered(Vec pos, bool isInput, Module *module, int portId,
                                               int* mode) {
	TDynamicPort *dynPort = isInput ? 
		createInput<TDynamicPort>(pos, module, portId) :
		createOutput<TDynamicPort>(pos, module, portId);
	dynPort->mode = mode;
	dynPort->box.pos = dynPort->box.pos.minus(dynPort->box.size.div(2));// centering
	return dynPort;
}

// Dynamic SVGPort (see SvgPort in app/SvgPort.hpp)
struct DynamicSVGPort : SvgPort {
    int* mode;
    int oldMode;
    std::vector<std::shared_ptr<Svg>> frames;

    DynamicSVGPort();
    void addFrame(std::shared_ptr<Svg> svg);
    void step() override;
};



// ******** Dynamic Params ********

// General Dynamic Param creation
template <class TDynamicParam>
TDynamicParam* createDynamicParam(Vec pos, Module *module, int paramId,
                                               int* mode) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId);
	dynParam->mode = mode;
	return dynParam;
}
template <class TDynamicParam>
TDynamicParam* createDynamicParamCentered(Vec pos, Module *module, int paramId,
                                               int* mode) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId);
	dynParam->mode = mode;
	dynParam->box.pos = dynParam->box.pos.minus(dynParam->box.size.div(2));// centering
	return dynParam;
}

// Dynamic SVGSwitch (see app/SvgSwitch.hpp)
struct DynamicSVGSwitch : app::SvgSwitch {
    int* mode;
    int oldMode;
	std::vector<std::shared_ptr<Svg>> framesAll;
	
    DynamicSVGSwitch();
	void addFrameAll(std::shared_ptr<Svg> svg);
    void step() override;
};

// Dynamic SVGKnob (see app/SvgKnob.hpp)
struct DynamicSVGKnob : SVGKnob {
    int* mode;
    int oldMode;
	std::vector<std::shared_ptr<Svg>> framesAll;
	widget::SvgWidget* effect;
	
    DynamicSVGKnob();
	void addFrameAll(std::shared_ptr<Svg> svg);
	void addEffect(std::shared_ptr<Svg> svg);// do this last
    void step() override;
};



// General Dynamic Param creation version two with float* instead of one int*
template <class TDynamicParam>
TDynamicParam* createDynamicParam2(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue,
                                               float* wider, float* paramReadRequest) {
	TDynamicParam *dynParam = createParam<TDynamicParam>(pos, module, paramId, minValue, maxValue, defaultValue);
	dynParam->wider = wider;
	dynParam->paramReadRequest = paramReadRequest;
	return dynParam;
}

/*
// Dynamic Tactile pad (see Knob in app.hpp and Knob.cpp, and see SVGSlider in SVGSlider.cpp and app.hpp)
struct DynamicIMTactile : ParamWidget, widget::FramebufferWidget {
	float* wider;// > 0.5f = true
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
	void process(const ProcessArgs &args) override;
	//void onDragStart(const widget::DragStartEvent &e) override; // TODO
	//void onDragMove(const widget::DragMoveEvent &e) override; // TODO	
	
	//void onMouseDown(EventMouseDown &e) override; // replaced by onButton()
	void onButton(const widget::ButtonEvent &e) override;// replaces onMouseDown() and onMouseUp()
};
*/

#endif