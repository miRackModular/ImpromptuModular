//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from Valley Rack Free by Dale Johnson
//See ./LICENSE.txt for all licenses
//***********************************************************************************************

#ifndef IM_WIDGETS_HPP
#define IM_WIDGETS_HPP


#include "rack.hpp"

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
	void draw(const DrawArgs &args) override;
};
struct DynamicSVGScrew : widget::FramebufferWidget {
    int* mode = NULL;
    int oldMode = -1;
	// for random rotated screw used in primary mode
	widget::SvgWidget *sw;
	// for fixed svg screw used in alternate mode
    widget::SvgWidget* swAlt;
	
    DynamicSVGScrew();
    void addSVGalt(std::shared_ptr<Svg> svg);
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
	TDynamicPort *dynPort = createDynamicPort<TDynamicPort>(pos, isInput, module, portId, mode);
	dynPort->box.pos = dynPort->box.pos.minus(dynPort->box.size.div(2));// centering
	return dynPort;
}

// Dynamic SVGPort (see SvgPort in app/SvgPort.hpp)
struct DynamicSVGPort : SvgPort {
    int* mode = NULL;
    int oldMode = -1;
    std::vector<std::shared_ptr<Svg>> frames;

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
	TDynamicParam *dynParam = createDynamicParam<TDynamicParam>(pos, module, paramId, mode);
	dynParam->box.pos = dynParam->box.pos.minus(dynParam->box.size.div(2));// centering
	return dynParam;
}

// Dynamic SVGSwitch (see app/SvgSwitch.hpp)
struct DynamicSVGSwitch : app::SvgSwitch {
    int* mode = NULL;
    int oldMode = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;
	
	void addFrameAll(std::shared_ptr<Svg> svg);
    void step() override;
};

// Dynamic SVGKnob (see app/SvgKnob.hpp)
struct DynamicSVGKnob : app::SvgKnob {
    int* mode = NULL;
    int oldMode = -1;
	std::vector<std::shared_ptr<Svg>> framesAll;
	widget::SvgWidget* effect;
	
	void addFrameAll(std::shared_ptr<Svg> svg);
	void addEffect(std::shared_ptr<Svg> svg);// do this last
    void step() override;
};



// Tactile pad

struct IMTactile : app::ParamWidget { // Note: double-click initialize doesn't work because onDragMove() gets calls after onDoubleClick()
	float dragY;
	float dragValue;
	static const int padWidth = 45;
	static const int padHeight = 200;
	
	IMTactile();
	void onDragStart(const widget::DragStartEvent &e) override;
	void onDragMove(const widget::DragMoveEvent &e) override;
	void onButton(const widget::ButtonEvent &e) override;
	void reset() override;
	void randomize() override;
};

#endif