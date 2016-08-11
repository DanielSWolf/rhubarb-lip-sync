# Version history

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