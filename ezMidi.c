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

#include "ezMidi.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


// MIDI EVENTS
// ===================================================================
#define read_data_params (const char* buffer_ptr, uint8_t type, void* data_ptr)
#define read_data_dclr(name) static const int name read_data_params

#define write_data_params (char* buffer_ptr, const MidiEvent_t* event)
#define write_data_dclr(name) static const int name write_data_params

#define print_data_params (const void* data_ptr, char output_text[])
#define print_data_dclr(name) static const int name print_data_params

struct MidiEventInterface {
    const int type;
    const char* description;
    const int alloc_size; // how much should we allocate for the data_ptr
    const int (*read_data) read_data_params;
    const int (*write_data) write_data_params; // writes type, length and data, returns number of bytes written
    const int (*print_data) print_data_params; // reads length and data, returns number of bytes read
};

// META EVENTS
// format: FF type len <data>
const static char* err_wrong_size = "\nWrong length for event type: expected %u but got %u";
#define declare_len() uint8_t len = *((uint8_t*)(buffer_ptr))
#define assert_len(desired_len) {if((len) != (desired_len)) {fprintf(stderr, err_wrong_size, (desired_len), (len)); return -1;}}
#define read_integer_at(i) ( *(uint8_t*)(buffer_ptr + (i)) )

// FF xx: len <text>
read_data_dclr(read_data_Text)
{
    declare_len();

    if (len > MidiEventData_Text_Max_Length - 1)
    {
        fprintf(stderr, "\nTextEvent has length (%u) larger than buffer!", len);
        return -1;
    }

    *(MidiEventData_Text_t*)data_ptr = (MidiEventData_Text_t){
        .length = len,
    };

    sprintf(((MidiEventData_Text_t*)data_ptr)->text, "%.*s", len, (char*)(buffer_ptr+1));

    return 1 + len;
}

write_data_dclr(write_data_Text)
{
    uint8_t len = ((MidiEventData_Text_t*)event->data)->length;

    if (event->interface->type == Midi_Event_Type_SysEx2)
    {
        char data[] = {Midi_Event_Type_SysEx2, (char)len};

        memcpy(buffer_ptr, data, 2);
        memcpy(&buffer_ptr[2], ((MidiEventData_Text_t*)event->data)->text, len);

        return 2 + len;
    }
    else
    {
        char data[] = {0xFF,  (char)(event->interface->type), (char)len};

        memcpy(buffer_ptr, data, 3);
        memcpy(&buffer_ptr[3], ((MidiEventData_Text_t*)event->data)->text, len);

        return 3 + len;
    }
}

print_data_dclr(print_data_Text)
{
    return sprintf(output_text, "\"%s\"", ((MidiEventData_Text_t*)data_ptr)->text);
}

// FF 00: 02 nn-nn
read_data_dclr(read_data_SequenceNumber)
{
    declare_len();
    assert_len(2);

    *(MidiEventData_SequenceNumber_t*)data_ptr = (MidiEventData_SequenceNumber_t){
        .number = read_integer_at(1)*256 + read_integer_at(2),
    };

    return 3;
}

write_data_dclr(write_data_SequenceNumber)
{
    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_SequenceNumber, 2, ((MidiEventData_SequenceNumber_t*)event->data)->number / 256, ((MidiEventData_SequenceNumber_t*)event->data)->number % 256}, 5);
    return 5;
}

print_data_dclr(print_data_SequenceNumber)
{
    return sprintf(output_text, "%u", ((MidiEventData_SequenceNumber_t*)data_ptr)->number);
}

// FF 20: 01 cc
read_data_dclr(read_data_ChannelPrefix)
{
    declare_len();
    assert_len(1);

    MidiEventData_ChannelPrefix_t* cp = (MidiEventData_ChannelPrefix_t*)data_ptr;
    (*cp) = (MidiEventData_ChannelPrefix_t){
        .channel = read_integer_at(1),
    };

    if (cp->channel > 15)
        fprintf(stderr, "\nWarning: ChannelPrefix has invalid channel %u (maximum is 15)", cp->channel);

    return 2;
}

write_data_dclr(write_data_ChannelPrefix)
{
    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_ChannelPrefix, 1, ((MidiEventData_ChannelPrefix_t*)event->data)->channel}, 4);
    return 4;
}

print_data_dclr(print_data_ChannelPrefix)
{
    return sprintf(output_text, "%u", ((MidiEventData_ChannelPrefix_t*)data_ptr)->channel);
}

// FF 21: 01 pp
read_data_dclr(read_data_MidiPort)
{
    declare_len();
    assert_len(1);

    *(MidiEventData_MidiPort_t*)data_ptr = (MidiEventData_MidiPort_t){
        .port = read_integer_at(1),
    };

    return 2;
}

write_data_dclr(write_data_MidiPort)
{
    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_MidiPort, 1, ((MidiEventData_MidiPort_t*)event->data)->port}, 4);
    return 4;
}

print_data_dclr(print_data_MidiPort)
{
    return sprintf(output_text, "%u", ((MidiEventData_MidiPort_t*)data_ptr)->port);
}

// FF 51: 03 tt-tt-tt
read_data_dclr(read_data_SetTempo)
{
    declare_len();
    assert_len(3);

    *(MidiEventData_SetTempo_t*)data_ptr = (MidiEventData_SetTempo_t){
        .tempo = (read_integer_at(1)*256 + read_integer_at(2))*256 + read_integer_at(3),
    };

    return 4;
}

write_data_dclr(write_data_SetTempo)
{
    uint32_t tempo = ((MidiEventData_SetTempo_t*)event->data)->tempo;

    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_SetTempo, 3, (tempo >> 16) & 0xFF, (tempo >> 8) & 0xFF, tempo & 0xFF}, 6);
    return 6;
}

print_data_dclr(print_data_SetTempo)
{
    return sprintf(output_text, "%u", ((MidiEventData_SetTempo_t*)data_ptr)->tempo);
}

// FF 54: 05 hr mn se fr ff
read_data_dclr(read_data_SMPTEoffset)
{
    declare_len();
    assert_len(5);

    *(MidiEventData_SMPTEoffset_t*)data_ptr = (MidiEventData_SMPTEoffset_t){
        .hr = read_integer_at(1),
        .mn = read_integer_at(2),
        .se = read_integer_at(3),
        .fr = read_integer_at(4),
        .ff = read_integer_at(5),
    };

    return 6;
}

write_data_dclr(write_data_SMPTEoffset)
{
    /* TODO */
    return 0;
}

print_data_dclr(print_data_SMPTEoffset)
{
    MidiEventData_SMPTEoffset_t* offset = (MidiEventData_SMPTEoffset_t*)data_ptr;

    return sprintf(output_text, "HR:%u  MN:%u  SE:%u  FR:%u  FF:%u", offset->hr, offset->mn, offset->se, offset->fr, offset->ff);
}

// FF 58 04 nn dd cc bb
read_data_dclr(read_data_TimeSignature)
{
    declare_len();
    assert_len(4);

    *(MidiEventData_TimeSignature_t*)data_ptr = (MidiEventData_TimeSignature_t){
        .nn = read_integer_at(1),
        .dd = read_integer_at(2),
        .cc = read_integer_at(3),
        .bb = read_integer_at(4),
    };

    return 5;
}

