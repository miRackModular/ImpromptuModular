//***********************************************************************************************
//Semi-Modular Synthesizer module for VCV Rack by Marc Boulé and Xavier Belmont
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept, desing and layout by Xavier Belmont
//Code by Marc Boulé
//
//Acknowledgements: please see README.md
//***********************************************************************************************


#include "FundamentalUtil.hpp"
#include "PhraseSeqUtil.hpp"


struct SemiModularSynth : Module {
	enum ParamIds {
		// SEQUENCER
		KEYNOTE_PARAM,// 0.6.12 replaces unused
		KEYGATE_PARAM,// 0.6.12 replaces unused
		KEYGATE2_PARAM,// no longer used
		EDIT_PARAM,
		SEQUENCE_PARAM,
		RUN_PARAM,
		COPY_PARAM,
		PASTE_PARAM,
		RESET_PARAM,
		ENUMS(OCTAVE_PARAM, 7),
		GATE1_PARAM,
		GATE2_PARAM,
		SLIDE_BTN_PARAM,
		SLIDE_KNOB_PARAM,
		ATTACH_PARAM,
		AUTOSTEP_PARAM,
		ENUMS(KEY_PARAMS, 12),
		RUNMODE_PARAM,
		TRAN_ROT_PARAM,
		GATE1_KNOB_PARAM,
		GATE1_PROB_PARAM,
		TIE_PARAM,// Legato
		CPMODE_PARAM,
		
		// VCO
		VCO_MODE_PARAM,
		VCO_OCT_PARAM,
		VCO_FREQ_PARAM,
		VCO_FINE_PARAM,
		VCO_FM_PARAM,
		VCO_PW_PARAM,
		VCO_PWM_PARAM,

		// CLK
		CLK_FREQ_PARAM,
		CLK_PW_PARAM,

		// VCA
		VCA_LEVEL1_PARAM,
		
		// ADSR
		ADSR_ATTACK_PARAM,
		ADSR_DECAY_PARAM,
		ADSR_SUSTAIN_PARAM,
		ADSR_RELEASE_PARAM,
		
		// VCF
		VCF_FREQ_PARAM,
		VCF_RES_PARAM,
		VCF_FREQ_CV_PARAM,
		VCF_DRIVE_PARAM,
		
		// LFO
		LFO_FREQ_PARAM,
		LFO_GAIN_PARAM,
		LFO_OFFSET_PARAM,
		
		// ALL NEW
		ENUMS(STEP_PHRASE_PARAMS, 16),
				
		NUM_PARAMS
	};
	enum InputIds {
		// SEQUENCER
		WRITE_INPUT,
		CV_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		LEFTCV_INPUT,
		RIGHTCV_INPUT,
		RUNCV_INPUT,
		SEQCV_INPUT,
		
		// VCO
		VCO_PITCH_INPUT,
		VCO_FM_INPUT,
		VCO_SYNC_INPUT,
		VCO_PW_INPUT,

		// CLK
		// none
		
		// VCA
		VCA_LIN1_INPUT,
		VCA_IN1_INPUT,
		
		// ADSR
		ADSR_GATE_INPUT,

		// VCF
		VCF_FREQ_INPUT,
		VCF_RES_INPUT,
		VCF_DRIVE_INPUT,
		VCF_IN_INPUT,
		
		// LFO
		LFO_RESET_INPUT,
		
		NUM_INPUTS
	};
	enum OutputIds {
		// SEQUENCER
		CV_OUTPUT,
		GATE1_OUTPUT,
		GATE2_OUTPUT,
		
		// VCO
		VCO_SIN_OUTPUT,
		VCO_TRI_OUTPUT,
		VCO_SAW_OUTPUT,
		VCO_SQR_OUTPUT,

		// CLK
		CLK_OUT_OUTPUT,
		
		// VCA
		VCA_OUT1_OUTPUT,
		
		// ADSR
		ADSR_ENVELOPE_OUTPUT,
		
		// VCF
		VCF_LPF_OUTPUT,
		VCF_HPF_OUTPUT,
		
		// LFO
		LFO_SIN_OUTPUT,
		LFO_TRI_OUTPUT,
		
		NUM_OUTPUTS
	};
	enum LightIds {
		// SEQUENCER
		ENUMS(STEP_PHRASE_LIGHTS, 16 * 3),// room for GreenRedWhite
		ENUMS(OCTAVE_LIGHTS, 7),// octaves 1 to 7
		ENUMS(KEY_LIGHTS, 12 * 2),// room for GreenRed
		RUN_LIGHT,
		RESET_LIGHT,
		ENUMS(GATE1_LIGHT, 2),// room for GreenRed
		ENUMS(GATE2_LIGHT, 2),// room for GreenRed
		SLIDE_LIGHT,
		ATTACH_LIGHT,
		ENUMS(GATE1_PROB_LIGHT, 2),// room for GreenRed
		TIE_LIGHT,
		KEYNOTE_LIGHT,
		ENUMS(KEYGATE_LIGHT, 2),// room for GreenRed
		
		// VCO, CLK, VCA
		// none
		
		NUM_LIGHTS
	};

	
	// SEQUENCER
	
	// Constants
	enum DisplayStateIds {DISP_NORMAL, DISP_MODE, DISP_LENGTH, DISP_TRANSPOSE, DISP_ROTATE};

	// Need to save
	int panelTheme = 1;
	bool autoseq;
	bool autostepLen;
	bool holdTiedNotes;
	int seqCVmethod;// 0 is 0-10V, 1 is C4-D5#, 2 is TrigIncr
	int pulsesPerStep;// 1 means normal gate mode, alt choices are 4, 6, 12, 24 PPS (Pulses per step)
	bool running;
	SeqAttributes sequences[16];
	int runModeSong; 
	int seqIndexEdit;
	int phrase[16];// This is the song (series of phases; a phrase is a patten number)
	int phrases;//1 to 16
	float cv[16][16];// [-3.0 : 3.917]. First index is patten number, 2nd index is step
	StepAttributes attributes[16][16];// First index is patten number, 2nd index is step (see enum AttributeBitMasks for details)
	bool resetOnRun;
	bool attached;

	// No need to save
	int stepIndexEdit;
	int stepIndexRun;
	int phraseIndexEdit;
	int phraseIndexRun;
	long infoCopyPaste;// 0 when no info, positive downward step counter timer when copy, negative upward when paste
	unsigned long editingGate;// 0 when no edit gate, downward step counter timer when edit gate
	float editingGateCV;// no need to initialize, this is a companion to editingGate (output this only when editingGate > 0)
	int editingGateKeyLight;// no need to initialize, this is a companion to editingGate (use this only when editingGate > 0)
	unsigned long editingType;// similar to editingGate, but just for showing remanent gate type (nothing played); uses editingGateKeyLight
	unsigned long stepIndexRunHistory;
	unsigned long phraseIndexRunHistory;
	int displayState;
	unsigned long slideStepsRemain;// 0 when no slide under way, downward step counter when sliding
	float slideCVdelta;// no need to initialize, this is a companion to slideStepsRemain
	float cvCPbuffer[16];// copy paste buffer for CVs
	StepAttributes attribCPbuffer[16];
	SeqAttributes seqAttribCPbuffer;
	bool seqCopied;
	int phraseCPbuffer[16];
	int countCP;// number of steps to paste (in case CPMODE_PARAM changes between copy and paste)
	int startCP;
	long clockIgnoreOnReset;
	unsigned long clockPeriod;// counts number of step() calls upward from last clock (reset after clock processed)
	long tiedWarning;// 0 when no warning, positive downward step counter timer when warning
	long attachedWarning;// 0 when no warning, positive downward step counter timer when warning
	int gate1Code;
	int gate2Code;
	long revertDisplay;
	long editingGateLength;// 0 when no info, positive when gate1, negative when gate2
	long lastGateEdit;
	long editingPpqn;// 0 when no info, positive downward step counter timer when editing ppqn
	int ppqnCount;
	
	// VCO
	// none
	
	// CLK
	float clkValue;
	
	// VCA
	// none
	
	// ADSR
	bool decaying = false;
	float env = 0.0f;
	
	// VCF
	LadderFilter filter;
	

	unsigned int lightRefreshCounter = 0;
	float resetLight = 0.0f;
	int sequenceKnob = 0;
	Trigger resetTrigger;
	Trigger leftTrigger;
	Trigger rightTrigger;
	Trigger runningTrigger;
	Trigger clockTrigger;
	Trigger octTriggers[7];
	Trigger octmTrigger;
	Trigger gate1Trigger;
	Trigger gate1ProbTrigger;
	Trigger gate2Trigger;
	Trigger slideTrigger;
	Trigger keyTriggers[12];
	Trigger writeTrigger;
	Trigger attachedTrigger;
	Trigger copyTrigger;
	Trigger pasteTrigger;
	Trigger modeTrigger;
	Trigger rotateTrigger;
	Trigger transposeTrigger;
	Trigger tiedTrigger;
	Trigger stepTriggers[16];
	Trigger keyNoteTrigger;
	Trigger keyGateTrigger;
	Trigger seqCVTrigger;
	HoldDetect modeHoldDetect;
	
	
	inline bool isEditingSequence(void) {return params[EDIT_PARAM].getValue() > 0.5f;}
	
	
	LowFrequencyOscillator oscillatorClk;
	LowFrequencyOscillator oscillatorLfo;
	VoltageControlledOscillator oscillatorVco;


	SemiModularSynth() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char strBuf[32];
		for (int x = 0; x < 16; x++) {
			snprintf(strBuf, 32, "Step/phrase %i", x + 1);
			params[STEP_PHRASE_PARAMS + x].config(0.0f, 1.0f, 0.0f, strBuf);
		}
		params[ATTACH_PARAM].config(0.0f, 1.0f, 0.0f, "Attach");
		params[KEYNOTE_PARAM].config(0.0f, 1.0f, 0.0f, "Keyboard note mode");
		params[KEYGATE_PARAM].config(0.0f, 1.0f, 0.0f, "Keyboard gate-type mode");
		for (int i = 0; i < 7; i++) {
			snprintf(strBuf, 32, "Octave %i", i + 1);
			params[OCTAVE_PARAM + i].config(0.0f, 1.0f, 0.0f, strBuf);
		}
		params[KEY_PARAMS + 1].config(0.0, 1.0, 0.0, "C# key");
		params[KEY_PARAMS + 3].config(0.0, 1.0, 0.0, "D# key");
		params[KEY_PARAMS + 6].config(0.0, 1.0, 0.0, "F# key");
		params[KEY_PARAMS + 8].config(0.0, 1.0, 0.0, "G# key");
		params[KEY_PARAMS + 10].config(0.0, 1.0, 0.0, "A# key");

		params[KEY_PARAMS + 0].config(0.0, 1.0, 0.0, "C key");
		params[KEY_PARAMS + 2].config(0.0, 1.0, 0.0, "D key");
		params[KEY_PARAMS + 4].config(0.0, 1.0, 0.0, "E key");
		params[KEY_PARAMS + 5].config(0.0, 1.0, 0.0, "F key");
		params[KEY_PARAMS + 7].config(0.0, 1.0, 0.0, "G key");
		params[KEY_PARAMS + 9].config(0.0, 1.0, 0.0, "A key");
		params[KEY_PARAMS + 11].config(0.0, 1.0, 0.0, "B key");
		
		params[EDIT_PARAM].config(0.0f, 1.0f, 1.0f, "Seq/song mode");
		params[RUNMODE_PARAM].config(0.0f, 1.0f, 0.0f, "Length / run mode");
		params[RUN_PARAM].config(0.0f, 1.0f, 0.0f, "Run");
		params[SEQUENCE_PARAM].config(-INFINITY, INFINITY, 0.0f, "Sequence");		
		params[TRAN_ROT_PARAM].config(0.0f, 1.0f, 0.0f, "Transpose / rotate");
		
		params[RESET_PARAM].config(0.0f, 1.0f, 0.0f, "Reset");
		params[COPY_PARAM].config(0.0f, 1.0f, 0.0f, "Copy");
		params[PASTE_PARAM].config(0.0f, 1.0f, 0.0f, "Paste");
		params[CPMODE_PARAM].config(0.0f, 2.0f, 2.0f, "Copy-paste mode");	// 0.0f is top position

		params[GATE1_PARAM].config(0.0f, 1.0f, 0.0f, "Gate 1");
		params[GATE2_PARAM].config(0.0f, 1.0f, 0.0f, "Gate 2");
		params[TIE_PARAM].config(0.0f, 1.0f, 0.0f, "Tied");

		params[GATE1_PROB_PARAM].config(0.0f, 1.0f, 0.0f, "Gate 1 probability");
		params[GATE1_KNOB_PARAM].config(0.0f, 1.0f, 1.0f, "Probability");
		params[SLIDE_BTN_PARAM].config(0.0f, 1.0f, 0.0f, "CV slide");
		params[SLIDE_KNOB_PARAM].config(0.0f, 2.0f, 0.2f, "Slide rate");
		params[AUTOSTEP_PARAM].config(0.0f, 1.0f, 1.0f, "Autostep");		
		
