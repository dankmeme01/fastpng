# FastPNG

## Mod is obsolete, was merged into a general optimization mod called [Blaze](https://github.com/dankmeme01/blaze)

Some of the code here is awful and dirty so please just visit blaze's repository if you're interested in how this is implemented lol

## How does it work?

By default, cocos2d-x (and by proxy Geometry Dash) uses libpng to decode PNG files. While it's not necessarily a slow library, there are definitely better options out there. Specifically, this mod utilizes [spng](https://github.com/randy408/libspng) and [fpng](https://github.com/richgel999/fpng) (slightly modified to suit my needs).

### Why use 2 different libraries?

While `fpng` is the fastest library out of 3 *by a long shot*, it can only decode images written with `fpng` itself. This means that when you launch the mod for the first time, it has to perform the conversion, loading all resources with `spng` and then storing them to the cache with `fpng`.

### Will this take up additional disk space?

Yes. However, the mod caches resources in a smart way. It does not convert every single `.png` file in the Resources folder, but rather it only converts a file once it's first accessed. For example, this means that if you play on medium settings, you don't have to worry about copies of high-detail textures taking up your disk space. Additionally, there's a way to clear the cache fully from within the game.

### What resources are cached?

All of them, including resources from other mods and texture packs. The only exception is if the mod uses a non-standard way of loading resources, such as `CCImage::initWithImageData` (for example images from Level Thumbnails).

### Will my resources break after a GD update?

No. Rather than relying on filenames, cached images are identified using the CRC32 checksum of the image data. Updating the game or applying texture packs should never cause any issues.

### Relative benchmarks

Loading GJ_GameSheet03-uhd.png 32 times on my PC

* libpng: 3.487s
* spng: 3.373s
* fpng: 2.373s

