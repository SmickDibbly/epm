# EPM
This is a hobby project with a very loosely defined goal of "make a program for designing interesting spaces." As 2023-07-09, EPM is best described as a mesh viewer with limited editing ability.

All platform-dependent code is encapsulated in a different project called Zigil (Zed's Intermediary Graphics and Input Library).

EPM is provided under the MIT license. See LICENSE.txt in the repository root directory for details.

I discourage the use of EPM for or in association with NFTs, Web3.0, cryptocurrency, machine learning, AI art, or AI-aided image generation, including promotional material. I may choose to expressly forbid such use in future versions of the license.

# What does EPM stand for?
EPM stands for "Electric Pentacle Machine", which is just a cool-sounding name based on a series of short horror stories by William Hope Hodgson, featuring a paranormal investigator named Carnacki who uses an eletricity-enhanced "pentacle" (think pentagram) as a tool of his trade. I chose this name because it fits the magical aesthetic of many games I like while still having a modern element to it, but this doesn't imply any particular theme or style upon EPM software. I will likely force-fit a new meaning to EPM when I think of one.

# What is the point?
- I enjoy programming.
- I like large, long-term projects that require a tremendous effort.
- I am curious to find out how far the NIH mindset can really take me.

# What is the plan?
This is an open-ended hobby project. As such, I don't know where it is going in the very long term. It would be nice if I could synthesize my favorite features of Unreal Editor 2.0, DoomBuilder, and Emacs.

# Notable states
- Lanuage: C99.
- Graphics: software rasterization (CPU)
- Support for *binary space partitioning* to aid in rendering (and eventually collision and more!).
- Basic windowing system.

# Notable constraints
- Integer-only (i.e. fixed-point) math.*
- No math.h
- No GPU support.

* As of 2023-07-09 there are a few places where floating types are used to help in debugging, but I intend to end this practice.

# Building
These instructions are for linux-kernel OSes. Only tested on Ubuntu as of 2023-07-07.

You need both Zigil and EPM. Ensure that the root directories for both Zigil and EPM are in the same directory. Ensure that the Zigil directory is called "zigil" and the EPM directory is called "epm".

In short, starting from zigil/build/:

```Shell
make --file=linux.mk
make --file=linux.mk install
cd ../../epm/build/
make --file=linux.mk
./epm
```

A longer explanation follows. From the zigil/build/ directory, run the shell command:

```Shell
make --file=linux.mk
```

Optionally, check that the file zigil/build/linux/libzigil.a has been created. To make Zigil available to EPM, it should be enough to run the shell command from zigil/build/:

```Shell
make --file=linux.mk install
```

which copies the libzigil.a and the Zigil API header files to a special subdirectory of the root EPM directory. Finally, change working directory to epm/build and run:

```Shell
make --file=linux.mk
```

If this has worked, you should now be able to run EPM simply with:

```Shell
./epm
```

# Links
Discord: https://discord.gg/KUmMRdd4bM
Youtube: https://youtube.com/@SmickEDibbly