write_data_dclr(write_data_TimeSignature)
{
    MidiEventData_TimeSignature_t* ts = (MidiEventData_TimeSignature_t*)event->data;

    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_TimeSignature, 4, ts->nn, ts->dd, ts->cc, ts->bb}, 7);
    return 7;
}

print_data_dclr(print_data_TimeSignature)
{
    MidiEventData_TimeSignature_t* ts = (MidiEventData_TimeSignature_t*)data_ptr;

    return sprintf(output_text, "numerator:%u  denominator:%u  cc:%u  bb:%u", ts->nn, ts->dd, ts->cc, ts->bb);
}

// FF 59: 02 sf mi
read_data_dclr(read_data_KeySignature)
{
    declare_len();
    assert_len(2);

    MidiEventData_KeySignature_t* ks = (MidiEventData_KeySignature_t*)data_ptr;
    (*ks) = (MidiEventData_KeySignature_t){
        .sf = read_integer_at(1),
        .mi = read_integer_at(2),
    };

    if (ks->mi != 0 && ks->mi != 1)
        fprintf(stderr, "\nWarning: KeySignature has invalid mi=%u", ks->mi);

    return 3;
}

write_data_dclr(write_data_KeySignature)
{
    MidiEventData_KeySignature_t* ks = (MidiEventData_KeySignature_t*)event->data;

    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_KeySignature, 2, ks->sf, ks->mi}, 5);
    return 5;
}

print_data_dclr(print_data_KeySignature)
{
    return sprintf(output_text, "sf:%d  mi:%d = %s", (int8_t)((MidiEventData_KeySignature_t*)data_ptr)->sf, ((MidiEventData_KeySignature_t*)data_ptr)->mi, Midi_GetKeySignatureTranspositionInfo((MidiEventData_KeySignature_t*)data_ptr)->description);
}

// FF 7F: len <data>
read_data_dclr(read_data_SysEx)
{
    return read_data_Text(buffer_ptr, type, data_ptr);
}

write_data_dclr(write_data_SysEx)
{
    return write_data_Text(buffer_ptr, event);
}

print_data_dclr(print_data_SysEx)
{
    return print_data_Text(data_ptr, output_text);
}

// FF 2F: 00
read_data_dclr(read_data_EndOfTrack)
{
    declare_len();
    assert_len(0);

    return 1;
}

write_data_dclr(write_data_EndOfTrack)
{
    memcpy(buffer_ptr, (char[]){0xFF, Midi_Event_Type_EndOfTrack, 0}, 3);
    return 3;
}

print_data_dclr(print_data_EndOfTrack)
{
    return sprintf(output_text, "End of Track");
}

// CHANNEL EVENTS
// format: xxxxnnnn aaaaaa bbbbb

// on = 1001nnnn: 0kkkkkkk 0vvvvvvv
// off = 1001nnnn: 0kkkkkkk 0vvvvvvv

read_data_dclr(read_data_Note)
{
    *(MidiEventData_NoteEvent_t*)data_ptr = (MidiEventData_NoteEvent_t){
        .channel = type & 0x0F,
        .key = read_integer_at(0),
        .velocity = read_integer_at(1),
        .OnOff = type & 0xF0
    };

    return 2;
}

write_data_dclr(write_data_Note)
{
    MidiEventData_NoteEvent_t* note = (MidiEventData_NoteEvent_t*)event->data;

    memcpy(buffer_ptr, (char[]){note->OnOff | note->channel, note->key, note->velocity}, 3);

    return 3;
}

print_data_dclr(print_data_Note)
{
    return sprintf(output_text, "ch:%u key:%u %s", ((MidiEventData_NoteEvent_t*)data_ptr)->channel, ((MidiEventData_NoteEvent_t*)data_ptr)->key, Midi_GetKeyName(((MidiEventData_NoteEvent_t*)data_ptr)->key));
}

// 1010nnnn: 0kkkkkkk 0vvvvvvv
read_data_dclr(read_data_PolyphonicKeyPressure)
{
    *(MidiEventData_PolyphonicKeyPressure_t*)data_ptr = (MidiEventData_PolyphonicKeyPressure_t){
        .channel = type & 0x0F,
        .key = read_integer_at(0),
        .pressure = read_integer_at(1),
    };

    return 2;
}

write_data_dclr(write_data_PolyphonicKeyPressure)
{
    MidiEventData_PolyphonicKeyPressure_t* pkp = (MidiEventData_PolyphonicKeyPressure_t*)event->data;

    memcpy(buffer_ptr, (char[]){Midi_Event_Type_PolyphonicKeyPressure | pkp->channel, pkp->key, pkp->pressure}, 3);

    return 3;
}

print_data_dclr(print_data_PolyphonicKeyPressure)
{
    return sprintf(output_text, "ch:%u  key:%u  pressure:%u", ((MidiEventData_PolyphonicKeyPressure_t*)data_ptr)->channel, ((MidiEventData_PolyphonicKeyPressure_t*)data_ptr)->key, ((MidiEventData_PolyphonicKeyPressure_t*)data_ptr)->pressure);
}

// 1011nnnn: 0ccccccc 0vvvvvvv
read_data_dclr(read_data_ControlChange)
{
    *(MidiEventData_ControlChange_t*)data_ptr = (MidiEventData_ControlChange_t){
        .channel = type & 0x0F,
        .control = read_integer_at(0),
        .value = read_integer_at(1)
    };

    return 2;
}

write_data_dclr(write_data_ControlChange)
{
    MidiEventData_ControlChange_t* cc = (MidiEventData_ControlChange_t*)event->data;

    memcpy(buffer_ptr, (char[]){Midi_Event_Type_ControlChange | cc->channel, cc->control, cc->value}, 3);

    return 3;
}

print_data_dclr(print_data_ControlChange)
{
    return sprintf(output_text, "ch:%u  control:%u  value:%u", ((MidiEventData_ControlChange_t*)data_ptr)->channel, ((MidiEventData_ControlChange_t*)data_ptr)->control, ((MidiEventData_ControlChange_t*)data_ptr)->value);
}

// 1100nnnn: 0ppppppp
read_data_dclr(read_data_ProgramChange)
{
    *(MidiEventData_ProgramChange_t*)data_ptr = (MidiEventData_ProgramChange_t){
        .channel = type & 0x0F,
        .program = read_integer_at(0),
    };

    return 1;
}

write_data_dclr(write_data_ProgramChange)
{
    MidiEventData_ProgramChange_t* pc = (MidiEventData_ProgramChange_t*)event->data;

    memcpy(buffer_ptr, (char[]){Midi_Event_Type_ProgramChange | pc->channel, pc->program}, 2);

    return 2;
}

print_data_dclr(print_data_ProgramChange)
{
    return sprintf(output_text, "ch:%u  program:%u %s", ((MidiEventData_ProgramChange_t*)data_ptr)->channel, ((MidiEventData_ProgramChange_t*)data_ptr)->program, Midi_GetInstrumentName(((MidiEventData_ProgramChange_t*)data_ptr)->program));
}

