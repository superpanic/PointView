#include <iostream>
#include "IllustratorSDK.h"
#include "AIAnnotator.h"
#include "AIAnnotatorDrawer.h"

// Tell Xcode to export the following symbols
#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

// Plug-in entry point
extern "C" ASAPI ASErr PluginMain(char * caller, char* selector, void* message);

// Tell Xcode to return to default visibility for symbols
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

extern "C" {
	SPBlocksSuite *sSPBlocks = NULL; // memory handling
	AIUnicodeStringSuite *sAIUnicodeString = NULL; // unicode string helper
	AIUserSuite *sAIUser = NULL; // alerts
	AIAnnotatorSuite *sAIAnnotator = NULL; // register plug-in to recieve annotation update messages
	AIAnnotatorDrawerSuite *sAIAnnotatorDrawer = NULL; // draw annotations
	AIMatchingArtSuite *sAIMatchingArt = NULL; // get selected art objects
	AIMdMemorySuite *sAIMemory = NULL; // handling memory
	AIArtSuite *sAIArt = NULL; // art objects
	AIPathSuite *sAIPath = NULL; // working with paths
	AIDocumentViewSuite *sAIDocumentView = NULL; // converting art to view coordinates
	AILayerSuite *sAILayer = NULL; // to access current layer color
	AINotifierSuite *sAINotifier = NULL; // register move art notification
	AIMenuSuite *sAIMenu = NULL; // adding menu selection

	AIPreferenceSuite *sAIPreference = NULL; // saving menu on/off state
}


typedef struct {
	AIAnnotatorHandle annotatorHandle;
	AIMenuItemHandle menuHandle;
	AIBoolean PointViewShowing;
} Globals;

Globals *g = nullptr;

static ai::int32 START_POINT_SIZE = 6;
static ai::int32 END_POINT_SIZE = 8;
static AIReal POINT_VIEW_LINE_WIDTH = 1.0;

static AIErr PV_StartupPlugin(SPInterfaceMessage *message, SPPlugin *self);
static AIErr PV_ShutdownPlugin(SPInterfaceMessage *message);
static AIErr PV_DrawAnnotation(void *message);
static void  PV_SetAIRectSize(AIRect *r, AIPoint *p, ai::int32 radius);
static bool  PV_ArePointsDifferent(AIPoint *p1, AIPoint *p2);

//bool PointViewIsActive = true;

