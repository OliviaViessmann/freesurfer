#include "ScubaFrame.h"
#include "PreferencesManager.h"
extern "C" {
#include "glut.h"
}
#include "ScubaView.h"

using namespace std;

ViewFactory* ScubaFrame::mFactory = NULL;

ScubaFrame::ScubaFrame( ToglFrame::ID iID ) 
  : ToglFrame( iID ) {

  DebugOutput( << "Created ScubaFrame " << iID );
  SetOutputStreamToCerr();

  mnSelectedViewCol = 0;
  mnSelectedViewRow = 0;

  SetViewConfiguration( c1 );

  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "SetFrameViewConfiguration", 2, 
			 "frameID, configuration", 
			 "Sets a frame's view configuration. Supported "
			 "configurations, where each number is the number "
			 "of columns in a row: c1 c22 c44 c13" );
  commandMgr.AddCommand( *this, "GetViewIDFromFrameColRow", 3, 
			 "frameID col row",
			 "Return the viewID from a view at a certain "
			 "location. col and row must be valid for the current "
			 "view configuration." );
  commandMgr.AddCommand( *this, "GetSelectedViewID", 1, "frameID",
			 "Return the viewID of the selected view." );
  commandMgr.AddCommand( *this, "SetSelectedViewID", 2, "frameID viewID",
			 "Sets the select view in a frame." );
  commandMgr.AddCommand( *this, "GetNumberOfRowsInFrame", 1, "frameID",
			 "Return the number of rows in a frame." );
  commandMgr.AddCommand( *this, "GetNumberOfColsAtRowInFrame", 2, 
			 "frameID row", "Return the number of columns in a"
			 "row." );
  commandMgr.AddCommand( *this, "GetViewIDAtFrameLocation", 3, 
			 "frameID windowX windowY", 
			 "Return the view ID at a window location." );
  commandMgr.AddCommand( *this, "GetColumnOfViewInFrame", 2, "frameID viewID", 
			 "Return the column of the view ID in a frame." );
  commandMgr.AddCommand( *this, "GetRowOfViewInFrame", 2, "frameID viewID", 
			 "Return the row of the view ID in a frame." );
  commandMgr.AddCommand( *this, "RedrawFrame", 1, "frameID", 
			 "Tells a frame to redraw." );
  commandMgr.AddCommand( *this, "CopyViewLayersToAllViewsInFrame", 2,
			 "frameID viewID", "Copies the layer settings "
			 "in a view to all other views in a frame." );
  commandMgr.AddCommand( *this, "GetToolIDForFrame", 1, "frameID",
			 "Returns the ID of the tool for this frame." );
  commandMgr.AddCommand( *this, "CycleCurrentViewInFrame", 1, "frameID",
			 "Selects the next view in a frame." );

}

ScubaFrame::~ScubaFrame() {
}

