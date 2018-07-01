# Lightmaps in Mafia

## Author
Originally, credits go to GOLOD55 for his raw lightmap description. More details and this docs
were added by Romop5.
## Briefing

In Mafia, there are two types of shadows:
- dynamic - computed every frame real-time
- static - prebaked using editors

*Dynamic shadows* are casted by player/NPCs and cars with relation to lights.

*Static shadows* (lightmaps) are precreated offline and can be stored in two formats:
* bitmaps - for each texel of mesh, the amount of occlusion (darkness / lightness) is stored
* vertex colour - for each face (triangle) of visual object, there are 3 floats describing the amout of darkness.

Lightmaps are stored in scene2.bin as a part of object definition. The section with lightmaps starts with magic byte *0x40A0*.

## Structure

The following paragraph describes how typical lightmap sections looks like:

1. `Header`

    Marks and comfirms the start of lightmap block.

~~~~c
struct {
    uint16_t magic_byte;    // must equal to 0x40A0
    uint8_t unk[3];         // unk
    uint32_t magic_string;  // ASCII "LMAP"
    uint16_t magin_byte_again; // again ? idk, but that's what I found
    uint16_t unk2; // checksum (according to GOLOD)
    uint16_t unk3;
} Header;
~~~~


2. `Section_Header`

    Consist of only one number: the bitmap where each bit states the presence of lightmap for specified Level of Details level.
For example, bit at possition 1 specifies if lightmap for LOD no. 1 is presented.

    In original Mafia, the number has to match the count of LOD of visual object (frame).
~~~~c
struct {
	uint8_t bitmapOfLevelOfDetails; 
} Section_Header;
~~~~


3. For each level in `Section_Header`: 
    * `General_Data`
    * if BITMAP:
        * `Header_Bitmap`

~~~~c
enum typeOfLightmap = { BITMAP = 0x0,   // lightmap stored as RGBA texels
                        VERTEX = 0x1    // a color value for each vertex in face
                        };
struct {
    uint8_t lightmapVersion;    // only 0x21 is supported by Mafia 
    uint8_t typeOfLightmap;     // usually 0x6 (BMP) / 0x5 (vertex) 
    uint32_t currentLOD;        // the level of currently parsed block ?
    float unkA;                 // somehow related to LOD of visual object
                                // maybe these 2 float relates to distance / blending 
    float unkB;             
    typeOfLightmap type;        // type of stored lightmap, which follows
} General_Data;

struct {
    uint16_t    numberOfVertices;
    uint16_t    numberOfFaceGroups;
    uint16_t    type; ?
} Header_Bitmap;
~~~~

## Example


~~~~Text
00000690        40 0a 00 00 00 4c  4d 41 50 a0 40 79 00 00  |..@....LMAP.@y..|
000006a0  00 01 21 06 01 00 00 00  00 00 00 3f 00 00 34 42  |..!........?..4B|
000006b0  01 04 00 01 00 01 00 01  43 3d 36 ff 01 00 00 00  |........C=6.....|
000006c0  03 00 00 00 03 00 00 00  04 00 00 00 aa 96 7a 3f  |..............z?|
000006d0  00 5f e3 3c 00 2b ad 3c  00 5f e3 3c aa 96 7a 3f  |._.<.+.<._.<..z?|
000006e0  08 e5 78 3f 00 2b ad 3c  08 e5 78 3f 00 00 00 00  |..x?.+.<..x?....|
000006f0  01 00 00 00 02 00 00 00  03 00 00 00 06 00 00 00  |................|
00000700  00 00 03 00 01 00 03 00  00 00 02 00 02 00 00 00  |................|
00000710  00 00 00 00 10 40 9f 00  00 00 11 40 0a 00 00 00  |.....@.....@....|
~~~~


1. Scene2 Bin Lightmap Header

    *40 0a* **00 00 00** ***4c 4d 41 50*** *a0 40* **79 00** 00 00

~~~~c
    struct {
    uint16_t magic_byte = 0x0a40
    uint8_t unk[3] = { 00, 00, 00};
    uint32_t magic_string = 'LMAP';
    uint16_t magin_byte_again = 0x0a40;
    uint16_t unk2 = 0x0079;
    uint16_t unk3 = 0x0;
} Header;
~~~~

2. Count of levels

    01

~~~~c
struct {
	uint8_t bitmapOfLevelOfDetails = 1;
} Section_Header;
~~~~




3. First level 

    *21* **06** 01 00 00 00  ***00 00 00 3f*** **00 00 34 42** 01

~~~~c
enum typeOfLightmap = { BITMAP = 0x0,   // lightmap stored as RGBA texels
                        VERTEX = 0x1    // a color value for each vertex in face
                        };
struct {
    uint8_t lightmapVersion = 0x21;
    uint8_t typeOfLightmap = 0x6;
    uint32_t currentLOD = 0x1;
    float unkA;                 // somehow related to LOD of visual object
                                // maybe these 2 float relates to distance / blending 
    float unkB;             
    typeOfLightmap type;        // type of stored lightmap, which follows
} General_Data;
~~~~

4. Unk level

    04 00

5. Unk level2

    01 *01*

~~~~c
    struct {
        uint16_t unk; // multiplied with 0x1c
        uint16_t faceGroupsCount;
    } vertex_header;
~~~~

    something with face groups

6. Presence flag

    01

    if byte = 1, then next 4 bytes are valid

7. Four bytes (optionial)

    43 3D 36 FF 

    0xFF363D43

8. Count of something

    4 bytes

9. Fixed 8 bytes

    03 00 00 00 *03 00 00 00*

    now there is a flag (isPresented)

    if( not IsPresented)
        read N bytes and fill some structure