// 1101nnnn: 0vvvvvvv
read_data_dclr(read_data_ChannelPressure)
{
    *(MidiEventData_ChannelPressure_t*)data_ptr = (MidiEventData_ChannelPressure_t){
        .channel = type & 0x0F,
        .pressure = read_integer_at(0),
    };

    return 1;
}

write_data_dclr(write_data_ChannelPressure)
{
    MidiEventData_ChannelPressure_t* cp = (MidiEventData_ChannelPressure_t*)event->data;

    memcpy(buffer_ptr, (char[]){Midi_Event_Type_ChannelPressure | cp->channel, cp->pressure}, 2);

    return 2;
}

print_data_dclr(print_data_ChannelPressure)
{
    return sprintf(output_text, "ch:%u  pressure:%u", ((MidiEventData_ChannelPressure_t*)data_ptr)->channel, ((MidiEventData_ChannelPressure_t*)data_ptr)->pressure);
}

// 1110nnnn: 0lllllll 0mmmmmmm
read_data_dclr(read_data_PitchWheelChange)
{
    *(MidiEventData_PitchWheelChange_t*)data_ptr = (MidiEventData_PitchWheelChange_t){
        .channel = type & 0x0F,
        .wheel = read_integer_at(0) + 128*read_integer_at(1),
    };

    return 2;
}

write_data_dclr(write_data_PitchWheelChange)
{
    MidiEventData_PitchWheelChange_t* pwc = (MidiEventData_PitchWheelChange_t*)event->data;

    memcpy(buffer_ptr, (char[]){Midi_Event_Type_PitchWheelChange | pwc->channel, pwc->wheel % 128, pwc->wheel / 128}, 3);

    return 3;
}

print_data_dclr(print_data_PitchWheelChange)
{
    return sprintf(output_text, "ch:%u  wheel:%u", ((MidiEventData_PitchWheelChange_t*)data_ptr)->channel, ((MidiEventData_PitchWheelChange_t*)data_ptr)->wheel);
}

static const MidiEventInterface_t InterfaceTable[] = {
    {Midi_Event_Type_Text,              "Text",             sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_Copyright,         "Copyright notice", sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_SequenceName,      "Sequence name",    sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_InstrumentName,    "Instrument name",  sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_Lyric,             "Lyric",            sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_Marker,            "Marker",           sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_CuePoint,          "Cue Point",        sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},
    {Midi_Event_Type_ProgramName,       "Program name",     sizeof(MidiEventData_Text_t), read_data_Text, write_data_Text, print_data_Text},


    {Midi_Event_Type_SequenceNumber,    "Sequence number",  sizeof(MidiEventData_SequenceNumber_t), read_data_SequenceNumber,   write_data_SequenceNumber,  print_data_SequenceNumber},
    {Midi_Event_Type_ChannelPrefix,     "Channel prefix",   sizeof(MidiEventData_ChannelPrefix_t),  read_data_ChannelPrefix,    write_data_ChannelPrefix,   print_data_ChannelPrefix},
    {Midi_Event_Type_MidiPort,          "Midi port",        sizeof(MidiEventData_MidiPort_t),       read_data_MidiPort,         write_data_MidiPort,        print_data_MidiPort},
    {Midi_Event_Type_EndOfTrack,        "End of Track",     0,                                      read_data_EndOfTrack,       write_data_EndOfTrack,      print_data_EndOfTrack},
    {Midi_Event_Type_SetTempo,          "Set tempo",        sizeof(MidiEventData_SetTempo_t),       read_data_SetTempo,         write_data_SetTempo,        print_data_SetTempo},
    {Midi_Event_Type_SMPTEoffset,       "SMPTE offset",     sizeof(MidiEventData_SMPTEoffset_t),    read_data_SMPTEoffset,      write_data_SMPTEoffset,     print_data_SMPTEoffset},
    {Midi_Event_Type_TimeSignature,     "Time signature",   sizeof(MidiEventData_TimeSignature_t),  read_data_TimeSignature,    write_data_TimeSignature,   print_data_TimeSignature},
    {Midi_Event_Type_KeySignature,      "KeySignature",     sizeof(MidiEventData_KeySignature_t),   read_data_KeySignature,     write_data_KeySignature,    print_data_KeySignature},
    {Midi_Event_Type_SysEx,             "SysEx",            sizeof(MidiEventData_SysEx_t),          read_data_SysEx,            write_data_SysEx,           print_data_SysEx},
    {Midi_Event_Type_SysEx2,            "SysEx2",           sizeof(MidiEventData_SysEx_t),          read_data_SysEx,            write_data_SysEx,           print_data_SysEx},


    {Midi_Event_Type_NoteOn,            "Note on",          sizeof(MidiEventData_NoteEvent_t),      read_data_Note,           write_data_Note,              print_data_Note},
    {Midi_Event_Type_NoteOff,           "Note off",         sizeof(MidiEventData_NoteEvent_t),      read_data_Note,          write_data_Note,             print_data_Note},
    {Midi_Event_Type_PolyphonicKeyPressure, "Polyphonic key pressure", sizeof(MidiEventData_PolyphonicKeyPressure_t), read_data_PolyphonicKeyPressure, write_data_PolyphonicKeyPressure, print_data_PolyphonicKeyPressure},
    {Midi_Event_Type_ControlChange,     "Control change",   sizeof(MidiEventData_ControlChange_t),  read_data_ControlChange,    write_data_ControlChange,       print_data_ControlChange},
    {Midi_Event_Type_ProgramChange,     "Program change",   sizeof(MidiEventData_ProgramChange_t),  read_data_ProgramChange,    write_data_ProgramChange,       print_data_ProgramChange},
    {Midi_Event_Type_ChannelPressure,   "Channel pressure", sizeof(MidiEventData_ChannelPressure_t),read_data_ChannelPressure,  write_data_ChannelPressure,     print_data_ChannelPressure},
    {Midi_Event_Type_PitchWheelChange,  "Pitch wheel change", sizeof(MidiEventData_PitchWheelChange_t),read_data_PitchWheelChange, write_data_PitchWheelChange, print_data_PitchWheelChange},
};

int MidiEvent_Create(MidiEvent_t* event, const uint8_t type, const uint32_t deltaTime, const uint8_t allocData)
{
    int interface = -1;

    for (int i = 0; i < sizeof(InterfaceTable)/sizeof(InterfaceTable[0]); i++)
    {
        if (InterfaceTable[i].type != type)
            continue;

        interface = i;
        break;
    }

    if (interface < 0)
    {
        fprintf(stderr, "\nCould not find interface for event type 0x%.02x", type);
        return -1;
    }

    *event = (MidiEvent_t){
        .interface = &InterfaceTable[interface],
        .data = (allocData) ? malloc(InterfaceTable[interface].alloc_size) : NULL,
        .deltaTime = deltaTime,
    };

    return 0;
}

void MidiEvent_Print(FILE* f, const MidiEvent_t* event)
{
    char text[256] = "";
    event->interface->print_data(event->data, text);

    fprintf(f, "\n%s: %s", event->interface->description, text);
}

MidiEventType_t MidiEvent_GetType(const MidiEvent_t* event)
{
    return (MidiEventType_t)event->interface->type;
}