extern "C" ASAPI ASErr PluginMain(char *caller, char *selector, void *message) {

	ASErr error = kNoErr;
	SPPlugin *self = ((SPMessageData *)message)->self;
	SPBasicSuite* sSPBasic = ((SPMessageData*)message)->basic;

	if (sSPBasic->IsEqual(caller, kSPInterfaceCaller)) {

		error = sSPBasic->AcquireSuite(kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**)&sSPBlocks);
		error = sSPBasic->AcquireSuite(kAIUserSuite, kAIUserSuiteVersion, (const void **)&sAIUser);
		error = sSPBasic->AcquireSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void **)&sAIUnicodeString);

		if (sSPBasic->IsEqual(selector, kSPInterfaceStartupSelector)) {
			// sAIUser->MessageAlert(ai::UnicodeString("PointView Loaded"));
			PV_StartupPlugin((SPInterfaceMessage *)message, self);
		} else if (sSPBasic->IsEqual(selector, kSPInterfaceShutdownSelector)) {
			// sAIUser->MessageAlert(ai::UnicodeString("Goodbye!"));
			PV_ShutdownPlugin((SPInterfaceMessage *)message);
		}
		
		// error = sSPBasic->AcquireSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion, (const void **)&sAIUnicodeString);
		error = sSPBasic->ReleaseSuite(kAIUnicodeStringSuite, kAIUnicodeStringSuiteVersion);
		// error = sSPBasic->AcquireSuite(kAIUserSuite, kAIUserSuiteVersion, (const void **)&sAIUser);
		error = sSPBasic->ReleaseSuite(kAIUserSuite, kAIUserSuiteVersion);
		// error = sSPBasic->AcquireSuite(kSPBlocksSuite, kSPBlocksSuiteVersion, (const void**)&sSPBlocks);
		error = sSPBasic->ReleaseSuite(kSPBlocksSuite, kSPBlocksSuiteVersion);

	} 
	
	else if (g!=nullptr && g->PointViewShowing && sSPBasic->IsEqual(caller, kCallerAIAnnotation)) {

		if (sSPBasic->IsEqual(selector, kSelectorAIDrawAnnotation)) {
			error = PV_DrawAnnotation((AIAnnotatorMessage *)message);
			
		} else if (sSPBasic->IsEqual(selector, kSelectorAIInvalAnnotation)) {
			error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void **)&sAIDocumentView);
			error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void **)&sAIAnnotatorDrawer);
			
			AIRealRect updateRect;
			AIRect portBounds;
			
			error = sAIDocumentView->GetDocumentViewInvalidRect(NULL, &updateRect);
			portBounds.left = _AIRealRoundToShort(updateRect.left) - 1;
			portBounds.top = _AIRealRoundToShort(updateRect.top) + 1;
			portBounds.right = _AIRealRoundToShort(updateRect.right) + 1;
			portBounds.bottom = _AIRealRoundToShort(updateRect.bottom) - 1;

			sAIAnnotator->InvalAnnotationRect(NULL, &portBounds);

			// error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void **)&sAIDocumentView);
			error = sSPBasic->ReleaseSuite(kAIDocumentViewSuite, kAIDocumentViewVersion);
			// error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void **)&sAIAnnotatorDrawer);
			error = sSPBasic->ReleaseSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion);
		}

	}

	else if (sSPBasic->IsEqual(caller, kCallerAIMenu)) {
		if (sSPBasic->IsEqual(selector, kSelectorAIGoMenuItem)) {
			AIMenuMessage *menuMessage = (AIMenuMessage *)message;
			error = sSPBasic->AcquireSuite(kAIMenuSuite, kAIMenuVersion, (const void **)&sAIMenu);
			if (g->PointViewShowing) {
				error = sAIMenu->SetItemText(menuMessage->menuItem, ai::UnicodeString("Show PointView"));
			} else {
				error = sAIMenu->SetItemText(menuMessage->menuItem, ai::UnicodeString("Hide PointView"));
			}
			error = sSPBasic->ReleaseSuite(kAIMenuSuite, kAIMenuSuiteVersion);
			g->PointViewShowing = !(g->PointViewShowing);
		}
	}

	return error;
}


static AIErr PV_StartupPlugin(SPInterfaceMessage *message, SPPlugin *self) {
	ASErr error = kNoErr;
	SPBasicSuite* sSPBasic = message->d.basic;

	error = sSPBasic->AllocateBlock(sizeof(Globals), (void **)&g);
	if (!error) message->d.globals = g;

	// register annotator
	error = sSPBasic->AcquireSuite(kAIAnnotatorSuite, kAIAnnotatorVersion, (const void **)&sAIAnnotator);
	error = sAIAnnotator->AddAnnotator(message->d.self, "PointView Annotator", &(g->annotatorHandle));
	error = sSPBasic->ReleaseSuite(kAIAnnotatorSuite, kAIAnnotatorVersion);
	
	// read menu on/off preference
	
	g->PointViewShowing = true; // set default value, used if preference not set.
	error = sSPBasic->AcquireSuite(kAIPreferenceSuite, kAIPreferenceVersion, (const void **)&sAIPreference);
	//AIAPI AIErr(* AIPreferenceSuite::PutBooleanPreference)(const char *prefix, const char *suffix, AIBoolean value)
	error = sAIPreference->GetBooleanPreference("PointView", "Show", &(g->PointViewShowing));
	error = sSPBasic->ReleaseSuite(kAIPreferenceSuite, kAIPreferenceVersion);
	
	
	// add menu item
	AIPlatformAddMenuItemDataUS menuData;
	menuData.groupName = kViewUtilsMenuGroup;
	if(g->PointViewShowing) menuData.itemText = ai::UnicodeString("Hide PointView");
	else menuData.itemText = ai::UnicodeString("Show PointView");
	error = sSPBasic->AcquireSuite(kAIMenuSuite, kAIMenuVersion, (const void **)&sAIMenu);
	error = sAIMenu->AddMenuItem(self, "PointView", &menuData, kMenuItemNoOptions, &(g->menuHandle) );
	error = sSPBasic->ReleaseSuite(kAIMenuSuite, kAIMenuSuiteVersion);

	return error;
}


