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
#include <conio.h>

int player_callback(MidiEvent_t* event, uint16_t track, uint32_t timeTicks, uint32_t timeUs)
{
    if (kbhit())
    {
        getc(stdin);
        return Player_Callback_Abort;
    }

    return Player_Callback_PlayEvent;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "\nUsage: filename.mid\n");
        return -1;
    }

    // load a file
    MidiFile_t* mf = MidiFile_Open(argv[1]);

    // read key signture
    const MidiTranspositionData_t* musicKey = Midi_GetKeySignatureTranspositionInfo(Midi_GetKeySignature(mf));
    if (musicKey != NULL)
    {
        printf("\nThe music key is %s", musicKey->description);

        /* uncomment to transpose */
        //int delta = Midi_Transpose(mf, &Midi_Transposition_Table[7]); // transpose to C major
        //printf("\nTransposition shifted notes by %d semitones", delta);
    }

    // testing saving a file
    MidiFile_Save("./midi/output.mid", mf);

    // test getting time information
    MidiAbsoluteTimeMap_t* list = NULL;
    int count = Midi_MapAbsoluteTime(&list, mf);
    for (int i = 0; i < count; i++)
    {
        /* uncomment to print time-map */
        //printf("\nNote %u start %f end %f", ((MidiEventData_NoteEvent_t*)list[i].OnEvent->data)->key, (float)list[i].startTime/1e6, (float)list[i].endTime/1e6);
    }
    free(list);

    MidiDevice_Open();

    // test playback
    printf("\n\nPress any key to stop");
    MidiFile_Play(mf, 0, player_callback);
    MidiDevice_Close();

    MidiFile_Close(mf);

    return 0;
}
