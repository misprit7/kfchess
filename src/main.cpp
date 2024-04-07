#include "Arduino.h"

// There are two speedup options for some of the FFT code:

// Define this to use reciprocal multiplication for division and some more speedups that might decrease precision
//#define FFT_SPEED_OVER_PRECISION

// Define this to use a low-precision square root approximation instead of the regular sqrt() call
// This might only work for specific use cases, but is significantly faster. Only works for ArduinoFFT<float>.
//#define FFT_SQRT_APPROXIMATION

#include "arduinoFFT.h"
#include "core_pins.h"

/*
These values can be changed in order to evaluate the functions
*/
const uint8_t input_pin = A9;
const uint16_t n_samples = 64; //This value MUST ALWAYS be a power of 2
const float samplingFrequency = 5000; //Hz, must be less than 10000 due to ADC
unsigned int sampling_period_us;
unsigned long t_us;

/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
float vReal[n_samples];
float vImag[n_samples];

float freq[n_samples/2];

/* Create FFT object with weighing factor storage */
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, n_samples, samplingFrequency, true);

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

const double freq_output = 1000;
long t_output = 0;
uint8_t output_state;
const int output_pin = 22;


long t_input = 0;
int input_idx = 0;

long t_last_print = 0;

const int lift_pin = 0;

void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        float abscissa;
        /* Print abscissa value */
        switch (scaleType)
        {
            case SCL_INDEX:
                abscissa = (i * 1.0);
        break;
            case SCL_TIME:
                abscissa = ((i * 1.0) / samplingFrequency);
        break;
            case SCL_FREQUENCY:
                abscissa = ((i * 1.0 * samplingFrequency) / n_samples);
        break;
        }
        Serial.print(abscissa, 6);
        if(scaleType==SCL_FREQUENCY)
            Serial.print("Hz");
        Serial.print(" ");
        Serial.println(vData[i], 4);
    }
    Serial.println();
}

long t_start = 0;

void setup()
{
    pinMode(22, OUTPUT);
    sampling_period_us = round(1000000*(1.0/samplingFrequency));
    Serial.begin(115200);
    /* while (!Serial); */ 
    Serial.println("Ready");
    pinMode(lift_pin, OUTPUT);
    digitalWrite(lift_pin, 0);
    t_input = micros();
    t_output = micros();
    t_last_print = micros();
    t_start = micros();
}

void loop()
{
    /* Serial.println(analogRead(CHANNEL)); */
    /* delay(10); */
    /* return; */
    /*SAMPLING*/

    /* if(micros() > t_start + 1e6){ */
    /*     digitalWrite(lift_pin, 0); */
    /* } */
    /* return; */

    if(micros() - t_output > 1e6/freq_output/2){
        output_state = !output_state;
        digitalWrite(output_pin, output_state);
        t_output += 1e6/freq_output/2;
    }

    if(micros() - t_input > sampling_period_us){
        vReal[input_idx] = analogRead(input_pin);
        vImag[input_idx] = 0;

        t_input += sampling_period_us;
        input_idx += 1;

        if(input_idx >= n_samples){
            input_idx = 0;

            if(micros() - t_last_print > 100000){
                /* Print the results of the sampling according to time */
                /* Serial.println("Data:"); */
                /* PrintVector(vReal, n_samples, SCL_TIME); */
                FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
                FFT.dcRemoval();
                /* Serial.println("Weighed data:"); */
                /* PrintVector(vReal, samples, SCL_TIME); */
                FFT.compute(FFTDirection::Forward); /* Compute FFT */
                /* Serial.println("Computed Real values:"); */
                /* PrintVector(vReal, samples, SCL_INDEX); */
                /* Serial.println("Computed Imaginary values:"); */
                /* PrintVector(vImag, samples, SCL_INDEX); */
                FFT.complexToMagnitude(); /* Compute magnitudes */
                /* Serial.println("Computed magnitudes:"); */
                /* PrintVector(vReal, (n_samples >> 1), SCL_FREQUENCY); */
                /* float x = FFT.majorPeak(); */
                /* Serial.println(x, 6); //Print out what frequency is the most dominant. */

                float max = 0;
                int maxi = 0;
                for(int i = 5; i < n_samples/2; ++i){
                    if(vReal[i] > max){
                        maxi = i;
                        max = vReal[i];
                    }
                }
                float df = samplingFrequency / n_samples;
                if(vReal[maxi] > 2000){
                    Serial.print("Touching: ");
                    digitalWrite(lift_pin, 1);
                } else {
                    Serial.print("Not touching:");
                    digitalWrite(lift_pin, 0);
                }
                Serial.print(maxi * df, 6);
                Serial.print(", ");
                Serial.println(vReal[maxi]);
                t_last_print = micros();
            }
        }
    }


    /* delay(100); */
}