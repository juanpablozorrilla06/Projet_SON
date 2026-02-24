import("stdfaust.lib");

bufSize = 48000;

note   = nentry("note", 69, 0, 127, 1);
gate   = nentry("gate", 0, 0, 1, 1) : si.smoo;
record = nentry("record", 0, 0, 1, 1);

ratio = pow(2.0, (note - 69.0) / 12.0);

// WRITE INDEX
writeIndex = (+(record) : %(bufSize)) ~ _;

// READ INDEX
readIndex = (+(ratio) : fmod(_, float(bufSize))) ~ _;

// TABLE
process = rwtable(
    bufSize,
    0.0,
    int(writeIndex),
    _,
    int(readIndex)
) * gate;
