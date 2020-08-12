q2chextex
=====

q2chextex is a command-line tool to determine where in a quake 2 directory a given file is found. Feed it newline-separated files and it will look in your baseq2 folder and optionally, a mod folder. Supports pak files.

Useful for people who maintain Quake 2 clients or mod distributions to make sure no files are missing.

Usage:

``` q2_chextex q2path modpath < filelist.txt``` 

where `filelist.txt` is a list of files separated by newlines, like so:
``` 
textures/e3u1/florr1_8.wal
textures/e3u2/lnum1_2.wal
textures/e3u2/bed3_3.wal
textures/e3u2/lnum1_1.wal
textures/e1u1/dr02_1.wal
textures/e1u1/trigger.wal
```
