
Tuning Difference Plugin
========================

### A Vamp plugin to calculate comparative tunings of audio recordings

 * Home page and downloads: https://code.soundsoftware.ac.uk/projects/tuning-difference
 * Linux and macOS CI build: [![Build Status](https://travis-ci.org/cannam/tuning-difference.svg?branch=master)](https://travis-ci.org/cannam/tuning-difference)
 * Windows CI build: [![Build status](https://ci.appveyor.com/api/projects/status/62xidu404phrkw05?svg=true)](https://ci.appveyor.com/project/cannam/tuning-difference)

This [Vamp plugin](http://vamp-plugins.org/) estimates the tuning
frequency ("concert-A") of one or more recordings, by comparing
against with a "reference" recording of the same music that has a
known tuning frequency. It was designed for the purpose of identifying
the tuning frequency of an unknown recording in difficult cases, such
as where A is tuned more than a semitone below 440Hz.

The reference could be another performance made at a known tuning
frequency or, for example, an audio rendering from MIDI at A=440Hz.

For the plugin to establish the tuning frequency correctly, the two
recordings must be in the same key. Alternatively, you can use this
plugin to estimate the pitch difference between two recordings played
in different keys - where the result will consist of the difference
attributable to key plus any difference in tuning.

The plugin expects to receive two or more different recordings of the
same piece of music as its channels of input; the first channel will
be taken as the reference, and the remaining channels will be
individually compared against it. (You can provide this channel layout
using the `--multiplex` option in Sonic Annotator, for example.)  If
you feed it a single piece of music, you won't get anything
worthwhile.

### Example

Example usage from the command line using [Sonic
Annotator](https://vamp-plugins.org/sonic-annotator/):

```
$ export VAMP_PATH=.  # if running from the build directory

$ sonic-annotator -m -d vamp:tuning-difference:tuning-difference:tuningfreq PreludeInCMajorBWV846.mp3 BWV846Egarr.mp3 -w csv --csv-stdout --csv-omit-filename
```

Here the first file is a MIDI rendering using a piano sample at 440Hz,
and the second is a harpsichord recording tuned with A=397Hz. After
processing, this prints

```
0.000000000,397.009
```

### Author and licence

Written by Chris Cannam at the Centre for Digital Music, Queen Mary
University of London. Copyright 2015-2019 QMUL. Published under the
GPL v2, see the file COPYING for details.