// UTIL
// ===================================================================
// this is necessary because the MIDI file is big-endian and this system could be little-endian
uint32_t u32fromarray(const uint8_t arr[4])
{
    return ((((uint32_t)arr[0]*256 + (uint32_t)arr[1])*256 + (uint32_t)arr[2])*256 + (uint32_t)arr[3]);
}

void u32toarray(uint32_t value, uint8_t arr[4])
{
    arr[0] = (value >> 24) & 0xFF;
    arr[1] = (value >> 16) & 0xFF;
    arr[2] = (value >> 8) & 0xFF;
    arr[3] = value & 0xFF;
}

uint16_t u16fromarray(const uint8_t arr[2])
{
    return ((uint16_t)arr[0]*256 + arr[1]);
}


void u16toarray(uint16_t value, uint8_t arr[2])
{
    arr[0] = (value >> 8) & 0xFF;
    arr[1] = value & 0xFF;
}

// reads a big-endian value with variable length from the file
// each byte is 7-bit data and the 7th bit (leftmost) is a flag informing to continue reading the next byte
// saves the read value in a pointer and return the total number of bytes read
int ReadVariableLength(const char* buffer, uint32_t bufflen, uint32_t *outvalue)
{
    int size_read = 0; // initialize bytes read counter to 0
    (*outvalue) = 0;   // initialize the output value

    uint8_t read;
    do {
        if(size_read >= bufflen)
            return -1;

        read = *buffer++; // read ONE byte
        size_read++; // count how many bytes read

        if (read < 128)
            (*outvalue) += read;
        else
        {
            (*outvalue) += read - 128;
            (*outvalue) *= 128;
        }
    } while (read >= 128);

    return size_read;
}

int WriteVariableLength(char* buffer, uint32_t bufflen, uint32_t value)
{
    uint32_t i = 0, j, v = value;

    while ((v /= 128) > 0) // count how many bytes
        i++;

    if ((j = i+1) > bufflen) // check if buffer big enough
        return -1;

    buffer[i] = value & 0x7f;
    while(i--)
        buffer[i] = ((value /= 128) & 0x7F) | 0x80; // work backwards

    return j;
}

// TRACKS
// ===================================================================
void MidiTrack_Read(const char* buffer, uint32_t length, MidiTrack_t* track)
{
    uint32_t read = 0;
    uint8_t running_status = 0;

    do {
        // read event delta-time
        uint32_t deltaTime;

        {
            int variablelen = ReadVariableLength(&buffer[read], length - read, &deltaTime); // returns number of bytes read or error code
            if (variablelen <= 0)
            {
                fprintf(stderr, "\nError reading delta-time!");
                return;
            }
            else
                read += variablelen;
        }

        // read the status byte ("type" of the event)
        uint8_t meta = 0;
        uint8_t statusByte = buffer[read++];

        if (statusByte == 0xFF) // this is a META event
        {
            statusByte = buffer[read++]; // read the actual type
            meta = 1;
        }
        else
        {
            // not a meta event
            if (statusByte < 0x80) // running status is in effect
            {
                statusByte = running_status;
                read--; // we actually read the first data byte ... let's go back
            }
            else
                running_status = statusByte;
        }

        // expand the list so it fits one more event
        {
            MidiEvent_t *new_list = (MidiEvent_t*)realloc(track->Events, sizeof(MidiEvent_t) * (track->NumEvents + 1));
            if (new_list == NULL)
            {
                fprintf(stderr, "\nError reallocating list of events for track");
                break;
            }
            else
            {
                track->Events = new_list;
                track->NumEvents++;
            }
        }

        MidiEvent_t* new_event = &track->Events[track->NumEvents - 1];

        if (MidiEvent_Create(new_event, ((meta) ? statusByte : (statusByte & 0xF0)), deltaTime, 1) != 0)
        {
            fprintf(stderr, "\nAborted reading of track because event size is unknown!");
            break;
        }

        read += new_event->interface->read_data(&buffer[read], statusByte, new_event->data); //MidiEvent_Read(new_event, &buffer[read]);
        //MidiEvent_Print(stdout, new_event);
    }
    while (read < length);
}

// MIDI FILES
// ===================================================================

void MidiFile_Close(MidiFile_t *close)
{
    // sanity check
    if (!close)
        return;

    // make sure there are tracks
    if (close->Tracks && close->nTrks)
    {
        for (int t = 0; t < close->nTrks; t++)
        {
            // make sure there are events
            if (!close->Tracks[t].Events || !close->Tracks[t].NumEvents)
                continue;

            for (int e = 0; e < close->Tracks[t].NumEvents; e++)
                free(close->Tracks[t].Events[e].data); // free the data for each event

            // free the list of events in each track
            free(close->Tracks[t].Events);
        }

        // free the list of tracks
        free(close->Tracks);
    }

    // free the file
    free(close);
}

