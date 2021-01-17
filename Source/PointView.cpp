//
//  PointView.cpp
//  PointView
//
//  Created by Fredrik Josefsson on 2021-01-10, 17:37.
//

#include "IllustratorSDK.h"
#include "AIAnnotator.h" // not included in IllustratorSDK.h
#include "AIAnnotatorDrawer.h" // not included in IllustratorSDK.h

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

AIErr PointView_FreeGlobals(SPInterfaceMessage *message);
AIErr PointView_AllocateGlobals(SPInterfaceMessage *message);
AIErr PointView_MessageAlert(char *string);

// SUITES
extern "C" {
	// the basic suite for loading/unloading other suites
	// it only takes two selectors, startup and shutdown:
	SPBasicSuite *sSPBasic = NULL;
	 // helper suite for strings:
	AIUnicodeStringSuite *sAIUnicodeString = NULL;
	// suite for managing memory (also used in IAIUnicodeString helper suite):
	SPBlocksSuite *sSPBlocks = NULL;
	// get-set art objects attributes:
	AIArtSuite *sAIArt = NULL;
	// suite for handling art objects paths:
	AIPathSuite *sAIPath = NULL;
	// suite for getting a list of currently selected art:
	AIMatchingArtSuite *sAIMatchingArt = NULL;
	// suite for adding notifiers (listen for specific events):
	AINotifierSuite *sAINotifier = NULL;
	// suite for displaying message alerts, unit conversions, progressbars etc:
	AIUserSuite *sAIUser = NULL;
	// suite for converting document and art coordinates to view coordinates:
	AIDocumentViewSuite *sAIDocumentView = NULL;
	// draw annotations that are not a part of the artwork:
	AIAnnotatorSuite * sAIAnnotator = NULL;
	AIAnnotatorDrawerSuite *sAIAnnotatorDrawer = NULL;
	AIAnnotatorHandle gAnnotatorHandle;
}

// GLOBALS
typedef struct {
	ai::int32 artObjectsCount = 0;
	AIArtHandle **artObjectsHandle;
	AIAnnotatorHandle annotatorHandle;
} Globals;

Globals *g = nullptr;

const AIRGBColor ANNOTATION_COLOR = {65000, 0, 0};
const float PV_RADIUS = 5.0;

