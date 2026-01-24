# Hakkun-Example

This branch is a mod for Super Mario Odyssey versions 1.0 to 1.4.1, using [Hakkun](https://github.com/fruityloops1/LibHakkun).

It makes left-stick inputs independent from the current camera angle. By default the left-stick inputs are normal, but they can be made to use this absolute angle by pressing the left stick.

I do this by overriding `PlayerFunction::getPlayerViewMtx` to return an identity matrix if we're using absolute stick angles.

This branch was made for `Intral` on Discord.