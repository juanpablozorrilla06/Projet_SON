import("stdfaust.lib");

// ==========================
// CONFIGURATION
bufSize = 48000; // 1 seconde max → ~192 kB sur Teensy

// ==========================
// MIDI / Teensy parameters
note = nentry("note", 69, 0, 127, 1); // pitch exposé
gate = nentry("gate", 0, 0, 1, 1);    // gate exposé

ratio = pow(2, (note - 69)/12.0);

// ==========================
// CURSEURS
// writeIndex : où écrire le signal entrant dans le ruban
writePhase = os.phasor(1.0 / bufSize);
writeIndex = writePhase * bufSize;

// readIndex : où lire dans le ruban
// Multiplié par gate pour ne lire que lorsqu’une note est jouée
readPhase = gate * os.phasor(ratio / 1.0);  // vitesse MIDI contrôlée par note
readIndex = readPhase * bufSize;

// ==========================
// TABLE / RUBAN
// Le ruban stocke le signal micro + sample
ruban = rwtable(bufSize, 0.0);

// ==========================
// PROCESS
// _ = signal d'entrée (micro + sample via mixer)
// On écrit le signal dans le ruban et on lit en même temps
process = _, writeIndex, readIndex : ruban;
