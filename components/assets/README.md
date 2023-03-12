# Assets

Assets or Resources could be: characters, objects, sounds, environments, animations or any other element in an application or game which has a more convenient representation. For images this could be, PNG, GIF or JPEG files. For models: glTF, FBX or OBJ files. The amount of different asset file types are endless and we will avoid listing them all.

Vulkan Samples depends on Images and Models. This folder contains all the logic for our asset loaders and the third parties that they use.

## Models

When loading models we turn the models representation into a tree hierarchy based on a models transforms (position, rotation and scale) and inheritance (parent <-> child relationships). 

### glTF

## Images

### STB

### KTX

### ASTC