TclCommandListener::TclCommandResult
ScubaFrame::DoListenToTclCommand( char* isCommand, int iArgc, char** iasArgv ) {

  // SetFrameViewConfiguration <frameID> <configuration>
  if( 0 == strcmp( isCommand, "SetFrameViewConfiguration" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      ViewConfiguration config;
      if( 0 == strcmp( iasArgv[2], "c1" ) ) {
	config = c1;
      } else if( 0 == strcmp( iasArgv[2], "c22" ) ) {
	config = c22;
      } else if( 0 == strcmp( iasArgv[2], "c44" ) ) {
	config = c44;
      } else if( 0 == strcmp( iasArgv[2], "c13" ) ) {
	config = c13;
      } else {
	sResult = "bad configuration \"" + string(iasArgv[2]) + 
	  "\", should be c1, c22, c44, or c13";
	return error;
      }
      
      SetViewConfiguration( config );
    }
  }

  // GetViewIDFromFrameColRow <frameID> <col> <row>
  if( 0 == strcmp( isCommand, "GetViewIDFromFrameColRow" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      int nCol = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad row";
	return error;
      }
      
      int nRow = strtol(iasArgv[3], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad col";
	return error;
      }
      
      if( nRow >= 0 && nRow < mcRows ) {
	
	if( nCol >= 0 && nCol < mcCols[nRow] ) {
	  
	  try { 
	    View* view = GetViewAtColRow( nCol, nRow );
	    int id = view->GetID();
	    stringstream sID;
	    sID << id;
	    sReturnFormat = "i";
	    sReturnValues = sID.str();
	    return ok;
	  }
	  catch(...) {
	    stringstream sError;
	    sError << "couldn't get view at col " << nCol <<
	      " row " << nRow;
	    sResult = sError.str();
	    DebugOutput( << sResult );
	    return error;
	  }
	  
	} else {
	  stringstream sError;
	  sError << "bad col \"" << string(iasArgv[1]) << 
	    "\", should be between 0 and " << mcCols[nRow];
	  sResult = sError.str();
	  DebugOutput( << sResult );
	  return error;
	}
	
      } else {
	stringstream sError;
	sError << "bad row \"" << string(iasArgv[1]) <<
	  "\", should be between 0 and " << mcRows;
	sResult = sError.str();
	DebugOutput( << sResult );
	return error;
      }
      
    }
  }

  // SetSelectedViewID <frameID> <viewID>
  if( 0 == strcmp( isCommand, "SetSelectedViewID" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {

      int viewID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad view ID";
	return error;
      }
      
      for( int nRow = 0; nRow < mcRows; nRow++ ) {
	int cCols = mcCols[nRow];
	for( int nCol = 0; nCol < cCols; nCol++ ) {
	  
	  try {
	    View* view = GetViewAtColRow( nCol, nRow );
	    if( view->GetID() == viewID ) {
	      mnSelectedViewRow = nRow;
	      mnSelectedViewCol = nCol;
	      return ok;
	    }
	  }
	  catch(...) {
	    stringstream ssError;
	    ssError << "couldn't get view at col " 
		    << nCol << " row " << nRow;
	    sResult = ssError.str();
	    return error;
	  }
	}
      }
      stringstream ssError;
      ssError << "View ID " << viewID << " is not currently on screen.";
      sResult = ssError.str();
      return error;
    }
  }
  

  // GetSelectedViewID <frameID>
  if( 0 == strcmp( isCommand, "GetSelectedViewID" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      try { 
	View* view = GetViewAtColRow( mnSelectedViewCol, mnSelectedViewRow );
	int id = view->GetID();
	stringstream sID;
	sID << id;
	sReturnFormat = "i";
	sReturnValues = sID.str();
	return ok;
      }
      catch(...) {
	stringstream sError;
	sError << "couldn't get selected view at col " 
	       << mnSelectedViewCol << " row " << mnSelectedViewRow;
	sResult = sError.str();
	return error;
      }
    }
  }
  
  // GetNumberOfRowsInFrame <frameID>
  if( 0 == strcmp( isCommand, "GetNumberOfRowsInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    if( mID == frameID ) {
      stringstream cRows;
      cRows << mcRows;
      sReturnFormat = "i";
      sReturnValues = cRows.str();
      return ok;
    }
  }

  // GetNumberOfColsAtRowInFrame <frameID> <row>
  if( 0 == strcmp( isCommand, "GetNumberOfColsAtRowInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      int row = strtol(iasArgv[2], (char**)NULL, 10);
      if( row >= 0 && row < mcRows ) {
	stringstream cCols;
	cCols << mcCols[row];
	sReturnFormat = "i";
	sReturnValues = cCols.str();
	return ok;
      } else {
	sResult = "bad row";
	DebugOutput( << sResult );
	return error;
      }
      
    }
  }

  // GetColumnOfViewInFrame <frameID> <viewID> 
  if( 0 == strcmp( isCommand, "GetColumnOfViewInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      int viewID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad view ID";
	return error;
      }
      
      for( int nRow = 0; nRow < mcRows; nRow++ ) {
	int cCols = mcCols[nRow];
	for( int nCol = 0; nCol < cCols; nCol++ ) {
	  
	  try {
	    View* view = GetViewAtColRow( nCol, nRow );
	    if( view->GetID() == viewID ) {
	      stringstream ssReturn;
	      ssReturn << nCol;
	      sReturnFormat = "i";
	      sReturnValues = ssReturn.str();
	      return ok;
	    }
	  } 
	  catch(...) {
	  }
	}
      }

      sResult = "bad view ID";
      return error;
    }
  }

  // GetRowOfViewInFrame <frameID> <viewID> 
  if( 0 == strcmp( isCommand, "GetRowOfViewInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      int viewID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad view ID";
	return error;
      }
      
      for( int nRow = 0; nRow < mcRows; nRow++ ) {
	int cCols = mcCols[nRow];
	for( int nCol = 0; nCol < cCols; nCol++ ) {
	  
	  try {
	    View* view = GetViewAtColRow( nCol, nRow );
	    if( view->GetID() == viewID ) {
	      stringstream ssReturn;
	      ssReturn << nRow;
	      sReturnFormat = "i";
	      sReturnValues = ssReturn.str();
	      return ok;
	    }
	  } 
	  catch(...) {
	  }
	}
      }

      sResult = "bad view ID";
      return error;
    }
  }

  // GetViewIDAtFrameLocation <frameID> <windowX> <windowY>
  if( 0 == strcmp( isCommand, "GetViewIDAtFrameLocation" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      int windowX = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad x";
	return error;
      }
      
      int windowY = strtol(iasArgv[3], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad y";
	return error;
      }
      
      if( windowX < 0 || windowX >= GetWidth() ||
	  windowY < 0 || windowY >= GetHeight() ) {
	sResult = "location is out of bounds";
	return error;
      }
	
      try { 
	// We need to y flip this since we're getting the coords right
	// from Tcl, just like we y flip them in ToglFrame.
	int windowCoords[2];
	windowCoords[0] = windowX;
	windowCoords[1] = (mHeight - windowY);
	View* view = FindViewAtWindowLoc( windowCoords, NULL, NULL );
	if( NULL != view ) {
	  int id = view->GetID();
	  stringstream sID;
	  sID << id;
	  sReturnFormat = "i";
	  sReturnValues = sID.str();
	  return ok;
	}
      }
      catch(...) {
	stringstream sError;
	sError << "couldn't find view at x " << windowX <<
	  " y " << windowY;
	sResult = sError.str();
	DebugOutput( << sResult );
	return error;
      }
    }
  }

  // RedrawFrame <frameID>
  if( 0 == strcmp( isCommand, "RedrawFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      RequestRedisplay();
    }
  }

  // CopyViewLayersToAllViewsInFrame <frameID> <viewID>
  if( 0 == strcmp( isCommand, "CopyViewLayersToAllViewsInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      int viewID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad view ID";
	return error;
      }

      try {

	// Try getting this view first.
	View& srcView = View::FindByID( viewID );
	// ScubaView& scubaView = dynamic_cast<ScubaView&>(view);
	ScubaView& srcScubaView = (ScubaView&)srcView;

	// For each view in this frame...
	for( int nRow = 0; nRow < mcRows; nRow++ ) {
	  int cCols = mcCols[nRow];
	  for( int nCol = 0; nCol < cCols; nCol++ ) {
	    
	    try {
	      View* destView = GetViewAtColRow( nCol, nRow );
	      ScubaView& destScubaView = *(ScubaView*)destView;
	      if( destView->GetID() != viewID ) {
		srcScubaView.CopyLayerSettingsToView( destScubaView );
	      }
	    } 
	    catch(...) {
	    }
	  }
	}
      }
      catch(...) {
	sResult = "bad view ID";
	return error;
      }

      RequestRedisplay();
    }
  }

  // GetToolIDForFrame <frameID>
  if( 0 == strcmp( isCommand, "GetToolIDForFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      stringstream sID;
      sID << mTool.GetID();;
      sReturnFormat = "i";
      sReturnValues = sID.str();
    }
  }

  // CycleCurrentViewInFrame <frameID>
  if( 0 == strcmp( isCommand, "CycleCurrentViewInFrame" ) ) {
    int frameID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad frame ID";
      return error;
    }
    
    if( mID == frameID ) {
      
      if( mnSelectedViewCol < mcCols[mnSelectedViewRow] - 1 ) {
	mnSelectedViewCol++;
      } else if( mnSelectedViewRow < mcRows - 1 ) {
	mnSelectedViewRow++;
	mnSelectedViewCol = 0;
      } else {
	mnSelectedViewRow = 0;
	mnSelectedViewCol = 0;
      }

    }
  }

  return ok;
}

