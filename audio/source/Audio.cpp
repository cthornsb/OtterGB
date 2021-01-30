#include <cmath>
#include <iostream>

#include "SoundManager.hpp"
#include "SimpleSynthesizers.hpp"
#include "WavFile.hpp"

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
	SoundManager proc(3);
	
	WavFile* wav = new WavFile("/home/kuras/Downloads/sample.wav");
	wav->print();
	proc[1].replaceInput(0, wav);
	SquareWave* swave = new SquareWave();
	SawtoothWave* swave2 = new SawtoothWave();
	TriangleWave* swave3 = new TriangleWave();
	swave->setFrequency(Key::C, 4);
	swave2->setFrequency(Key::E, 4);
	swave3->setFrequency(Key::G, 4);
	proc[0].replaceInput(0, swave);
	proc[0].replaceInput(1, swave2);
	proc[0].replaceInput(2, swave3);
	
	proc.init();
	proc.start();

	// Main loop
	proc.sleep(5000);

	proc.stop();
	
	return 0;
}
