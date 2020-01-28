Freeverb - Free, studio-quality reverb SOURCE CODE in the public domain
-----------------------------------------------------------------------

Written by Jezar at Dreampoint - http://www.dreampoint.co.uk


Introduction
------------

Hello.

I'll try to keep this "readme" reasonably small. There are few things in the world that I hate more than long "readme" files. Except "coding conventions" - but more on that later...

In this zip file you will find two folders of C++ source code:

"Components" - Contains files that should clean-compile ON ANY TYPE OF COMPUTER OR SYSTEM WHATSOEVER. It should not be necessary to make ANY changes to these files to get them to compile, except to make up for inadequacies of certain compilers. These files create three classes - a comb filter, an allpass filter, and a reverb model made up of a number of instances of the filters, with some features to control the filters at a macro level. You will need to link these classes into another program that interfaces with them. The files in the components drawer are completely independant, and can be built without dependancies on anything else. Because of the simple interface, it should be possible to interface these files to any system - VST, DirectX, anything - without changing them AT ALL.

"FreeverbVST" - Contains a Steinberg VST implementation of this version of Freeverb, using the components in (surprise) the components folder. It was built on a PC but may compile properly for the Macintosh with no problems. I don't know - I don't have a Macintosh. If you've figured out how to compile the examples in the Steinberg VST Development Kit, then you should easilly figure out how to bring the files into a project and get it working in a few minutes. It should be very simple.

Note that this version of Freeverb doesn't contain predelay, or any EQ. I thought that might make it difficult to understand the "reverb" part of the code. Once you figure out how Freeverb works, you should find it trivial to add such features with little CPU overhead.

Also, the code in this version of Freeverb has been optimised. This has changed the sound *slightly*, but not significantly compared to how much processing power it saves.

Finally, note that there is also a built copy of this version of Freeverb called "Freeverb3.dll" - this is a VST plugin for the PC. If you want a version for the Mac or anything else, then you'll need to build it yourself from the code.


Technical Explanation
---------------------

Freeverb is a simple implementation of the standard Schroeder/Moorer reverb model. I guess the only reason why it sounds better than other reverbs, is simply because I spent a long while doing listening tests in order to create the values found in "tuning.h". It uses 8 comb filters on both the left and right channels), and you might possibly be able to get away with less if CPU power is a serious constraint for you. It then feeds the result of the reverb through 4 allpass filters on both the left and right channels. These "smooth" the sound. Adding more than four allpasses doesn't seem to add anything significant to the sound, and if you use less, the sound gets a bit "grainy". The filters on the right channel are slightly detuned compared to the left channel in order to create a stereo effect.

Hopefully, you should find the code in the components drawer a model of brevity and clarity. Notice that I don't use any "coding conventions". Personally, I think that coding conventions suck. They are meant to make the code "clearer", but they inevitably do the complete opposite, making the code completely unfathomable. Anyone whose done Windows programming with its - frankly stupid - "Hungarian notation" will know exactly what I mean. Coding conventions typically promote issues that are irrelevant up to the status of appearing supremely important. It may have helped back people in the days when compilers where somewhat feeble in their type-safety, but not in the new millenium with advanced C++ compilers.

Imagine if we rewrote the English language to conform to coding conventions. After all, The arguments should be just as valid for the English language as they are for a computer language. For example, we could put a lower-case "n" in front of every noun, a lower-case "p" in front of a persons name, a lower-case "v" in front of every verb, and a lower-case "a" in front of every adjective. Can you imagine what the English language would look like? All in the name of "clarity". It's just as stupid to do this for computer code as it would be to do it for the English language. I hope that the code for Freeverb in the components drawer demonstrates this, and helps start a movement back towards sanity in coding practices.


Background
----------

Why is the Freeverb code now public domain? Simple. I only intended to create Freeverb to provide me and my friends with studio-quality reverb for free. I never intended to make any money out of it. However, I simply do not have the time to develop it any further. I'm working on a "concept album" at the moment, and I'll never finish it if I spend any more time programming. 

In any case, I make more far money as a contract programmer - making Mobile Internet products - than I ever could writing plugins, so it simply doesn't make financial sense for me to spend any more time on it.

Rather than give Freeverb to any particular individual or organisation to profit from it, I've decided to give it away to the internet community at large, so that quality, FREE (or at the very least, low-cost) reverbs can be developed for all platforms.

Feel free to use the source code for Freeverb in any of your own products, whether they are also available for free, or even if they are commercial - I really don't mind. You may do with the code whatever you wish. If you use it in a product (whether commercial or not), it would be very nice of you, if you were to send me a copy of your product - although I appreciate that this isn't always possible in all circumstances.

HOWEVER, please don't bug me with questions about how to use this code. I gave away Freeverb because I don't have time to maintain it. That means I *certainly* don't have time to answer questions about the source code, so please don't email questions to me. I *will* ignore them. If you can't figure the code for Freeverb out - then find somebody who can. I hope that either way, you enjoy experimenting with it.


Disclaimer
----------

This software and source code is given away for free, without any warranties of any kind. It has been given away to the internet community as a free gift, so please treat it in the same spirit.


I hope this code is useful and interesting to you all!
I hope you have lots of fun experimenting with it and make good products!

Very best regards,
Jezar.
Technology Consultant
Dreampoint Design and Engineering
http://www.dreampoint.co.uk


//ends