void
ScubaFrame::TranslateWindowToView ( int iWindow[2], int inCol, int inRow,
				    int oView[2] ) {

    oView[0] = iWindow[0] - (inCol * (mWidth / mcCols[inRow]));
    oView[1] = iWindow[1] - ((mcRows-1 - inRow) * (mHeight/mcRows));
}

void
ScubaFrame::SizeViewsToConfiguration() {


  for( int nRow = 0; nRow < mcRows; nRow++ ) {
    int cCols = mcCols[nRow];
    for( int nCol = 0; nCol < cCols; nCol++ ) {
      
      View* view;
      try {
	view = GetViewAtColRow( nCol, nRow );
	view->Reshape( mWidth / cCols, mHeight / mcRows );
      } 
      catch(...) {
	DebugOutput( << "Couldn't find a view where there was supposed "
		     << "to be one: " << nCol << ", " << nRow );
      }
      
    }
  }  
}

void
ScubaFrame::DoDraw() {
  
  for( int nRow = 0; nRow < mcRows; nRow++ ) {
    int cCols = mcCols[nRow];
    for( int nCol = 0; nCol < cCols; nCol++ ) {

      glMatrixMode( GL_PROJECTION );
      glLoadIdentity();
      glOrtho( 0, mWidth/cCols-1, 0, mHeight/mcRows-1, -1.0, 1.0 );
      glMatrixMode( GL_MODELVIEW );
      
      // We change the y position so that the 0,0 view is in the top
      // left corner and the cCols-1,mcRows-1 view is in the bottom
      // right, even tho the GL port's 0,0 is in the lower left
      // corner. 
      int x = (mWidth/cCols) * nCol;
      int y = (mHeight/mcRows) * (mcRows - nRow-1);

      // Use glViewport to change the origin of the gl context so our
      // views can start drawing at 0,0.
      glViewport( x, y, mWidth/cCols, mHeight/mcRows );
      glRasterPos2i( 0, 0 );

      // Tell the view to draw.
      try {
	View* view = GetViewAtColRow( nCol, nRow );
	view->Draw();
      }
      catch(...){
	cerr << "Error drawing view at " << nCol << ", " << nRow << endl;
      }

      // If this is our selected view, draw a green box around it.
      if( nRow == mnSelectedViewRow && 
	  nCol == mnSelectedViewCol ) {

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, mWidth, 0, mHeight, -1.0, 1.0 );
	glMatrixMode( GL_MODELVIEW );
	glColor3f ( 0.0, 1.0, 0.0 );
	glViewport( 0, 0, mWidth, mHeight );
	glBegin( GL_LINE_STRIP );
	glVertex2d( x, y );
	glVertex2d( x + (mWidth/cCols)-1, y );
	glVertex2d( x + (mWidth/cCols)-1, y + (mHeight/mcRows)-1 );
	glVertex2d( x, y + (mHeight/mcRows)-1 );
	glVertex2d( x, y );
	glEnd ();
      }
    }
  }
}

