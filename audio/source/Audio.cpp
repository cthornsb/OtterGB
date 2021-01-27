#include <cmath>
#include <iostream>

#include "Audio.hpp"

typedef struct{
	float left;
	float right;
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/ 
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    paTestData *data = static_cast<paTestData*>(userData);
    float *out = static_cast<float*>(outputBuffer);
    for(unsigned int i=0; i<framesPerBuffer; i++ )
    {
        out[2*i] = data->left;  /* left */
        out[2*i+1] = data->right;  /* right */
        data->left += 0.01f;
        if( data->left >= 1.0f ) data->left -= 2.0f;
        data->right += 0.03f;
        if( data->right >= 1.0f ) data->right -= 2.0f;
    }
    return 0;
}

using namespace PianoKeys;
using namespace SimpleSynthesizers;

int main(){
	SoundProcessor proc;
	AudioData data(2, &proc);
	
	WavFile* wav = new WavFile("/home/kuras/Downloads/sample.wav");
	wav->print();
	data.replace(0, wav);
	/*SquareWave* swave = new SquareWave();
	SawtoothWave* swave2 = new SawtoothWave();
	swave->setFrequency(Key::C, 4);
	swave2->setFrequency(Key::E, 4);
	data.replace(0, swave);
	data.replace(1, swave2);*/
	
	proc.setAudioData(static_cast<void*>(&data));
	
	proc.init();
	proc.start();

	// Main loop
	proc.sleep(5000);

	proc.stop();
	
	return 0;
}