static AIErr PV_ShutdownPlugin(SPInterfaceMessage *message) {
	ASErr error = kNoErr;
	SPBasicSuite* sSPBasic = message->d.basic;
	
	error = sSPBasic->AcquireSuite(kAIPreferenceSuite, kAIPreferenceVersion, (const void **)&sAIPreference);
	//AIAPI AIErr(* AIPreferenceSuite::PutBooleanPreference)(const char *prefix, const char *suffix, AIBoolean value)
	error = sAIPreference->PutBooleanPreference("PointView", "Show", g->PointViewShowing);
	error = sSPBasic->ReleaseSuite(kAIPreferenceSuite, kAIPreferenceVersion);
	
	if (g != nullptr) {
		message->d.basic->FreeBlock(g);
		g = nullptr;
		message->d.globals = nullptr;
	}
	return error;
}

static void PV_DrawCrissShape(AIAnnotatorDrawer *annotatorDrawer, const AIPoint &startPointView, AIPoint &p1, AIPoint &p2) {
	p1.h = startPointView.h-START_POINT_SIZE;
	p1.v = startPointView.v-START_POINT_SIZE;
	p2.h = startPointView.h+START_POINT_SIZE;
	p2.v = startPointView.v+START_POINT_SIZE;
	sAIAnnotatorDrawer->DrawLine(annotatorDrawer, p1, p2);
	p1.h = startPointView.h+START_POINT_SIZE;
	p1.v = startPointView.v-START_POINT_SIZE;
	p2.h = startPointView.h-START_POINT_SIZE;
	p2.v = startPointView.v+START_POINT_SIZE;
	sAIAnnotatorDrawer->DrawLine(annotatorDrawer, p1, p2);
}

static void PV_DrawCrossShape(AIAnnotatorDrawer *annotatorDrawer, const AIPoint &endPointView, AIPoint &p1, AIPoint &p2) {
	p1.h = endPointView.h-END_POINT_SIZE;
	p1.v = endPointView.v;
	p2.h = endPointView.h+END_POINT_SIZE;
	p2.v = endPointView.v;
	sAIAnnotatorDrawer->DrawLine(annotatorDrawer, p1, p2);
	p1.h = endPointView.h;
	p1.v = endPointView.v-END_POINT_SIZE;
	p2.h = endPointView.h;
	p2.v = endPointView.v+END_POINT_SIZE;
	sAIAnnotatorDrawer->DrawLine(annotatorDrawer, p1, p2);
}

