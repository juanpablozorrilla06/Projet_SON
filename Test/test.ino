#include <Audio.h>
#include <SD.h>
#include <Bounce.h>
#include "sampler.h"   // ton Faust généré

// ================= FAUST =================
sampler faust;             // objet Faust pour traitement du ruban

// ================= AUDIO =================
AudioInputI2S        micInput;       // entrée micro
AudioRecordQueue     recorder;       // enregistrement
AudioPlayMemory      playMem;        // lecture en mémoire
AudioOutputI2S       audioOutput;    // sortie stéréo
AudioMixer4          mixer1;         // mélange micro + sample

// Connexions audio
AudioConnection patchCord1(micInput, 0, mixer1, 0);    // micro -> mixer
AudioConnection patchCord2(playMem, 0, mixer1, 1);     // sample -> mixer
AudioConnection patchCord3(mixer1, 0, faust, 0);       // mixer -> Faust
AudioConnection patchCord4(faust, 0, audioOutput, 0);  // Faust -> sortie L
AudioConnection patchCord5(faust, 0, audioOutput, 1);  // Faust -> sortie R

AudioControlSGTL5000 audioShield;

// ================= BOUTON =================
const int buttonPin = 0;
Bounce button(buttonPin, 10);

// ================= SD / WAV =================
File wavFile;
const int chipSelect = 10;
bool isRecording = false;
int16_t* sampleBuffer = nullptr;
unsigned int* playBuffer = nullptr;   // buffer pour AudioPlayMemory
uint32_t sampleLength = 0;

// ================= HEADER WAV =================
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

// ================= LOAD WAV INTO MEMORY =================
bool loadWavIntoMemory(const char* filename) {
    if (sampleBuffer) free(sampleBuffer);
    if (playBuffer) free(playBuffer);

    File f = SD.open(filename);
    if (!f) return false;

    f.seek(44); // skip WAV header
    sampleLength = (f.size() - 44) / 2; // nombre d'échantillons 16-bit

    sampleBuffer = (int16_t*)malloc(sampleLength * sizeof(int16_t));
    playBuffer   = (unsigned int*)malloc(sampleLength * sizeof(unsigned int));

    if (!sampleBuffer || !playBuffer) {
        f.close();
        return false;
    }

    f.read((uint8_t*)sampleBuffer, sampleLength * 2);
    f.close();

    // conversion int16 -> unsigned int 16.16 (Teensy Audio)
    for (uint32_t i = 0; i < sampleLength; i++) {
        playBuffer[i] = (unsigned int)((int32_t)sampleBuffer[i] << 16);
    }

    return true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);

  AudioMemory(120);
  audioShield.enable();
  audioShield.volume(0.6);
  audioShield.inputSelect(AUDIO_INPUT_MIC);
  audioShield.micGain(30);

  recorder.begin();

  if (!SD.begin(chipSelect)) {
    Serial.println("Erreur SD !");
  } else {
    Serial.println("SD OK");
  }

  usbMIDI.begin(); // USB MIDI natif

  // ==== INIT FAUST PARAMETERS ====
  faust.setParamValue("note", 69.0f);
  faust.setParamValue("gate", 0.0f);
}

// ================= LOOP =================
void loop() {
  button.update();

  // ===== START RECORD =====
  if (button.read() == HIGH && !isRecording) {
    Serial.println("Recording...");
    SD.remove("REC1.WAV");
    wavFile = SD.open("REC1.WAV", FILE_WRITE);
    if (wavFile) {
      writeWavHeader(wavFile);
      isRecording = true;
    } else {
      Serial.println("Erreur ouverture fichier !");
    }
  }

  // ===== WRITE AUDIO =====
  if (isRecording && recorder.available() > 0) {
    int16_t* buffer = recorder.readBuffer();
    wavFile.write((uint8_t*)buffer, 256);
    recorder.freeBuffer();
  }

  // ===== STOP RECORD =====
  if (button.read() == LOW && isRecording) {
    finalizeWavFile(wavFile);
    wavFile.close();
    isRecording = false;
    Serial.println("Stopped.");

    if (loadWavIntoMemory("REC1.WAV")) {
      Serial.println("Sample chargé en mémoire !");
    } else {
      Serial.println("Erreur chargement sample !");
    }
  }

  // ===== USB MIDI =====
  while (usbMIDI.read()) {
    byte type = usbMIDI.getType();
    byte data1 = usbMIDI.getData1();
    byte data2 = usbMIDI.getData2();

    if (type == usbMIDI.NoteOn && data2 > 0) {
      Serial.print("NoteOn: "); Serial.print(data1);
      Serial.print(" velocity: "); Serial.println(data2);

      // joue le sample en mémoire
      if (playBuffer) {
        playMem.play(playBuffer); // lance le sample
      }

      // contrôle du ruban Faust
      faust.setParamValue("note", (float)data1);       // vitesse / pitch
      faust.setParamValue("gate", 1.0f);               // active la lecture
    }
    else if (type == usbMIDI.NoteOff || (type == usbMIDI.NoteOn && data2 == 0)) {
      playMem.stop();
      faust.setParamValue("gate", 0.0f);               // désactive la lecture
    }
  }
}
