Spliter
------

This program splits the keyboard and alows you to play two instruments same time. You can use this program with ZynAddSubFX or any other synthesizer.
This requires ALSA 0.9.x.

To compile it, run "make".
If you want to use with ZynAddSubFX send the midi events thru Spliter with aconnect like this:

 - connect the keyboard port to "Spliter IN" port
 - connect the "Spliter OUT" to ZynAddSubFX
 - change the midi channels that you want to play. Be sure that the both output channels are enabled and receive NoteOn in ZynAddSubFX.
 
If you change some settings from Spliter while you are playing to keyboard you may ecounter "stucked keys". To clear all theese press to "Panic" button from ZynAddSubFX.

