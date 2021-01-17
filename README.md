# Plug-in for Adobe Illustrator
An Adobe Illustrator plug-in that visually highlights path endpoints. (A tool I need when editing imported CAD artwork.) The idea is to draw small circles around all open path's end points using the current layer specific selection color. The circles should not be part of the artwork, but is drawn on the UI layer; using the AIAnnotationDrawerSuite. 

![PointView path selection](http://superpanic.com/pointview/pvart.png)

Written in C.  

Based on the HelloWorld project from the `getting-started-guide.pdf` in [Adobe Illustrator SDK Docs](https://console.adobe.io/downloads/ai).  

# Build and run:
Use Xcode 11.3 (not the latest Xcode).

1. [Download the Illustrator SDK](https://console.adobe.io/downloads/ai)  
`https://console.adobe.io/downloads/ai`

2. Place project in the `samplecode` folder

3. Open Illustrator. Go to `Preferences/Plug-Ins & Scratch Disks` and set the additional plug-in:s folder to `SDK/samplecode/output`

4. Build project

5. Restart Illustrator

# Additional setup needed for the HelloWorld project:  
These things seems to be missing in the SDK documentation for the HelloWorld project setup for Macos Xcode.

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

# Development notes:

## PICA means the host (Adobe Illustrator) application

## *SetNote* can be used for debug output
I used the Attributes panel for debug output. Each Illustrator object can have a user note (string) that is displayed in the Attributes panel. This is usually empty, and can be set with `SetNote` from the `AIArtSuite`.

## Plug-in notifiers and timers
Plug-in notifiers and timers are used by a plug-in to have Illustrator inform it of certain events.
A notifier plug-in is notified when the state of an Illustrator document changes. For example, **a plug-in may request to be notified when the selection state changes.** A notifier plug-in registers for one or more notifications during start-up.
A timer plug-in is notified at regular time intervals. For example, a timer plug-in may request to be notified
once a second.
For more information, see AINotifierSuite and AITimerSuite in Adobe Illustrator API Reference.

## Plug-In Property List
PiPL is mentioned everywhere in the documentation and seems important. All plug-ins need a PiPL that describes the plugin type etc.
It seems like all the sample projects share a PiPL file in the `./../common/mac` folder. There is also a PiPL project in the `samplecode` folder.

## Plug-ing code entry point
By convention, the entry point is called `PluginMain` and is compiled with C linkage:  
`extern "C" ASAPI ASErr PluginMain(char* caller, char* selector, void* message);`

All plugin's recieve at least four message actions (during it's lifetime):  
`reload`  
`unload`  
`startup`  
`shutdown`  

`reload` seems to be called before `startup`
and probably `unload` before `shutdown`for 