		params[VCO_FREQ_PARAM].config(-54.0f, 54.0f, 0.0f, "VCO freq", " Hz", std::pow(2, 1/12.f), dsp::FREQ_C4);// https://community.vcvrack.com/t/rack-v1-development-blog/1149/230
		params[VCO_FINE_PARAM].config(-1.0f, 1.0f, 0.0f, "VCO freq fine");
		params[VCO_PW_PARAM].config(0.0f, 1.0f, 0.5f, "VCO pulse width", "%", 0.f, 100.f);
		params[VCO_FM_PARAM].config(0.0f, 1.0f, 0.0f, "VCO FM");
		params[VCO_PWM_PARAM].config(0.0f, 1.0f, 0.0f, "VCO PWM", "%", 0.f, 100.f);
		params[VCO_MODE_PARAM].config(0.0f, 1.0f, 1.0f, "VCO mode");
		params[VCO_OCT_PARAM].config(-2.0f, 2.0f, 0.0f, "VCO octave");
		
		params[CLK_FREQ_PARAM].config(-2.0f, 4.0f, 1.0f, "CLK freq", " BPM", 2.0f, 60.0f);// 120 BMP when default value  (120 = 60*2^1) diplay params are: base, mult, offset
		params[CLK_PW_PARAM].config(0.0f, 1.0f, 0.5f, "CLK PW");
		
		params[VCA_LEVEL1_PARAM].config(0.0f, 1.0f, 1.0f, "VCA level");
	
		params[ADSR_ATTACK_PARAM].config(0.0f, 1.0f, 0.1f, "ADSR attack");
		params[ADSR_DECAY_PARAM].config( 0.0f, 1.0f, 0.5f, "ADSR decay");
		params[ADSR_SUSTAIN_PARAM].config(0.0f, 1.0f, 0.5f, "ADSR sustain");
		params[ADSR_RELEASE_PARAM].config(0.0f, 1.0f, 0.5f, "ADSR release");
		
		params[VCF_FREQ_PARAM].config(0.0f, 1.0f, 0.666f, "VCF freq");
		params[VCF_RES_PARAM].config(0.0f, 1.0f, 0.0f, "VCF res");
		params[VCF_FREQ_CV_PARAM].config(-1.0f, 1.0f, 0.0f, "VCF freq cv");
		params[VCF_DRIVE_PARAM].config(0.0f, 1.0f, 0.0f, "VCF drive");
		
		params[LFO_FREQ_PARAM].config(-8.0f, 6.0f, -1.0f, "LFO freq");
		params[LFO_GAIN_PARAM].config(0.0f, 1.0f, 0.5f, "LFO gain");
		params[LFO_OFFSET_PARAM].config(-1.0f, 1.0f, 0.0f, "LFO offset");

		
		onReset();
		
		// VCO
		oscillatorVco.soft = false;
		
		// CLK 
		oscillatorClk.offset = true;
		oscillatorClk.invert = false;
		