MidiFile_t *MidiFile_Open(const char* filename)
{
    // sanity check
    if (filename == NULL)
        return NULL;

    // open the file
    FILE* fp;
    if ((fp = fopen(filename, "rb+")) == NULL)
    {
        fprintf(stderr, "\nFailed to open MIDI file %s: %d %s", filename, errno, strerror(errno));
        return NULL;
    }

    // read to the midi file structure
    MidiFile_t *mf;
    if ((mf = (MidiFile_t*)malloc(sizeof(MidiFile_t))) == NULL)
    {
        fprintf(stderr, "\nAllocation error!");
        fclose(fp);
        return NULL;
    }
    else
    {
        // make sure we start fresh on the file
        (*mf) = (MidiFile_t){0, 0, 0, NULL};
    }

    uint8_t trackNumber = 0;

    // Read chunks
    while (feof(fp) == 0) // read until end of file
    {
        // read chunk type
        #define MIDI_CHUNK_SIZE 4
        char chunktype[MIDI_CHUNK_SIZE];
        int readlen;

        if ( (readlen = fread(chunktype, sizeof(char),MIDI_CHUNK_SIZE, fp)) != MIDI_CHUNK_SIZE)
        {
            if (feof(fp)) // Could not read because end of file is reached
                break; // exit read loop without error

            fprintf(stderr, "\nError reading chunk: read %d of expected %d: %d %s", readlen, MIDI_CHUNK_SIZE, errno, strerror(errno));
            goto error;
        }

        // read chunk length
        #define MIDI_CHUNK_LEN_BYTES 4
        uint8_t __chunklength[MIDI_CHUNK_LEN_BYTES];
        fread(__chunklength, sizeof(uint8_t), MIDI_CHUNK_LEN_BYTES, fp); // MThd|MTrk len-len-len-len <data>
        uint32_t chunklength = u32fromarray(__chunklength);

        // treat chunk type accordingly
        #define MIDI_CHUNK_HEADER "MThd"
        #define MIDI_CHUNK_TRACK "MTrk"

        if (memcmp(&chunktype, &MIDI_CHUNK_HEADER, sizeof(chunktype)) == 0) // MThd (header chunk)
        {
            #define MIDI_CHUNK_HEADER_LEN 6
            if (chunklength != MIDI_CHUNK_HEADER_LEN) // header is comprised of 3 words of 16-bits
            {
                fprintf(stderr, "\nError reading header: expected size is 6 but got %u", chunklength);
                goto error;
            }


            uint8_t header[MIDI_CHUNK_HEADER_LEN];

            if (fread(header, 1, MIDI_CHUNK_HEADER_LEN, fp) != MIDI_CHUNK_HEADER_LEN)
            {
                fprintf(stderr, "\nError reading header");
                goto error;
            }

            mf->Format = u16fromarray(&header[0]);
            mf->nTrks = u16fromarray(&header[2]);
            mf->PulsesPerQuarterNote = u16fromarray(&header[4]);

            if ((mf->Format == MIDI_FORMAT_SINGLE_TRACK) && (mf->nTrks != 1))
            {
                fprintf(stderr, "\nFile format is 'Single track' but number of tracks is %u ", mf->nTrks);
                goto error;
            }

            mf->Tracks = (MidiTrack_t*)calloc(mf->nTrks, sizeof(MidiTrack_t));
            if (!mf->Tracks)
            {
                fprintf(stderr, "\nError trying to allocate memory for tracks: %u x %u bytes ", mf->nTrks, (unsigned int)sizeof(MidiTrack_t));
                goto error;
            }
        }
        else if (memcmp( &chunktype, &MIDI_CHUNK_TRACK, sizeof(chunktype)) == 0) // MTrk (track chunk)
        {
            // sanity check
            if (!mf->Tracks)
            {
                 fprintf(stderr, "\nFound track before header!");
                 goto error;
            }
            else if (trackNumber >= mf->nTrks)
            {
                fprintf(stderr, "\nFound more tracks than specified in header!");
                goto error;
            }

            // read the track to buffer
            char* track_buff;
            if( ( track_buff = (char*)malloc(chunklength) ) == NULL )
            {
                fprintf(stderr, "\nError allocating buffer of %u bytes to read track", chunklength);
                goto error;
            }

            if ( (readlen = fread(track_buff, sizeof(char), chunklength, fp)) != chunklength)
            {
                fprintf(stderr, "\nError reading track: read %d of expected %u", readlen, chunklength);
                free(track_buff);
                goto error;
            }

            // actually read the track
            printf("\n\nReading track #%u:", trackNumber);
            MidiTrack_t *track = &mf->Tracks[trackNumber++];
            MidiTrack_Read(track_buff, chunklength, track);
            free(track_buff);
        }
        else
        {
            printf("\nUnknown chunk type %.*s", MIDI_CHUNK_SIZE, chunktype);
            goto error;
        }
    }

    // cleanup and return
    fclose(fp);
    return mf;

    error:
    MidiFile_Close(mf);
    fclose(fp);
    return NULL;
}

int MidiFile_Save(const char* filename, const MidiFile_t *save)
{
    if (filename == NULL || save == NULL)
        return -1;

    FILE* fp;
    if ((fp = fopen(filename, "wb+")) == NULL)
    {
        fprintf(stderr, "\nFailed to open/create MIDI file %s: %d %s", filename, errno, strerror(errno));
        return -1;
    }

    // write header
    {
        uint8_t header_data[MIDI_CHUNK_HEADER_LEN];
        u16toarray(save->Format, &header_data[0]);
        u16toarray(save->nTrks, &header_data[2]);
        u16toarray(save->PulsesPerQuarterNote, &header_data[4]);

        fwrite(MIDI_CHUNK_HEADER, 1, 4, fp); // header name
        fwrite((uint8_t[]){0x00, 0x00, 0x00, 0x06}, 1, 4, fp); // header size
        fwrite(header_data, sizeof(uint8_t), MIDI_CHUNK_HEADER_LEN, fp); // header content
    }

    for (uint16_t t = 0; t < save->nTrks; t++)
    {
        uint32_t trackLen = 0;
        MidiTrack_t* track = &save->Tracks[t];

        long allocSize = 1024;
        char* buffer = (char*)malloc(allocSize);

        for (int e = 0; e < track->NumEvents; e++)
        {
            if (allocSize - trackLen < 128)
                allocSize += 1024, buffer = (char*)realloc(buffer, allocSize);

            MidiEvent_t* event = &track->Events[e];

            //if (event->interface->type != Midi_Event_Type_NoteOn && event->interface->type != Midi_Event_Type_NoteOff)
            //    continue;

            trackLen += WriteVariableLength(&buffer[trackLen], 5, event->deltaTime);
            trackLen += event->interface->write_data(&buffer[trackLen], event);
        }

        uint8_t arrlen[4];
        u32toarray(trackLen, arrlen);

        fwrite(MIDI_CHUNK_TRACK, 1, 4, fp); // chunk tag
        fwrite(arrlen, 1, 4, fp); // chunk length
        fwrite(buffer, sizeof(char), trackLen, fp); // write data
        free(buffer);
    }


    fclose(fp);
    return 0;
}

// TIME MAP
// ===================================================================
uint32_t Midi_MapAbsoluteTime(MidiAbsoluteTimeMap_t** list, const MidiFile_t* midi)
{
    if (*list != NULL)
    {
        fprintf(stderr, "\nMake sure your pointer starts NULL!");
        return -1;
    }

    uint32_t numEvents = 0;
    MidiAbsoluteTimeMap_t* addMap()
    {
        MidiAbsoluteTimeMap_t* new_list = (MidiAbsoluteTimeMap_t*)realloc((*list), sizeof(MidiAbsoluteTimeMap_t)*(numEvents+1));

        if (new_list == NULL)
            return NULL; // failed to realloc - original pointer still good

        *list = new_list; // update old pointer
        numEvents++;

        return &new_list[numEvents - 1];
    }

    int playerCb(MidiEvent_t* event, uint16_t track, uint32_t timeTicks, uint32_t timeUs)
    {
        if ((event->interface->type == Midi_Event_Type_NoteOff) || (event->interface->type == Midi_Event_Type_NoteOn))
        {
            if (!event->data) // sanity check
                goto bail;

            MidiEventData_NoteEvent_t* this_note = (MidiEventData_NoteEvent_t*)event->data;

            if ((this_note->OnOff == Midi_Event_Type_NoteOff) || (this_note->velocity == 0))
            {
                // NOTE OFF
                // find a matching unterminated note event in the list
                for (int i = numEvents - 1; i >= 0; i--)
                {
                    MidiAbsoluteTimeMap_t* match = &((*list)[i]);

                    if (match->OffEvent != NULL)
                        continue;

                    MidiEventData_NoteEvent_t* match_data = (MidiEventData_NoteEvent_t*)match->OnEvent->data;

                    if (match_data == NULL) // sanity check
                        continue;

                    if ((match_data->channel == this_note->channel) && (match->track == track) && (match_data->key == this_note->key) && (match_data->OnOff == Midi_Event_Type_NoteOn))
                    {
                        match->OffEvent = event;
                        match->endTime = timeUs; // found an "unterminated" note on with same key and channel
                        break;
                    }
                }
            }
            else
            {
                // NOTE ON
                MidiAbsoluteTimeMap_t* new_entry = addMap();

                if (new_entry == NULL) // some allocation error
                    goto bail;

                (*new_entry) = (MidiAbsoluteTimeMap_t) {
                    .OnEvent = event,
                    .OffEvent = NULL,
                    .track = track,
                    .startTime = timeUs,
                    .endTime = UINT32_MAX, // we figure out end time when we get the note-off
                };
            }
        }

        bail:
        return Player_Callback_IgnoreEvent;
    }

    MidiFile_Play(midi, UINT32_MAX, playerCb);

    return numEvents;
}