extern "C" ASAPI ASErr PluginMain(char *caller, char *selector, void *message) {
	ASErr error = kNoErr;
	SPMessageData *msgData = (SPMessageData *)message;
	sSPBasic = msgData->basic;
	
	if(sSPBasic->IsEqual(caller, kSPInterfaceCaller)) {
		
		// START UP MESSAGE RECIEVED
		// different messages depending on if we got a startup or a shutdown selector message
		if(sSPBasic->IsEqual(selector, kSPInterfaceStartupSelector)) {
			
			char s[] = "PointView plug-in loaded!";
			PointView_MessageAlert(s);
			
			// add a notifier for selection change events
			char notifierName[kMaxStringLength];
			sprintf(notifierName, "PointView Art Selection Notifier");
			error = sSPBasic->AcquireSuite(kAINotifierSuite, kAINotifierVersion, (const void**) &sAINotifier);
			error = sAINotifier->AddNotifier(msgData->self, notifierName, kAIArtSelectionChangedNotifier, NULL);
			error = sSPBasic->ReleaseSuite(kAINotifierSuite, kAINotifierVersion);
			
			// register plug-in as annotator
			error = sSPBasic->AcquireSuite(kAIAnnotatorSuite, kAIAnnotatorVersion, (const void**) &sAIAnnotator);
			error = sAIAnnotator->AddAnnotator(msgData->self, "PointView Annotator", &gAnnotatorHandle);
			error = sSPBasic->ReleaseSuite(kAIAnnotatorSuite, kAIAnnotatorVersion);
			
			PointView_AllocateGlobals((SPInterfaceMessage *)message);
			
		// SHUT DOWN MESSAGE RECIEVED
		} else if(sSPBasic->IsEqual(selector, kSPInterfaceShutdownSelector)) {
			PointView_FreeGlobals((SPInterfaceMessage *)message);
		}
		
	// NOTIFICATION MESSAGE RECIEVED
	} else if(sSPBasic->IsEqual(caller, kCallerAINotify)) { // we got a notification!
		
		error = sSPBasic->AcquireSuite(kAIMatchingArtSuite, kAIMatchingArtVersion, (const void**) &sAIMatchingArt);
		if( sAIMatchingArt->IsSomeArtSelected() ) {
			error = sSPBasic->AcquireSuite(kAIArtSuite, kAIArtVersion, (const void**) &sAIArt);
			error = sSPBasic->AcquireSuite(kAIPathSuite, kAIPathVersion, (const void**) &sAIPath);
			
			error = sAIMatchingArt->GetSelectedArt(&g->artObjectsHandle, &g->artObjectsCount);
			
			for(int i=0; (error==kNoErr) && (i<g->artObjectsCount); i++) {
				AIArtHandle art = (*g->artObjectsHandle)[i];
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

				if (sSPBlocks==nullptr) error = sSPBasic->AcquireSuite(kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**) &sSPBlocks);
				if (sAIUnicodeString==nullptr) error = sSPBasic->AcquireSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void**) &sAIUnicodeString);
				
				error = sAIArt->SetNote(art,ai::UnicodeString(artNote));
				
				error = sSPBasic->ReleaseSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion);
				error = sSPBasic->ReleaseSuite(kSPBlocksSuite, kSPBlocksSuiteVersion);
			}
			
			error = sSPBasic->ReleaseSuite(kAIPathSuite, kAIPathVersion);
			error = sSPBasic->ReleaseSuite(kAIArtSuite, kAIArtVersion);
		}
		error = sSPBasic->ReleaseSuite(kAIMatchingArtSuite, kAIMatchingArtVersion);
		
	} else if(sSPBasic->IsEqual(caller, kCallerAIAnnotation)) {
		
		AIAnnotatorMessage *annotatorMessage = (AIAnnotatorMessage *)message;
		AIAnnotatorDrawer *annotatorDrawer = (AIAnnotatorDrawer *)annotatorMessage->drawer;

		if(sSPBasic->IsEqual(selector, kSelectorAIInvalAnnotation)) {
			error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void**) &sAIDocumentView);
			error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void**) &sAIAnnotatorDrawer);
			
			AIRealRect updateRect;
			AIRect portBounds;
			//error = sAIDocumentView->GetDocumentViewBounds( NULL ,&updateRect);
			error = sAIDocumentView->GetDocumentViewInvalidRect( NULL ,&updateRect);
			portBounds.left = _AIRealRoundToShort(updateRect.left) - 1;
			portBounds.top = _AIRealRoundToShort(updateRect.top) + 1;
			portBounds.right = _AIRealRoundToShort(updateRect.right) + 1;
			portBounds.bottom = _AIRealRoundToShort(updateRect.bottom) - 1;
			
			sAIAnnotator->InvalAnnotationRect(NULL, &portBounds);
			
			error = sSPBasic->ReleaseSuite(kAIDocumentViewSuite, kAIDocumentViewVersion);
			error = sSPBasic->ReleaseSuite(kAIDocumentViewSuite, kAIDocumentViewVersion);
			
		}
		
		if(g->artObjectsCount > 0) {
			if(sSPBasic->IsEqual(selector, kSelectorAIDrawAnnotation)) {
				error = sSPBasic->AcquireSuite(kAIArtSuite, kAIArtVersion, (const void**) &sAIArt);
				error = sSPBasic->AcquireSuite(kAIPathSuite, kAIPathVersion, (const void**) &sAIPath);
				error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void**) &sAIAnnotatorDrawer);
				error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void**) &sAIDocumentView);

				for(int i=0; i<g->artObjectsCount; i++) {
					
					AIArtHandle art = (*g->artObjectsHandle)[i];
					
					bool closed;
					error = sAIPath->GetPathClosed(art, (AIBoolean *)&closed);
					
					if(!closed) {
						
						ai::int16 segmentCount;
						error = sAIPath->GetPathSegmentCount(art, &segmentCount);
						
						if(segmentCount>1) {
							AIPathSegment segments[segmentCount];
							error = sAIPath->GetPathSegments(art, 0, segmentCount, segments);
							
							// to access AIPoint x, y values use p.h, p.v
							AIRealPoint startPointArt = segments[0].p;
							AIRealPoint endPointArt = segments[segmentCount-1].p;
							
							AIPoint startPointView;
							AIPoint endPointView;
							
							error = sAIDocumentView->ArtworkPointToViewPoint(NULL, &startPointArt, &startPointView);
							error = sAIDocumentView->ArtworkPointToViewPoint(NULL, &endPointArt, &endPointView);
							
							AIRect startRect;
							startRect.left = startPointView.h-PV_RADIUS;
							startRect.right = startPointView.h+PV_RADIUS;
							startRect.top = startPointView.v-PV_RADIUS;
							startRect.bottom = startPointView.v+PV_RADIUS;
							
							AIRect endRect;
							endRect.left = endPointView.h-PV_RADIUS;
							endRect.right = endPointView.h+PV_RADIUS;
							endRect.top = endPointView.v-PV_RADIUS;
							endRect.bottom = endPointView.v+PV_RADIUS;
							
							sAIAnnotatorDrawer->SetColor(annotatorDrawer, ANNOTATION_COLOR);
							sAIAnnotatorDrawer->SetLineWidth(annotatorDrawer, 1);
							
							error = sAIAnnotatorDrawer->DrawEllipse(annotatorDrawer, startRect, false);
							error = sAIAnnotatorDrawer->DrawEllipse(annotatorDrawer, endRect, false);
						}
					}
				}
				
				error = sSPBasic->ReleaseSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion);
				error = sSPBasic->ReleaseSuite(kAIPathSuite, kAIPathVersion);
				error = sSPBasic->ReleaseSuite(kAIArtSuite, kAIArtVersion);
				error = sSPBasic->ReleaseSuite(kAIDocumentViewSuite, kAIDocumentViewVersion);
			} // end if(sSPBasic->IsEqual(selector, kSelectorAIDrawAnnotation))
		} // end if(g->artObjectsCount > 0)
		
	}
		 
	return error;
}

	
// ###