void
ScubaFrame::DoReshape() {

  SizeViewsToConfiguration();

}

void
ScubaFrame::DoTimer() {

  // In our timer function we scan our views and ask if they want
  // redisplays.
  for( int nRow = 0; nRow < mcRows; nRow++ ) {
    int cCols = mcCols[nRow];
    for( int nCol = 0; nCol < cCols; nCol++ ) {
      
      View* view;
      try {
	view = GetViewAtColRow( nCol, nRow );
	if( view->WantRedisplay() ) {
	  RequestRedisplay();
	  view->RedisplayPosted();
	}
      } 
      catch(...) {
      }
    }
  }  
}

void
ScubaFrame::DoMouseMoved( int iWindow[2], InputState& iInput ) {

  try {
    int nRow, nCol;
    View* view = FindViewAtWindowLoc( iWindow, &nCol, &nRow );
    int viewCoords[2];
    TranslateWindowToView( iWindow, nCol, nRow, viewCoords );
    view->MouseMoved( viewCoords, iInput, mTool );
  }
  catch(...) {
  }
}

void
ScubaFrame::DoMouseUp( int iWindow[2], InputState& iInput ) {

  try {
    int nRow, nCol;
    View* view = FindViewAtWindowLoc( iWindow, &nCol, &nRow );
    int viewCoords[2];
    TranslateWindowToView( iWindow, nCol, nRow, viewCoords );
    view->MouseUp( viewCoords, iInput, mTool );
  }
  catch(...) {
  } 
}

void
ScubaFrame::DoMouseDown( int iWindow[2], InputState& iInput ) {

  try {
    int nRow, nCol;
    View* view = FindViewAtWindowLoc( iWindow, &nCol, &nRow );

    // Select this view and request a redisplay that we can draw our
    // frame around it.
    mnSelectedViewCol = nCol;
    mnSelectedViewRow = nRow;

    RequestRedisplay();

    int viewCoords[2];
    TranslateWindowToView( iWindow, nCol, nRow, viewCoords );
    view->MouseDown( viewCoords, iInput, mTool );
  }
  catch(...) {
  } 
}

void
ScubaFrame::DoKeyDown( int iWindow[2], InputState& iInput ) {

  try {
    View* view = GetViewAtColRow( mnSelectedViewCol, mnSelectedViewRow );

    int viewCoords[2];
    TranslateWindowToView( iWindow, mnSelectedViewCol, mnSelectedViewRow,
			   viewCoords );
    view->KeyDown( viewCoords, iInput, mTool );
  }
  catch(...) {
  } 
}