// PLAYER
// ===================================================================

#if defined(unix) || defined(__unix__) || defined(__unix)

// apt-get install fluidsynth; apt-get install libfluidsynth-dev;
// must include "-lfluidsynth" in linker options
#include <fluidsynth.h>

fluid_settings_t* settings;
fluid_synth_t* synth;
fluid_audio_driver_t* adriver;

#define SOUND_FONT_PATH "/usr/share/sounds/sf2/FluidR3_GM.sf2"

#include <unistd.h>
void SLEEPUS(uint32_t us) { usleep(us); }

int MidiDevice_Open()
{
    settings = new_fluid_settings();
    fluid_settings_setstr(settings, "audio.driver", "alsa");

    synth = new_fluid_synth(settings);
    fluid_synth_sfload(synth, SOUND_FONT_PATH, 1);

    adriver = new_fluid_audio_driver(settings, synth);

    return 0;
}

void MidiDevice_Reset()
{
    for (uint8_t ch = 0; ch < 16; ch++)
        fluid_synth_all_sounds_off(synth, ch);
}

void MidiDevice_Close()
{
    delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
}

int MidiDevice_SetChannelInstrument(uint8_t channel, uint8_t instrument)
{
     return (fluid_synth_program_change(synth, channel, instrument) == FLUID_OK) ? 0 : -1;
}

int MidiDevice_PlayNote(uint8_t key, uint8_t channel, uint8_t velocity, uint8_t state)
{
    int result;

    if (state)
        result = fluid_synth_noteon(synth, channel, key, velocity);
    else
        result = fluid_synth_noteoff(synth, channel, key);

    return (result == FLUID_OK) ? 0 : -1;
}

#endif

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__WIN32__)

// must include "-lwinmm" in linker options
#include <windows.h>
#include <mmsystem.h>

HMIDIOUT    midiOutHandle;

void SLEEPUS(uint32_t us)
{
    LARGE_INTEGER freq, start, current;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // because sleep is not precise, let's sleep short of actual value and let performance counter fill-in the rest
    uint32_t sleepfunc = us / 1500;

    if (sleepfunc > 0)
        Sleep(sleepfunc); // never call Sleep with 0 because it will relinquish time slice for unknown amount of time

    // busy-wait remaining microseconds
    do
    {
        QueryPerformanceCounter(&current);
    }
    while ( ((current.QuadPart - start.QuadPart)*1e6)/freq.QuadPart < us );
}

