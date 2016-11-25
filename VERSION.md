# Version history

## Version 1.2.0

* **Dialog file needn't be exact**

  Since version 1.0.0, Rhubarb Lip-Sync can handle situations where the dialog text is specified (using the `-dialogFile` option), but the actual recording omits some words. For instance, the specified dialog text can be "That's all gobbledygook to me," but the recording only says "That's gobbledygook to me," dropping the word "all."

  Until now, however, Rhubarb Lip-Sync couldn't handle *changed* or *inserted* words, such as a recording saying "That's *just* gobbledygook to me." This restriction has been removed. As of version 1.2.0, the actual recording may freely deviate from the specified dialog text. Rhubarb Lip-Sync will ignore the dialog file where it audibly differs from the recording, and benefit from it where it matches. 

## Version 1.1.0

* **More reliable speech recognition**

  The first step in automatic lip-sync is speech recognition.
  Rhubarb Lip-Sync 1.1.0 recognizes spoken dialog more accurately, especially at the beginning of recordings.
  This improves the overall quality of the resulting animation.

* **More accurate breath detection**

  Rhubarb Lip-Sync animates not only dialog, but also noises such as taking a breath.
  For this version, the accuracy of breath detection has been improved.
  You shouldn't see actors opening their mouth for no reason any more.

* **Better animation of short pauses**

  During short pauses between words or sentences (up to 0.35s), the mouth is kept open.
  Now, this open mouth shape is chosen based on the previous and following mouth shapes.
  This gives pauses in speech a more natural, less mechanical look.

* **Builds on Linux**

  In addition to Windows and OS X, Rhubarb Lip-Sync can now be built on Linux systems.
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
