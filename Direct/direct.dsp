import("stdfaust.lib");

// Paramètres
bufSize = 48000; 
note = nentry("note", 69, 0, 127, 1);
gate = nentry("gate", 0, 0, 1, 1) : si.smoo;

// Ratio de vitesse
ratio = pow(2.0, (note - 69.0) / 12.0);

// Compteurs manuels (plus robustes pour l'export)
writeIndex = (+(1) : %(bufSize)) ~ _;
readIndex = (+(ratio) : fmod(_, float(bufSize))) ~ _;

// Table de lecture/écriture
// On utilise 'itbl' pour une interpolation linéaire simple et robuste
process = rwtable(bufSize, 0.0, int(writeIndex), _, int(readIndex)) * gate;