int MidiDevice_Open()
{
    const UINT devid = -1;

    if ( midiOutOpen(&midiOutHandle, devid, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR )
    {
        fprintf(stderr, "\nError opening MIDI output device");
        return -1;
    }

    MIDIOUTCAPS     moc;
    if (!midiOutGetDevCaps(devid, &moc, sizeof(MIDIOUTCAPS)))
    {
        printf("\nOPENED MIDI DEVICE: %s", moc.szPname);
    }

    return 0;
}

void MidiDevice_Reset()
{
    midiOutReset(midiOutHandle); // stop all notes
}

void MidiDevice_Close()
{
    midiOutReset(midiOutHandle); // stop all notes
    midiOutClose(midiOutHandle); // release resources
}

int MidiSendShortMessage(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    union
     {
         uint32_t raw;
         uint8_t data[4];
     } midiMessage;

     midiMessage.data[0] = d0;
     midiMessage.data[1] = d1;
     midiMessage.data[2] = d2;
     midiMessage.data[3] = d3;

     if (midiOutShortMsg(midiOutHandle, midiMessage.raw ) != MMSYSERR_NOERROR)
     {
         fprintf(stderr, "\nError sending midi-message [%.8x]", midiMessage.raw);
         return -1;
     }

     return 0;
}

int MidiDevice_SetChannelInstrument(uint8_t channel, uint8_t instrument)
{
    return MidiSendShortMessage(0xC0 + (channel & 0x0F), instrument, 0, 0);
}

int MidiDevice_PlayNote(uint8_t key, uint8_t channel, uint8_t velocity, uint8_t state)
{
     return MidiSendShortMessage((state ? 0x90 : 0x80) + (channel & 0x0F), key, velocity, 0);
}

//#endif

int trivial_callback(MidiEvent_t* event, uint16_t track, uint32_t timeTicks, uint32_t timeUs)
{
    return Player_Callback_PlayEvent;
}

void MidiFile_Play(const MidiFile_t* midi, uint32_t start_usec, playerCallback cbFunc)
{
    // sanity check
    if (midi == NULL)
        return;

    if (cbFunc == NULL)
        cbFunc = trivial_callback;

    uint32_t waitTicks[midi->nTrks]; // how many ticks before the next event for each track
    uint32_t minimumWaitTicks = 0; // the smallest number in list above

    uint16_t countFinishedTracks = 0; // how many tracks have finished already
    uint16_t currentEvent[midi->nTrks]; // the index of the event queue to play next, for each track

    uint32_t TickDurationUs = 2602; // conversion from tick to actual time

    for (uint16_t trackIndex = 0; trackIndex < midi->nTrks; trackIndex++) // initialization
        currentEvent[trackIndex] = 0, waitTicks[trackIndex] = 0;

    // converting between ticks and seconds is not trivial because "SetTempo" events may change the conversion factor several times along the song
    uint32_t currentTotalTime_Ticks = 0;
    uint32_t currentTotalTime_Usecs = 0;

    while (countFinishedTracks < midi->nTrks)
    {
        // keep track of the time we're waiting
        currentTotalTime_Ticks += minimumWaitTicks;
        currentTotalTime_Usecs += TickDurationUs*minimumWaitTicks;

        if (currentTotalTime_Usecs >= start_usec)
            SLEEPUS(TickDurationUs*minimumWaitTicks); // fast-forward to the start time

        uint32_t decrementTicks = minimumWaitTicks;
        minimumWaitTicks = UINT32_MAX;

        for (uint16_t trackIndex = 0; trackIndex < midi->nTrks; trackIndex++)
        {
            waitTicks[trackIndex] -= decrementTicks;

            if (waitTicks[trackIndex] != 0)
            {
                minimumWaitTicks = min(minimumWaitTicks, waitTicks[trackIndex]);
                continue;
            }

            MidiEvent_t *event = &midi->Tracks[trackIndex].Events[currentEvent[trackIndex]];
            MidiEventType_t type = MidiEvent_GetType(event);

            int cbResult = cbFunc(event, trackIndex, currentTotalTime_Ticks, currentTotalTime_Usecs);

            if (cbResult == Player_Callback_Abort)
                return;

            if ((cbResult == Player_Callback_IgnoreEvent) && (type != Midi_Event_Type_SetTempo)) // we may never ignore tempo events
                goto ignore;

            if ((type == Midi_Event_Type_NoteOn || type == Midi_Event_Type_NoteOff) && (currentTotalTime_Usecs >= start_usec)) // don't play notes when skipping ahead to start time
            {
                MidiEventData_NoteEvent_t* data = (MidiEventData_NoteEvent_t*)event->data;
                MidiDevice_PlayNote(data->key, data->channel, data->velocity, (type == Midi_Event_Type_NoteOn) );
            }
            else if (type == Midi_Event_Type_SetTempo)
            {
                MidiEventData_SetTempo_t* data = (MidiEventData_SetTempo_t*)event->data;

                TickDurationUs = (data->tempo / midi->PulsesPerQuarterNote); // convert to micro-seconds

                //printf("\nSet tick duration = %u us", TickDurationUs);
            }
            else if (type == Midi_Event_Type_ProgramChange)
            {
                MidiEventData_ProgramChange_t* data = (MidiEventData_ProgramChange_t*)event->data;

                MidiDevice_SetChannelInstrument(data->channel, data->program);
                //printf("\nSet instrument Ch=%d P=%d %s", data->channel, data->program, Midi_GetInstrumentName(data->program));
            }

            ignore:
            currentEvent[trackIndex]++;
            if (currentEvent[trackIndex] >= midi->Tracks[trackIndex].NumEvents)
                waitTicks[trackIndex] = UINT32_MAX, countFinishedTracks++;
            else
                waitTicks[trackIndex] = midi->Tracks[trackIndex].Events[currentEvent[trackIndex]].deltaTime;

            minimumWaitTicks = min(minimumWaitTicks, waitTicks[trackIndex]);
        }
    }
}

// ADDITIONAL FEATURES
// ===================================================================

static const char* MIDI_NOTE_NAMES[] = {
    "C",    // C   =   0
    "C#",   // C#  =   1
    "D",    // D   =   2
    "D#",   // D#  =   3
    "E",    // E   =   4
    "F",    // F   =   5
    "F#",   // F#  =   6
    "G",    // G   =   7
    "G#",   // G#  =   8
    "A",    // A   =   9
    "A#",   // A#  =   10
    "B",    // B   =   11
};

const char* Midi_GetKeyName(uint8_t key)
{
    return MIDI_NOTE_NAMES[key % 12];
}

static const char* MIDI_INSTRUMENT_NAMES[] = {
    "Piano", // 0
    "Acoustic Grand Piano", // 1
    "Bright Acoustic Piano", // 2
    "Electric Grand Piano", // 3
    "Honky-tonk Piano", // 4
    "Electric Piano 1 (Rhodes Piano)", // 5
    "Electric Piano 2 (Chorused Piano)", // 6
    "Harpsichord", // 7
    "Clavinet", // 8
    "Celesta", // 9
    "Glockenspiel", // 10
    "Music Box", // 11
    "Vibraphone", // 12
    "Marimba", // 13
    "Xylophone", // 14
    "Tubular Bells", // 15
    "Dulcimer (Santur)", // 16
    "Drawbar Organ (Hammond)", // 17
    "Percussive Organ", // 18
    "Rock Organ", // 19
    "Church Organ", // 20
    "Reed Organ", // 21
    "Accordion (French)", // 22
    "Harmonica", // 23
    "Tango Accordion (Band neon)", // 24
    "Acoustic Guitar (nylon)", // 25
    "Acoustic Guitar (steel)", // 26
    "Electric Guitar (jazz)", // 27
    "Electric Guitar (clean)", // 28
    "Electric Guitar (muted)", // 29
    "Overdriven Guitar", // 30
    "Distortion Guitar", // 31
    "Guitar harmonics", // 32
    "Acoustic Bass", // 33
    "Electric Bass (fingered)", // 34
    "Electric Bass (picked)", // 35
    "Fretless Bass", // 36
    "Slap Bass 1", // 37
    "Slap Bass 2", // 38
    "Synth Bass 1", // 39
    "Synth Bass 2", // 40
    "Violin", // 41
    "Viola", // 42
    "Cello", // 43
    "Contrabass", // 44
    "Tremolo Strings", // 45
    "Pizzicato Strings", // 46
    "Orchestral Harp", // 47
    "Timpani", // 48
    "String Ensemble 1 (strings)", // 49
    "String Ensemble 2 (slow strings)", // 50
    "SynthStrings 1", // 51
    "SynthStrings 2", // 52
    "Choir Aahs", // 53
    "Voice Oohs", // 54
    "Synth Voice", // 55
    "Orchestra Hit", // 56
    "Trumpet", // 57
    "Trombone", // 58
    "Tuba", // 59
    "Muted Trumpet", // 60
    "French Horn", // 61
    "Brass Section", // 62
    "SynthBrass 1", // 63
    "SynthBrass 2", // 64
    "Soprano Sax", // 65
    "Alto Sax", // 66
    "Tenor Sax", // 67
    "Baritone Sax", // 68
    "Oboe", // 69
    "English Horn", // 70
    "Bassoon", // 71
    "Clarinet", // 72
    "Piccolo", // 73
    "Flute", // 74
    "Recorder", // 75
    "Pan Flute", // 76
    "Blown Bottle", // 77
    "Shakuhachi", // 78
    "Whistle", // 79
    "Ocarina", // 80
    "Lead 1 (square wave)", // 81
    "Lead 2 (sawtooth wave)", // 82
    "Lead 3 (calliope)", // 83
    "Lead 4 (chiffer)", // 84
    "Lead 5 (charang)", // 85
    "Lead 6 (voice solo)", // 86
    "Lead 7 (fifths)", // 87
    "Lead 8 (bass + lead)", // 88
    "Pad 1 (new age Fantasia)", // 89
    "Pad 2 (warm)", // 90
    "Pad 3 (polysynth)", // 91
    "Pad 4 (choir space voice)", // 92
    "Pad 5 (bowed glass)", // 93
    "Pad 6 (metallic pro)", // 94
    "Pad 7 (halo)", // 95
    "Pad 8 (sweep)", // 96
    "FX 1 (rain)", // 97
    "FX 2 (soundtrack)", // 98
    "FX 3 (crystal)", // 99
    "FX 4 (atmosphere)", // 100
    "FX 5 (brightness)", // 101
    "FX 6 (goblins)", // 102
    "FX 7 (echoes, drops)", // 103
    "FX 8 (sci-fi, star theme)", // 104
    "Sitar", // 105
    "Banjo", // 106
    "Shamisen", // 107
    "Koto", // 108
    "Kalimba", // 109
    "Bag pipe", // 110
    "Fiddle", // 111
    "Shanai", // 112
    "Tinkle Bell", // 113
    "Agogo", // 114
    "Steel Drums", // 115
    "Woodblock", // 116
    "Taiko Drum", // 117
    "Melodic Tom", // 118
    "Synth Drum", // 119
    "Reverse Cymbal", // 120
    "Guitar Fret Noise", // 121
    "Breath Noise", // 122
    "Seashore", // 123
    "Bird Tweet", // 124
    "Telephone Ring", // 125
    "Helicopter", // 126
    "Applause", // 127
    "Gunshot", // 128
};

const char* Midi_GetInstrumentName(uint8_t program)
{
    if (program > 128)
        return NULL;

    return MIDI_INSTRUMENT_NAMES[program];
}

const MidiTranspositionData_t Midi_Transposition_Table[] = {
                                    /*MAJOR KEY*/
/*00*/     {.sf = 7,   .mi = 0, .tranposeDelta = 1,    .description = "C# MAJ"},  // 7 sharps
/*01*/     {.sf = 6,   .mi = 0, .tranposeDelta = 6,    .description = "F# MAJ"},  // 6 sharps
/*02*/     {.sf = 5,   .mi = 0, .tranposeDelta = 11,   .description = "B MAJ"},   // 5 sharps
/*03*/     {.sf = 4,   .mi = 0, .tranposeDelta = 4,    .description = "E MAJ"},   // 4 sharps
/*04*/     {.sf = 3,   .mi = 0, .tranposeDelta = 9,    .description = "A MAJ"},   // 3 sharps
/*05*/     {.sf = 2,   .mi = 0, .tranposeDelta = 2,    .description = "D MAJ"},   // 2 sharps
/*06*/     {.sf = 1,   .mi = 0, .tranposeDelta = 7,    .description = "G MAJ"},   // 1 sharps
/*07*/     {.sf = 0,   .mi = 0, .tranposeDelta = 0,    .description = "C MAJ"},   // 0
/*08*/     {.sf = -1,  .mi = 0, .tranposeDelta = 5,    .description = "F MAJ"},   // 1 flats
/*09*/     {.sf = -2,  .mi = 0, .tranposeDelta = 10,   .description = "Bb MAJ"},  // 2 flats
/*10*/     {.sf = -3,  .mi = 0, .tranposeDelta = 3,    .description = "Eb MAJ"},  // 3 flats
/*11*/     {.sf = -4,  .mi = 0, .tranposeDelta = 8,    .description = "Ab MAJ"},  // 4 flats
/*12*/     {.sf = -5,  .mi = 0, .tranposeDelta = 1,    .description = "Db MAJ"},  // 5 flats
/*13*/     {.sf = -6,  .mi = 0, .tranposeDelta = 6,    .description = "Gb MAJ"},  // 6 flats
/*14*/     {.sf = -7,  .mi = 0, .tranposeDelta = 11,   .description = "Cb MAJ"},  // 7 flats
                                    /*MINOR KEY*/
/*15*/     {.sf = 7,   .mi = 1, .tranposeDelta = 10,   .description = "A# MIN"},  // 7 sharps
/*16*/     {.sf = 6,   .mi = 1, .tranposeDelta = 3,    .description = "D# MIN"},  // 6 sharps
/*17*/     {.sf = 5,   .mi = 1, .tranposeDelta = 8,    .description = "G# MIN"},  // 5 sharps
/*18*/     {.sf = 4,   .mi = 1, .tranposeDelta = 1,    .description = "C# MIN"},  // 4 sharps
/*19*/     {.sf = 3,   .mi = 1, .tranposeDelta = 6,    .description = "F# MIN"},  // 3 sharps
/*20*/     {.sf = 2,   .mi = 1, .tranposeDelta = 11,   .description = "B MIN"},   // 2 sharps
/*21*/     {.sf = 1,   .mi = 1, .tranposeDelta = 4,    .description = "E MIN"},   // 1 sharps
/*22*/     {.sf = 0,   .mi = 1, .tranposeDelta = 9,    .description = "A MIN"},   // 0
/*23*/     {.sf = -1,  .mi = 1, .tranposeDelta = 2,    .description = "D MIN"},   // 1 flats
/*24*/     {.sf = -2,  .mi = 1, .tranposeDelta = 7,    .description = "G MIN"},   // 2 flats
/*25*/     {.sf = -3,  .mi = 1, .tranposeDelta = 0,    .description = "C MIN"},   // 3 flats
/*26*/     {.sf = -4,  .mi = 1, .tranposeDelta = 5,    .description = "F MIN"},   // 4 flats
/*27*/     {.sf = -5,  .mi = 1, .tranposeDelta = 10,   .description = "Bb MIN"},  // 5 flats
/*28*/     {.sf = -6,  .mi = 1, .tranposeDelta = 3,    .description = "Eb MIN"},  // 6 flats
/*29*/     {.sf = -7,  .mi = 1, .tranposeDelta = 8,    .description = "Ab MIN"},  // 7 flats
};

const MidiTranspositionData_t* Midi_GetKeySignatureTranspositionInfo(const MidiEventData_KeySignature_t* ks)
{
    if (ks == NULL)
        return NULL;

    for (int i = 0; i < sizeof(Midi_Transposition_Table)/sizeof(Midi_Transposition_Table[0]); i++)
        if (Midi_Transposition_Table[i].sf == ks->sf && Midi_Transposition_Table[i].mi == ks->mi)
            return &Midi_Transposition_Table[i];

    return NULL;
}

MidiEventData_KeySignature_t* Midi_GetKeySignature(const MidiFile_t* midi)
{
    if (midi == NULL)
        return NULL;

    for (uint16_t t = 0; t< midi->nTrks; t++)
    {
        for (uint32_t e = 0; e < midi->Tracks[t].NumEvents; e++)
        {
            MidiEvent_t* event = &midi->Tracks[t].Events[e];

            if (MidiEvent_GetType(event) == Midi_Event_Type_KeySignature)
            {
                return ((MidiEventData_KeySignature_t*)event->data);
            }
        }
    }

    return NULL;
}

int8_t Midi_Transpose(MidiFile_t* file, const MidiTranspositionData_t* newKey)
{
    if (file == NULL)
    {
        error:
        fprintf(stderr, "Midi_Transpose was passed a NULL pointer!");
        return 0;
    }

    MidiEventData_KeySignature_t* midi_ks = Midi_GetKeySignature(file); // get the midi current key signature
    const MidiTranspositionData_t* oldKey = Midi_GetKeySignatureTranspositionInfo(midi_ks); // get the matching entry from table

    if (newKey == NULL || oldKey == NULL)
        goto error;

    if (newKey->mi != oldKey->mi)
    {
        fprintf(stderr, "\nCannot transpose Major <-> Minor or vice-versa!");
        return 0;
    }

    int8_t delta = newKey->tranposeDelta - oldKey->tranposeDelta; // how many octaves must we shift ?

    for (uint16_t t = 0; t< file->nTrks; t++)
    {
        for (uint32_t e = 0; e < file->Tracks[t].NumEvents; e++)
        {
            MidiEvent_t* event = &file->Tracks[t].Events[e];

            uint8_t eventType = MidiEvent_GetType(event);

            if (eventType == Midi_Event_Type_NoteOff || eventType == Midi_Event_Type_NoteOn)
            {
                ((MidiEventData_NoteEvent_t*)event->data)->key += delta;
            }
        }
    }

    // update the key signature event
    midi_ks->mi = newKey->mi;
    midi_ks->sf = newKey->sf;

    return delta;
}

uint8_t Midi_IsSharp(uint8_t key)
{
    uint8_t note = key % 12;

    if (note == 1 /*C#*/ || note == 3/*D#*/ || note == 6/*F#*/ || note == 8/*G#*/ || note == 10/*A#*/)
        return 1;

    return 0;
}
