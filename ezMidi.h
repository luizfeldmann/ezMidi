// ===================================================================================  //
//    This program is free software: you can redistribute it and/or modify              //
//    it under the terms of the GNU General Public License as published by              //
//    the Free Software Foundation, either version 3 of the License, or                 //
//    (at your option) any later version.                                               //
//                                                                                      //
//    This program is distributed in the hope that it will be useful,                   //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                    //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                     //
//    GNU General Public License for more details.                                      //
//                                                                                      //
//    You should have received a copy of the GNU General Public License                 //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.            //
//                                                                                      //
//    Copyright: Luiz Gustavo Pfitscher e Feldmann, 2020                                //
// ===================================================================================  //

#ifndef _EZ_MIDI_LIB_H_
#define _EZ_MIDI_LIB_H_

#include <stdint.h>

// DEFINITION OF MULTIPLE EVENT TYPES AND PARAMETERS
// ===================================================================
typedef enum MidiEventType {
    // Meta-events
    // ===============================================================

    // Text data
    Midi_Event_Type_Text            = 0x01, // FF 01 len <text>
    Midi_Event_Type_Copyright       = 0x02, // FF 02 len <text>
    Midi_Event_Type_SequenceName    = 0x03, // FF 03 len <text>
    Midi_Event_Type_InstrumentName  = 0x04, // FF 04 len <text>
    Midi_Event_Type_Lyric           = 0x05, // FF 05 len <text>
    Midi_Event_Type_Marker          = 0x06, // FF 06 len <text>
    Midi_Event_Type_CuePoint        = 0x07, // FF 07 len <text>
    Midi_Event_Type_ProgramName     = 0x08, // FF 08 len <text>

    // numerical data
    Midi_Event_Type_SequenceNumber  = 0x00, // FF 00 02 nn-nn
    Midi_Event_Type_ChannelPrefix   = 0x20, // FF 20 01 cc
    Midi_Event_Type_MidiPort        = 0x21, // FF 21 01 pp
    Midi_Event_Type_EndOfTrack      = 0x2F, // FF 2F 00
    Midi_Event_Type_SetTempo        = 0x51, // FF 51 03 tt-tt-tt
    Midi_Event_Type_SMPTEoffset     = 0x54, // FF 54 05 hr mn se fr ff
    Midi_Event_Type_TimeSignature   = 0x58, // FF 58 04 nn dd cc bb
    Midi_Event_Type_KeySignature    = 0x59, // FF 59 02 sf mi
    Midi_Event_Type_SysEx           = 0x7F, // FF 7F len <data>
    Midi_Event_Type_SysEx2          = 0xF0, // F0 len <data>

    // Channel-events
    // ===============================================================
    Midi_Event_Type_NoteOn                 = 0x90,   // 1001nnnn 0kkkkkkk 0vvvvvvv
    Midi_Event_Type_NoteOff                = 0x80,   // 1000nnnn 0kkkkkkk 0vvvvvvv
    Midi_Event_Type_PolyphonicKeyPressure  = 0xA0,   // 1010nnnn 0kkkkkkk 0vvvvvvv
    Midi_Event_Type_ControlChange          = 0xB0,   // 1011nnnn 0ccccccc 0vvvvvvv
    Midi_Event_Type_ProgramChange          = 0xC0,   // 1100nnnn 0ppppppp
    Midi_Event_Type_ChannelPressure        = 0xD0,   // 1101nnnn 0vvvvvvv
    Midi_Event_Type_PitchWheelChange       = 0xE0,   // 1110nnnn 0lllllll 0mmmmmmm
} MidiEventType_t;

typedef struct MidiEventData_Text
{
    #define MidiEventData_Text_Max_Length 255
    uint8_t length;
    char text[MidiEventData_Text_Max_Length];
} MidiEventData_Text_t;

typedef struct MidiEventData_SequenceNumber
{
    uint16_t number;
} MidiEventData_SequenceNumber_t;

typedef struct MidiEventData_ChannelPrefix
{
    uint8_t channel; // The MIDI channel (0-15) contained in this event may be used to associate a MIDI channel with all events which follow, including System exclusive and meta-events. This channel is "effective" until the next normal MIDI event (which contains a channel) or the next MIDI Channel Prefix meta-event.
} MidiEventData_ChannelPrefix_t;

typedef struct MidiEventData_MidiPort
{
    uint8_t port;
} MidiEventData_MidiPort_t;

typedef struct MidiEventData_SetTempo
{
    uint32_t tempo;
} MidiEventData_SetTempo_t;

typedef struct MidiEventData_SMPTEoffset // This event, if present, designates the SMPTE time at which the track chunk is supposed to start. It should be present at the beginning of the track, that is, before any nonzero delta-times, and before any transmittable MIDI events.
{
    uint8_t hr;
    uint8_t mn;
    uint8_t se;
    uint8_t fr; // contains fractional frames, in 100ths of a frame
    uint8_t ff;
} MidiEventData_SMPTEoffset_t;

