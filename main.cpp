#include <cmath>
#include <fstream>
#include <iostream>
#include <string.h>
using namespace std;

void writeint(std::ostream& fout, unsigned int num) {
    /*Takes a reference to the file stream as well as an integer
    and writes the integer to the file in little endian form*/
    char integer[4] = {};
    integer[3] = (unsigned char) (((unsigned int)(num & 0xFF000000)) >> 24);
    integer[2] = (unsigned char) (((unsigned int)(num & 0x00FF0000)) >> 16);
    integer[1] = (unsigned char) (((unsigned int)(num & 0x0000FF00)) >> 8);
    integer[0] = (unsigned char) (((unsigned int)(num & 0x000000FF)));
    fout.write(integer, 4);
}

void writeshort(std::ostream& fout, unsigned short num) {
    /*Takes a reference to the file stream as well as a short
    and writes the short to the file in little endian form*/
    char shortnum[2] = {};
    shortnum[1] = (unsigned char) (((unsigned short)(num & 0xFF00)) >> 8);
    shortnum[0] = (unsigned char) (((unsigned short)(num & 0x00FF)));
    fout.write(shortnum, 2);
}

void makeHeader(std::ostream& fout, int sampleRate, short noChannels, short subChunkSize1) {
    /*Takes a reference to the file stream as sampleRate, number of channels and
    size of subchunk1. Writes the header for the .wav file and a placeholder for subchunk1 
    which is contained in bytes 4-8 and will be overwritten later, header format taken from
    http://soundfile.sapp.org/doc/WaveFormat/ where the commented numbers behind the const are
    the bytes allocated in the file*/
    const char riff[10] = "RIFF"; // 1-4
    unsigned int chunkSize = 40; // 4-8
    const char wave[10] = "WAVEfmt "; // 8-16
    const unsigned short audioFormat = 1; // 20-22
    const unsigned int byteRate = sampleRate * noChannels * subChunkSize1/8; // 28-32
    const short blockAlign = noChannels * subChunkSize1/8; // 32-34
    const short bitsPerSample = 16; // 34-36
    const char data[10] = "data"; // 36-40

    fout.write(riff, 4);
    writeint(fout, chunkSize);
    fout.write(wave, 8);
    writeint(fout, subChunkSize1);
    writeshort(fout, audioFormat);
    writeshort(fout, noChannels);
    writeint(fout, sampleRate);
    writeint(fout, byteRate);
    writeshort(fout, blockAlign);
    writeshort(fout, bitsPerSample);
    fout.write(data, 4);
}

float frequencies(char in) {
    /* Takes in char for key and returns the float value of that keys 0th 
    octave, frequency values taken from amendments*/
    switch (in){
        case 'a':
            return 440/2;
        case 'A':
            return 466/2;
        case 'b':
            return 494/2;
        case 'c':
            return 523/2;
        case 'C':
            return 554/2;
        case 'd':
            return 587/2;
        case 'D':
            return 622/2;
        case 'e':
            return 659/2;
        case 'f':
            return 698/2;
        case 'F':
            return 740/2;
        case 'g':
            return 784/2;
        case 'G':
            return 831/2;
        default:
            return 0;
    }
}

int main(int argc, char* argv[]) {
    const unsigned int subChunkSize1 = 16; // 16-20
    const short numChannels = 1; // 22-24
    const unsigned int sampleRate = 44100; // 24-28
    const short blockAlign = numChannels * subChunkSize1/8; // 32-34
    
    ofstream fout;
    ifstream fin;
    fin.open(argv[1], ios::in);
    if (!fin) {cerr << "FILE NOT FOUND!\n";} // if fin is empty the file is not correct
    char name[20], bpm[4], key[5], oct[2], num[3], denum[3]; 
    fin >> name >> bpm;

    fout.open(strcat(name,".wav"), ios::binary);
    makeHeader(fout, sampleRate, numChannels, subChunkSize1);

    float spb = 60/atof(bpm); // seconds per beat
    float note_dur; // Duration of note
    float dur = 0; // Counts the duration of the file
    int freq;

    fout.seekp(44, ios::beg); // goes to the 44th byte where the sound data begins

    while (fin) {
        fin >> key;
        if (!fin) {break;}
        
        if (key[0] == 's') {
            fin >> num >> denum;
            note_dur = (atof(num)/atof(denum) * spb * 4); //spb * 4 = length of whole note in s (4 beats) num/denum is number of notes
            dur += note_dur;
            for (int i = 0; i < (note_dur * sampleRate); i++) {
                /* Silence (freq = 0) is written for the allocated duration*/
                short sample;
                sample = 0;
                writeshort(fout, sample);
            }

        } else {
            fin >> oct >> num >> denum;

            freq = frequencies(key[0]) * pow(2, atoi(oct)); // frequency of key in the given octave, works beyond 5 octaves
            note_dur = (atof(num)/atof(denum) * spb * 4); //spb * 4 = length of whole note in s (4 beats) num/denum is number of notes
            dur += note_dur; 

            for (int i = 0; i < (note_dur * sampleRate); i++) {
                /* note is written for the allocated duration*/
                short sample;
                sample = cos(freq * i * 3.142/sampleRate) * 32767;
                writeshort(fout, sample);
            }
        }
    }

    int samples = dur * sampleRate;
    fout.seekp(40, ios::beg);
    int subChunkSize2 = samples * numChannels * subChunkSize1/8; // 40-44, size of data chunk
    writeint(fout, subChunkSize2);

    fout.seekp(4, ios::beg);
    unsigned int chunkSize = 36 + subChunkSize2; // size of file 4-8
    writeint(fout, chunkSize); // Overwrites the default (40) size of the file

    fout.close();

    return 0;
}