static AIErr PV_DrawAnnotation(void *message) {

	AIErr error = kNoErr;

	SPBasicSuite* sSPBasic = ((SPMessageData*)message)->basic;

	AIAnnotatorMessage *annotatorMessage = (AIAnnotatorMessage *)message;
	AIAnnotatorDrawer *annotatorDrawer = (AIAnnotatorDrawer *)annotatorMessage->drawer;

	error = sSPBasic->AcquireSuite(kAIMatchingArtSuite, kAIMatchingArtVersion, (const void **)&sAIMatchingArt);
	error = sSPBasic->AcquireSuite(kAIMdMemorySuite, kAIMdMemoryVersion, (const void **)&sAIMemory);
	error = sSPBasic->AcquireSuite(kAIArtSuite, kAIArtVersion, (const void **)&sAIArt);
	error = sSPBasic->AcquireSuite(kAIPathSuite, kAIPathVersion, (const void **)&sAIPath);
	error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void **)&sAIDocumentView);
	error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void **)&sAIAnnotatorDrawer);
	error = sSPBasic->AcquireSuite(kAILayerSuite, kAILayerVersion, (const void **)&sAILayer);

	if (sAIMatchingArt->IsSomeArtSelected()) {

		AIArtHandle **selectedArtHandle;
		ai::int32 selectedArtCount;

		// selectedArtHandle is allocated - needs free
		sAIMatchingArt->GetSelectedArt(&selectedArtHandle, &selectedArtCount);

		AIArtHandle art;

		for (int i = 0; i < selectedArtCount; i++) {
			art = (*selectedArtHandle)[i];
			short artType;
			sAIArt->GetArtType(art, &artType);

			if (artType == kPathArt) {
				AIBoolean closed;
				sAIPath->GetPathClosed(art, &closed);
				if (!closed) {
					ai::int16 segmentCount;
					sAIPath->GetPathSegmentCount(art, &segmentCount);
					if (segmentCount > 1) {

						AIPathSegment firstSegment;
						AIPathSegment lastSegment;

						sAIPath->GetPathSegments(art, 0, 1, &firstSegment);
						sAIPath->GetPathSegments(art, segmentCount - 1, 1, &lastSegment);

						AIPoint startPointView;
						AIPoint endPointView;
						
						error = sAIDocumentView->ArtworkPointToViewPoint(NULL, &(firstSegment.p), &startPointView);
						error = sAIDocumentView->ArtworkPointToViewPoint(NULL, &(lastSegment.p), &endPointView);


						AILayerHandle layer;
						sAIArt->GetLayerOfArt(art, &layer);

						AIRGBColor col;
						sAILayer->GetLayerColor(layer, &col);

						sAIAnnotatorDrawer->SetColor(annotatorDrawer, col);
						sAIAnnotatorDrawer->SetLineWidth(annotatorDrawer, POINT_VIEW_LINE_WIDTH);
						
						AIPoint p1;
						AIPoint p2;
						
						PV_DrawCrissShape(annotatorDrawer, startPointView, p1, p2);
						
						if( PV_ArePointsDifferent(&startPointView, &endPointView) ) {
							PV_DrawCrossShape(annotatorDrawer, endPointView, p1, p2);
						}

					}
				}
			}


		}

		// free selected art handle
		sAIMemory->MdMemoryDisposeHandle((AIMdMemoryHandle)selectedArtHandle);
		selectedArtHandle = nullptr;
	}

	// error = sSPBasic->AcquireSuite(kAIMatchingArtSuite, kAIMatchingArtVersion, (const void **)&sAIMatchingArt);
	error = sSPBasic->ReleaseSuite(kAIMatchingArtSuite, kAIMatchingArtVersion);
	// error = sSPBasic->AcquireSuite(kAIMdMemorySuite, kAIMdMemoryVersion, (const void **)&sAIMemory);
	error = sSPBasic->ReleaseSuite(kAIMdMemorySuite, kAIMdMemoryVersion);
	// error = sSPBasic->AcquireSuite(kAIArtSuite, kAIArtVersion, (const void **)&sAIArt);
	error = sSPBasic->ReleaseSuite(kAIArtSuite, kAIArtVersion);
	// error = sSPBasic->AcquireSuite(kAIPathSuite, kAIPathVersion, (const void **)&sAIPath);
	error = sSPBasic->ReleaseSuite(kAIPathSuite, kAIPathVersion);
	// error = sSPBasic->AcquireSuite(kAIDocumentViewSuite, kAIDocumentViewVersion, (const void **)&sAIDocumentView);
	error = sSPBasic->ReleaseSuite(kAIDocumentViewSuite, kAIDocumentViewVersion);
	// error = sSPBasic->AcquireSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion, (const void **)&sAIAnnotatorDrawer);
	error = sSPBasic->ReleaseSuite(kAIAnnotatorDrawerSuite, kAIAnnotatorDrawerVersion);
	// error = sSPBasic->AcquireSuite(kAILayerSuite, kAILayerVersion, (const void **)&sAILayer);
	error = sSPBasic->ReleaseSuite(kAILayerSuite, kAILayerVersion);

	return error;
}

static bool  PV_ArePointsDifferent(AIPoint *p1, AIPoint *p2) {
	return (p1->h!=p2->h || p1->v!=p2->v);
}

static void PV_SetAIRectSize(AIRect *r, AIPoint *p, ai::int32 radius) {
	r->left = p->h - radius;
	r->right = p->h + radius;
	r->top = p->v - radius;
	r->bottom = p->v + radius;
}