		// LFO
		oscillatorLfo.setPulseWidth(0.5f);
		oscillatorLfo.offset = false;
		oscillatorLfo.invert = false;
	}
	

	void onReset() override {
		// SEQUENCER
		autoseq = false;
		autostepLen = false;
		holdTiedNotes = true;
		seqCVmethod = 0;
		pulsesPerStep = 1;
		running = true;
		runModeSong = MODE_FWD;
		stepIndexEdit = 0;
		phraseIndexEdit = 0;
		seqIndexEdit = 0;
		phrases = 4;
		for (int i = 0; i < 16; i++) {
			for (int s = 0; s < 16; s++) {
				cv[i][s] = 0.0f;
				attributes[i][s].init();
			}
			sequences[i].init(16, MODE_FWD);
			phrase[i] = 0;
			cvCPbuffer[i] = 0.0f;
			attribCPbuffer[i].init();
			phraseCPbuffer[i] = 0;
		}
		initRun();
		seqAttribCPbuffer.init(16, MODE_FWD);
		seqCopied = true;
		countCP = 16;
		startCP = 0;
		editingGate = 0ul;
		editingType = 0ul;
		infoCopyPaste = 0l;
		displayState = DISP_NORMAL;
		slideStepsRemain = 0ul;
		attached = false;
		clockPeriod = 0ul;
		tiedWarning = 0ul;
		attachedWarning = 0l;
		revertDisplay = 0l;
		resetOnRun = false;
		editingGateLength = 0l;
		lastGateEdit = 1l;
		editingPpqn = 0l;
		
		// VCO
		// none
		
		// CLK
		clkValue = 0.0f;
		
		// VCF
		filter.reset();
	}

	
	void onRandomize() override {
		if (isEditingSequence()) {
			for (int s = 0; s < 16; s++) {
				cv[seqIndexEdit][s] = ((float)(random::u32() % 7)) + ((float)(random::u32() % 12)) / 12.0f - 3.0f;
				attributes[seqIndexEdit][s].randomize();
				// if (attributes[seqIndexEdit][s].getTied()) {
					// activateTiedStep(seqIndexEdit, s);
				// }
			}
			sequences[seqIndexEdit].randomize(16, NUM_MODES - 1);
		}
	}
	
	
	void initRun() {// run button activated or run edge in run input jack
		phraseIndexRun = (runModeSong == MODE_REV ? phrases - 1 : 0);
		phraseIndexRunHistory = 0;

		int seq = (isEditingSequence() ? seqIndexEdit : phrase[phraseIndexRun]);
		stepIndexRun = (sequences[seq].getRunMode() == MODE_REV ? sequences[seq].getLength() - 1 : 0);
		stepIndexRunHistory = 0;

		ppqnCount = 0;
		gate1Code = calcGate1Code(attributes[seq][stepIndexRun], 0, pulsesPerStep, params[GATE1_KNOB_PARAM].getValue());
		gate2Code = calcGate2Code(attributes[seq][stepIndexRun], 0, pulsesPerStep);
		slideStepsRemain = 0ul;
		clockIgnoreOnReset = (long) (clockIgnoreOnResetDuration * APP->engine->getSampleRate());
	}
	
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// autostepLen
		json_object_set_new(rootJ, "autostepLen", json_boolean(autostepLen));
		
		// autoseq
		json_object_set_new(rootJ, "autoseq", json_boolean(autoseq));
		
		// holdTiedNotes
		json_object_set_new(rootJ, "holdTiedNotes", json_boolean(holdTiedNotes));
		
		// seqCVmethod
		json_object_set_new(rootJ, "seqCVmethod", json_integer(seqCVmethod));

		// pulsesPerStep
		json_object_set_new(rootJ, "pulsesPerStep", json_integer(pulsesPerStep));

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));
		
		// runModeSong
		json_object_set_new(rootJ, "runModeSong3", json_integer(runModeSong));

		// seqIndexEdit
		json_object_set_new(rootJ, "sequence", json_integer(seqIndexEdit));

		// phrase 
		json_t *phraseJ = json_array();
		for (int i = 0; i < 16; i++)
			json_array_insert_new(phraseJ, i, json_integer(phrase[i]));
		json_object_set_new(rootJ, "phrase", phraseJ);

		// phrases
		json_object_set_new(rootJ, "phrases", json_integer(phrases));

		// CV
		json_t *cvJ = json_array();
		for (int i = 0; i < 16; i++)
			for (int s = 0; s < 16; s++) {
				json_array_insert_new(cvJ, s + (i * 16), json_real(cv[i][s]));
			}
		json_object_set_new(rootJ, "cv", cvJ);

		// attributes
		json_t *attributesJ = json_array();
		for (int i = 0; i < 16; i++)
			for (int s = 0; s < 16; s++) {
				json_array_insert_new(attributesJ, s + (i * 16), json_integer(attributes[i][s].getAttribute()));
			}
		json_object_set_new(rootJ, "attributes", attributesJ);

		// attached
		json_object_set_new(rootJ, "attached", json_boolean(attached));

		// resetOnRun
		json_object_set_new(rootJ, "resetOnRun", json_boolean(resetOnRun));
		
		// stepIndexEdit
		json_object_set_new(rootJ, "stepIndexEdit", json_integer(stepIndexEdit));
	
		// phraseIndexEdit
		json_object_set_new(rootJ, "phraseIndexEdit", json_integer(phraseIndexEdit));

		// sequences
		json_t *sequencesJ = json_array();
		for (int i = 0; i < 16; i++)
			json_array_insert_new(sequencesJ, i, json_integer(sequences[i].getSeqAttrib()));
		json_object_set_new(rootJ, "sequences", sequencesJ);
		
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ) {
			panelTheme = json_integer_value(panelThemeJ);
			if (panelTheme > 1) panelTheme = 1;
		}

		// autostepLen
		json_t *autostepLenJ = json_object_get(rootJ, "autostepLen");
		if (autostepLenJ)
			autostepLen = json_is_true(autostepLenJ);

		// autoseq
		json_t *autoseqJ = json_object_get(rootJ, "autoseq");
		if (autoseqJ)
			autoseq = json_is_true(autoseqJ);

		// holdTiedNotes
		json_t *holdTiedNotesJ = json_object_get(rootJ, "holdTiedNotes");
		if (holdTiedNotesJ)
			holdTiedNotes = json_is_true(holdTiedNotesJ);
		else
			holdTiedNotes = false;// legacy
		
		// seqCVmethod
		json_t *seqCVmethodJ = json_object_get(rootJ, "seqCVmethod");
		if (seqCVmethodJ)
			seqCVmethod = json_integer_value(seqCVmethodJ);

		// pulsesPerStep
		json_t *pulsesPerStepJ = json_object_get(rootJ, "pulsesPerStep");
		if (pulsesPerStepJ)
			pulsesPerStep = json_integer_value(pulsesPerStepJ);

		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		
		// sequences
		json_t *sequencesJ = json_object_get(rootJ, "sequences");
		if (sequencesJ) {
			for (int i = 0; i < 16; i++)
			{
				json_t *sequencesArrayJ = json_array_get(sequencesJ, i);
				if (sequencesArrayJ)
					sequences[i].setSeqAttrib(json_integer_value(sequencesArrayJ));
			}			
		}
		else {// legacy
			int lengths[16];//1 to 16
			int runModeSeq[16]; 
			int transposeOffsets[16];

		
			// lengths
			json_t *lengthsJ = json_object_get(rootJ, "lengths");
			if (lengthsJ) {
				for (int i = 0; i < 16; i++)
				{
					json_t *lengthsArrayJ = json_array_get(lengthsJ, i);
					if (lengthsArrayJ)
						lengths[i] = json_integer_value(lengthsArrayJ);
				}			
			}
		
			// runModeSeq
			json_t *runModeSeqJ = json_object_get(rootJ, "runModeSeq3");
			if (runModeSeqJ) {
				for (int i = 0; i < 16; i++)
				{
					json_t *runModeSeqArrayJ = json_array_get(runModeSeqJ, i);
					if (runModeSeqArrayJ)
						runModeSeq[i] = json_integer_value(runModeSeqArrayJ);
				}			
			}		
			else {// legacy
				runModeSeqJ = json_object_get(rootJ, "runModeSeq2");
				if (runModeSeqJ) {
					for (int i = 0; i < 16; i++)
					{
						json_t *runModeSeqArrayJ = json_array_get(runModeSeqJ, i);
						if (runModeSeqArrayJ) {
							runModeSeq[i] = json_integer_value(runModeSeqArrayJ);
							if (runModeSeq[i] >= MODE_PEN)// this mode was not present in version runModeSeq2
								runModeSeq[i]++;
						}
					}			
				}		
			}
		
			// transposeOffsets
			json_t *transposeOffsetsJ = json_object_get(rootJ, "transposeOffsets");
			if (transposeOffsetsJ) {
				for (int i = 0; i < 16; i++)
				{
					json_t *transposeOffsetsArrayJ = json_array_get(transposeOffsetsJ, i);
					if (transposeOffsetsArrayJ)
						transposeOffsets[i] = json_integer_value(transposeOffsetsArrayJ);
				}			
			}
			
			// now write into new object
			for (int i = 0; i < 16; i++) {
				sequences[i].init(lengths[i], runModeSeq[i]);
				sequences[i].setTranspose(transposeOffsets[i]);
			}
		}

		// runModeSong
		json_t *runModeSongJ = json_object_get(rootJ, "runModeSong3");
		if (runModeSongJ)
			runModeSong = json_integer_value(runModeSongJ);
		else {// legacy
			runModeSongJ = json_object_get(rootJ, "runModeSong");
			if (runModeSongJ) {
				runModeSong = json_integer_value(runModeSongJ);
				if (runModeSong >= MODE_PEN)// this mode was not present in original version
					runModeSong++;
			}
		}
		
		// seqIndexEdit
		json_t *seqIndexEditJ = json_object_get(rootJ, "sequence");
		if (seqIndexEditJ)
			seqIndexEdit = json_integer_value(seqIndexEditJ);
		
		// phrase
		json_t *phraseJ = json_object_get(rootJ, "phrase");
		if (phraseJ)
			for (int i = 0; i < 16; i++)
			{
				json_t *phraseArrayJ = json_array_get(phraseJ, i);
				if (phraseArrayJ)
					phrase[i] = json_integer_value(phraseArrayJ);
			}
			
		// phrases
		json_t *phrasesJ = json_object_get(rootJ, "phrases");
		if (phrasesJ)
			phrases = json_integer_value(phrasesJ);
		
		// CV
		json_t *cvJ = json_object_get(rootJ, "cv");
		if (cvJ) {
			for (int i = 0; i < 16; i++)
				for (int s = 0; s < 16; s++) {
					json_t *cvArrayJ = json_array_get(cvJ, s + (i * 16));
					if (cvArrayJ)
						cv[i][s] = json_number_value(cvArrayJ);
				}
		}

		// attributes
		json_t *attributesJ = json_object_get(rootJ, "attributes");
		if (attributesJ) {
			for (int i = 0; i < 16; i++)
				for (int s = 0; s < 16; s++) {
					json_t *attributesArrayJ = json_array_get(attributesJ, s + (i * 16));
					if (attributesArrayJ)
						attributes[i][s].setAttribute((unsigned short)json_integer_value(attributesArrayJ));
				}
		}
	
		// attached
		json_t *attachedJ = json_object_get(rootJ, "attached");
		if (attachedJ)
			attached = json_is_true(attachedJ);
		
		// resetOnRun
		json_t *resetOnRunJ = json_object_get(rootJ, "resetOnRun");
		if (resetOnRunJ)
			resetOnRun = json_is_true(resetOnRunJ);

		// stepIndexEdit
		json_t *stepIndexEditJ = json_object_get(rootJ, "stepIndexEdit");
		if (stepIndexEditJ)
			stepIndexEdit = json_integer_value(stepIndexEditJ);
		
		// phraseIndexEdit
		json_t *phraseIndexEditJ = json_object_get(rootJ, "phraseIndexEdit");
		if (phraseIndexEditJ)
			phraseIndexEdit = json_integer_value(phraseIndexEditJ);
		
		// Initialize dependants after everything loaded
		initRun();
	}


	void rotateSeq(int seqNum, bool directionRight, int seqLength) {
		float rotCV;
		StepAttributes rotAttributes;
		int iStart = 0;
		int iEnd = seqLength - 1;
		int iRot = iStart;
		int iDelta = 1;
		if (directionRight) {
			iRot = iEnd;
			iDelta = -1;
		}
		rotCV = cv[seqNum][iRot];
		rotAttributes = attributes[seqNum][iRot];
		for ( ; ; iRot += iDelta) {
			if (iDelta == 1 && iRot >= iEnd) break;
			if (iDelta == -1 && iRot <= iStart) break;
			cv[seqNum][iRot] = cv[seqNum][iRot + iDelta];
			attributes[seqNum][iRot] = attributes[seqNum][iRot + iDelta];
		}
		cv[seqNum][iRot] = rotCV;
		attributes[seqNum][iRot] = rotAttributes;
	}
	

	void process(const ProcessArgs &args) override {
		float sampleRate = args.sampleRate;
	
		// SEQUENCER

		static const float gateTime = 0.4f;// seconds
		static const float revertDisplayTime = 0.7f;// seconds
		static const float warningTime = 0.7f;// seconds
		static const float holdDetectTime = 2.0f;// seconds
		static const float editGateLengthTime = 3.5f;// seconds
		
		
		//********** Buttons, knobs, switches and inputs **********
		
		// Edit mode
		bool editingSequence = isEditingSequence();// true = editing sequence, false = editing song
		
		// Run button
		if (runningTrigger.process(params[RUN_PARAM].getValue() + inputs[RUNCV_INPUT].getVoltage())) {// no input refresh here, don't want to introduce startup skew
			running = !running;
			if (running) {
				if (resetOnRun)
					initRun();
				//if (resetOnRun || clockIgnoreOnRun)
					clockIgnoreOnReset = (long) (clockIgnoreOnResetDuration * sampleRate);// always keep this since CLK gets reset when run turned on
			}
			displayState = DISP_NORMAL;
		}

		if ((lightRefreshCounter & userInputsStepSkipMask) == 0) {

			// Seq CV input
			if (inputs[SEQCV_INPUT].isConnected()) {
				if (seqCVmethod == 0) {// 0-10 V
					int newSeq = (int)( inputs[SEQCV_INPUT].getVoltage() * (16.0f - 1.0f) / 10.0f + 0.5f );
					seqIndexEdit = clamp(newSeq, 0, 16 - 1);
				}
				else if (seqCVmethod == 1) {// C4-D5#
					int newSeq = (int)( (inputs[SEQCV_INPUT].getVoltage()) * 12.0f + 0.5f );
					seqIndexEdit = clamp(newSeq, 0, 16 - 1);
				}
				else {// TrigIncr
					if (seqCVTrigger.process(inputs[SEQCV_INPUT].getVoltage()))
						seqIndexEdit = clamp(seqIndexEdit + 1, 0, 16 - 1);
				}	
			}
			
			// Attach button
			if (attachedTrigger.process(params[ATTACH_PARAM].getValue())) {
				attached = !attached;	
				displayState = DISP_NORMAL;			
			}
			if (running && attached) {
				if (editingSequence)
					stepIndexEdit = stepIndexRun;
				else
					phraseIndexEdit = phraseIndexRun;
			}
			
			// Copy button
			if (copyTrigger.process(params[COPY_PARAM].getValue())) {
				if (!attached) {
					startCP = editingSequence ? stepIndexEdit : phraseIndexEdit;
					countCP = 16;
					if (params[CPMODE_PARAM].getValue() > 1.5f)// all
						startCP = 0;
					else if (params[CPMODE_PARAM].getValue() < 0.5f)// 4
						countCP = std::min(4, 16 - startCP);
					else// 8
						countCP = std::min(8, 16 - startCP);
					if (editingSequence) {
						for (int i = 0, s = startCP; i < countCP; i++, s++) {
							cvCPbuffer[i] = cv[seqIndexEdit][s];
							attribCPbuffer[i] = attributes[seqIndexEdit][s];
						}
						seqAttribCPbuffer.setSeqAttrib(sequences[seqIndexEdit].getSeqAttrib());
						seqCopied = true;
					}
					else {
						for (int i = 0, p = startCP; i < countCP; i++, p++)
							phraseCPbuffer[i] = phrase[p];
						seqCopied = false;// so that a cross paste can be detected
					}
					infoCopyPaste = (long) (revertDisplayTime * sampleRate / displayRefreshStepSkips);
					displayState = DISP_NORMAL;
				}
				else
					attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
			}
			// Paste button
			if (pasteTrigger.process(params[PASTE_PARAM].getValue())) {
				if (!attached) {
					infoCopyPaste = (long) (-1 * revertDisplayTime * sampleRate / displayRefreshStepSkips);
					startCP = 0;
					if (countCP <= 8) {
						startCP = editingSequence ? stepIndexEdit : phraseIndexEdit;
						countCP = std::min(countCP, 16 - startCP);
					}
					// else nothing to do for ALL

					if (editingSequence) {
						if (seqCopied) {// non-crossed paste (seq vs song)
							for (int i = 0, s = startCP; i < countCP; i++, s++) {
								cv[seqIndexEdit][s] = cvCPbuffer[i];
								attributes[seqIndexEdit][s] = attribCPbuffer[i];
							}
							if (params[CPMODE_PARAM].getValue() > 1.5f) {// all
								sequences[seqIndexEdit].setSeqAttrib(seqAttribCPbuffer.getSeqAttrib());
							}
						}
						else {// crossed paste to seq (seq vs song)
							if (params[CPMODE_PARAM].getValue() > 1.5f) { // ALL (init steps)
								for (int s = 0; s < 16; s++) {
									//cv[seqIndexEdit][s] = 0.0f;
									//attributes[seqIndexEdit][s].init();
									attributes[seqIndexEdit][s].toggleGate1();
								}
								sequences[seqIndexEdit].setTranspose(0);
								sequences[seqIndexEdit].setRotate(0);
							}
							else if (params[CPMODE_PARAM].getValue() < 0.5f) {// 4 (randomize CVs)
								for (int s = 0; s < 16; s++)
									cv[seqIndexEdit][s] = ((float)(random::u32() % 7)) + ((float)(random::u32() % 12)) / 12.0f - 3.0f;
								sequences[seqIndexEdit].setTranspose(0);
								sequences[seqIndexEdit].setRotate(0);
							}
							else {// 8 (randomize gate 1)
								for (int s = 0; s < 16; s++)
									if ( (random::u32() & 0x1) != 0)
										attributes[seqIndexEdit][s].toggleGate1();
							}
							startCP = 0;
							countCP = 16;
							infoCopyPaste *= 2l;
						}
					}
					else {
						if (!seqCopied) {// non-crossed paste (seq vs song)
							for (int i = 0, p = startCP; i < countCP; i++, p++)
								phrase[p] = phraseCPbuffer[i];
						}
						else {// crossed paste to song (seq vs song)
							if (params[CPMODE_PARAM].getValue() > 1.5f) { // ALL (init phrases)
								for (int p = 0; p < 16; p++)
									phrase[p] = 0;
							}
							else if (params[CPMODE_PARAM].getValue() < 0.5f) {// 4 (phrases increase from 1 to 16)
								for (int p = 0; p < 16; p++)
									phrase[p] = p;						
							}
							else {// 8 (randomize phrases)
								for (int p = 0; p < 16; p++)
									phrase[p] = random::u32() % 16;
							}
							startCP = 0;
							countCP = 16;
							infoCopyPaste *= 2l;
						}					
					}
					displayState = DISP_NORMAL;
				}
				else
					attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
			}
			
			// Write input (must be before Left and Right in case route gate simultaneously to Right and Write for example)
			//  (write must be to correct step)
			bool writeTrig = writeTrigger.process(inputs[WRITE_INPUT].getVoltage());
			if (writeTrig) {
				if (editingSequence) {
					if (!attributes[seqIndexEdit][stepIndexEdit].getTied()) {
						cv[seqIndexEdit][stepIndexEdit] = inputs[CV_INPUT].getVoltage();
						propagateCVtoTied(seqIndexEdit, stepIndexEdit);
					}
					editingGate = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
					editingGateCV = inputs[CV_INPUT].getVoltage();// cv[seqIndexEdit][stepIndexEdit];
					editingGateKeyLight = -1;
					// Autostep (after grab all active inputs)
					if (params[AUTOSTEP_PARAM].getValue() > 0.5f) {
						stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + 1, autostepLen ? sequences[seqIndexEdit].getLength() : 16);
						if (stepIndexEdit == 0 && autoseq)
							seqIndexEdit = moveIndex(seqIndexEdit, seqIndexEdit + 1, 16);
					}
				}
				displayState = DISP_NORMAL;
			}
			// Left and Right CV inputs
			int delta = 0;
			if (leftTrigger.process(inputs[LEFTCV_INPUT].getVoltage())) { 
				delta = -1;
				if (displayState != DISP_LENGTH)
					displayState = DISP_NORMAL;
			}
			if (rightTrigger.process(inputs[RIGHTCV_INPUT].getVoltage())) {
				delta = +1;
				if (displayState != DISP_LENGTH)
					displayState = DISP_NORMAL;
			}
			if (delta != 0) {
				if (!running || !attached) {// don't move heads when attach and running
					if (editingSequence) {
						stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + delta, 16);
						if (!attributes[seqIndexEdit][stepIndexEdit].getTied()) {// play if non-tied step
							if (!writeTrig) {// in case autostep when simultaneous writeCV and stepCV (keep what was done in Write Input block above)
								editingGate = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
								editingGateCV = cv[seqIndexEdit][stepIndexEdit];
								editingGateKeyLight = -1;
							}
						}
					}
					else {
						phraseIndexEdit = moveIndex(phraseIndexEdit, phraseIndexEdit + delta, 16);
						if (!running)
							phraseIndexRun = phraseIndexEdit;
					}
				}
			}

			// Step button presses
			int stepPressed = -1;
			for (int i = 0; i < 16; i++) {
				if (stepTriggers[i].process(params[STEP_PHRASE_PARAMS + i].getValue()))
					stepPressed = i;
			}
			if (stepPressed != -1) {
				if (displayState == DISP_LENGTH) {
					if (editingSequence)
						sequences[seqIndexEdit].setLength(stepPressed + 1);
					else
						phrases = stepPressed + 1;
					revertDisplay = (long) (revertDisplayTime * sampleRate / displayRefreshStepSkips);
				}
				else {
					if (!running || !attached) {// not running or detached
						if (editingSequence) {
							stepIndexEdit = stepPressed;
							if (!attributes[seqIndexEdit][stepIndexEdit].getTied()) {// play if non-tied step
								editingGate = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
								editingGateCV = cv[seqIndexEdit][stepIndexEdit];
								editingGateKeyLight = -1;
							}
						}
						else {
							phraseIndexEdit = stepPressed;
							if (!running)
								phraseIndexRun = phraseIndexEdit;
						}
					}
					else if (attached)
						attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
					displayState = DISP_NORMAL;
				}
			} 
			
			// Mode/Length button
			if (modeTrigger.process(params[RUNMODE_PARAM].getValue())) {
				if (!attached) {
					if (editingPpqn != 0l)
						editingPpqn = 0l;			
					if (displayState == DISP_NORMAL || displayState == DISP_TRANSPOSE || displayState == DISP_ROTATE)
						displayState = DISP_LENGTH;
					else if (displayState == DISP_LENGTH)
						displayState = DISP_MODE;
					else
						displayState = DISP_NORMAL;
					modeHoldDetect.start((long) (holdDetectTime * sampleRate / displayRefreshStepSkips));
				}
				else
					attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
			}
			
			// Transpose/Rotate button
			if (transposeTrigger.process(params[TRAN_ROT_PARAM].getValue())) {
				if (editingSequence && !attached) {
					if (displayState == DISP_NORMAL || displayState == DISP_MODE || displayState == DISP_LENGTH) {
						displayState = DISP_TRANSPOSE;
					}
					else if (displayState == DISP_TRANSPOSE) {
						displayState = DISP_ROTATE;
					}
					else 
						displayState = DISP_NORMAL;
				}
				else if (attached)
					attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
			}			
			
			// Sequence knob  
			float seqParamValue = params[SEQUENCE_PARAM].getValue();
			int newSequenceKnob = (int)roundf(seqParamValue * 7.0f);
			if (seqParamValue == 0.0f)// true when constructor or dataFromJson() occured
				sequenceKnob = newSequenceKnob;
			int deltaKnob = newSequenceKnob - sequenceKnob;
			if (deltaKnob != 0) {
				if (abs(deltaKnob) <= 3) {// avoid discontinuous step (initialize for example)
					// any changes in here should may also require right click behavior to be updated in the knob's onMouseDown()
					if (editingPpqn != 0) {
						pulsesPerStep = indexToPps(ppsToIndex(pulsesPerStep) + deltaKnob);// indexToPps() does clamping
						editingPpqn = (long) (editGateLengthTime * sampleRate / displayRefreshStepSkips);
					}
					else if (displayState == DISP_MODE) {
						if (editingSequence) {
							sequences[seqIndexEdit].setRunMode(clamp(sequences[seqIndexEdit].getRunMode() + deltaKnob, 0, (NUM_MODES - 1 - 1)));
						}
						else {
							runModeSong = clamp(runModeSong + deltaKnob, 0, 6 - 1);
						}
					}
					else if (displayState == DISP_LENGTH) {
						if (editingSequence) {
							sequences[seqIndexEdit].setLength(clamp(sequences[seqIndexEdit].getLength() + deltaKnob, 1, 16));
						}
						else {
							phrases = clamp(phrases + deltaKnob, 1, 16);
						}
					}
					else if (displayState == DISP_TRANSPOSE) {
						if (editingSequence) {
							sequences[seqIndexEdit].setTranspose(clamp(sequences[seqIndexEdit].getTranspose() + deltaKnob, -99, 99));
							float transposeOffsetCV = ((float)(deltaKnob))/12.0f;// Tranpose by deltaKnob number of semi-tones
							for (int s = 0; s < 16; s++) {
								cv[seqIndexEdit][s] += transposeOffsetCV;
							}
						}
					}
					else if (displayState == DISP_ROTATE) {
						if (editingSequence) {
							int slength = sequences[seqIndexEdit].getLength();
							sequences[seqIndexEdit].setRotate(clamp(sequences[seqIndexEdit].getRotate() + deltaKnob, -99, 99));
							if (deltaKnob > 0 && deltaKnob < 201) {// Rotate right, 201 is safety
								for (int i = deltaKnob; i > 0; i--) {
									rotateSeq(seqIndexEdit, true, slength);
									if (stepIndexEdit < slength)
										stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + 1, slength);
								}
							}
							if (deltaKnob < 0 && deltaKnob > -201) {// Rotate left, 201 is safety
								for (int i = deltaKnob; i < 0; i++) {
									rotateSeq(seqIndexEdit, false, slength);
									if (stepIndexEdit < slength)
										stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit - 1, slength);
								}
							}
						}						
					}
					else {// DISP_NORMAL
						if (editingSequence) {
							if (!inputs[SEQCV_INPUT].isConnected()) {
								seqIndexEdit = clamp(seqIndexEdit + deltaKnob, 0, 16 - 1);
							}
						}
						else {
							if (!attached || (attached && !running))
								phrase[phraseIndexEdit] = clamp(phrase[phraseIndexEdit] + deltaKnob, 0, 16 - 1);
							else
								attachedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
						}
					}
				}
				sequenceKnob = newSequenceKnob;
			}	
			
			// Octave buttons
			for (int i = 0; i < 7; i++) {
				if (octTriggers[i].process(params[OCTAVE_PARAM + i].getValue())) {
					if (editingSequence) {
						displayState = DISP_NORMAL;
						if (attributes[seqIndexEdit][stepIndexEdit].getTied())
							tiedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
						else {			
							cv[seqIndexEdit][stepIndexEdit] = applyNewOct(cv[seqIndexEdit][stepIndexEdit], 6 - i);
							propagateCVtoTied(seqIndexEdit, stepIndexEdit);
							editingGate = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
							editingGateCV = cv[seqIndexEdit][stepIndexEdit];
							editingGateKeyLight = -1;
						}
					}
				}
			}
			
			// Keyboard buttons
			for (int i = 0; i < 12; i++) {
				if (keyTriggers[i].process(params[KEY_PARAMS + i].getValue())) {
					if (editingSequence) {
						displayState = DISP_NORMAL;
						if (editingGateLength != 0l) {
							int newMode = keyIndexToGateMode(i, pulsesPerStep);
							if (newMode != -1) {
								editingPpqn = 0l;
								attributes[seqIndexEdit][stepIndexEdit].setGateMode(newMode, editingGateLength > 0l);
								if (params[KEY_PARAMS + i].maxValue > 1.5f) {// if double-click
									stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + 1, 16);
									editingType = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
									editingGateKeyLight = i;
									if ((APP->window->getMods() & WINDOW_MOD_MASK) == WINDOW_MOD_CTRL)
										attributes[seqIndexEdit][stepIndexEdit].setGateMode(newMode, editingGateLength > 0l);
								}
							}
							else
								editingPpqn = (long) (editGateLengthTime * sampleRate / displayRefreshStepSkips);
						}
						else if (attributes[seqIndexEdit][stepIndexEdit].getTied()) {
							if (params[KEY_PARAMS + i].maxValue > 1.5f)// if double-click
								stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + 1, 16);
							else
								tiedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
						}
						else {			
							float newCV = floor(cv[seqIndexEdit][stepIndexEdit]) + ((float) i) / 12.0f;
							cv[seqIndexEdit][stepIndexEdit] = newCV;
							propagateCVtoTied(seqIndexEdit, stepIndexEdit);
							editingGate = (unsigned long) (gateTime * sampleRate / displayRefreshStepSkips);
							editingGateCV = cv[seqIndexEdit][stepIndexEdit];
							editingGateKeyLight = -1;
							if (params[KEY_PARAMS + i].maxValue > 1.5f) {// if double-click
								stepIndexEdit = moveIndex(stepIndexEdit, stepIndexEdit + 1, 16);
								editingGateKeyLight = i;
								if ((APP->window->getMods() & WINDOW_MOD_MASK) == WINDOW_MOD_CTRL)
									cv[seqIndexEdit][stepIndexEdit] = newCV;
							}
						}						
					}
				}
			}
			
			// Keyboard mode (note or gate type)
			if (keyNoteTrigger.process(params[KEYNOTE_PARAM].getValue())) {
				editingGateLength = 0l;
			}
			if (keyGateTrigger.process(params[KEYGATE_PARAM].getValue())) {
				if (editingGateLength == 0l) {
					editingGateLength = lastGateEdit;
				}
				else {
					editingGateLength *= -1l;
					lastGateEdit = editingGateLength;
				}
			}

			// Gate1, Gate1Prob, Gate2, Slide and Tied buttons
			if (gate1Trigger.process(params[GATE1_PARAM].getValue())) {
				if (editingSequence) {
					displayState = DISP_NORMAL;
					attributes[seqIndexEdit][stepIndexEdit].toggleGate1();
				}
			}		
			if (gate1ProbTrigger.process(params[GATE1_PROB_PARAM].getValue())) {
				if (editingSequence) {
					displayState = DISP_NORMAL;
					if (attributes[seqIndexEdit][stepIndexEdit].getTied())
						tiedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
					else
						attributes[seqIndexEdit][stepIndexEdit].toggleGate1P();
				}
			}		
			if (gate2Trigger.process(params[GATE2_PARAM].getValue())) {
				if (editingSequence) {
					displayState = DISP_NORMAL;
					attributes[seqIndexEdit][stepIndexEdit].toggleGate2();
				}
			}		
			if (slideTrigger.process(params[SLIDE_BTN_PARAM].getValue())) {
				if (editingSequence) {
					displayState = DISP_NORMAL;
					if (attributes[seqIndexEdit][stepIndexEdit].getTied())
						tiedWarning = (long) (warningTime * sampleRate / displayRefreshStepSkips);
					else
						attributes[seqIndexEdit][stepIndexEdit].toggleSlide();
				}
			}		
			if (tiedTrigger.process(params[TIE_PARAM].getValue())) {
				if (editingSequence) {
					displayState = DISP_NORMAL;
					if (attributes[seqIndexEdit][stepIndexEdit].getTied()) {
						deactivateTiedStep(seqIndexEdit, stepIndexEdit);
					}
					else {
						activateTiedStep(seqIndexEdit, stepIndexEdit);
					}
				}
			}		
		
		}// userInputs refresh


		
		//********** Clock and reset **********
		
		// Clock
		float clockInput = inputs[CLOCK_INPUT].isConnected() ? inputs[CLOCK_INPUT].getVoltage() : clkValue;// Pre-patching
		if (running && clockIgnoreOnReset == 0l) {
			if (clockTrigger.process(clockInput)) {
				ppqnCount++;
				if (ppqnCount >= pulsesPerStep)
					ppqnCount = 0;

				int newSeq = seqIndexEdit;// good value when editingSequence, overwrite if not editingSequence
				if (ppqnCount == 0) {
					float slideFromCV = 0.0f;
					if (editingSequence) {
						slideFromCV = cv[seqIndexEdit][stepIndexRun];
						moveIndexRunMode(&stepIndexRun, sequences[seqIndexEdit].getLength(), sequences[seqIndexEdit].getRunMode(), &stepIndexRunHistory);
					}
					else {
						slideFromCV = cv[phrase[phraseIndexRun]][stepIndexRun];
						if (moveIndexRunMode(&stepIndexRun, sequences[phrase[phraseIndexRun]].getLength(), sequences[phrase[phraseIndexRun]].getRunMode(), &stepIndexRunHistory)) {
							moveIndexRunMode(&phraseIndexRun, phrases, runModeSong, &phraseIndexRunHistory);
							stepIndexRun = (sequences[phrase[phraseIndexRun]].getRunMode() == MODE_REV ? sequences[phrase[phraseIndexRun]].getLength() - 1 : 0);// must always refresh after phraseIndexRun has changed
						}
						newSeq = phrase[phraseIndexRun];
					}
					
					// Slide
					if (attributes[newSeq][stepIndexRun].getSlide()) {
						slideStepsRemain =   (unsigned long) (((float)clockPeriod * pulsesPerStep) * params[SLIDE_KNOB_PARAM].getValue() / 2.0f);
						if (slideStepsRemain != 0ul) {
							float slideToCV = cv[newSeq][stepIndexRun];
							slideCVdelta = (slideToCV - slideFromCV)/(float)slideStepsRemain;
						}
					}
					else 
						slideStepsRemain = 0ul;
				}
				else {
					if (!editingSequence)
						newSeq = phrase[phraseIndexRun];
				}
				if (gate1Code != -1 || ppqnCount == 0)
					gate1Code = calcGate1Code(attributes[newSeq][stepIndexRun], ppqnCount, pulsesPerStep, params[GATE1_KNOB_PARAM].getValue());
				gate2Code = calcGate2Code(attributes[newSeq][stepIndexRun], ppqnCount, pulsesPerStep);
				clockPeriod = 0ul;				
			}
			clockPeriod++;
		}	
		
		// Reset
		if (resetTrigger.process(inputs[RESET_INPUT].getVoltage() + params[RESET_PARAM].getValue())) {
			initRun();// must be after sequence reset
			resetLight = 1.0f;
			displayState = DISP_NORMAL;
			if (inputs[SEQCV_INPUT].isConnected() && seqCVmethod == 2)
				seqIndexEdit = 0;
		}
		
		
		//********** Outputs and lights **********
				
		// CV and gates outputs
		int seq = editingSequence ? (seqIndexEdit) : (running ? phrase[phraseIndexRun] : phrase[phraseIndexEdit]);
		int step = editingSequence ? (running ? stepIndexRun : stepIndexEdit) : (stepIndexRun);
		if (running) {
			bool muteGate1 = !editingSequence && (params[GATE1_PARAM].getValue() > 0.5f);// live mute
			bool muteGate2 = !editingSequence && (params[GATE2_PARAM].getValue() > 0.5f);// live mute
			float slideOffset = (slideStepsRemain > 0ul ? (slideCVdelta * (float)slideStepsRemain) : 0.0f);
			outputs[CV_OUTPUT].setVoltage(cv[seq][step] - slideOffset);
			bool retriggingOnReset = (clockIgnoreOnReset != 0l && retrigGatesOnReset);
			outputs[GATE1_OUTPUT].setVoltage((calcGate(gate1Code, clockTrigger, clockPeriod, sampleRate) && !muteGate1 && !retriggingOnReset) ? 10.0f : 0.0f);
			outputs[GATE2_OUTPUT].setVoltage((calcGate(gate2Code, clockTrigger, clockPeriod, sampleRate) && !muteGate2 && !retriggingOnReset) ? 10.0f : 0.0f);
		}
		else {// not running 
			outputs[CV_OUTPUT].setVoltage((editingGate > 0ul) ? editingGateCV : cv[seq][step]);
			outputs[GATE1_OUTPUT].setVoltage((editingGate > 0ul) ? 10.0f : 0.0f);
			outputs[GATE2_OUTPUT].setVoltage((editingGate > 0ul) ? 10.0f : 0.0f);
		}
		if (slideStepsRemain > 0ul)
			slideStepsRemain--;
		
		lightRefreshCounter++;
		if (lightRefreshCounter >= displayRefreshStepSkips) {
			lightRefreshCounter = 0;

			// Step/phrase lights
			for (int i = 0; i < 16; i++) {
				float red = 0.0f;
				float green = 0.0f;
				float white = 0.0f;
				if (infoCopyPaste != 0l) {
					if (i >= startCP && i < (startCP + countCP))
						green = 0.5f;
				}
				else if (displayState == DISP_LENGTH) {
					if (editingSequence) {
						if (i < (sequences[seqIndexEdit].getLength() - 1))
							green = 0.1f;
						else if (i == (sequences[seqIndexEdit].getLength() - 1))
							green = 1.0f;
					}
					else {
						if (i < phrases - 1)
							green = 0.1f;
						else
							green = (i == phrases - 1) ? 1.0f : 0.0f;
					}					
				}
				else if (displayState == DISP_TRANSPOSE) {
					red = 0.5f;
				}
				else if (displayState == DISP_ROTATE) {
					red = (i == stepIndexEdit ? 1.0f : (i < sequences[seqIndexEdit].getLength() ? 0.2f : 0.0f));
				}
				else {// normal led display (i.e. not length)
					// Run cursor (green)
					if (editingSequence)
						green = ((running && (i == stepIndexRun)) ? 1.0f : 0.0f);
					else {
						green = ((running && (i == phraseIndexRun)) ? 1.0f : 0.0f);
						green += ((running && (i == stepIndexRun) && i != phraseIndexEdit) ? 0.1f : 0.0f);
						green = clamp(green, 0.0f, 1.0f);
					}
					// Edit cursor (red)
					if (editingSequence)
						red = (i == stepIndexEdit ? 1.0f : 0.0f);
					else
						red = (i == phraseIndexEdit ? 1.0f : 0.0f);
					bool gate = false;
					if (editingSequence)
						gate = attributes[seqIndexEdit][i].getGate1();
					else if (!editingSequence && (attached && running))
						gate = attributes[phrase[phraseIndexRun]][i].getGate1();
					white = ((green == 0.0f && red == 0.0f && gate && displayState != DISP_MODE) ? 0.04f : 0.0f);
					if (editingSequence && white != 0.0f) {
						green = 0.02f; white = 0.0f;
					}
				}
				setGreenRed(STEP_PHRASE_LIGHTS + i * 3, green, red);
				lights[STEP_PHRASE_LIGHTS + i * 3 + 2].value = white;
			}
		
			// Octave lights
			float octCV = 0.0f;
			if (editingSequence)
				octCV = cv[seqIndexEdit][stepIndexEdit];
			else
				octCV = cv[phrase[phraseIndexEdit]][stepIndexRun];
			int octLightIndex = (int) floor(octCV + 3.0f);
			for (int i = 0; i < 7; i++) {
				if (!editingSequence && (!attached || !running))// no oct lights when song mode and either (detached [1] or stopped [2])
												// [1] makes no sense, can't mod steps and stepping though seq that may not be playing
												// [2] CV is set to 0V when not running and in song mode, so cv[][] makes no sense to display
					lights[OCTAVE_LIGHTS + i].value = 0.0f;
				else {
					if (tiedWarning > 0l) {
						bool warningFlashState = calcWarningFlash(tiedWarning, (long) (warningTime * sampleRate / displayRefreshStepSkips));
						lights[OCTAVE_LIGHTS + i].value = (warningFlashState && (i == (6 - octLightIndex))) ? 1.0f : 0.0f;
					}
					else				
						lights[OCTAVE_LIGHTS + i].value = (i == (6 - octLightIndex) ? 1.0f : 0.0f);
				}
			}
			
			// Keyboard lights
			float cvValOffset;
			if (editingSequence) 
				cvValOffset = cv[seqIndexEdit][stepIndexEdit] + 10.0f;//to properly handle negative note voltages
			else	
				cvValOffset = cv[phrase[phraseIndexEdit]][stepIndexRun] + 10.0f;//to properly handle negative note voltages
			int keyLightIndex = clamp( (int)((cvValOffset-floor(cvValOffset)) * 12.0f + 0.5f),  0,  11);
			if (editingPpqn != 0) {
				for (int i = 0; i < 12; i++) {
					if (keyIndexToGateMode(i, pulsesPerStep) != -1) {
						setGreenRed(KEY_LIGHTS + i * 2, 1.0f, 1.0f);
					}
					else {
						setGreenRed(KEY_LIGHTS + i * 2, 0.0f, 0.0f);
					}
				}
			} 
			else if (editingGateLength != 0l && editingSequence) {
				int modeLightIndex = gateModeToKeyLightIndex(attributes[seqIndexEdit][stepIndexEdit], editingGateLength > 0l);
				for (int i = 0; i < 12; i++) {
					float green = editingGateLength > 0l ? 1.0f : 0.2f;
					float red = editingGateLength > 0l ? 0.2f : 1.0f;
					if (editingType > 0ul) {
						if (i == editingGateKeyLight) {
							float dimMult = ((float) editingType / (float)(gateTime * sampleRate / displayRefreshStepSkips));
							setGreenRed(KEY_LIGHTS + i * 2, green * dimMult, red * dimMult);
						}
						else
							setGreenRed(KEY_LIGHTS + i * 2, 0.0f, 0.0f);
					}
					else {
						if (i == modeLightIndex) {
							setGreenRed(KEY_LIGHTS + i * 2, green, red);
						}
						else { // show dim note if gatetype is different than note
							setGreenRed(KEY_LIGHTS + i * 2, 0.0f, (i == keyLightIndex ? 0.1f : 0.0f));
						}
					}
				}
			}
			else {
				for (int i = 0; i < 12; i++) {
					lights[KEY_LIGHTS + i * 2 + 0].value = 0.0f;
					if (!editingSequence && (!attached || !running))// no keyboard lights when song mode and either (detached [1] or stopped [2])
													// [1] makes no sense, can't mod steps and stepping though seq that may not be playing
													// [2] CV is set to 0V when not running and in song mode, so cv[][] makes no sense to display
						lights[KEY_LIGHTS + i * 2 + 1].value = 0.0f;
					else {
						if (tiedWarning > 0l) {
							bool warningFlashState = calcWarningFlash(tiedWarning, (long) (warningTime * sampleRate / displayRefreshStepSkips));
							lights[KEY_LIGHTS + i * 2 + 1].value = (warningFlashState && i == keyLightIndex) ? 1.0f : 0.0f;
						}
						else {
							if (editingGate > 0ul && editingGateKeyLight != -1)
								lights[KEY_LIGHTS + i * 2 + 1].value = (i == editingGateKeyLight ? ((float) editingGate / (float)(gateTime * sampleRate / displayRefreshStepSkips)) : 0.0f);
							else
								lights[KEY_LIGHTS + i * 2 + 1].value = (i == keyLightIndex ? 1.0f : 0.0f);
						}
					}
				}	
			}	
			
			// Key mode light (note or gate type)
			lights[KEYNOTE_LIGHT].value = editingGateLength == 0l ? 10.0f : 0.0f;
			if (editingGateLength == 0l)
				setGreenRed(KEYGATE_LIGHT, 0.0f, 0.0f);
			else if (editingGateLength > 0l)
				setGreenRed(KEYGATE_LIGHT, 1.0f, 0.2f);
			else
				setGreenRed(KEYGATE_LIGHT, 0.2f, 1.0f);

			// Gate1, Gate1Prob, Gate2, Slide and Tied lights
			if (!editingSequence && (!attached || !running)) {// no oct lights when song mode and either (detached [1] or stopped [2])
											// [1] makes no sense, can't mod steps and stepping though seq that may not be playing
											// [2] CV is set to 0V when not running and in song mode, so cv[][] makes no sense to display
				setGateLight(false, GATE1_LIGHT);
				setGateLight(false, GATE2_LIGHT);
				setGreenRed(GATE1_PROB_LIGHT, 0.0f, 0.0f);
				lights[SLIDE_LIGHT].value = 0.0f;
				lights[TIE_LIGHT].value = 0.0f;
			}
			else {
				StepAttributes attributesVal = attributes[seqIndexEdit][stepIndexEdit];
				if (!editingSequence)
					attributesVal = attributes[phrase[phraseIndexEdit]][stepIndexRun];
				//
				setGateLight(attributesVal.getGate1(), GATE1_LIGHT);
				setGateLight(attributesVal.getGate2(), GATE2_LIGHT);
				setGreenRed(GATE1_PROB_LIGHT, attributesVal.getGate1P() ? 1.0f : 0.0f, attributesVal.getGate1P() ? 1.0f : 0.0f);
				lights[SLIDE_LIGHT].value = attributesVal.getSlide() ? 1.0f : 0.0f;
				if (tiedWarning > 0l) {
					bool warningFlashState = calcWarningFlash(tiedWarning, (long) (warningTime * sampleRate / displayRefreshStepSkips));
					lights[TIE_LIGHT].value = (warningFlashState) ? 1.0f : 0.0f;
				}
				else
					lights[TIE_LIGHT].value = attributesVal.getTied() ? 1.0f : 0.0f;
			}

			// Attach light
			if (attachedWarning > 0l) {
				bool warningFlashState = calcWarningFlash(attachedWarning, (long) (warningTime * sampleRate / displayRefreshStepSkips));
				lights[ATTACH_LIGHT].value = (warningFlashState) ? 1.0f : 0.0f;
			}
			else
				lights[ATTACH_LIGHT].value = (attached ? 1.0f : 0.0f);
			
			// Reset light
			lights[RESET_LIGHT].value =	resetLight;	
			resetLight -= (resetLight / lightLambda) * args.sampleTime * displayRefreshStepSkips;
			
			// Run light
			lights[RUN_LIGHT].value = running ? 1.0f : 0.0f;
			
			if (editingGate > 0ul)
				editingGate--;
			if (editingType > 0ul)
				editingType--;
			if (infoCopyPaste != 0l) {
				if (infoCopyPaste > 0l)
					infoCopyPaste --;
				if (infoCopyPaste < 0l)
					infoCopyPaste ++;
			}
			if (editingPpqn > 0l)
				editingPpqn--;
			if (tiedWarning > 0l)
				tiedWarning--;
			if (attachedWarning > 0l)
				attachedWarning--;
			if (modeHoldDetect.process(params[RUNMODE_PARAM].getValue())) {
				displayState = DISP_NORMAL;
				editingPpqn = (long) (editGateLengthTime * sampleRate / displayRefreshStepSkips);
			}
			if (revertDisplay > 0l) {
				if (revertDisplay == 1)
					displayState = DISP_NORMAL;
				revertDisplay--;
			}
		}// lightRefreshCounter
		
		if (clockIgnoreOnReset > 0l)
			clockIgnoreOnReset--;

		
		// VCO
		oscillatorVco.analog = params[VCO_MODE_PARAM].getValue() > 0.0f;
		float pitchFine = 3.0f * dsp::quadraticBipolar(params[VCO_FINE_PARAM].getValue());
		float pitchCv = 12.0f * (inputs[VCO_PITCH_INPUT].isConnected() ? inputs[VCO_PITCH_INPUT].getVoltage() : outputs[CV_OUTPUT].value);// Pre-patching
		float pitchOctOffset = 12.0f * params[VCO_OCT_PARAM].getValue();
		if (inputs[VCO_FM_INPUT].isConnected()) {
			pitchCv += dsp::quadraticBipolar(params[VCO_FM_PARAM].getValue()) * 12.0f * inputs[VCO_FM_INPUT].getVoltage();
		}
		oscillatorVco.setPitch(params[VCO_FREQ_PARAM].getValue(), pitchFine + pitchCv + pitchOctOffset);
		oscillatorVco.setPulseWidth(params[VCO_PW_PARAM].getValue() + params[VCO_PWM_PARAM].getValue() * inputs[VCO_PW_INPUT].getVoltage() / 10.0f);
		oscillatorVco.syncEnabled = inputs[VCO_SYNC_INPUT].isConnected();
		oscillatorVco.process(args.sampleTime, inputs[VCO_SYNC_INPUT].getVoltage());
		if (outputs[VCO_SIN_OUTPUT].isConnected())
			outputs[VCO_SIN_OUTPUT].setVoltage(5.0f * oscillatorVco.sin());
		if (outputs[VCO_TRI_OUTPUT].isConnected())
			outputs[VCO_TRI_OUTPUT].setVoltage(5.0f * oscillatorVco.tri());
		if (outputs[VCO_SAW_OUTPUT].isConnected())
			outputs[VCO_SAW_OUTPUT].setVoltage(5.0f * oscillatorVco.saw());
		//if (outputs[VCO_SQR_OUTPUT].isConnected())
			outputs[VCO_SQR_OUTPUT].setVoltage(5.0f * oscillatorVco.sqr());		
			
			
		// CLK
		if ((lightRefreshCounter & userInputsStepSkipMask) == 0) {
			oscillatorClk.setPitch(params[CLK_FREQ_PARAM].getValue() + log2f(pulsesPerStep));
			oscillatorClk.setPulseWidth(params[CLK_PW_PARAM].getValue());
		}	
		oscillatorClk.step(args.sampleTime);
		oscillatorClk.setReset(inputs[RESET_INPUT].getVoltage() + params[RESET_PARAM].getValue() + params[RUN_PARAM].getValue() + inputs[RUNCV_INPUT].getVoltage());//inputs[RESET_INPUT].getVoltage());
		clkValue = 5.0f * oscillatorClk.sqr();	
		outputs[CLK_OUT_OUTPUT].setVoltage(clkValue);
		
		
		// VCA
		float vcaIn = inputs[VCA_IN1_INPUT].isConnected() ? inputs[VCA_IN1_INPUT].getVoltage() : outputs[VCO_SQR_OUTPUT].value;// Pre-patching
		float vcaLin = inputs[VCA_LIN1_INPUT].isConnected() ? inputs[VCA_LIN1_INPUT].getVoltage() : outputs[ADSR_ENVELOPE_OUTPUT].value;// Pre-patching
		float v = vcaIn * params[VCA_LEVEL1_PARAM].getValue();
		v *= clamp(vcaLin / 10.0f, 0.0f, 1.0f);
		outputs[VCA_OUT1_OUTPUT].setVoltage(v);

				
		// ADSR
		float attack = clamp(params[ADSR_ATTACK_PARAM].getValue(), 0.0f, 1.0f);
		float decay = clamp(params[ADSR_DECAY_PARAM].getValue(), 0.0f, 1.0f);
		float sustain = clamp(params[ADSR_SUSTAIN_PARAM].getValue(), 0.0f, 1.0f);
		float release = clamp(params[ADSR_RELEASE_PARAM].getValue(), 0.0f, 1.0f);
		// Gate
		float adsrIn = inputs[ADSR_GATE_INPUT].isConnected() ? inputs[ADSR_GATE_INPUT].getVoltage() : outputs[GATE1_OUTPUT].value;// Pre-patching
		bool gated = adsrIn >= 1.0f;
		const float base = 20000.0f;
		const float maxTime = 10.0f;
		if (gated) {
			if (decaying) {
				// Decay
				if (decay < 1e-4) {
					env = sustain;
				}
				else {
					env += powf(base, 1 - decay) / maxTime * (sustain - env) * args.sampleTime;
				}
			}
			else {
				// Attack
				// Skip ahead if attack is all the way down (infinitely fast)
				if (attack < 1e-4) {
					env = 1.0f;
				}
				else {
					env += powf(base, 1 - attack) / maxTime * (1.01f - env) * args.sampleTime;
				}
				if (env >= 1.0f) {
					env = 1.0f;
					decaying = true;
				}
			}
		}
		else {
			// Release
			if (release < 1e-4) {
				env = 0.0f;
			}
			else {
				env += powf(base, 1 - release) / maxTime * (0.0f - env) * args.sampleTime;
			}
			decaying = false;
		}
		outputs[ADSR_ENVELOPE_OUTPUT].setVoltage(10.0f * env);
		
		
		// VCF
		if (outputs[VCF_LPF_OUTPUT].isConnected() || outputs[VCF_HPF_OUTPUT].isConnected()) {
		
			float input = (inputs[VCF_IN_INPUT].isConnected() ? inputs[VCF_IN_INPUT].getVoltage() : outputs[VCA_OUT1_OUTPUT].value) / 5.0f;// Pre-patching
			float drive = clamp(params[VCF_DRIVE_PARAM].getValue() + inputs[VCF_DRIVE_INPUT].getVoltage() / 10.0f, 0.f, 1.f);
			float gain = powf(1.f + drive, 5);
			input *= gain;
			// Add -60dB noise to bootstrap self-oscillation
			input += 1e-6f * (2.f * random::uniform() - 1.f);
			// Set resonance
			float res = clamp(params[VCF_RES_PARAM].getValue() + inputs[VCF_RES_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			filter.resonance = powf(res, 2) * 10.f;
			// Set cutoff frequency
			float pitch = 0.f;
			if (inputs[VCF_FREQ_INPUT].isConnected())
				pitch += inputs[VCF_FREQ_INPUT].getVoltage() * dsp::quadraticBipolar(params[VCF_FREQ_CV_PARAM].getValue());
			pitch += params[VCF_FREQ_PARAM].getValue() * 10.f - 5.f;
			//pitch += dsp::quadraticBipolar(params[FINE_PARAM].getValue() * 2.f - 1.f) * 7.f / 12.f;
			float cutoff = 261.626f * powf(2.f, pitch);
			cutoff = clamp(cutoff, 1.f, 8000.f);
			filter.setCutoff(cutoff);
			filter.process(input, args.sampleTime);
			outputs[VCF_LPF_OUTPUT].setVoltage(5.f * filter.lowpass);
			outputs[VCF_HPF_OUTPUT].setVoltage(5.f * filter.highpass);	
		}			
		else {
			outputs[VCF_LPF_OUTPUT].setVoltage(0.0f);
			outputs[VCF_HPF_OUTPUT].setVoltage(0.0f);
		}
		
		// LFO
		if (outputs[LFO_SIN_OUTPUT].isConnected() || outputs[LFO_TRI_OUTPUT].isConnected()) {
			if ((lightRefreshCounter & userInputsStepSkipMask) == 0) {
				oscillatorLfo.setPitch(params[LFO_FREQ_PARAM].getValue());
			}
			oscillatorLfo.step(args.sampleTime);
			oscillatorLfo.setReset(inputs[LFO_RESET_INPUT].getVoltage() + inputs[RESET_INPUT].getVoltage() + params[RESET_PARAM].getValue() + params[RUN_PARAM].getValue() + inputs[RUNCV_INPUT].getVoltage());
			float lfoGain = params[LFO_GAIN_PARAM].getValue();
			float lfoOffset = (2.0f - lfoGain) * params[LFO_OFFSET_PARAM].getValue();
			outputs[LFO_SIN_OUTPUT].setVoltage(5.0f * (lfoOffset + lfoGain * oscillatorLfo.sin()));
			outputs[LFO_TRI_OUTPUT].setVoltage(5.0f * (lfoOffset + lfoGain * oscillatorLfo.tri()));	
		} 
		else {
			outputs[LFO_SIN_OUTPUT].setVoltage(0.0f);
			outputs[LFO_TRI_OUTPUT].setVoltage(0.0f);
		}			
		
	}// process()
	

	inline void setGreenRed(int id, float green, float red) {
		lights[id + 0].value = green;
		lights[id + 1].value = red;
	}
	
	inline void propagateCVtoTied(int seqn, int stepn) {
		for (int i = stepn + 1; i < 16; i++) {
			if (!attributes[seqn][i].getTied())
				break;
			cv[seqn][i] = cv[seqn][i - 1];
		}	
	}

	void activateTiedStep(int seqn, int stepn) {
		attributes[seqn][stepn].setTied(true);
		if (stepn > 0) 
			propagateCVtoTied(seqn, stepn - 1);
		
		if (holdTiedNotes) {// new method
			attributes[seqn][stepn].setGate1(true);
			for (int i = std::max(stepn, 1); i < 16 && attributes[seqn][i].getTied(); i++) {
				attributes[seqn][i].setGate1Mode(attributes[seqn][i - 1].getGate1Mode());
				attributes[seqn][i - 1].setGate1Mode(5);
				attributes[seqn][i - 1].setGate1(true);
			}
		}
		else {// old method
			if (stepn > 0) {
				attributes[seqn][stepn] = attributes[seqn][stepn - 1];
				attributes[seqn][stepn].setTied(true);
			}
		}
	}
	
	void deactivateTiedStep(int seqn, int stepn) {
		attributes[seqn][stepn].setTied(false);
		if (holdTiedNotes) {// new method
			int lastGateType = attributes[seqn][stepn].getGate1Mode();
			for (int i = stepn + 1; i < 16 && attributes[seqn][i].getTied(); i++)
				lastGateType = attributes[seqn][i].getGate1Mode();
			if (stepn > 0)
				attributes[seqn][stepn - 1].setGate1Mode(lastGateType);
		}
		//else old method, nothing to do
	}
	
	inline void setGateLight(bool gateOn, int lightIndex) {
		if (!gateOn) {
			lights[lightIndex + 0].value = 0.0f;
			lights[lightIndex + 1].value = 0.0f;
		}
		else if (editingGateLength == 0l) {
			lights[lightIndex + 0].value = 0.0f;
			lights[lightIndex + 1].value = 1.0f;
		}
		else {
			lights[lightIndex + 0].value = lightIndex == GATE1_LIGHT ? 1.0f : 0.2f;
			lights[lightIndex + 1].value = lightIndex == GATE1_LIGHT ? 0.2f : 1.0f;
		}
	}

};



