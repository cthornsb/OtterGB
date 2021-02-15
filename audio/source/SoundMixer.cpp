#include <algorithm>

#include "SoundMixer.hpp"

void SoundMixer::getCurrentSample(float& l, float& r) {
	l = fOutputSamples[0];
	r = fOutputSamples[1];
}

void SoundMixer::setOutputLevels(const float& l, const float& r){
	fOutputVolume[0] = clamp(l, 0.f, 1.f); // left
	fOutputVolume[1] = clamp(r, 0.f, 1.f); // right
}

void SoundMixer::setBalance(const float& bal){
	if(bal <= 0.f){ // Left balance
		fOutputVolume[0] = 1.f;
		fOutputVolume[1] = (bal + 1.f);
	}
	else{ // Right balance
		fOutputVolume[0] = (1.f - bal);
		fOutputVolume[1] = 1.f;
	}
}

bool SoundMixer::update(){
	if(bMuted){
		fOutputSamples[0] = 0.f;
		fOutputSamples[1] = 0.f;
		return false;
	}
	for(int i = 0; i < 2; i++){ // Over left and right output channels
		fOutputSamples[i] = 0.f;
		for(int j = 0; j < 4; j++){ // Over input channels
			fOutputSamples[i] += fInputVolume[j] * (bSendInputToOutput[i][j] ? fInputSamples[j] : 0.f);
		}
		// Normalize audio output and apply master and channel volumes
		fOutputSamples[i] = ((1.f + fOffsetDC) * fMasterVolume * fOutputVolume[i] * (fOutputSamples[i] / 4.f)) - fOffsetDC; // Translate to range [-DC, 1]
	}
	if(!bStereoOutput){ // Re-mix for mono output
		fOutputSamples[0] = (fOutputSamples[0] + fOutputSamples[1]) / 2.f;
		fOutputSamples[1] = fOutputSamples[0];
	}
	bModified = false;
	return true;
}

float SoundMixer::clamp(const float& input, const float& low, const float& high) const {
	return std::max(low, std::min(high, input));
}

void SoundMixer::rollover(){
	reload(); // Refill timer period
	if(bModified) // Update output samples if one or more input samples were modified
		update();
	pushSample(fOutputSamples[0], fOutputSamples[1]); // Push current sample onto the fifo buffer (mutex protected)
}