void
ScubaFrame::DoKeyUp( int iWindow[2], InputState& iInput ) {

  try {
    View* view = GetViewAtColRow( mnSelectedViewCol, mnSelectedViewRow );

    int viewCoords[2];
    TranslateWindowToView( iWindow, mnSelectedViewCol, mnSelectedViewRow,
			   viewCoords );
    view->KeyUp( viewCoords, iInput, mTool );
  }
  catch(...) {
  } 
}

void
ScubaFrame::SetViewConfiguration( ScubaFrame::ViewConfiguration iConfig ) {

  mViewConfiguration = iConfig;

  int cNewRows;
  map<int,int> cNewCols;

  switch( mViewConfiguration ) {
    case c1:
      cNewRows = 1;
      cNewCols[0] = 1;
      break;
    case c22:
      cNewRows = 2;
      cNewCols[0] = cNewCols[1] = 2;
      break;
    case c44:
      cNewRows = 4;
      cNewCols[0] = cNewCols[1] = cNewCols[2] = cNewCols[3] = 4;
      break;
    case c13:
      cNewRows = 2;
      cNewCols[0] = 1;
      cNewCols[1] = 3;
      break;
  default:
    cNewRows = 0;
  }

  // First disable existing views that won't be in the new
  // configuration.
  if( cNewRows < mcRows ) {
    for( int nRow = cNewRows-1; nRow < mcRows; nRow++ ) {
      int cCols = mcCols[nRow];
      if( cNewCols[nRow] < cCols ) {
	for( int nCol = cNewCols[nRow]-1; nCol < cCols; nCol++ ) {
	  View* view;
	  try {
	    view = GetViewAtColRow( nCol, nRow );
	    view->SetVisibleInFrame( false );
	  } 
	  catch(...) {}
	}
      }
    }
  }

  // Now make sure all the new views are made, saving the new
  // configuration in the process.
  mcRows = cNewRows;
  for( int nRow = 0; nRow < mcRows; nRow++ ) {
    int cCols = mcCols[nRow] = cNewCols[nRow];
    for( int nCol = 0; nCol < cCols; nCol++ ) {
      
      View* view;
      try {
	view = GetViewAtColRow( nCol, nRow );
	view->SetVisibleInFrame( true );
      } 
      catch(...) {
	if( NULL != mFactory ) {
	  
	  view = mFactory->NewView();
	  SetViewAtColRow( nCol, nRow, view );

	  stringstream sID;
	  sID << nCol << ", " << nRow;
	  view->SetLabel( sID.str() );
	  
	  view->SetVisibleInFrame( true );
	
	} else {
	  DebugOutput( << "Couldn't create new view because factory "
		       << "has not been set" );
	}
      }
    }
  }  

  SizeViewsToConfiguration();

  // Make sure the selected col/row are in bounds. 
  if( mnSelectedViewRow >= mcRows ) {
    mnSelectedViewRow = mcRows - 1;
  }
  if( mnSelectedViewCol >= mcCols[mnSelectedViewRow] ) {
    mnSelectedViewRow = mcCols[mnSelectedViewRow] - 1;
  }


  RequestRedisplay();
}


View*
ScubaFrame::GetViewAtColRow( int iCol, int iRow ) {
  try { 
    View* view = (mViews[iRow])[iCol];
    if( NULL == view ) {
      throw logic_error( "no view" );
    }
    return view;
  }
  catch(...) {
    stringstream sError;
    sError << "Requested view that doesn't exist at " 
	   << iCol << ", " << iRow;
    throw new out_of_range( sError.str() );;
  }
}

void 
ScubaFrame::SetViewAtColRow( int iCol, int iRow, View* const iView ) {
  (mViews[iRow])[iCol] = iView;
}

View*
ScubaFrame::FindViewAtWindowLoc( int iWindow[2], int* onCol, int* onRow ) {

  try {

    int nRow = (int)floor( (float)(mHeight - iWindow[1]) / 
			   ((float)mHeight / (float)mcRows) );
    int nCol = (int)floor( (float)iWindow[0] / 
			   ((float)mWidth / (float)mcCols[nRow]) );
    
    if( NULL != onCol ) 
      *onCol = nCol;
    if( NULL != onRow )
      *onRow = nRow;

    return GetViewAtColRow( nCol, nRow );
  }
  catch(...) {
    return NULL;
  }
}
