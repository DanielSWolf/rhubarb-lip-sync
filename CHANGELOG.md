# Version history

## Version 1.13.0

* **Improved** animation rules for "F" sound when using just the basic mouth shapes.

## Version 1.12.0

* **Added** support for skinning in Rhubarb for Spine ([issue #108](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/108))

## Version 1.11.0

* **Added** support for more WAVE file features ([issue #101](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/101))
* **Changed** Rhubarb Lip Sync for Spine so that it works with any modern JRE ([issue #97](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/97))
* **Changed** Windows build from 32 bit to 64 bit ([issue #98](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/98))

## Version 1.10.0

* **Added** switch data file exporter for Moho (formerly Anime Studio) and OpenToonz ([issue #69](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/69))
* **Added** support for Spine 3.8 beta ([issue #74](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/74))
* **Improved** animation rule for OW sound: animating it as E-F rather than F.

## Version 1.9.1

* **Fixed** segmentation fault on OS X ([issue #65](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/65)).

## Version 1.9.0

* **Added** basic support for non-English recordings through phonetic recognition ([issue #45](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/45)).
* **Improved** processing speed for WAVE files ([issue #58](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/58)).
* **Fixed** a bug that resulted in unwanted mouth movement at beginning of a recording ([issue #53](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/53)).
* **Fixed** a bug that garbled special characters in the output file path ([issue #54](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/54)).
* **Fixed** a bug that prevented the progress bar from reaching 100% ([issue #48](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/48)).
* **Fixed** file paths in exported XML and JSON files ([issue #59](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/59)).

## Version 1.8.0

* **Added** support for Ogg Vorbis (.ogg) file format ([issue #40](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/40)).
* **Fixed** build error with some versions of Boost ([issue #41](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/41)).

## Version 1.7.2

* **Fixed** bug in Rhubarb for Spine where processing failed depending on the number of existing animations ([issue #34](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/34#issuecomment-378198776)).

## Version 1.7.1

* **Added** more helpful error dialogs for internal errors in Rhubarb Lip Sync for Spine.
* **Added**: Internal errors in Rhubarb Lip Sync for Spine are logged to the console (`stderr`).
* **Fixed** generic error message in Rhubarb for Spine ([issue #34](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/34)).

## Version 1.7.0

* **Added** integration with Spine animation software (Rhubarb Lip Sync for Spine).
* **Added** full Unicode support: File names, dialog files, strings in exported files etc. should now be fully Unicode-compatible.
* **Added** `--machineReadable` command-line option to allow for better integration with other applications.
* **Added** `--consoleLevel` command-line option to control how much detail to log to the console (`stderr`).
* **Changed** message output to the console: Unless specified using `--consoleLevel`, only errors and fatal errors are printed to the console. Previously, warnings were also printed.
* **Fixed** segfault with WAVE file containing some initial music before spoken words ([issue #25](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/25))

## Version 1.6.0

* **Added** a script for lip-syncing in Adobe After Effects.
* **Added** `--output` command-line option.
* **Changed** the official spelling of the project: Rhubarb Lip-Sync is now Rhubarb Lip Sync (without the hyphen).

## Version 1.5.0

* **Added** animation code optimizing animation for words containing "to".
* **Improved** animation rules: better animation of ER and AW sounds.
* **Fixed** compilation with Boost 1.56.0+ ([issue #9](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/9)).

## Version 1.4.2

* **Fixed** incorrect animation before some pauses ([issue #7](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/7)).

## Version 1.4.1

* **Fixed** crash with message "Time range start must not be less than end." ([issue #6](https://github.com/DanielSWolf/rhubarb-lip-sync/issues/6))

## Version 1.4.0

* **Added** animation code preventing long static segments.

  Watch yourself in a mirror saying "He seized his keys." Your lips barely moved, right? That's exactly what would happen in previous versions of Rhubarb Lip Sync. Only worse: Because there is only one "clenched teeth" mouth shape, the mouth would stay completely static during phrases like this. Rhubarb Lip Sync 1.4.0 now does what [a professional animator would do](http://animateducated.blogspot.de/2016/10/lip-sync-animation-2.html?showComment=1478861729702#c2940729096183546458): It opens the mouth a bit wider for some syllables, keeping the lips moving. This may be cheating, but it looks much better!

* **Improved** animation rules to use wide-open mouth shape more often.

  Previous versions used mouth shape D (the wide-open mouth) very sparingly. This release uses it more often, which makes the resulting animation more lively and interesting.

## Version 1.3.0

* **Improved** animation algorithm: Implemented new, bidirectional animation algorithm.

  Since version 1.0.0, Rhubarb Lip Sync has used a predictive animation algorithm. That means that in many situations (usually before a vowel), the mouth *anticipates* the upcoming sound. It moves *ahead of time*, resulting in more natural animation.

  For version 1.3.0, this core animation algorithm has been re-written from scratch. The new algorithm still anticipates the *next* vowel, but now also considers the *previous* vowel. The resulting animation is even closer to human speech.

* **Added** artistic timing.

  Previous versions of Rhubarb Lip Sync have tried to reproduce the timing of the recording as precisely as possible. For rapid speech, this often resulted in jittery animation that didn't look good: It tried to fit too much information into the available time. Traditional animators have known this problem since the 1930s. Instead of slavishly following the timing of the recording, they focus on important sounds and mouth shapes, showing them earlier (and thus longer) than would be realistic. On the other hand, they often skip unimportant sounds and mouth shapes altogether.

  Rhubarb Lip Sync 1.3.0 adds a new step in the animation pipeline that emulates this artistic approach. The resulting animation looks much cleaner and smoother. Ironically, it also looks more in-sync than the precise animation created by earlier versions.

* **Added** `--extendedShapes` command-line option.

  Previous versions of Rhubarb Lip Sync used a fixed set of eight or nine mouth shapes for animation. If users wanted to use fewer mouth shapes, they had to modify the output, for instance by replacing every "X" shape with an "A". This version of Rhubarb Lip Sync introduces the `--extendedShapes` command-line option that allows the user to specify which mouth shapes should be used. This is not only more convenient; knowing which mouth shapes are actually available also allows Rhubarb Lip Sync to create better animation.

* **Added** `--quiet` mode.

  A "quiet" mode has been added. In that mode, Rhubarb Lip Sync doesn't create any output except for animation data and error messages. This is helpful when using Rhubarb Lip Sync as part of an automated process.

* **Improved** animation rules and tweening for better animation.

  Animation rules define which mouth shapes can be used to represent a specific sound. For this release, there have been many tweaks to the animation rules, making some sounds look much more convincing. In addition, the rules for inbetweens ("tweening") have been improved. As in traditional animation, the mouth now "pops" open without inbetweens, then closes smoothly.

* **Improved** pause animations.

  Pauses in speech are tricky to animate. Early version of Rhubarb Lip Sync always closed the mouth, which looks strange for very short pauses. Later versions kept the mouth open for short pauses, which can also look weird if the first mouth shape *after* the pause is identical to the mouth shape *during* the pause: It looks as if somebody just forgot to animate that part.

  This version of Rhubarb Lip Sync uses three different strategies for animating pauses, depending on the duration of the pause and the mouth shapes before and after it.

* **Fixed** bugs in the grapheme-to-phoneme algorithm.

  Rhubarb Lip Sync comes with a huge dictionary containing pronunciations for more than 100,000 English words. If the dialog text contains words not found in this dictionary, Rhubarb Lip Sync will try to guess the correct pronunciation. I've fixed several bugs in the G2P algorithm that does this. As a result, using the `--dialogFile` option now results in even better animation.

## Version 1.2.0

* **Improved** dialog handling to allow for incorrect input dialog.

  Since version 1.0.0, Rhubarb Lip Sync can handle situations where the dialog text is specified (using the `-dialogFile` option), but the actual recording omits some words. For instance, the specified dialog text can be "That's all gobbledygook to me," but the recording only says "That's gobbledygook to me," dropping the word "all."

  Until now, however, Rhubarb Lip Sync couldn't handle *changed* or *inserted* words, such as a recording saying "That's *just* gobbledygook to me." This restriction has been removed. As of version 1.2.0, the actual recording may freely deviate from the specified dialog text. Rhubarb Lip Sync will ignore the dialog file where it audibly differs from the recording, and benefit from it where it matches.

## Version 1.1.0

* **Improved** speech recognition to be more reliable.

  The first step in automatic lip sync is speech recognition.
  Rhubarb Lip Sync 1.1.0 recognizes spoken dialog more accurately, especially at the beginning of recordings.
  This improves the overall quality of the resulting animation.

* **Improved** breath detection to be more accurate.

  Rhubarb Lip Sync animates not only dialog, but also noises such as taking a breath.
  For this version, the accuracy of breath detection has been improved.
  You shouldn't see actors opening their mouth for no reason any more.

* **Improved** animation of short pauses.

  During short pauses between words or sentences (up to 0.35s), the mouth is kept open.
  Now, this open mouth shape is chosen based on the previous and following mouth shapes.
  This gives pauses in speech a more natural, less mechanical look.

* **Added** capability to build on Linux

  In addition to Windows and OS X, Rhubarb Lip Sync can now be built on Linux systems.
  I'm not offering binary distributions for Linux at this time.
  To build the application yourself, you need CMake, Boost, and a C++14-compatible compiler.

## Version 1.0.0

* **Improved** animation algorithm: More realistic animation using new, predictive algorithm.
* **Added** tweening for smoother animation.
* **Added** support for non-dialog noises (breathing, smacking, etc.)
* **Improved** processing speed substantially through multithreading.
* **Improved** reliability of voice recognition.
* **Added** support for long recordings (I've tested a 30-minute file).
* **Added** capability to handle recording that deviate from the specified dialog text.
* **Added** capability to handle unknown words as well as numbers, abbreviations, etc. in the specified dialog text.

## Version 0.2.0

* **Added** multiple output formats: TSV, XML, JSON.
* **Added** experimental option to supply dialog text.
* **Improved** error handling and error messages.

## Version 0.1.0

* **Added** two-pass phone detection using [CMU PocketSphinx](http://cmusphinx.sourceforge.net/).
* **Added** fixed set of eight mouth shapes, based on the Hanna-Barbera shapes.
* **Added** naive (but well-tuned) mapping from phones to mouth shapes.