typedef struct MidiEventData_TimeSignature
{
    uint8_t nn; // numerator
    uint8_t dd; // denominator / a negative power of two: 2 represents a quarter-note (2^-2 = 1/4), 3 represents an eighth-note (2^-3 = 1/8)
    uint8_t cc; // the number of MIDI clocks in a metronome click
    uint8_t bb; // number of notated 32nd-notes in a MIDI quarter-note (24 MIDI clocks).
} MidiEventData_TimeSignature_t;

typedef struct MidiEventData_KeySignature
{
    uint8_t sf; // if negative: number of flats; if positive: number of sharps; If 0: C key
    uint8_t mi; // if 0: major key; if 1: minor key
} MidiEventData_KeySignature_t;

typedef struct MidiEventData_NoteEvent
{
    uint8_t channel;
    uint8_t key;
    uint8_t velocity;
    uint8_t OnOff; // 0x90 (on) or 0x80 (off)
} MidiEventData_NoteEvent_t;

typedef struct MidiEventData_PolyphonicKeyPressure
{
    uint8_t channel;
    uint8_t key;
    uint8_t pressure;
} MidiEventData_PolyphonicKeyPressure_t;

typedef struct MidiEventData_ControlChange
{
    uint8_t channel;
    uint8_t control;
    uint8_t value;
} MidiEventData_ControlChange_t;

typedef struct MidiEventData_ProgramChange
{
    uint8_t channel;
    uint8_t program;
} MidiEventData_ProgramChange_t;

typedef struct MidiEventData_ChannelPressure
{
    uint8_t channel;
    uint8_t pressure;
} MidiEventData_ChannelPressure_t;

typedef struct MidiEventData_PitchWheelChange
{
    uint8_t channel;
    uint16_t wheel;
} MidiEventData_PitchWheelChange_t;

typedef MidiEventData_Text_t MidiEventData_SysEx_t;

// EVENT
// ===================================================================

// interface/vtable
typedef struct MidiEventInterface MidiEventInterface_t;

typedef struct MidiEvent
{
    uint32_t deltaTime;
    const MidiEventInterface_t* interface;
    void* data;
} MidiEvent_t;

MidiEventType_t MidiEvent_GetType(const MidiEvent_t* event);


// FILES AND TRACKS
// ===================================================================

typedef enum MidiFileFormat {
    MIDI_FORMAT_SINGLE_TRACK = 0,
    MIDI_FORMAT_SIMULTANEOUS_TRACKS = 1,
    MIDI_FORMAT_SEQUENTIAL_TRACKS = 2,
} MidiFileFormat_t;

typedef struct MidiTrack {
    uint32_t NumEvents; // Number of events in the array
    MidiEvent_t *Events; // pointer to the first event in track
} MidiTrack_t;

typedef struct MidiFile {
    uint16_t Format;
    uint16_t nTrks; // must be 1 if format is 0
    uint16_t PulsesPerQuarterNote;
    MidiTrack_t *Tracks;
} MidiFile_t;

MidiFile_t *MidiFile_Open(const char* filename);
int MidiFile_Save(const char* filename, const MidiFile_t *save);
void MidiFile_Close(MidiFile_t *close);

// TIME MAP
// ===================================================================
typedef struct MidiAbsoluteTimeMap {
    uint16_t track;
    MidiEvent_t* OnEvent;
    MidiEvent_t* OffEvent;
    uint32_t startTime;
    uint32_t endTime;
} MidiAbsoluteTimeMap_t;

uint32_t Midi_MapAbsoluteTime(MidiAbsoluteTimeMap_t** list, const MidiFile_t* midi);

// PLAYER
// ===================================================================
typedef enum Player_Callback_Result
{
    Player_Callback_PlayEvent = 1,
    Player_Callback_IgnoreEvent = 0,
    Player_Callback_Abort = -1,
} Player_Callback_Result_t;

typedef int (*playerCallback)(MidiEvent_t* event, uint16_t track, uint32_t timeTicks, uint32_t timeUs);

int MidiDevice_Open();
void MidiDevice_Close();
void MidiDevice_Reset();
int MidiDevice_SetChannelInstrument(uint8_t channel, uint8_t instrument);
int MidiDevice_PlayNote(uint8_t key, uint8_t channel, uint8_t velocity, uint8_t state);
void MidiFile_Play(const MidiFile_t* midi, uint32_t start_usec, playerCallback cbFunc);

// ADDITIONAL FEATURES
// ===================================================================
typedef struct MidiTranspositionData {
    const uint8_t sf;
    const uint8_t mi;
    const int8_t tranposeDelta;
    const char* description;
} MidiTranspositionData_t;

extern const MidiTranspositionData_t Midi_Transposition_Table[];

MidiEventData_KeySignature_t* Midi_GetKeySignature(const MidiFile_t* midi);
const MidiTranspositionData_t* Midi_GetKeySignatureTranspositionInfo(const MidiEventData_KeySignature_t* ks);
int8_t Midi_Transpose(MidiFile_t* file, const MidiTranspositionData_t* newKey);

const char* Midi_GetKeyName(uint8_t key);
const char* Midi_GetInstrumentName(uint8_t program);
uint8_t Midi_IsSharp(uint8_t key);

#endif // _EZ_MIDI_LIB_H_
