#include <Audio.h>
#include <SD.h>
#include <Bounce.h>
#include "sampler.h" // Ton fichier Faust généré

// ================= FAUST =================
sampler faust; 

// ================= AUDIO =================
AudioInputI2S         micInput;       // Entrée Micro Shield
AudioRecordQueue      recorder;      // Pour l'enregistrement SD
AudioOutputI2S        audioOutput;   // Sortie Casque Shield
AudioMixer4           mixer1;

// Connexions
AudioConnection patchCord1(micInput, 0, mixer1, 0);   // Micro vers Mixer
AudioConnection patchCord2(mixer1, 0, faust, 0);      // Mixer vers Faust
AudioConnection patchCord3(faust, 0, audioOutput, 0); // Faust vers Sortie L
AudioConnection patchCord4(faust, 0, audioOutput, 1); // Faust vers Sortie R

AudioControlSGTL5000 audioShield;

// ================= BOUTON =================
const int buttonPin = 0; 
Bounce button = Bounce(buttonPin, 15); // 15ms debounce

// ================= SD / WAV =================
File wavFile;
const int chipSelect = 10;
bool isRecording = false;

// Header WAV standard
void writeWavHeader(File &file) {
  file.seek(0);
  file.write("RIFF", 4);
  uint32_t chunkSize = 36;
  file.write((uint8_t*)&chunkSize, 4);
  file.write("WAVE", 4);
  file.write("fmt ", 4);
  uint32_t subchunk1Size = 16;
  file.write((uint8_t*)&subchunk1Size, 4);
  uint16_t audioFormat = 1;
  uint16_t channels = 1;
  uint32_t sampleRate = 44100;
  uint16_t bitsPerSample = 16;
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  file.write((uint16_t*)&audioFormat, 2);
  file.write((uint16_t*)&channels, 2);
  file.write((uint32_t*)&sampleRate, 4);
  file.write((uint32_t*)&byteRate, 4);
  file.write((uint16_t*)&blockAlign, 2);
  file.write((uint16_t*)&bitsPerSample, 2);
  file.write("data", 4);
  uint32_t dataSize = 0;
  file.write((uint8_t*)&dataSize, 4);
}

void finalizeWavFile(File &file) {
  uint32_t fileSize = file.size();
  uint32_t dataChunkSize = fileSize - 44;
  file.seek(4);
  uint32_t chunkSize = fileSize - 8;
  file.write((uint8_t*)&chunkSize, 4);
  file.seek(40);
  file.write((uint8_t*)&dataChunkSize, 4);
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  
  // Si ton bouton est branché entre PIN 0 et GND
  pinMode(buttonPin, INPUT_PULLUP); 

  AudioMemory(140);
  audioShield.enable();
  audioShield.volume(0.5);
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(40);

  // CRITIQUE : Activer le flux dans le mixer
  mixer1.gain(0, 1.0); 

  if (!SD.begin(chipSelect)) {
    Serial.println("Erreur SD !");
  }

  usbMIDI.begin();

  // Init Faust
  faust.setParamValue("note", 69.0f);
  faust.setParamValue("gate", 0.0f);
}

// ================= LOOP =================
void loop() {
  button.update();

  // GESTION ENREGISTREMENT SD
  if (button.fallingEdge()) {
    Serial.println("Recording...");
    SD.remove("REC1.WAV");
    wavFile = SD.open("REC1.WAV", FILE_WRITE);
    if (wavFile) {
      writeWavHeader(wavFile);
      isRecording = true;
      recorder.begin();
    }
  }

  if (isRecording) {
    if (recorder.available() > 0) {
      int16_t* buffer = recorder.readBuffer();
      wavFile.write((uint8_t*)buffer, 256);
      recorder.freeBuffer();
    }
    
    if (button.risingEdge()) {
      finalizeWavFile(wavFile);
      wavFile.close();
      isRecording = false;
      recorder.end();
      Serial.println("Enregistrement SD termine.");
    }
  }

  // GESTION MIDI (VMPK)
  while (usbMIDI.read()) {
    byte type = usbMIDI.getType();
    byte data1 = usbMIDI.getData1();
    byte data2 = usbMIDI.getData2();

    if (type == usbMIDI.NoteOn && data2 > 0) {
      faust.setParamValue("note", (float)data1);
      faust.setParamValue("gate", 1.0f);
      Serial.print("Note On: "); Serial.println(data1);
    } 
    else if (type == usbMIDI.NoteOff || (type == usbMIDI.NoteOn && data2 == 0)) {
      faust.setParamValue("gate", 0.0f);
      Serial.println("Note Off");
    }
  }
}
