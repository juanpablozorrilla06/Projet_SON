import("stdfaust.lib");

// --- Contrôles ---
freq = nentry("v:Sampler/freq", 440, 20, 20000, 0.01);  // Fréquence note MIDI
gate = button("v:Sampler/gate");                        // Gate note MIDI
vol = hslider("v:Sampler/Gain", 0.5, 0, 1, 0.01);       // Volume

// --- Table WAV (à remplir depuis le Teensy) ---
size = 131072; // Taille max du buffer pour 3 secondes à 44.1kHz
read_ptr = os.phasor(size, freq / 440.0 * gate) : int;
sampler = rwtable(size, 0.0, _, read_ptr);  // Table à remplir via SD

// --- Sortie stéréo ---
process = sampler : *(vol) <: _,_;  // Dupliquer mono vers gauche et droite
