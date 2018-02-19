# Version history

## Version 1.7.0

* Integration with Spine animation software
* Full Unicode support. File names, dialog files, strings in exported files etc. should now be fully Unicode-compatible.
* Added `--machineReadable` command-line option to allow for better integration with other applications.
* Added `--consoleLevel` command-line option to control how much detail to log to the console (`stderr`).
* Unless specified using `--consoleLevel`, only errors and fatal errors are printed to the console. Previously, warnings were also printed.
* Fixed [issue #25](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/25): Segfault with WAVE file containing some initial music before spoken words

## Version 1.6.0

* Added a script for lip-syncing in Adobe After Effects.
* Added `--output` command-line option.
* Dropped the hyphen: Rhubarb Lip-Sync is now Rhubarb Lip Sync.

## Version 1.5.0

* Improved animation rules for ER and AW sounds
* Optimized animation for words containing "to"
* Fixed [issue #9](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/9): Fails to compile with Boost 1.56.0+

## Version 1.4.2

* Fixed [issue #7](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/7): Incorrect animation before some pauses

## Version 1.4.1

* Fixed [issue #6](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/6): Crash with message "Time range start must not be less than end."

## Version 1.4.0

* **Preventing long static segments**

  Watch yourself in a mirror saying "He seized his keys." Your lips barely moved, right? That's exactly what would happen in previous versions of Rhubarb Lip Sync. Only worse: Because there is only one "clenched teeth" mouth shape, the mouth would stay completely static during phrases like this. Rhubarb Lip Sync 1.4.0 now does what [a professional animator would do](http://animateducated.blogspot.de/2016/10/lip-sync-animation-2.html?showComment=1478861729702#c2940729096183546458): It opens the mouth a bit wider for some syllables, keeping the lips moving. This may be cheating, but it looks much better!

* **Using wide-open mouth shape more often**

  Previous versions used mouth shape D (the wide-open mouth) very sparingly. This release uses it more often, which makes the resulting animation more lively and interesting.

## Version 1.3.0

* **New, bidirectional animation algorithm**

  Since version 1.0.0, Rhubarb Lip Sync has used a predictive animation algorithm. That means that in many situations (usually before a vowel), the mouth *anticipates* the upcoming sound. It moves *ahead of time*, resulting in more natural animation.

  For version 1.3.0, this core animation algorithm has been re-written from scratch. The new algorithm still anticipates the *next* vowel, but now also considers the *previous* vowel. The resulting animation is even closer to human speech.

* **Artistic timing**

  Previous versions of Rhubarb Lip Sync have tried to reproduce the timing of the recording as precisely as possible. For rapid speech, this often resulted in jittery animation that didn't look good: It tried to fit too much information into the available time. Traditional animators have known this problem since the 1930s. Instead of slavishly following the timing of the recording, they focus on important sounds and mouth shapes, showing them earlier (and thus longer) than would be realistic. On the other hand, they often skip unimportant sounds and mouth shapes altogether.

  Rhubarb Lip Sync 1.3.0 adds a new step in the animation pipeline that emulates this artistic approach. The resulting animation looks much cleaner and smoother. Ironically, it also looks more in-sync than the precise animation created by earlier versions.

* **Tweaks to the animation rules and tweening**

  Animation rules define which mouth shapes can be used to represent a specific sound. For this release, there have been many tweaks to the animation rules, making some sounds look much more convincing. In addition, the rules for inbetweens ("tweening") have been improved. As in traditional animation, the mouth now "pops" open without inbetweens, then closes smoothly.

* **Improved pause animations**

  Pauses in speech are tricky to animate. Early version of Rhubarb Lip Sync always closed the mouth, which looks strange for very short pauses. Later versions kept the mouth open for short pauses, which can also look weird if the first mouth shape *after* the pause is identical to the mouth shape *during* the pause: It looks as if somebody just forgot to animate that part.

  This version of Rhubarb Lip Sync uses three different strategies for animating pauses, depending on the duration of the pause and the mouth shapes before and after it.

* **`--extendedShapes` command-line option**

  Previous versions of Rhubarb Lip Sync used a fixed set of eight or nine mouth shapes for animation. If users wanted to use fewer mouth shapes, they had to modify the output, for instance by replacing every "X" shape with an "A". This version of Rhubarb Lip Sync introduces the `--extendedShapes` command-line option that allows the user to specify which mouth shapes should be used. This is not only more convenient; knowing which mouth shapes are actually available also allows Rhubarb Lip Sync to create better animation.

* **`--quiet` mode**

  A "quiet" mode has been added. In that mode, Rhubarb Lip Sync doesn't create any output except for animation data and error messages. This is helpful when using Rhubarb Lip Sync as part of an automated process.

* **Fixes to the grapheme-to-phoneme algorithm**

  Rhubarb Lip Sync comes with a huge dictionary containing pronunciations for more than 100,000 English words. If the dialog text contains words not found in this dictionary, Rhubarb Lip Sync will try to guess the correct pronunciation. I've fixed several bugs in the G2P algorithm that does this. As a result, using the `--dialogFile` option now results in even better animation.

## Version 1.2.0

* **Dialog file needn't be exact**

  Since version 1.0.0, Rhubarb Lip Sync can handle situations where the dialog text is specified (using the `-dialogFile` option), but the actual recording omits some words. For instance, the specified dialog text can be "That's all gobbledygook to me," but the recording only says "That's gobbledygook to me," dropping the word "all."

  Until now, however, Rhubarb Lip Sync couldn't handle *changed* or *inserted* words, such as a recording saying "That's *just* gobbledygook to me." This restriction has been removed. As of version 1.2.0, the actual recording may freely deviate from the specified dialog text. Rhubarb Lip Sync will ignore the dialog file where it audibly differs from the recording, and benefit from it where it matches.

## Version 1.1.0

* **More reliable speech recognition**

  The first step in automatic lip sync is speech recognition.
  Rhubarb Lip Sync 1.1.0 recognizes spoken dialog more accurately, especially at the beginning of recordings.
  This improves the overall quality of the resulting animation.

* **More accurate breath detection**

  Rhubarb Lip Sync animates not only dialog, but also noises such as taking a breath.
  For this version, the accuracy of breath detection has been improved.
  You shouldn't see actors opening their mouth for no reason any more.

* **Better animation of short pauses**

  During short pauses between words or sentences (up to 0.35s), the mouth is kept open.
  Now, this open mouth shape is chosen based on the previous and following mouth shapes.
  This gives pauses in speech a more natural, less mechanical look.

* **Builds on Linux**

  In addition to Windows and OS X, Rhubarb Lip Sync can now be built on Linux systems.
  I'm not offering binary distributions for Linux at this time.
  To build the application yourself, you need CMake, Boost, and a C++14-compatible compiler.

## Version 1.0.0

* More realistic animation using new, predictive algorithm
* Smoother animation due to tweening
* Support for non-dialog noises (breathing, smacking, etc.)
* Substantial speed improvement through multithreading
* More reliable voice recognition
* Support for long recordings (I've tested a 30-minute file)
* Recording may deviate from specified dialog text
* Specified dialog text may contain unknown words as well as numbers, abbreviations, etc.

## Version 0.2.0

* Multiple output formats: TSV, XML, JSON
* Experimental option to supply dialog text
* Improved error handling and error messages

## Version 0.1.0

* Two-pass phone detection using [CMU PocketSphinx](http://cmusphinx.sourceforge.net/)
* Fixed set of eight mouth shapes, based on the Hanna-Barbera shapes
* Naive (but well-tuned) mapping from phones to mouth shapes