AIErr PointView_FreeGlobals(SPInterfaceMessage *message) {
	AIErr error = kNoErr;
	if ( g != nil ) {
		message->d.basic->FreeBlock(g); g = nil;
		message->d.globals = nil;
	}
	return error;
}

AIErr PointView_AllocateGlobals(SPInterfaceMessage *message) {
	AIErr error = kNoErr;
	error = message->d.basic->AllocateBlock( sizeof(Globals), (void **) &g );
	return error;
}

AIErr PointView_MessageAlert(char *string) {
	ASErr error = kNoErr;
	bool userSuiteOpened = false;
	bool blocksSuiteOpened = false;
	bool unicodeSuiteOpened = false;
	if (sAIUser==nullptr) {
		error = sSPBasic->AcquireSuite(kAIUserSuite, kAIUserSuiteVersion, (const void**) &sAIUser);
		userSuiteOpened = true;
	}
	if (sSPBlocks==nullptr) {
		error = sSPBasic->AcquireSuite(kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**) &sSPBlocks);
		blocksSuiteOpened = true;
	}
	if (sAIUnicodeString==nullptr) {
		error = sSPBasic->AcquireSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void**) &sAIUnicodeString);
		unicodeSuiteOpened = true;
	}
	
	sAIUser->MessageAlert(ai::UnicodeString(string));
	
	if (sAIUnicodeString!=nullptr && unicodeSuiteOpened) {
		error = sSPBasic->ReleaseSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion);
		sAIUnicodeString = nullptr;
	}
	if (sSPBlocks!=nullptr && blocksSuiteOpened) {
		error = sSPBasic->ReleaseSuite(kSPBlocksSuite, kSPBlocksSuiteVersion);
		sSPBlocks = nullptr;
	}
	if (sAIUser!=nullptr && userSuiteOpened) {
		error = sSPBasic->ReleaseSuite(kAIUserSuite, kAIUserSuiteVersion);
		sAIUser = nullptr;
	}
	
	return error;
}

