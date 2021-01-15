//
//  PointView.cpp
//  PointView
//
//  Created by Fredrik Josefsson on 2021-01-10, 17:37.
//

#include "IllustratorSDK.h"
#include "AIAnnotator.h"
#include "AIAnnotatorDrawer.h"

// Tell Xcode to export the following symbols
#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

// Plugin entry point
extern "C" ASAPI ASErr PluginMain(char *caller, char *selector, void *message);

// Tell Xcode to return to default visibility for symbols
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

extern "C" {
	// the basic suite for loading/unloading other suites
	// it only takes two selectors, startup and shutdown
	SPBasicSuite *sSPBasic = NULL;

	 // helper suite for strings
	AIUnicodeStringSuite *sAIUnicodeString = NULL;

	// suite for managing memory (also used in IAIUnicodeString helper suite).
	SPBlocksSuite *sSPBlocks = NULL;

	// get-set art objects attributes
	AIArtSuite *sAIArt = NULL;

	// suite for handling art objects paths
	AIPathSuite *sAIPath = NULL;

	// load-unload memory handling suite
	AIMdMemorySuite *sAIMdMemory = NULL;

	// suite for getting a list of currently selected art
	AIMatchingArtSuite *sAIMatchingArt = NULL;

	// suite for adding notifiers (listen for specific events)
	AINotifierSuite *sAINotifier = NULL;

	// suite for displaying message alerts, unit conversions, progressbars etc
	AIUserSuite *sAIUser = NULL;

	// draw annotations that are not a part of the artwork
	AIAnnotatorSuite * sAIAnnotator = NULL;
	AIAnnotatorDrawerSuite *sAIAnnotatorDrawer = NULL;
}

extern "C" ASAPI ASErr PluginMain(char *caller, char *selector, void *message) {
	ASErr error = kNoErr;
	SPMessageData *msgData = (SPMessageData *)message;
	sSPBasic = msgData->basic;
	
	if(sSPBasic->IsEqual(caller, kSPInterfaceCaller)) {
		
		// acquire the suites we need
		error = sSPBasic->AcquireSuite(kAIUserSuite, kAIUserSuiteVersion, (const void**) &sAIUser);
		error = sSPBasic->AcquireSuite(kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**) &sSPBlocks);
		error = sSPBasic->AcquireSuite(kAIMdMemorySuite, kAIMdMemoryVersion, (const void**) &sAIMdMemory);
		error = sSPBasic->AcquireSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void**) &sAIUnicodeString);
		
		// START UP MESSAGE RECIEVED
		// different messages depending on if we got a startup or a shutdown selector message
		if(sSPBasic->IsEqual(selector, kSPInterfaceStartupSelector)) {
			sAIUser->MessageAlert(ai::UnicodeString("PointView plug-in loaded!"));

			// load suites
			error = sSPBasic->AcquireSuite(kAINotifierSuite, kAINotifierVersion, (const void**) &sAINotifier);
			error = sSPBasic->AcquireSuite(kAIMatchingArtSuite, kAIMatchingArtVersion, (const void**) &sAIMatchingArt);
			error = sSPBasic->AcquireSuite(kAIPathSuite, kAIPathVersion, (const void**) &sAIPath);
			error = sSPBasic->AcquireSuite(kAIArtSuite, kAIArtVersion, (const void**) &sAIArt);
			error = sSPBasic->AcquireSuite(kAIAnnotatorSuite, kAIAnnotatorVersion, (const void**) &sAIAnnotator);
			error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void**) &sAIAnnotatorDrawer);

			// add a notifier for selection change events
			char notifierName[kMaxStringLength];
			sprintf(notifierName, "PointView Art Selection Notifier");
			error = sAINotifier->AddNotifier(msgData->self, notifierName, kAIArtSelectionChangedNotifier, NULL);
			
		// SHUT DOWN MESSAGE RECIEVED
		} else if(sSPBasic->IsEqual(selector, kSPInterfaceShutdownSelector)) {
			// release the notifier suite and the path suite
			error = sSPBasic->ReleaseSuite(kAINotifierSuite, kAINotifierVersion);
			error = sSPBasic->ReleaseSuite(kAIMatchingArtSuite, kAIMatchingArtVersion);
			error = sSPBasic->ReleaseSuite(kAIPathSuite, kAIPathVersion);
			error = sSPBasic->ReleaseSuite(kAIArtSuite, kAIArtVersion);
			
			// release user suite and unicode string helper suite
			error = sSPBasic->ReleaseSuite(kAIMdMemorySuite, kAIMdMemoryVersion);
			error = sSPBasic->ReleaseSuite(kAIUserSuite, kAIUserSuiteVersion);
			error = sSPBasic->ReleaseSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion);
		}
		
	// NOTIFICATION MESSAGE RECIEVED
	} else if(sSPBasic->IsEqual(caller, kCallerAINotify)) { // we got a notification!
		if(sAIMatchingArt) { // is the matching art suite loaded?
			if( sAIMatchingArt->IsSomeArtSelected() ) { // is any art selected?
				
				ai::int32 artObjectsCount;
				AIArtHandle **artObjectsHandle = NULL;
				error = sAIMatchingArt->GetSelectedArt(&artObjectsHandle, &artObjectsCount);
				
				for(int i=0; (error==kNoErr) && (i<artObjectsCount); i++) {
					AIArtHandle art = (*artObjectsHandle)[i];
					char artNote[kMaxStringLength];
					bool closed;
					error = sAIPath->GetPathClosed(art, (AIBoolean *)&closed);
					
					ai::int16 segmentCount;
					error = sAIPath->GetPathSegmentCount(art, &segmentCount);
					
					// if(error == kNoErr && closed) {
					if(closed) {
						sprintf(artNote, "Closed, with %d points", segmentCount);
					} else {

						// TODO: the selected path is open find the in and out points print the positions to art object notes!
						AIPathSegment segments[segmentCount];
						error = sAIPath->GetPathSegments(art, 0, segmentCount, segments);
						AIReal startPointH = segments[0].p.h;
						AIReal startPointV = segments[0].p.v;
						AIReal endPointH = segments[segmentCount-1].p.h;
						AIReal endPointV = segments[segmentCount-1].p.v;
						
						sprintf(artNote, "Open path, with %d points.\nStart point at %lf %lf\nEnd point at %lf %lf", segmentCount, startPointH, startPointV, endPointH, endPointV);
					}
					// Set object's note (string) in the Attributes panel.
					error = sAIArt->SetNote(art,ai::UnicodeString(artNote));
				}
				
				/*
				// Alert message: "Selected art: n objects"
				char selectedArtAlertMessage[kMaxStringLength];
				sprintf(selectedArtAlertMessage, "Selected art: %d objects", artObjectsCount-1);
				sAIUser->MessageAlert(ai::UnicodeString(selectedArtAlertMessage));
				*/
				 
				// free the selected art memory block
				error = sAIMdMemory->MdMemoryDisposeHandle((AIMdMemoryHandle) artObjectsHandle);
			}
		}
	}
	
	return error;
}
