# Hakkun-Example

This branch is a mod for Super Mario Odyssey versions 1.0 to 1.4.1, using [Hakkun](https://github.com/fruityloops1/LibHakkun).

It overrides the call to `al::initActorWithArchiveName` in `GrowPlantSeed::init` to allow changing the `GrowPlantSeed`'s `ModelName` in the stage file.
The same method might work for every actor using this function, but more testing needs to be done.
I would like to check all of the game's stage files to see if any objects whose classes don't check `ModelName` have it set to anything.

This branch was made for `theblazingspud` on Discord.


