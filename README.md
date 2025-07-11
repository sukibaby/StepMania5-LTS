StepMania 5 LTS
========

I am one of the developers of [ITGmania](http://github.com/itgmania/itgmania), which itself is based on [StepMania 5.1](https://github.com/stepmania/stepmania/tree/5_1-new).

With development of StepMania 5 being stagnant for so long, I wanted to bring back ITGmania's engine improvements for the greater StepMania 5 community, so that people don't have to be reliant on the 32-bit 2018 release. I also see many people who have made their own forks of StepMania  who have all done similar work trying to get it to buid on modern OS's.

The goal of this project is complete compatibility with StepMania 5.1 beta 2. However, in the interest of security, its capability to a open URL via Lua is blocked, and other relevant security fixes in ITGmania are present.

Besides that, it has fully been reverted to StepMania 5 branding, and the default StepMania 5 theme restored.

I hope you enjoy StepMania 5 LTS.

~ sukibaby

## Changes to StepMania 5.1

- Built-in networking and GrooveStats support
- Fully 64-bit, optimized & updated for modern OSes
   - I do not plan to restore 32-bit support myself, but would welcome PR's with this goal
- The mine fix applied (courtesy of [DinsFire64](https://gist.github.com/DinsFire64/4a3f763cd3033afd55a176980b32a3b5))
- Held misses tracked in the engine for pad debugging
- Fixed overlapping hold bug
- Per-player visual delay
- Per-player disabling of timing windows
- New preference to control note render ordering
- Increased the Stats.xml file size limit to 100MB
- Changed the default binding for P2/back from hyphen to backslash
- Greatly improved sync stability
- Fixed sync in gameplay and edit modes when using MP3 audio
- Miscellaneous engine patches, bugfixes and improvements affecting code shared by ITGmania and StepMania 5

## Changes to ITGmania 1.1.0

- Removed Simply Love, restored default StepMania 5 theme as default
- Fully reverted ITGmania branding to original StepMania 5 branding
- Installs to the SM5-LTS directory
- ITGmania engine-based features have been removed:
    - GrooveStats chart hashing
    - Step rescoring
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

StepMania 5 LTS can be compiled using [CMake](http://www.cmake.org/). More information about using CMake to build StepMania 5 LTS can be found in both the `Build` directory and CMake's documentation.

## Resources

* [ITGmania Website](https://www.itgmania.com/)
* [StepMania 5.1 to ITGmania Migration Guide](Docs/Userdocs/sm5_migration.md)
   - Since StepMania 5 LTS is based off of ITGmania 1.1.0, this is still relevant
* [Lua for ITGmania](https://quietly-turning.github.io/Lua-For-SM5/LuaAPI?engine=ITGmania)
* Lua API Documentation can be found in the Docs folder.

## Licensing Terms

StepMania 5 LTS is under the GPLv3 license, or at your option, any later version. This project is based off of [ITGmania 1.1.0](https://github.com/itgmania/itgmania/tree/v1.1.0), so this project's license is equivalent to that of ITGmania 1.1.0.

For specific information/legalese:

* All of our source code is under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.en.html). 
* Songs included within the 'StepMania 5' folder are under the [<abbr title="Creative Commons Non-Commercial">CC-NC</abbr> license](https://creativecommons.org/).
* The [MAD library](http://www.underbit.com/products/mad/) and [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [GPL license](http://www.gnu.org).
* This software is based in part on the work of the Independent JPEG Group.
* Check the [Docs/Licenses.txt](Docs/Licenses.txt) for the licenses of the used libraries.

## Credits

### StepMania 5 LTS Team
- Martin Natano (natano)
- teejusb

### Contributors
- [Club Fantastic](https://wiki.clubfantastic.dance/en/Credits)
- [DinsFire64](https://gist.github.com/DinsFire64/4a3f763cd3033afd55a176980b32a3b5) (Mine Fix)
- [EvocaitArt](https://twitter.com/EvocaitArt) (Enchantment NoteSkin)
- [jenx](https://www.amarion.net/) (Fast Profile Switching)
- [LightningXCE](https://twitter.com/lightningxce) (Cyber NoteSkin)
- [MegaSphere](https://github.com/Pete-Lawrence/Peters-Noteskins) (Note/Rainbow/Vivid NoteSkins)
- [StepMania 5](Docs/credits_SM5.txt)
- [Old StepMania Team](Docs/credits_old_Stepmania_Team.txt)
