//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#include <jni.h>
#include "ece420_main.h"
#include "ece420_lib.h"
#include "kiss_fft/kiss_fft.h"

// Declare JNI function
extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getFftBuffer(JNIEnv *env, jclass, jobject bufferPtr);
}

// FRAME_SIZE is 1024 and we zero-pad it to 2048 to do FFT
#define FRAME_SIZE 1024
#define ZP_FACTOR 2
#define FFT_SIZE (FRAME_SIZE * ZP_FACTOR)
// Variable to store final FFT output
float fftOut[FFT_SIZE] = {};
bool isWritingFft = false;

void ece420ProcessFrame(sample_buf *dataBuf) {
    isWritingFft = false;

    // Keep in mind, we only have 20ms to process each buffer!
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    // Data is encoded in signed PCM-16, little-endian, mono channel
    float bufferIn[FRAME_SIZE];
    for (int i = 0; i < FRAME_SIZE; i++) {
        int16_t val = ((uint16_t) dataBuf->buf_[2 * i]) | (((uint16_t) dataBuf->buf_[2 * i + 1]) << 8);
        bufferIn[i] = (float) val;
    }

    // Spectrogram is just a fancy word for short time fourier transform
    // 1. Apply hamming window to the entire FRAME_SIZE
    // 2. Zero padding to FFT_SIZE = FRAME_SIZE * ZP_FACTOR
    // 3. Apply fft with KISS_FFT engine
    // 4. Scale fftOut[] to between 0 and 1 with log() and linear scaling
    // NOTE: This code block is a suggestion to get you started. You will have to
    // add/change code outside this block to implement FFT buffer overlapping (extra credit part).
    // Keep all of your code changes within java/MainActivity and cpp/ece420_*
    // ********************* START YOUR CODE HERE *********************** //

    // Create new buffer of FFT_SIZE and initialize to zero to get the padding
    kiss_fft_cpx fft_buffer_in[FFT_SIZE] = {};
    // Create new buffer for complex fft output
    kiss_fft_cpx fft_buffer_out[FFT_SIZE] = {};

    // Apply hamming window to input
    for(int n = 0; n < FRAME_SIZE; n++){
        fft_buffer_in[n].r = bufferIn[n]*(0.54 - 0.46 * cos((2*M_PI*n)/(FRAME_SIZE-1)));
    }

    // Apply FFT
    kiss_fft_cfg cfg = kiss_fft_alloc( FFT_SIZE , 0,NULL,NULL );
    kiss_fft(cfg, fft_buffer_in, fft_buffer_out);

    // Take the absolute value to get magnitude and square it
    for (int i = 0; i < FFT_SIZE; i++){
        float real = fft_buffer_out[i].r;
        float imag = fft_buffer_out[i].i;
        fftOut[i] = real*real+imag*imag; // magnitude would be sqrt of this so mag^2 is this
    }

    // scale logarithmically
    for (int i = 0; i < FFT_SIZE; i++){
        fftOut[i] = 10* log(fftOut[i]);
    }

    // Normalize
    float max = 0;
    for (int i = 0; i < FFT_SIZE; i++){
        if (fftOut[i] > max)
            max = fftOut[i];
    }
    for (int i = 0; i < FFT_SIZE; i++){
        fftOut[i] = fftOut[i] / max;
    }

    // thread-safe
    isWritingFft = true;
    // Currently set everything to 0 or 1 so the spectrogram will just be blue and red stripped
//    for (int i = 0; i < FRAME_SIZE; i++) {
//        fftOut[i] = 0;//(i/20)%2;
//    }

    // ********************* END YOUR CODE HERE ************************* //
    // Flip the flag so that the JNI thread will update the buffer
    isWritingFft = false;

    gettimeofday(&end, NULL);
    LOGD("Time delay: %ld us",  ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
}


// http://stackoverflow.com/questions/34168791/ndk-work-with-floatbuffer-as-parameter
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getFftBuffer(JNIEnv *env, jclass, jobject bufferPtr) {
    jfloat *buffer = (jfloat *) env->GetDirectBufferAddress(bufferPtr);
    // thread-safe, kinda
    while (isWritingFft) {}
    // We will only fetch up to FRAME_SIZE data in fftOut[] to draw on to the screen
    for (int i = 0; i < FRAME_SIZE; i++) {
        buffer[i] = fftOut[i];
    }
}