struct SemiModularSynthWidget : ModuleWidget {
	SvgPanel* lightPanel;
	SvgPanel* darkPanel;

	struct SequenceDisplayWidget : TransparentWidget {
		SemiModularSynth *module;
		std::shared_ptr<Font> font;
		char displayStr[4];
		
		SequenceDisplayWidget() {
			font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}
		
		void runModeToStr(int num) {
			if (num >= 0 && num < (NUM_MODES - 1))
				snprintf(displayStr, 4, "%s", modeLabels[num].c_str());
		}

		void draw(const DrawArgs &args) override {
			NVGcolor textColor = prepareDisplay(args.vg, &box, 18);
			nvgFontFaceId(args.vg, font->handle);

			Vec textPos = Vec(6, 24);
			nvgFillColor(args.vg, nvgTransRGBA(textColor, displayAlpha));
			nvgText(args.vg, textPos.x, textPos.y, "~~~", NULL);
			nvgFillColor(args.vg, textColor);
			
			if (module == NULL) {
				snprintf(displayStr, 4, "  1");
			}
			else {
				bool editingSequence = module->isEditingSequence();
				if (module->infoCopyPaste != 0l) {
					if (module->infoCopyPaste > 0l)
						snprintf(displayStr, 4, "CPY");
					else {
						float cpMode = module->params[SemiModularSynth::CPMODE_PARAM].getValue();
						if (editingSequence && !module->seqCopied) {// cross paste to seq
							if (cpMode > 1.5f)// All = toggle gate 1
								snprintf(displayStr, 4, "TG1");
							else if (cpMode < 0.5f)// 4 = random CV
								snprintf(displayStr, 4, "RCV");
							else// 8 = random gate 1
								snprintf(displayStr, 4, "RG1");
						}
						else if (!editingSequence && module->seqCopied) {// cross paste to song
							if (cpMode > 1.5f)// All = init
								snprintf(displayStr, 4, "CLR");
							else if (cpMode < 0.5f)// 4 = increase by 1
								snprintf(displayStr, 4, "INC");
							else// 8 = random phrases
								snprintf(displayStr, 4, "RPH");
						}
						else
							snprintf(displayStr, 4, "PST");
					}
				}
				else if (module->editingPpqn != 0ul) {
					snprintf(displayStr, 4, "x%2u", (unsigned) module->pulsesPerStep);
				}
				else if (module->displayState == SemiModularSynth::DISP_MODE) {
					if (editingSequence)
						runModeToStr(module->sequences[module->seqIndexEdit].getRunMode());
					else
						runModeToStr(module->runModeSong);
				}
				else if (module->displayState == SemiModularSynth::DISP_LENGTH) {
					if (editingSequence)
						snprintf(displayStr, 4, "L%2u", (unsigned) module->sequences[module->seqIndexEdit].getLength());
					else
						snprintf(displayStr, 4, "L%2u", (unsigned) module->phrases);
				}
				else if (module->displayState == SemiModularSynth::DISP_TRANSPOSE) {
					snprintf(displayStr, 4, "+%2u", (unsigned) abs(module->sequences[module->seqIndexEdit].getTranspose()));
					if (module->sequences[module->seqIndexEdit].getTranspose() < 0)
						displayStr[0] = '-';
				}
				else if (module->displayState == SemiModularSynth::DISP_ROTATE) {
					snprintf(displayStr, 4, ")%2u", (unsigned) abs(module->sequences[module->seqIndexEdit].getRotate()));
					if (module->sequences[module->seqIndexEdit].getRotate() < 0)
						displayStr[0] = '(';
				}
				else {// DISP_NORMAL
					snprintf(displayStr, 4, " %2u", (unsigned) (editingSequence ? 
						module->seqIndexEdit : module->phrase[module->phraseIndexEdit]) + 1 );
				}
			}
			nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};		
	
