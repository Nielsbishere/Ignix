# Igx: Ignis extended
Extensions of Ignis to handle resources more easily; such as easier refcount and managing UI.

For better compilation,  use clangcl as a toolchain when compiling on windows (`-G "Visual Studio 16 2019" -A x64 -T clangcl`).

![](https://github.com/Nielsbishere/ignix/workflows/C%2FC++%20CI/badge.svg)



## Windows

### User setup: font rendering properly for BGR screens

Some monitors have a different physical layout for the subpixels, one of those is a BGR screen. Since the order is reversed as opposed to RGB, the text won't look right.

Unfortunately, monitors don't have the ability to report their physical color layout. This can make font rendering difficult (and not look right). Luckily, using "Adjust ClearType text" built-in to windows, you can define how fonts are rendered and they will all change (including the igx text rendering).

