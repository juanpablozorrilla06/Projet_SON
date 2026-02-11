import("stdfaust.lib");

// --- Contrôles ---
// Le bouton physique sera lié à "Record"
rec = checkbox("v:Sampler/Record"); 
// Le potentiomètre sera lié à "Gain"
vol = hslider("v:Sampler/Gain", 0.5, 0, 1, 0.01);

// Paramètres MIDI (automatiquement mappés par la Teensy)
freq = nentry("v:Sampler/freq", 440, 20, 20000, 0.01);
gate = button("v:Sampler/gate");

// --- Mémoire (RAM) ---
// 131072 samples = ~3 secondes. La Teensy 4.1 a assez de RAM pour cela.
size = 131072; 

// --- Logique ---
// 1. Enregistrement (repart à 0 quand on appuie sur le bouton)
write_ptr = (ba.countup(size, rec)) : int;

// 2. Lecture (Vitesse variable selon la note)
// On définit le LA (440Hz) comme note originale du son enregistré
ratio = freq / 440.0;
read_ptr = os.phasor(size, ratio * (gate > 0)) : int;

// 3. Table de données
// L'entrée micro (input) est stockée ici
sampler = rwtable(size, 0.0, write_ptr, _, read_ptr);

// 4. Sortie avec lissage pour éviter les clics
process = _ : sampler : *(gate : si.smoo) : *(vol) : _ ;