	struct PanelThemeItem : MenuItem {
		SemiModularSynth *module;
		int panelTheme;
		void onAction(const widget::ActionEvent &e) override {
			module->panelTheme = panelTheme;
		}
		void step() override {
			rightText = (module->panelTheme == panelTheme) ? "✔" : "";
		}
	};
	struct ResetOnRunItem : MenuItem {
		SemiModularSynth *module;
		void onAction(const widget::ActionEvent &e) override {
			module->resetOnRun = !module->resetOnRun;
		}
	};
	struct AutoStepLenItem : MenuItem {
		SemiModularSynth *module;
		void onAction(const widget::ActionEvent &e) override {
			module->autostepLen = !module->autostepLen;
		}
	};
	struct AutoseqItem : MenuItem {
		SemiModularSynth *module;
		void onAction(const widget::ActionEvent &e) override {
			module->autoseq = !module->autoseq;
		}
	};
	struct HoldTiedItem : MenuItem {
		SemiModularSynth *module;
		void onAction(const widget::ActionEvent &e) override {
			module->holdTiedNotes = !module->holdTiedNotes;
		}
	};
	struct SeqCVmethodItem : MenuItem {
		SemiModularSynth *module;
		void onAction(const widget::ActionEvent &e) override {
			module->seqCVmethod++;
			if (module->seqCVmethod > 2)
				module->seqCVmethod = 0;
		}
		void step() override {
			if (module->seqCVmethod == 0)
				text = "Seq CV in: <0-10V>,  C4-D5#,  Trig-Incr";
			else if (module->seqCVmethod == 1)
				text = "Seq CV in: 0-10V,  <C4-D5#>,  Trig-Incr";
			else
				text = "Seq CV in: 0-10V,  C4-D5#,  <Trig-Incr>";
		}	
	};	void appendContextMenu(Menu *menu) override {
		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		SemiModularSynth *module = dynamic_cast<SemiModularSynth*>(this->module);
		assert(module);

		MenuLabel *themeLabel = new MenuLabel();
		themeLabel->text = "Panel Theme";
		menu->addChild(themeLabel);

		PanelThemeItem *classicItem = new PanelThemeItem();
		classicItem->text = lightPanelID;// ImpromptuModular.hpp
		classicItem->module = module;
		classicItem->panelTheme = 0;
		menu->addChild(classicItem);

		PanelThemeItem *darkItem = new PanelThemeItem();
		darkItem->text = darkPanelID;// ImpromptuModular.hpp
		darkItem->module = module;
		darkItem->panelTheme = 1;
		menu->addChild(darkItem);

		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Settings";
		menu->addChild(settingsLabel);
		
		ResetOnRunItem *rorItem = createMenuItem<ResetOnRunItem>("Reset on run", CHECKMARK(module->resetOnRun));
		rorItem->module = module;
		menu->addChild(rorItem);
		
		AutoStepLenItem *astlItem = createMenuItem<AutoStepLenItem>("AutoStep write bounded by seq length", CHECKMARK(module->autostepLen));
		astlItem->module = module;
		menu->addChild(astlItem);

		AutoseqItem *aseqItem = createMenuItem<AutoseqItem>("AutoSeq when writing via CV inputs", CHECKMARK(module->autoseq));
		aseqItem->module = module;
		menu->addChild(aseqItem);

		HoldTiedItem *holdItem = createMenuItem<HoldTiedItem>("Hold tied notes", CHECKMARK(module->holdTiedNotes));
		holdItem->module = module;
		menu->addChild(holdItem);

		SeqCVmethodItem *seqcvItem = createMenuItem<SeqCVmethodItem>("Seq CV in: ", "");
		seqcvItem->module = module;
		menu->addChild(seqcvItem);
	}	
	
