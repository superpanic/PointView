# Build and run:
1. Download the Illustrator SDK  
`https://console.adobe.io/downloads/ai`

2. Place project in the `samplecode` folder

3. Open Illustrator. Go to `Preferences/Plug-Ins & Scratch Disks` and set the additional plug-in:s folder to `SDK/samplecode/output`

4. Build project

5. Restart Illustrator

# Development Notes:

# Additional preperation needed for HelloWorld project

## Add Core Foundation
-> Targets  
-> Build Phases  
-> Link Binary with Libraries  
Add (+): CoreFoundation.framework

## The Rez Error: "Command Rez failed with a nonzero exit code"
-> Project  
-> Build settings  
-> Search Paths  
Set Rez Search Path to: `./../common/mac`

## When creating the Target (page 36), set the bundle extension to `aip` in the save dialogue  

## Set Wrapper Extension to aip
-> Targets  
-> Build Settings  
-> Packaging  
-> Wrapper Extension  
(add (+) `Wrapper Extension` if property is missing!)  
Set property value to: `aip`


# Plug-in notifiers and timers
Plug-in notifiers and timers are used by a plug-in to have Illustrator inform it of certain events.
A notifier plug-in is notified when the state of an Illustrator document changes. For example, **a plug-in may request to be notified when the selection state changes.** A notifier plug-in registers for one or more notifications during start-up.
A timer plug-in is notified at regular time intervals. For example, a timer plug-in may request to be notified
once a second.
For more information, see AINotifierSuite and AITimerSuite in Adobe Illustrator API Reference.

# Notifiers
Some message actions also are referred to as notifiers, indicating something in Illustrator was changed by the user; **for example, when the user selects an object.**
Plug-ins must register for the notifiers in which they are interested. The Notifier suite is used to register and remove notification requests (see AINotifierSuite).

# Suite examples

**struct AIPathSuite**
This suite provides functions that allow you to examine and manipulate paths, which are art objects of type kPathArt. More...

**struct AIPathConstructionSuite**
Utilities to convert paths to and from line segments. More...

**struct AIPathfinderSuite**
This suite provides functions that perform operations on paths (art objects of type kPathArt), which apply various effects to selected path art. More...

**struct AIRealMathSuite**
This suite provides functions for working with the AIReal values that Illustrator uses for coordinates. More...

**struct AIAnnotatorDrawerSuite**
The annotator drawer suite allows plug-ins to draw annotations into the document window that are not a part of the artwork. More...

**struct AIDocumentViewSuite**
Use these functions to get and set properties of a document view, including the bounds, center, zoom, and screen mode. More...

AIAPI AIErr(* AIDocumentViewSuite::GetDocumentViewZoom)(AIDocumentViewHandle view, AIReal *zoom)
Retrieves the zoom factor for a view.
This is the scale factor from artwork coordinates to window coordinates.
Parameters:
view 	The view reference, or NULL for the current view.
zoom 	[out] A buffer in which to return the zoom factor, a percentage value where 1 is 100% (actual size), 0.5 is 50% and 2 is 200%.  

## Plug-In Property List
PiPL seems important. All plug-ins need a PiPL that describes the plugin type etc.
It seems like all the sample projects share a PiPL file in the `./../common/mac` folder.

## Plug-ing code entry point
By convention, the entry point is called `PluginMain` and is compiled with C linkage:  
`extern "C" ASAPI ASErr PluginMain(char* caller, char* selector, void* message);`

All plugin's recieve at least four message actions (during it's lifetime):  
`reload`  
`unload`  
`startup`  
`shutdown`  

`reload` seems to be called before `startup`
and probably `unload` before `shutdown`?

## PICA means the host application  

## Notifiers
Plug-ins must register for the notifiers in which they are interested. The Notifier suite is used to register and remove notification requests (see `AINotifierSuite`).


## AIAPI AIErr(* AIPathSuite::GetPathSegments)(AIArtHandle path, ai::int16 segNumber, ai::int16 count, AIPathSegment segments[])
Retrieves a set of segments from a path.

Parameters:  
path 	The path object.  
segNumber 	The 0-based index of the first segment.  
count 	The number of segments to retrieve.  
segments 	[out] An array in which to return the segments. You must allocate an array of AIPathSegment the size of count.  

# Should I allocate the array using the Illustrator memory allocation suite?
