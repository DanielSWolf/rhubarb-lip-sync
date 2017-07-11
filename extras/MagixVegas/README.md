# Scripts for Magix Vegas

If you own a copy of [Magix Vegas](http://www.vegascreativesoftware.com/) (previously Sony Vegas), you can use this script to visualize Rhubarb Lip Sync's output on the timeline. This can be useful for creating lip-synced videos or for debugging.

Copy (or symlink) the files in this directory to `<Vegas installation directory>\Script Menu`. When you restart Vegas, you'll find two new menu items:

* *Tools > Scripting > Import Rhubarb:* This will create a new Vegas project and add two tracks: a video track with a visualization of Rhubarb Lip Sync's output and an audio track with the original recording.
* *Tools > Scripting > Debug Rhubarb:* This will create markers or regions on the timeline visualizing Rhubarb Lip Sync's internal data from a log file. You can obtain a log file by redirecting `stdout`. I've written this script mainly as a debugging aid for myself; feel free to contact me if you're interested and need a more thorough explanation.