# Currently work in progress:
I'm trying to build an Adobe Illustrator plug-in that visually displays path endpoints. (A tool I need when editing imported CAD artwork.) The idea is to draw small circles around all open path's end points using the current layer specific selection color. The circles should not be part of the artwork, but is drawn on the UI layer; something I hope to accomplish using the AIAnnotationDrawerSuite.

# Build and run:
Use Xcode 11.3 (not the latest Xcode).

1. Download the Illustrator SDK  
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
and probably `unload` before `shutdown`?  


From A. Patterson, Adobe forum:  

The AIDocumentViewSuite has everything you need to center on the selected art. I can't remember off the top of my head, but we've got almost exactly what you're talking about in our plugin and the centering stuff was pretty easy. Just poke through that header, it shouldn't be a problem. Just bear in mind that you might need to use one of its methods to convert from page coordinates to 'view coordinates' (which are basically pixels).

As for the annotation, that's fairly easy for something like this, so long as you understand annoations. There are several threads on that already, explaining the basics, and if you're not familliar with them I'd go read them first. But assuming you grasp the basics of annotations, here's the high level for what you're looking for:

Use AIArtSuite::GetBounds() on the handle you want to denote
Call AIAnnotatorSuite::InvalAnnotationRect() on the bounds from Using the bounds from (1), after converting them into document view coordinates using the AIDocumentViewSuite (very easy).
Assuming you've registered an annotator with Illustrator, you should get a caller/selector for drawing the annotation. The message will include an AIAnnotatorDrawer, which you'll need to actually draw your mark up.
Draw whatever it is you want around the art; you'll probably want to cache the art in question so you know what art you're supposed to be marking up.
One thing to note: to clear the annoation (let's say you can pick nothing on your list) you'd do steps (1) & (2) and you'd just do nothing in (3) or (4). **For us, in that situation I just clear the cached handle and then in (3) & (4) I do nothing if the handle is null.**

Additional notes: AIDrawArtSuite::DrawHilite() is a nice method you can use to hilite paths. It works on other types of art, but it looks best on paths.