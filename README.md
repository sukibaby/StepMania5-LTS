StepMania 5 Long Term Support
========

I am one of the developers of [ITGmania](http://github.com/itgmania/itgmania), a [StepMania 5.1](https://github.com/stepmania/stepmania/tree/5_1-new)-based engine.

The goal of this project is complete compatibility and identical user experience to the final StepMania 5 release, 5.1 beta 2. I've taken care to revert all ITG-oriented code to the original StepMania 5 code.

With development of the original StepMania being stagnant for so long, I wanted to bring back ITGmania's engine improvements, security patches, native support for newer operating systems, and code modernization for the greater StepMania 5 community.

My goal is to keep it maintained and updated for a long time. I hope you enjoy StepMania 5 LTS.

~ sukibaby

## Changes to ITGmania 1.1.0

This project contains all of [ITGmania](http://github.com/itgmania/itgmania)'s improvements, including built-in libusb and hidapi, and GPL licensing, but has the following changes:

- Restored default StepMania 5 theme as default
- Fully reverted ITGmania branding to original StepMania 5 branding
- ITGmania specific features have been removed / fully reverted to StepMania 5's code:
    - GrooveStats integration
    - Early way-off rescoring
    - Other misc small changes
- Bug fix for video playback affecting ITGmania 1.0.0-1.1.0
- Automatically detects the sample rate of the default audio device, and initializes the game with a matching sample rate

## Installation

The installers provided are built by GitHub. Since I don't have access to a Mac, this is necessary for me to be able to provide Mac builds, but it also ensures that the builds come from a trusted source.

### Windows

**Windows 7 64-bit is the minimum supported version.**

 * You will likely have to manually allow the installer to start.
 * ITGmania is 64-bit only, so StepMania 5 LTS is as well.

### macOS

**macOS users need to have macOS 11 (Big Sur) or higher to run StepMania 5 LTS.**
* Move StepMania 5 LTS.app to the Applications folder, and then run the following command in Terminal:

   * `xattr -dr com.apple.quarantine /Applications/SM5-LTS`

* You should then add StepMania 5 LTS to the "Input Monitoring" section of System Preferences (under Security & Privacy)

### Linux

**Linux users should receive all they need from the package manager of their choice.**

* **Debian-based**:

  * `sudo apt install libgdk-pixbuf-2.0-0 libgl1 libglvnd0 libgtk-3-0 libusb-0.1-4 libxinerama1 libxtst6`

* **Fedora-based**:

  * `sudo dnf install gdk-pixbuf2 gtk3 libusb-compat-0.1 libXinerama libXtst`

*  **Arch Linux**:

   * `sudo pacman -S mesa gtk3 libusb-compat libxinerama libxtst llvm-libs`

* **OpenSUSE**:

   * OpenSUSE comes with everything you need pre-installed.


### Build From Source

StepMania 5 LTS can be built for 64-bit with modern build systems, such as Visual Studio 2022, Xcode, or GCC 11+. Generate the build files using [CMake](http://www.cmake.org/), and then use your preferred compiler or IDE. 

More information about using CMake to build StepMania 5 LTS can be found in both the `Build` directory and CMake's documentation.

## Resources

* [ITGmania Website](https://www.itgmania.com/)
* [StepMania 5.1 to ITGmania Migration Guide](Docs/Userdocs/sm5_migration.md)
   - Since StepMania 5 LTS is based off of ITGmania 1.1.0, this is still relevant
* [Lua for ITGmania](https://quietly-turning.github.io/Lua-For-SM5/LuaAPI?engine=ITGmania)
* Lua API Documentation can be found in the Docs folder.

## Licensing Terms

StepMania 5 LTS is under the GPLv3 license, or at your option, any later version. This project is based off of [ITGmania 1.1.0](https://github.com/itgmania/itgmania/tree/v1.1.0), so this project's license is equivalent to that of ITGmania 1.1.0.

For specific information/legalese:

* The original SM5 themes are merged in with this project.
* All of our source code is under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.en.html). 
* Songs included within the 'StepMania 5' folder are under the [<abbr title="Creative Commons Non-Commercial">CC-NC</abbr> license](https://creativecommons.org/).
* The [MAD library](http://www.underbit.com/products/mad/) and [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [GPL license](http://www.gnu.org).
* This software is based in part on the work of the Independent JPEG Group.
* Check the [Docs/Licenses.txt](Docs/Licenses.txt) for the licenses of the used libraries.

### Contributors

- [Club Fantastic](https://wiki.clubfantastic.dance/en/Credits)
- [DinsFire64](https://gist.github.com/DinsFire64/4a3f763cd3033afd55a176980b32a3b5) (Mine Fix)
- [EvocaitArt](https://twitter.com/EvocaitArt) (Enchantment NoteSkin)
- [jenx](https://www.amarion.net/) (Fast Profile Switching)
- [LightningXCE](https://twitter.com/lightningxce) (Cyber NoteSkin)
- [MegaSphere](https://github.com/Pete-Lawrence/Peters-Noteskins) (Note/Rainbow/Vivid NoteSkins)
- [StepMania 5](Docs/credits_SM5.txt)
- [Old StepMania Team](Docs/credits_old_Stepmania_Team.txt)

Of course, this project wouldn't be possible without all the work done on [ITGmania](http://github.com/itgmania/itgmania).