As of May 11, 2023, this is repo just a dump of (most of) the project directory. I will gradually be cleaning it all up, organizing, writing better comments and documentation, etc.

# Electric Pentacle Machine
This is an attempt to "make a game from scratch": a graphics library (Zigil), a game engine (Pentacle Engine), and an editor (Pentacle Editor) for making game assets, and an actual game. Collectively I refer to these as the Electric Pentacle Machine.

# What does the name mean?
The name "Electric Pentacle Machine" is just a cool-sounding name based on a series of short horror stories by William Hope Hodgson, featuring a paranormal investigator named Carnacki who uses an eletricity-enhanced "pentacle" (think "pentagram", or an encircled five-pointed star) as a tool of his trade. I chose this name because it fits the horror aesthetic of many games I like, and because Google turned up almost no results for it, so it's quite unique.

# What is the point?
When I was a kid I liked playing around in Unreal Editor 2.0, making bad modifications to the good maps that came with Unreal Tournament (1999). I didn't understand most of the tools in the editor but I could make box shaped rooms, cylinders, stairs, I could change wall textures, and I could place lights at random places until it looked okay. But what was BSP? What was CSG? What was "ambient occlusion"? Lightmaps? Pathnodes? "Extrude to bevel"? It all seemed impossibe to comprehend. Twenty years later I still don't understand most of these concepts, but now I am prepared to learn. And what better way to learn than to create? 

# What is the plan?
This is an open-ended hobby project. As such, I don't know where it is going in the long term. I'd be satisfied if I could make an editor superficially similar to the Unreal Editor 2.0 of my childhood. I'd like to be able to design a game-world by placing and modifying geometric "brushes", which then get processed and transformed into an actual playable game-world.

# Notable features
- Written in C99.
- Software rasterization.
- Support for *binary space partitioning* to aid in rendering and collision (collision not implemented yet).

# Notable antifeatures
- No floating point types in the code (well... almost).
- No GPU support.

# Major TODO Items
- Make it easier for other people to build the editor from source. No Makefile is yet included in the repo because what I currently use is dependent on my own system's file structure.

# Links
Discord: https://discord.gg/KUmMRdd4bM
Youtube: https://youtube.com/@SmickEDibbly
