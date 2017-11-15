# Contributing to LMMS

Related wiki pages:
* [Coding conventions](https://github.com/LMMS/lmms/wiki/Coding-conventions) 
* [Submitting a patch](https://github.com/LMMS/lmms/wiki/Submitting-a-patch)
* [Labels information](https://github.com/LMMS/lmms/wiki/Labels-Information)

## Posting Issues

### Finding the bug
In your daily endeavors through the mystical paths of music, if you encounter a bug in LMMS, you should first find a valuable way of reproducing it. If there is a non-reproducible glitch, it's going to be hard to fix. If it isn't an issue that affects your workflow in LMMS drastically, you might want to hold off on posting it, since small non-reproducible issues will just clutter up the issue tracker.

Before even looking to make an issue about a problem, __search the issue tracker__ for the problem you experienced. There is a big chance someone (maybe even you!) had the same problem and submitted it. 

### Issue title

After finding the problem you should make an issue for it. Issues should have a clear and readable title. No `/this/` or `Master bug please fix` or `--this--(asdfg)`. Punctuation and correct capitalization are nice things to do. The title should look like a sentence, because it is one.

 __Don't try to put labels in your title.__ If you are not in the developers group you can't assign labels, but when someone from the dev team reads your issue, they'll label it properly. If we don't do that in like a week or so, you can politely poke @Umcaruje or @tresf or someone else from the dev team to do it.

### Issue description

After a nice title, here's what you need to have in your issue:

Most importantly, your issue should be clean. /There/ ___is___ no _need_ to __have__ excessive -formatting-. 

Your issue should have steps to reproduce the issue. When writing the steps,

* You should use lists.
* Lists are much more readable.

1. There are also numbered lists.
1. You can use them too, if you fancy that kind of thing.

If the issue is a graphical bug, you should provide a screenshot or a screen recording of it (preferably as a GIF).

If the issue is a crash, you should go and take a backtrace of it. [We have a tutorial for that here.](https://github.com/LMMS/lmms/wiki/Debugging-LMMS) Note that this is not applicable for windows at the moment, but soon will be.

At the end you should provide your LMMS version, and your OS. If the issue is somehow hardware related (faulty MIDI controller or whatever) provide the name of the hardware that makes that problem.

Nobody needs to know your processor or your motherboard or your RAM. 99% of the time that is not needed, so don't put it there. If it is needed, someone will ask you to provide that info.

An example of a decent bug report is [#1875](https://github.com/LMMS/lmms/issues/1875): if your issue looks similar to that, you did a good job.

### A word or two about enhancement issues and feature requests

When filing enhancement issues, you should also follow the simple rules from above: always search before you make an issue, and have a clean title and description. Note that `Please fix these 146 things` or `Make LMMS work like FL Studio` or `Add this here cause I said so` kinds of issues aren't acceptable and they will be closed.

Your enhancement should be reasonable, and doable in some amount of time. If you just have a vague idea instead of a clear concept, it's better to post on the [forums](https://lmms.io/forum) first, to get feedback from users, and only then come back to the issue tracker when you reach some kind of concept for your idea to be implemented in LMMS.
