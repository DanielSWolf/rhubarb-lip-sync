# Rhubarb Lip-Sync

[Rhubarb Lip-Sync](https://github.com/DanielSWolf/rhubarb-lip-sync) is a command-line tool that automatically creates mouth animation from voice recordings. You can use it for characters in computer games, in animated cartoons, or in any other project that requires animating mouths based on existing recordings.

Right now, Rhubarb Lip-Sync produces files in XML format (a special text format). If you're a programmer, this makes it easy for you to use the output in whatever way you like. If you're not a programmer, however, there is currently no direct way to import the result into your favorite animation tool. If this is what you need, feel free to [create an issue](https://github.com/DanielSWolf/rhubarb-lip-sync/issues) telling me what tool you're using. I might add support for a few popular animation tools in the future.

## Mouth shapes

At the moment, Rhubarb Lip-Sync uses a fixed set of eight mouth shapes, named from A-H. These mouth shapes are based on the six mouth shapes (A-F) originally developed at the Hanna-Barbera studios for classic shows such as Scooby-Doo and The Flintstones.

| Name | Image | Description |
| ---- | ----- | ----------- |
| A | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/A.png) | Closed mouth for rest position and the *P*, *B*, and *M* sounds. |
| B | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/B.png) | Slightly open mouth with clenched teeth. Used for most consonants as well as the *EE* sound in b**ee** or sh**e**. |
| C | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/C.png) | Open mouth for the vowels *EH* as in r**e**d, m**e**n; *IH* as in b**i**g, w**i**n; *AH* as in b**u**t, s**u**n, **a**lone; and *EY* as in s**a**y, **e**ight. |
| D | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/D.png) | Wide open mouth for the vowels *AA* as in f**a**ther; *AE* as in **a**t, b**a**t; *AY* as in m**y**, wh**y**, r**i**de; and *AW* as in h**o**w, n**o**w. |
| E | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/E.png) | Slightly rounded mouth for the vowels *AO* as in **o**ff, f**a**ll; *UH* as in sh**ou**ld, c**ou**ld; *OW* as in sh**o**w, c**o**at; and *ER* as in h**er**, b**ir**d. |
| F | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/F.png) | Small rounded mouth for *UW* as in y**ou**, n**ew**; *OY* as in b**o**y, t**o**y; and *W* as in **w**ay. |
| G | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/G.png) | Biting the lower lip for the *F* and *V* sounds. |
| H | ![](http://sunewatts.dk/lipsync/lipsync/img/adam/H.png) | The *L* sound with the tongue slightly visible. |

## How to run Rhubarb Lip-Sync

Rhubarb Lip-Sync is a command-line tool that is currently available for Windows and OS X.

* Download the [latest release](https://github.com/DanielSWolf/rhubarb-lip-sync/releases) and unzip the file anywhere on your computer.
* Call `rhubarb`, passing it a WAVE file as argument, and redirecting the output to a file. This might look like this: `rhubarb my-recording.wav > output.xml`.
* Rhubarb Lip-Sync will analyze the sound file and print the result to `stdout`. If you've redirected `stdout` to a file like above, you will now have an XML file containing the lip-sync data.

## How to use the output

The output of Rhubarb Lip-Sync is an XML file containing information about the sounds in the recording and -- more importantly -- the resulting mouth shapes. A (shortened) sample output looks like this:

```xml
<?xml version="1.0" encoding="utf-8"?>
<rhubarbResult>
  <info>
    <soundFile>C:\...\my-recording.wav</soundFile>
  </info>
  <phones>
    <phone start="0.00" duration="0.08">None</phone>
    <phone start="0.08" duration="0.16">M</phone>
    <phone start="0.24" duration="0.15">AA</phone>
    <phone start="0.39" duration="0.04">R</phone>
    ...
  </phones>
  <mouthCues>
    <mouthCue start="0.00" duration="0.24">A</mouthCue>
    <mouthCue start="0.24" duration="0.15">F</mouthCue>
    <mouthCue start="0.39" duration="0.04">B</mouthCue>
    <mouthCue start="0.43" duration="0.14">H</mouthCue>
    <mouthCue start="0.57" duration="0.22">C</mouthCue>
    ...
  </mouthCues>
</rhubarbResult>
```

* The `info` element tells you the name of the original sound file.
* The `phones` element contains the individual sounds found in the recording. You won't usually need them.
* The `mouthCues` element tells you which mouth shape needs to be displayed at what time interval. The `start` and `duration` values are in seconds. There are no gaps between mouth cues, so `entry.start` + `entry.duration` = `nextEntry.start`.

## Limitations

Rhubarb Lip-Sync has some limitations you should be aware of.

### Only English dialog

Rhubarb Lip-Sync only produces good results when you give it recordings in English. You'll get best results with American English.

### Fixed set of mouth shapes

Right now, Rhubarb Lip-Sync uses a fixed set of eight mouth shapes, as shown above. If you want to use fewer shapes, you can apply a custom mapping in your own code.

I'm planning to make the mouth animation logic more flexible (and even better-looking) in future releases.

## Tell me what you think!

Right now, Rhubarb Lip-Sync is very much work in progress. If you need help or have any suggestions, feel free to [create an issue](https://github.com/DanielSWolf/rhubarb-lip-sync/issues).