	struct SequenceKnob : IMBigKnobInf {
		SequenceKnob() {};		
		void onDoubleClick(const widget::DoubleClickEvent &e) override {
			if (paramQuantity) {
				SemiModularSynth* module = dynamic_cast<SemiModularSynth*>(paramQuantity->module);
				// same code structure below as in sequence knob in main step()
				if (module->editingPpqn != 0) {
					module->pulsesPerStep = 1;
					//editingPpqn = (long) (editGateLengthTime * sampleRate / displayRefreshStepSkips);
				}
				else if (module->displayState == SemiModularSynth::DISP_MODE) {
					if (module->isEditingSequence()) {
						module->sequences[module->seqIndexEdit].setRunMode(MODE_FWD);
					}
					else {
						module->runModeSong = MODE_FWD;
					}
				}
				else if (module->displayState == SemiModularSynth::DISP_LENGTH) {
					if (module->isEditingSequence()) {
						module->sequences[module->seqIndexEdit].setLength(16);
					}
					else {
						module->phrases = 4;
					}
				}
				else if (module->displayState == SemiModularSynth::DISP_TRANSPOSE) {
					// nothing
				}
				else if (module->displayState == SemiModularSynth::DISP_ROTATE) {
					// nothing			
				}
				else {// DISP_NORMAL
					if (module->isEditingSequence()) {
						if (!module->inputs[SemiModularSynth::SEQCV_INPUT].isConnected()) {
							module->seqIndexEdit = 0;;
						}
					}
					else {
						module->phrase[module->phraseIndexEdit] = 0;
					}
				}
			}
			ParamWidget::onDoubleClick(e);
		}
	};		
	
	
	SemiModularSynthWidget(SemiModularSynth *module) {
		setModule(module);
		
		// Main panels from Inkscape
        lightPanel = new SvgPanel();
        lightPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/SemiModular.svg")));
        box.size = lightPanel->box.size;
        addChild(lightPanel);
        darkPanel = new SvgPanel();
		darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/SemiModular_dark.svg")));
		darkPanel->visible = false;
		addChild(darkPanel);

		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec((box.size.x - 90) * 1 / 3 + 30 , 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec((box.size.x - 90) * 1 / 3 + 30 , 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec((box.size.x - 90) * 2 / 3 + 45 , 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec((box.size.x - 90) * 2 / 3 + 45 , 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));


		// SEQUENCER 
		
		// ****** Top row ******
		
		static const int rowRulerT0 = 54;
		static const int columnRulerT0 = 22;// Step LED buttons
		static const int columnRulerT3 = 404;// Attach (also used to align rest of right side of module)

		// Step/Phrase LED buttons
		int posX = columnRulerT0;
		static int spacingSteps = 20;
		static int spacingSteps4 = 4;
		for (int x = 0; x < 16; x++) {
			// First row
			addParam(createParam<LEDButton>(Vec(posX, rowRulerT0 - 7 + 3 - 4.4f), module, SemiModularSynth::STEP_PHRASE_PARAMS + x));
			addChild(createLight<MediumLight<GreenRedWhiteLight>>(Vec(posX + 4.4f, rowRulerT0 - 7 + 3), module, SemiModularSynth::STEP_PHRASE_LIGHTS + (x * 3)));
			// step position to next location and handle groups of four
			posX += spacingSteps;
			if ((x + 1) % 4 == 0)
				posX += spacingSteps4;
		}
		// Attach button and light
		addParam(createDynamicParam<IMPushButton>(Vec(columnRulerT3 - 4, rowRulerT0 - 6 + 2 + offsetTL1105), module, SemiModularSynth::ATTACH_PARAM, module ? &module->panelTheme : NULL));
		addChild(createLight<MediumLight<RedLight>>(Vec(columnRulerT3 + 12 + offsetMediumLight, rowRulerT0 - 6 + offsetMediumLight), module, SemiModularSynth::ATTACH_LIGHT));


		// Key mode LED buttons	
		static const int rowRulerKM = rowRulerT0 + 26 - 2;
		static const int colRulerKM = columnRulerT0 + 113;
		addParam(createParam<LEDButton>(Vec(colRulerKM + 112, rowRulerKM - 4.4f), module, SemiModularSynth::KEYNOTE_PARAM));
		addChild(createLight<MediumLight<RedLight>>(Vec(colRulerKM + 112 + 4.4f,  rowRulerKM), module, SemiModularSynth::KEYNOTE_LIGHT));
		addParam(createParam<LEDButton>(Vec(colRulerKM, rowRulerKM - 4.4f), module, SemiModularSynth::KEYGATE_PARAM));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(colRulerKM + 4.4f, rowRulerKM), module, SemiModularSynth::KEYGATE_LIGHT));
		
		
		
		// ****** Octave and keyboard area ******
		
		// Octave LED buttons
		static const float octLightsIntY = 20.0f;
		for (int i = 0; i < 7; i++) {
			addParam(createParam<LEDButton>(Vec(19 + 3, 86 + 24 + i * octLightsIntY- 4.4f), module, SemiModularSynth::OCTAVE_PARAM + i));
			addChild(createLight<MediumLight<RedLight>>(Vec(19 + 3 + 4.4f, 86 + 24 + i * octLightsIntY), module, SemiModularSynth::OCTAVE_LIGHTS + i));
		}
		// Keys and Key lights
		static const int keyNudgeX = 7 + 4;
		static const int KeyBlackY = 103 + 4;
		static const int KeyWhiteY = 141 + 4;
		static const int offsetKeyLEDx = 6;
		static const int offsetKeyLEDy = 16;
		// Black keys and lights
		addParam(createParam<InvisibleKeySmall>(			Vec(65+keyNudgeX, KeyBlackY), module, SemiModularSynth::KEY_PARAMS + 1));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(65+keyNudgeX+offsetKeyLEDx, KeyBlackY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 1 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(93+keyNudgeX, KeyBlackY), module, SemiModularSynth::KEY_PARAMS + 3));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(93+keyNudgeX+offsetKeyLEDx, KeyBlackY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 3 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(150+keyNudgeX, KeyBlackY), module, SemiModularSynth::KEY_PARAMS + 6));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(150+keyNudgeX+offsetKeyLEDx, KeyBlackY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 6 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(178+keyNudgeX, KeyBlackY), module, SemiModularSynth::KEY_PARAMS + 8));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(178+keyNudgeX+offsetKeyLEDx, KeyBlackY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 8 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(206+keyNudgeX, KeyBlackY), module, SemiModularSynth::KEY_PARAMS + 10));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(206+keyNudgeX+offsetKeyLEDx, KeyBlackY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 10 * 2));
		// White keys and lights
		addParam(createParam<InvisibleKeySmall>(			Vec(51+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 0));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(51+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 0 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(79+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 2));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(79+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 2 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(107+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 4));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(107+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 4 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(136+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 5));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(136+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 5 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(164+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 7));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(164+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 7 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(192+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 9));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(192+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 9 * 2));
		addParam(createParam<InvisibleKeySmall>(			Vec(220+keyNudgeX, KeyWhiteY), module, SemiModularSynth::KEY_PARAMS + 11));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(220+keyNudgeX+offsetKeyLEDx, KeyWhiteY+offsetKeyLEDy), module, SemiModularSynth::KEY_LIGHTS + 11 * 2));		
		
		
		
		// ****** Right side control area ******

		static const int rowRulerMK0 = 105;// Edit mode row
		static const int rowRulerMK1 = rowRulerMK0 + 56; // Run row
		static const int rowRulerMK2 = rowRulerMK1 + 54; // Reset row
		static const int columnRulerMK0 = 280;// Edit mode column
		static const int columnRulerMK1 = columnRulerMK0 + 59;// Display column
		static const int columnRulerMK2 = columnRulerT3;// Run mode column
		
		// Edit mode switch
		addParam(createParam<CKSSNoRandom>(Vec(columnRulerMK0 + hOffsetCKSS, rowRulerMK0 + vOffsetCKSS), module, SemiModularSynth::EDIT_PARAM));
		// Sequence display
		SequenceDisplayWidget *displaySequence = new SequenceDisplayWidget();
		displaySequence->box.pos = Vec(columnRulerMK1-15, rowRulerMK0 + 3 + vOffsetDisplay);
		displaySequence->box.size = Vec(55, 30);// 3 characters
		displaySequence->module = module;
		addChild(displaySequence);
		// Len/mode button
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMK2 + offsetCKD6b, rowRulerMK0 + 0 + offsetCKD6b), module, SemiModularSynth::RUNMODE_PARAM, module ? &module->panelTheme : NULL));

		// Run LED bezel and light
		addParam(createParam<LEDBezel>(Vec(columnRulerMK0 + offsetLEDbezel, rowRulerMK1 + 7 + offsetLEDbezel), module, SemiModularSynth::RUN_PARAM));
		addChild(createLight<MuteLight<GreenLight>>(Vec(columnRulerMK0 + offsetLEDbezel + offsetLEDbezelLight, rowRulerMK1 + 7 + offsetLEDbezel + offsetLEDbezelLight), module, SemiModularSynth::RUN_LIGHT));
		// Sequence knob
		addParam(createDynamicParam<SequenceKnob>(Vec(columnRulerMK1 + 1 + offsetIMBigKnob, rowRulerMK0 + 55 + offsetIMBigKnob), module, SemiModularSynth::SEQUENCE_PARAM, module ? &module->panelTheme : NULL));		
		// Transpose/rotate button
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMK2 + offsetCKD6b, rowRulerMK1 + 4 + offsetCKD6b), module, SemiModularSynth::TRAN_ROT_PARAM, module ? &module->panelTheme : NULL));
		
		// Reset LED bezel and light
		addParam(createParam<LEDBezel>(Vec(columnRulerMK0 + offsetLEDbezel, rowRulerMK2 + 5 + offsetLEDbezel), module, SemiModularSynth::RESET_PARAM));
		addChild(createLight<MuteLight<GreenLight>>(Vec(columnRulerMK0 + offsetLEDbezel + offsetLEDbezelLight, rowRulerMK2 + 5 + offsetLEDbezel + offsetLEDbezelLight), module, SemiModularSynth::RESET_LIGHT));
		// Copy/paste buttons
		addParam(createDynamicParam<IMPushButton>(Vec(columnRulerMK1 - 10, rowRulerMK2 + 5 + offsetTL1105), module, SemiModularSynth::COPY_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMPushButton>(Vec(columnRulerMK1 + 20, rowRulerMK2 + 5 + offsetTL1105), module, SemiModularSynth::PASTE_PARAM, module ? &module->panelTheme : NULL));
		// Copy-paste mode switch (3 position)
		addParam(createParam<CKSSThreeInvNoRandom>(Vec(columnRulerMK2 + hOffsetCKSS + 1, rowRulerMK2 - 3 + vOffsetCKSSThree), module, SemiModularSynth::CPMODE_PARAM));	// 0.0f is top position

		
		
		// ****** Gate and slide section ******
		
		static const int rowRulerMB0 = 214 + 4;
		static const int columnRulerMBspacing = 70;
		static const int columnRulerMB2 = 134;// Gate2
		static const int columnRulerMB1 = columnRulerMB2 - columnRulerMBspacing;// Gate1 
		static const int columnRulerMB3 = columnRulerMB2 + columnRulerMBspacing;// Tie
		static const int posLEDvsButton = + 25;
		
		// Gate 1 light and button
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(columnRulerMB1 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, SemiModularSynth::GATE1_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB1 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, SemiModularSynth::GATE1_PARAM, module ? &module->panelTheme : NULL));
		// Gate 2 light and button
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(columnRulerMB2 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, SemiModularSynth::GATE2_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB2 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, SemiModularSynth::GATE2_PARAM, module ? &module->panelTheme : NULL));
		// Tie light and button
		addChild(createLight<MediumLight<RedLight>>(Vec(columnRulerMB3 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, SemiModularSynth::TIE_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB3 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, SemiModularSynth::TIE_PARAM, module ? &module->panelTheme : NULL));

						
		
		// ****** Bottom two rows ******
		
		static const int outputJackSpacingX = 54;
		static const int rowRulerB0 = 327;
		static const int rowRulerB1 = 273;
		static const int columnRulerB0 = 26;
		static const int columnRulerB1 = columnRulerB0 + outputJackSpacingX;
		static const int columnRulerB2 = columnRulerB1 + outputJackSpacingX;
		static const int columnRulerB3 = columnRulerB2 + outputJackSpacingX;
		static const int columnRulerB4 = columnRulerB3 + outputJackSpacingX;
		static const int columnRulerB7 = columnRulerMK2 + 1;
		static const int columnRulerB6 = columnRulerB7 - outputJackSpacingX;
		static const int columnRulerB5 = columnRulerB6 - outputJackSpacingX;

		
		// Gate 1 probability light and button
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(columnRulerB0 + posLEDvsButton + offsetMediumLight, rowRulerB1 + offsetMediumLight), module, SemiModularSynth::GATE1_PROB_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerB0 + offsetCKD6b, rowRulerB1 + offsetCKD6b), module, SemiModularSynth::GATE1_PROB_PARAM, module ? &module->panelTheme : NULL));
		// Gate 1 probability knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(columnRulerB1 + offsetIMSmallKnob, rowRulerB1 + offsetIMSmallKnob), module, SemiModularSynth::GATE1_KNOB_PARAM, module ? &module->panelTheme : NULL));
		// Slide light and button
		addChild(createLight<MediumLight<RedLight>>(Vec(columnRulerB2 + posLEDvsButton + offsetMediumLight, rowRulerB1 + offsetMediumLight), module, SemiModularSynth::SLIDE_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerB2 + offsetCKD6b, rowRulerB1 + offsetCKD6b), module, SemiModularSynth::SLIDE_BTN_PARAM, module ? &module->panelTheme : NULL));
		// Slide knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(columnRulerB3 + offsetIMSmallKnob, rowRulerB1 + offsetIMSmallKnob), module, SemiModularSynth::SLIDE_KNOB_PARAM, module ? &module->panelTheme : NULL));
		// Autostep
		addParam(createParam<CKSSNoRandom>(Vec(columnRulerB4 + hOffsetCKSS, rowRulerB1 + vOffsetCKSS), module, SemiModularSynth::AUTOSTEP_PARAM));		
		// CV in
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB5, rowRulerB1), true, module, SemiModularSynth::CV_INPUT, module ? &module->panelTheme : NULL));
		// Reset
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB6, rowRulerB1), true, module, SemiModularSynth::RESET_INPUT, module ? &module->panelTheme : NULL));
		// Clock
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB7, rowRulerB1), true, module, SemiModularSynth::CLOCK_INPUT, module ? &module->panelTheme : NULL));

		

		// ****** Bottom row (all aligned) ******

	
		// CV control Inputs 
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB0, rowRulerB0), true, module, SemiModularSynth::LEFTCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB1, rowRulerB0), true, module, SemiModularSynth::RIGHTCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB2, rowRulerB0), true, module, SemiModularSynth::SEQCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB3, rowRulerB0), true, module, SemiModularSynth::RUNCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB4, rowRulerB0), true, module, SemiModularSynth::WRITE_INPUT, module ? &module->panelTheme : NULL));
		// Outputs
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB5, rowRulerB0), false, module, SemiModularSynth::CV_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB6, rowRulerB0), false, module, SemiModularSynth::GATE1_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB7, rowRulerB0), false, module, SemiModularSynth::GATE2_OUTPUT, module ? &module->panelTheme : NULL));
		
		
		// VCO
		static const int rowRulerVCO0 = 66;// Freq
		static const int rowRulerVCO1 = rowRulerVCO0 + 40;// Fine, PW
		static const int rowRulerVCO2 = rowRulerVCO1 + 35;// FM, PWM, exact value from svg
		static const int rowRulerVCO3 = rowRulerVCO2 + 46;// switches (Don't change this, tweak the switches' v offset instead since jacks lines up with this)
		static const int rowRulerSpacingJacks = 35;// exact value from svg
		static const int rowRulerVCO4 = rowRulerVCO3 + rowRulerSpacingJacks;// jack row 1
		static const int rowRulerVCO5 = rowRulerVCO4 + rowRulerSpacingJacks;// jack row 2
		static const int rowRulerVCO6 = rowRulerVCO5 + rowRulerSpacingJacks;// jack row 3
		static const int rowRulerVCO7 = rowRulerVCO6 + rowRulerSpacingJacks;// jack row 4
		static const int colRulerVCO0 = 460;
		static const int colRulerVCO1 = colRulerVCO0 + 55;// exact value from svg

		addParam(createDynamicParam<IMBigKnob>(Vec(colRulerVCO0 + offsetIMBigKnob + 55 / 2, rowRulerVCO0 + offsetIMBigKnob), module, SemiModularSynth::VCO_FREQ_PARAM, module ? &module->panelTheme : NULL));
		
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCO0 + offsetIMSmallKnob, rowRulerVCO1 + offsetIMSmallKnob), module, SemiModularSynth::VCO_FINE_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCO1 + offsetIMSmallKnob, rowRulerVCO1 + offsetIMSmallKnob), module, SemiModularSynth::VCO_PW_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCO0 + offsetIMSmallKnob, rowRulerVCO2 + offsetIMSmallKnob), module, SemiModularSynth::VCO_FM_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCO1 + offsetIMSmallKnob, rowRulerVCO2 + offsetIMSmallKnob), module, SemiModularSynth::VCO_PWM_PARAM, module ? &module->panelTheme : NULL));

		addParam(createParam<CKSS>(Vec(colRulerVCO0 + hOffsetCKSS, rowRulerVCO3 + vOffsetCKSS), module, SemiModularSynth::VCO_MODE_PARAM));
		addParam(createDynamicParam<IMFivePosSmallKnob>(Vec(colRulerVCO1 + offsetIMSmallKnob, rowRulerVCO3 + offsetIMSmallKnob), module, SemiModularSynth::VCO_OCT_PARAM, module ? &module->panelTheme : NULL));

		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCO0, rowRulerVCO4), false, module, SemiModularSynth::VCO_SIN_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCO1, rowRulerVCO4), false, module, SemiModularSynth::VCO_TRI_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCO0, rowRulerVCO5), false, module, SemiModularSynth::VCO_SAW_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCO1, rowRulerVCO5), false, module, SemiModularSynth::VCO_SQR_OUTPUT, module ? &module->panelTheme : NULL));		
		
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCO0, rowRulerVCO6), true, module, SemiModularSynth::VCO_PITCH_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCO1, rowRulerVCO6), true, module, SemiModularSynth::VCO_FM_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCO0, rowRulerVCO7), true, module, SemiModularSynth::VCO_SYNC_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCO1, rowRulerVCO7), true, module, SemiModularSynth::VCO_PW_INPUT, module ? &module->panelTheme : NULL));

		
		// CLK
		static const int rowRulerClk0 = 41;
		static const int rowRulerClk1 = rowRulerClk0 + 45;// exact value from svg
		static const int rowRulerClk2 = rowRulerClk1 + 38;
		static const int colRulerClk0 = colRulerVCO1 + 55;// exact value from svg
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerClk0 + offsetIMSmallKnob, rowRulerClk0 + offsetIMSmallKnob), module, SemiModularSynth::CLK_FREQ_PARAM, module ? &module->panelTheme : NULL));// 120 BMP when default value
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerClk0 + offsetIMSmallKnob, rowRulerClk1 + offsetIMSmallKnob), module, SemiModularSynth::CLK_PW_PARAM, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerClk0, rowRulerClk2), false, module, SemiModularSynth::CLK_OUT_OUTPUT, module ? &module->panelTheme : NULL));
		
		
		// VCA
		static const int colRulerVca1 = colRulerClk0 + 55;// exact value from svg
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerClk0 + offsetIMSmallKnob, rowRulerVCO3 + offsetIMSmallKnob), module, SemiModularSynth::VCA_LEVEL1_PARAM, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerClk0, rowRulerVCO4), true, module, SemiModularSynth::VCA_LIN1_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerClk0, rowRulerVCO5), true, module, SemiModularSynth::VCA_IN1_INPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVca1, rowRulerVCO5), false, module, SemiModularSynth::VCA_OUT1_OUTPUT, module ? &module->panelTheme : NULL));
		
		
		// ADSR
		static const int rowRulerAdsr0 = rowRulerClk0;
		static const int rowRulerAdsr3 = rowRulerVCO2 + 6;
		static const int rowRulerAdsr1 = rowRulerAdsr0 + (rowRulerAdsr3 - rowRulerAdsr0) * 1 / 3;
		static const int rowRulerAdsr2 = rowRulerAdsr0 + (rowRulerAdsr3 - rowRulerAdsr0) * 2 / 3;
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVca1 + offsetIMSmallKnob, rowRulerAdsr0 + offsetIMSmallKnob), module, SemiModularSynth::ADSR_ATTACK_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVca1 + offsetIMSmallKnob, rowRulerAdsr1 + offsetIMSmallKnob), module, SemiModularSynth::ADSR_DECAY_PARAM,  module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVca1 + offsetIMSmallKnob, rowRulerAdsr2 + offsetIMSmallKnob), module, SemiModularSynth::ADSR_SUSTAIN_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVca1 + offsetIMSmallKnob, rowRulerAdsr3 + offsetIMSmallKnob), module, SemiModularSynth::ADSR_RELEASE_PARAM, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVca1, rowRulerVCO3), false, module, SemiModularSynth::ADSR_ENVELOPE_OUTPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVca1, rowRulerVCO4), true, module, SemiModularSynth::ADSR_GATE_INPUT, module ? &module->panelTheme : NULL));

		
		// VCF
		static const int colRulerVCF0 = colRulerVca1 + 55;// exact value from svg
		static const int colRulerVCF1 = colRulerVCF0 + 55;// exact value from svg
		addParam(createDynamicParam<IMBigKnob>(Vec(colRulerVCF0 + offsetIMBigKnob + 55 / 2, rowRulerVCO0 + offsetIMBigKnob), module, SemiModularSynth::VCF_FREQ_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCF0 + offsetIMSmallKnob + 55 / 2, rowRulerVCO1 + offsetIMSmallKnob), module, SemiModularSynth::VCF_RES_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCF0 + offsetIMSmallKnob , rowRulerVCO2 + offsetIMSmallKnob), module, SemiModularSynth::VCF_FREQ_CV_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerVCF1 + offsetIMSmallKnob , rowRulerVCO2 + offsetIMSmallKnob), module, SemiModularSynth::VCF_DRIVE_PARAM, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCF0, rowRulerVCO3), true, module, SemiModularSynth::VCF_FREQ_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCF1, rowRulerVCO3), true, module, SemiModularSynth::VCF_RES_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCF0, rowRulerVCO4), true, module, SemiModularSynth::VCF_DRIVE_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerVCF0, rowRulerVCO5), true, module, SemiModularSynth::VCF_IN_INPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCF1, rowRulerVCO4), false, module, SemiModularSynth::VCF_HPF_OUTPUT, module ? &module->panelTheme : NULL));		
		addOutput(createDynamicPort<IMPort>(Vec(colRulerVCF1, rowRulerVCO5), false, module, SemiModularSynth::VCF_LPF_OUTPUT, module ? &module->panelTheme : NULL));
		
		
		// LFO
		static const int colRulerLfo = colRulerVCF1 + 55;// exact value from svg
		static const int rowRulerLfo0 = rowRulerAdsr0;
		static const int rowRulerLfo2 = rowRulerVCO2;
		static const int rowRulerLfo1 = rowRulerLfo0 + (rowRulerLfo2 - rowRulerLfo0) / 2;
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerLfo + offsetIMSmallKnob, rowRulerLfo0 + offsetIMSmallKnob), module, SemiModularSynth::LFO_FREQ_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerLfo + offsetIMSmallKnob, rowRulerLfo1 + offsetIMSmallKnob), module, SemiModularSynth::LFO_GAIN_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerLfo + offsetIMSmallKnob, rowRulerLfo2 + offsetIMSmallKnob), module, SemiModularSynth::LFO_OFFSET_PARAM, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(colRulerLfo, rowRulerVCO3), false, module, SemiModularSynth::LFO_TRI_OUTPUT, module ? &module->panelTheme : NULL));		
		addOutput(createDynamicPort<IMPort>(Vec(colRulerLfo, rowRulerVCO4), false, module, SemiModularSynth::LFO_SIN_OUTPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerLfo, rowRulerVCO5), true, module, SemiModularSynth::LFO_RESET_INPUT, module ? &module->panelTheme : NULL));
	}
	
	void step() override {
		if (module) {
			lightPanel->visible = ((((SemiModularSynth*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((SemiModularSynth*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelSemiModularSynth = createModel<SemiModularSynth, SemiModularSynthWidget>("Semi-Modular Synth");

/*CHANGE LOG

1.0.0:
right-click keys to autostep replaced by double click

*/
