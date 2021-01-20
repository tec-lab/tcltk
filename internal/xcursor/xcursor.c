#include "xcursor.h"
#include <X11/Xcursor/Xcursor.h>
#include <X11/X.h>
#include <string.h>
#include <stdlib.h>
#include "tk.h"

// keep a list of already loaded cursors
typedef struct TkCursor {
  char* fileName;
  Cursor cursor;
  void* next;
} tkCursor;
static tkCursor* tkCursors = NULL;

extern int Xcursor_Init(Tcl_Interp *interp) {
  // provide package information
  if (Tcl_PkgProvide(interp, "XCursor", "0.3") != TCL_OK) {return (TCL_ERROR);}

  // Install "xcursor" command.
  Tcl_CreateObjCommand(interp, "xcursor", Tk_xcursor_cmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  // fini
  return (TCL_OK);
}

int Tk_xcursor_cmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  char* fileName = NULL;
  char* tkWinName = NULL;
  Tk_Window tkWin;
  Cursor cursor;
  tkCursor* tkCursor = NULL;

  // reset result
  Tcl_ResetResult(interp);

  // check argc
  if (objc != 3) {
    Tcl_AppendResult(interp, "Invalid command count, use: xcursor <tkwin> <cursorfile>", 0);
    return(TCL_ERROR);
  }

  // get the window name
  tkWinName = Tcl_GetString(objv[1]);
  // get the window object
  tkWin = Tk_NameToWindow(interp, tkWinName, (ClientData)Tk_MainWindow(interp));
  if (tkWin == NULL) {return(TCL_ERROR);}
  // check
  if (None == Tk_WindowId(tkWin)) Tk_MakeWindowExist(tkWin);

  // get the cursor name
  fileName = Tcl_GetString(objv[2]);
  if (fileName[0] == 0) {
    XUndefineCursor(Tk_Display(tkWin), Tk_WindowId(tkWin));
    return(TCL_OK);
  }

  // search existing cursors
  tkCursor = tkCursors;
  while (tkCursor != NULL) {
    if (strcmp(fileName, tkCursor->fileName) == 0) break;
    tkCursor = tkCursor->next;
  }

  // if we did not find a valid cursor then create a new one from scratch
  if (tkCursor == NULL) {
    Cursor cursor = XcursorFilenameLoadCursor(Tk_Display(Tk_MainWindow(interp)), fileName);
    // basic check
    if (cursor == 0) {
      Tcl_AppendResult(interp, "invalid cursor file '", fileName, "'", 0);
      return(TCL_ERROR);
    }

    // first time call
    if (tkCursors == NULL) {
      // create initial tkCursor
      tkCursors = calloc(0, sizeof(tkCursor));
      tkCursor = tkCursors;
    } else {
      // find last pointer to attach a new one
      tkCursor = tkCursors;
      while (tkCursor->next != NULL) {tkCursor = tkCursor->next;}
      // create next tkCursor
      tkCursor->next = calloc(0, sizeof(tkCursor));
      tkCursor = tkCursor->next;
    }
    // fill the new tkCursor with data
    tkCursor->fileName = calloc(strlen(fileName) + 1, sizeof(char));
    strcpy(tkCursor->fileName, fileName);
    tkCursor->cursor = cursor;
  }

  // assign new cursor
  //XUndefineCursor(Tk_Display(tkWin), Tk_WindowId(tkWin));
  XDefineCursor(Tk_Display(tkWin), Tk_WindowId(tkWin), tkCursor->cursor);
  //XFreeCursor (Display *display, Cursor cursor)

  // fini
  return(TCL_OK);
}


