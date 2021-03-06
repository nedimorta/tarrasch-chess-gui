/****************************************************************************
 * Top level app file for Tarrasch chess GUI
 *  Similar to boilerplate app code for wxWidgets example apps
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#include "Portability.h"
#ifdef THC_WINDOWS
#include <windows.h>    // Windows headers for RedirectIoToConsole()
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <time.h>
#endif
#include <locale>
#include "wx/wx.h"
#include "wx/intl.h"
#include "wx/file.h"
#include "wx/listctrl.h"
#include "wx/clipbrd.h"
#include "wx/sysopt.h"
#include "wx/log.h"
#include "Appdefs.h"
#include "wx/aui/aui.h"
#ifdef AUI_NOTEBOOK
#include "wx/aui/auibook.h"
#else
#include "wx/notebook.h"
#endif
#include "PanelBoard.h"
#include "PanelContext.h"
#include "GameLogic.h"
#include "Lang.h"
#include "CentralWorkSaver.h"
#include "Session.h"
#include "Log.h"
#include "thc.h"
#include "UciInterface.h"
#include "DebugPrintf.h"
#include "AutoTimer.h"
#include "Book.h"
#include "Database.h"
#include "Objects.h"
#include "BookDialog.h"
#include "LogDialog.h"
#include "EngineDialog.h"
#include "MaintenanceDialog.h"
#include "TrainingDialog.h"
#include "GeneralDialog.h"
#include "Repository.h"
#include "CtrlBox.h"
#include "CtrlBoxBookMoves.h"
#include "CtrlChessTxt.h"

#if 1
    #include "bitmaps/myicons.xpm"
    //#include "bitmaps/new.xpm"
    #include "bitmaps/open.xpm"
    #include "bitmaps/save.xpm"
    #include "bitmaps/copy.xpm"
    #include "bitmaps/cut.xpm"
    #include "bitmaps/paste.xpm"
    #include "bitmaps/undo.xpm"
    #include "bitmaps/redo.xpm"

#else
    #include "bitmaps24/sample.xpm"
    #include "bitmaps24/new.xpm"
    #include "bitmaps24/open.xpm"
    #include "bitmaps24/save.xpm"
    #include "bitmaps24/copy.xpm"
    #include "bitmaps24/cut.xpm"
    #include "bitmaps24/preview.xpm"  // paste XPM
    #include "bitmaps24/print.xpm"
    #include "bitmaps24/help.xpm"
#endif

using namespace std;
using namespace thc;

Objects objs;

// Should really be a little more sophisticated about this
#define TIMER_ID 2001

// ----------------------------------------------------------------------------
// application class
// ----------------------------------------------------------------------------

class ChessApp: public wxApp
{
public:
    virtual bool OnInit();
    virtual int  OnExit();
};

CtrlBoxBookMoves *gbl_book_moves;

class PanelNotebook: public wxWindow
{
public:
    PanelNotebook( wxWindow *parent, wxWindowID, const wxPoint &pos, const wxSize &size, int book_moves_width_ );
#ifdef AUI_NOTEBOOK
    void OnTabSelected( wxAuiNotebookEvent& event );
    void OnTabClose( wxAuiNotebookEvent& event );
    wxAuiNotebook   *notebook;
#else
    void OnTabSelected( wxBookCtrlEvent& event );
    void OnSize( wxSizeEvent &evt );
    wxNotebook *notebook;
#endif
	void OnTabNew( wxCommandEvent& event );
    int book_moves_width;
    int new_tab_button_width;
	wxButton *new_tab_button;
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(PanelNotebook, wxWindow)
    EVT_SIZE( PanelNotebook::OnSize )
#ifdef AUI_NOTEBOOK
    EVT_AUINOTEBOOK_PAGE_CHANGED( wxID_ANY, PanelNotebook::OnTabSelected)   //user selects a tab
    EVT_AUINOTEBOOK_PAGE_CLOSE( wxID_ANY, PanelNotebook::OnTabClose)        //user closes a tab
#else
    EVT_NOTEBOOK_PAGE_CHANGED( wxID_ANY, PanelNotebook::OnTabSelected)   //user selects a tab
#endif
    EVT_BUTTON( ID_BUTTON_TAB_NEW, PanelNotebook::OnTabNew )
END_EVENT_TABLE()


// Constructor
PanelNotebook::PanelNotebook
(
    wxWindow *parent, 
    wxWindowID id,
    const wxPoint &point,
    const wxSize &siz,
    int book_moves_width_
)
    : wxWindow( parent, id, point, siz, wxNO_BORDER )
{
    // Note that the wxNotebook is twice as high as the panel it's in. If the wxNotebook is very short,
    // a weird erasure effect occurs on the first tab header when creating subsequent tabs. Making the
    // wxNotebook think it's less short fixes this - and it still appears nice and short through the
    // short panel parenting it. [Removed this when changing to wxAuiNotebook]
    book_moves_width = book_moves_width_;
	new_tab_button = new wxButton( this, ID_BUTTON_TAB_NEW, "New Tab" );
    wxClientDC dc(new_tab_button);
    wxCoord width, height, descent, external_leading;
    dc.GetTextExtent( "New Tab", &width, &height, &descent, &external_leading );
    new_tab_button_width = (width*130) / 100;
	//wxSize butt_sz = new_tab_button->GetSize();
	//new_tab_button_width = butt_sz.x;
#ifdef AUI_NOTEBOOK
    notebook = new wxAuiNotebook(this, wxID_ANY, wxPoint(5,0), wxSize(siz.x,siz.y), 
        wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | /*wxAUI_NB_SCROLL_BUTTONS |*/ wxAUI_NB_CLOSE_ON_ALL_TABS );
#else
    notebook = new wxNotebook(this, wxID_ANY, wxPoint(5,0), wxSize(siz.x-book_moves_width-10-new_tab_button_width-10,siz.y*5) );
#endif
    wxTextCtrl *notebook_page1 = new wxTextCtrl(notebook, wxID_ANY,"", wxDefaultPosition, wxDefaultSize /*wxPoint(0,0), wxSize((90*siz.x)/100,(90*siz.y)/100)*/, wxNO_BORDER );
    notebook->AddPage(notebook_page1,"New Game",true);

    wxPoint pt(siz.x-book_moves_width-2,8);
    wxSize  sz(book_moves_width,siz.y-8);
    gbl_book_moves = new CtrlBoxBookMoves( this,
                          wxID_ANY,
                          pt,
                          sz );
	pt.x -= (new_tab_button_width+10);
	sz.x  =  new_tab_button_width;
	new_tab_button->SetPosition(pt);
	new_tab_button->SetSize(sz);
}

void PanelNotebook::OnSize( wxSizeEvent &evt )
{
    wxSize siz = evt.GetSize();
    wxSize sz1;
    sz1.x = siz.x-book_moves_width-10-new_tab_button_width-10;
    sz1.y = siz.y*5;
    notebook->SetSize(sz1);
    wxSize  sz(book_moves_width,siz.y-8);
    wxPoint pt(siz.x-book_moves_width-2,8);
    gbl_book_moves->SetPosition(pt);
	pt.x -= (new_tab_button_width+10);
	sz.x  =  new_tab_button_width;
	new_tab_button->SetPosition(pt);
	new_tab_button->SetSize(sz);
}

void PanelNotebook::OnTabNew( wxCommandEvent& WXUNUSED(event) )
{
	objs.gl->CmdTabNew();
}

#ifdef AUI_NOTEBOOK
void PanelNotebook::OnTabClose( wxAuiNotebookEvent& event )
{
    int idx = event.GetSelection();
    if( objs.gl )
        objs.gl->OnTabClose(idx);
}

void PanelNotebook::OnTabSelected( wxAuiNotebookEvent& event )
#else
void PanelNotebook::OnTabSelected( wxBookCtrlEvent& event )
#endif
{
    int idx = event.GetSelection();
    if( objs.gl )
        objs.gl->OnTabSelected(idx);
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

//  We also optionally prepend the time - to prepend the time instantiate a DebugPrintfTime object
//  on the stack - no need to use it
static int dbg_printf_prepend_time=0;
static FILE *teefile;   // private debugging log
#ifdef DURING_DEVELOPMENT
static bool dbg_console_enabled = true;     // set this to false except during development
#else
static bool dbg_console_enabled = false;    // set this to false except during development
#endif
DebugPrintfTime::DebugPrintfTime()  { dbg_printf_prepend_time++; }
DebugPrintfTime::~DebugPrintfTime() { dbg_printf_prepend_time--; if(dbg_printf_prepend_time<0) dbg_printf_prepend_time=0; }
int AutoTimer::instance_cnt;
AutoTimer *AutoTimer::instance_ptr;

#ifndef KILL_DEBUG_COMPLETELY
int core_printf( const char *fmt, ... )
{
    if( !dbg_console_enabled )
        return 0;
    int ret=0;
	va_list args;
	va_start( args, fmt );
    char buf[1000];
    char *p = buf;
    if( dbg_printf_prepend_time )
    {
        AutoTimer *at = AutoTimer::instance_ptr;
        if( at )
        {
            sprintf( p, "%fms ", at->Elapsed() );
            p = strchr(buf,'\0');
        }
        else
        {
            time_t rawtime;
            struct tm * timeinfo;
            time ( &rawtime );
            timeinfo = localtime ( &rawtime );
            strcpy( p, asctime(timeinfo) );
            p = strchr(buf,'\n');
        }
    }
    vsnprintf( p, sizeof(buf)-2-(p-buf), fmt, args ); 
    fputs(buf,stdout);
    if( teefile )
        fputs(buf,teefile);
    va_end(args);
    return ret;
}
#endif

// New approach to logging - use a console window to simulate Mac behaviour on Windows
#ifdef THC_WINDOWS
static const WORD MAX_CONSOLE_LINES = 500;      // maximum mumber of lines the output console should have
void RedirectIOToConsole()
{
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    // allocate a console for this app
    AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(handle, &coninfo);

	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(handle, coninfo.dwSize);

	int height = 60;
	int width = 80;

	_SMALL_RECT rect;
	rect.Top = 0;
	rect.Left = 0;
	rect.Bottom = height - 1;
	rect.Right = width - 1;

	SetConsoleWindowInfo(handle, TRUE, &rect);            // Set Window Size

#if 1
	// Redirect the CRT standard input, output, and error handles to the console
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	//Clear the error state for each of the C++ standard stream objects. We need to do this, as
	//attempts to access the standard streams before they refer to a valid target will cause the
	//iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
	//to always occur during startup regardless of whether anything has been read from or written to
	//the console or not.
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
#else


    // redirect unbuffered STDOUT to the console
    int hConHandle;
    long lStdHandle;
    FILE *fp;
    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
	/*
    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console
    lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );

    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    ios::sync_with_stdio(); */
#endif
}
#endif


class ChessFrame: public wxFrame
{
public:
    ChessFrame(const wxString& title, const wxPoint& pos, const wxSize& size, bool pos_siz_restored );
    ~ChessFrame()
    {
        context->resize_ready = false;   // stops a bogus resize during shutdown on mac
        m_mgr.UnInit();                 // deinitialize the frame manager
    }  
    void OnIdle(wxIdleEvent& event);
    void OnMove       (wxMoveEvent &event);
    void OnMouseWheel( wxMouseEvent &event );
    void OnChar( wxKeyEvent &event );
    void OnTimeout    (wxTimerEvent& event);
    void OnQuit       (wxCommandEvent &);
    void OnClose      (wxCloseEvent &);
    void OnAbout      (wxCommandEvent &);
//  void OnUnimplemented    (wxCommandEvent &);
    void OnHelp             (wxCommandEvent &);
    void OnCredits          (wxCommandEvent &);
    void OnFlip             (wxCommandEvent &);
    void OnButtonUp         (wxCommandEvent &);
    void OnButtonDown       (wxCommandEvent &);
    void OnButtonLeft       (wxCommandEvent &);
    void OnButtonRight      (wxCommandEvent &);

    void OnSetPosition(wxCommandEvent &);
    void OnNewGame    (wxCommandEvent &);
    void OnTakeback   (wxCommandEvent &);
        void OnUpdateTakeback(wxUpdateUIEvent &);
    void OnMoveNow    (wxCommandEvent &);
        void OnUpdateMoveNow(wxUpdateUIEvent &);
    void OnDraw       (wxCommandEvent &);
        void OnUpdateDraw(wxUpdateUIEvent &);
    void OnWhiteResigns(wxCommandEvent &);
        void OnUpdateWhiteResigns(wxUpdateUIEvent &);
    void OnBlackResigns(wxCommandEvent &);
        void OnUpdateBlackResigns(wxUpdateUIEvent &);
    void OnPlayWhite  (wxCommandEvent &);
        void OnUpdatePlayWhite(wxUpdateUIEvent &);
    void OnPlayBlack  (wxCommandEvent &);
        void OnUpdatePlayBlack(wxUpdateUIEvent &);
    void OnSwapSides  (wxCommandEvent &);
        void OnUpdateSwapSides(wxUpdateUIEvent &);
    void OnTabNew  (wxCommandEvent &);
        void OnUpdateTabNew(wxUpdateUIEvent &);
    void OnTabClose  (wxCommandEvent &);
        void OnUpdateTabClose(wxUpdateUIEvent &);
    void OnTabInclude (wxCommandEvent &);
        void OnUpdateTabInclude(wxUpdateUIEvent &);
    void OnKibitz    (wxCommandEvent &);
        void OnUpdateKibitz(wxUpdateUIEvent &);
    void OnClearKibitz    (wxCommandEvent &);
        void OnUpdateClearKibitz(wxUpdateUIEvent &);
    void OnPlayers    (wxCommandEvent &);
        void OnUpdatePlayers(wxUpdateUIEvent &);
    void OnClocks     (wxCommandEvent &);
        void OnUpdateClocks(wxUpdateUIEvent &);
    void OnOptionsReset(wxCommandEvent &);
        void OnUpdateOptionsReset(wxUpdateUIEvent &);
    void OnBook       (wxCommandEvent &);
        void OnUpdateBook(wxUpdateUIEvent &);
    void OnLog        (wxCommandEvent &);
        void OnUpdateLog(wxUpdateUIEvent &);
    void OnEngine     (wxCommandEvent &);
        void OnUpdateEngine(wxUpdateUIEvent &);
    void OnDatabaseMaintenance(wxCommandEvent &);
        void OnUpdateDatabaseMaintenance(wxUpdateUIEvent &);
    void OnDatabaseSearch(wxCommandEvent &);
        void OnUpdateDatabaseSearch(wxUpdateUIEvent &);
    void OnDatabaseShowAll(wxCommandEvent &);
        void OnUpdateDatabaseShowAll(wxUpdateUIEvent &);
    void OnDatabasePlayers(wxCommandEvent &);
        void OnUpdateDatabasePlayers(wxUpdateUIEvent &);
    void OnDatabaseSelect(wxCommandEvent &);
        void OnUpdateDatabaseSelect(wxUpdateUIEvent &);
    void OnDatabaseCreate(wxCommandEvent &);
        void OnUpdateDatabaseCreate(wxUpdateUIEvent &);
    void OnDatabaseAppend(wxCommandEvent &);
        void OnUpdateDatabaseAppend(wxUpdateUIEvent &);
    void OnDatabasePattern(wxCommandEvent &);
        void OnUpdateDatabasePattern(wxUpdateUIEvent &);
    void OnDatabaseMaterial(wxCommandEvent &);
        void OnUpdateDatabaseMaterial(wxUpdateUIEvent &);
    void OnTraining   (wxCommandEvent &);
        void OnUpdateTraining(wxUpdateUIEvent &);
    void OnGeneral    (wxCommandEvent &);
        void OnUpdateGeneral (wxUpdateUIEvent &);
    void RefreshLanguageFont( const char *from, bool before_large_font, bool before_no_italics,
                              const char *to,   bool after_large_font,  bool after_no_italics );

    void OnFileNew (wxCommandEvent &);
        void OnUpdateFileNew( wxUpdateUIEvent &);
    void OnFileOpen (wxCommandEvent &);
        void OnUpdateFileOpen(wxUpdateUIEvent &);
    void OnFileOpenLog (wxCommandEvent &);
        void OnUpdateFileOpenLog(wxUpdateUIEvent &);
    void OnFileSave (wxCommandEvent &);
        void OnUpdateFileSave( wxUpdateUIEvent &);
    void OnFileSaveAs (wxCommandEvent &);
        void OnUpdateFileSaveAs( wxUpdateUIEvent &);
    void OnFileSaveGameAs (wxCommandEvent &);
        void OnUpdateFileSaveGameAs( wxUpdateUIEvent &);
    void OnGamesCurrent (wxCommandEvent &);
        void OnUpdateGamesCurrent( wxUpdateUIEvent &);
    void OnGamesDatabase (wxCommandEvent &);
        void OnUpdateGamesDatabase( wxUpdateUIEvent &);
    void OnGamesSession (wxCommandEvent &);
        void OnUpdateGamesSession( wxUpdateUIEvent &);
    void OnGamesClipboard (wxCommandEvent &);
        void OnUpdateGamesClipboard( wxUpdateUIEvent &);
    void OnNextGame (wxCommandEvent &);
        void OnUpdateNextGame( wxUpdateUIEvent &);
    void OnPreviousGame (wxCommandEvent &);
        void OnUpdatePreviousGame( wxUpdateUIEvent &);

    void OnEditCopy (wxCommandEvent &);
        void OnUpdateEditCopy( wxUpdateUIEvent &);
    void OnEditCut (wxCommandEvent &);
        void OnUpdateEditCut( wxUpdateUIEvent &);
    void OnEditPaste (wxCommandEvent &);
        void OnUpdateEditPaste( wxUpdateUIEvent &);
    void OnEditUndo (wxCommandEvent &);
        void OnUpdateEditUndo( wxUpdateUIEvent &);
    void OnEditRedo (wxCommandEvent &);
        void OnUpdateEditRedo( wxUpdateUIEvent &);
    void OnEditDelete (wxCommandEvent &);
        void OnUpdateEditDelete( wxUpdateUIEvent &);
    void OnEditGameDetails (wxCommandEvent &);
        void OnUpdateEditGameDetails( wxUpdateUIEvent &);
    void OnEditGamePrefix (wxCommandEvent &);
        void OnUpdateEditGamePrefix( wxUpdateUIEvent &);
    void OnEditCopyGamePGNToClipboard (wxCommandEvent &);
        void OnUpdateEditCopyGamePGNToClipboard(wxUpdateUIEvent &);
    void OnEditDeleteVariation (wxCommandEvent &);
        void OnUpdateEditDeleteVariation( wxUpdateUIEvent &);
    void OnEditPromote (wxCommandEvent &);
        void OnUpdateEditPromote( wxUpdateUIEvent &);
    void OnEditDemote (wxCommandEvent &);
        void OnUpdateEditDemote( wxUpdateUIEvent &);
    void OnEditDemoteToComment (wxCommandEvent &);
        void OnUpdateEditDemoteToComment( wxUpdateUIEvent &);
    void OnEditPromoteToVariation (wxCommandEvent &);
        void OnUpdateEditPromoteToVariation( wxUpdateUIEvent &);
    void OnEditPromoteRestToVariation (wxCommandEvent &);
        void OnUpdateEditPromoteRestToVariation( wxUpdateUIEvent &);


    DECLARE_EVENT_TABLE()
private:
    PanelContext *context;
    wxTimer m_timer;
    void SetFocusOnList() { if(context) context->SetFocusOnList(); }
    wxAuiManager m_mgr;
};


//-----------------------------------------------------------------------------
// Application class
//-----------------------------------------------------------------------------

IMPLEMENT_APP(ChessApp)

extern void JobBegin();
extern void JobEnd();

wxString argv1;  // can pass a .pgn file on command line
bool gbl_spelling_us;			// True for US spelling
const char *gbl_spell_colour;	// "colour" or "color"

bool ChessApp::OnInit()
{
	//_CrtSetBreakAlloc(1050656);
    srand(time(NULL));
    //_CrtSetBreakAlloc( 198300 ); //563242 );
    //_CrtSetBreakAlloc( 195274 );
    //_CrtSetBreakAlloc( 21007 );
    JobBegin();
    if( argc == 2 )
    {
        argv1 = argv[1];
        if( argv1[0] == '-' )   // any switch is interpreted as turn on console
        {
            argv1 = "";
            dbg_console_enabled = true;
        }
    }
    #ifndef KILL_DEBUG_COMPLETELY
    if( dbg_console_enabled )
    {
        #ifdef THC_WINDOWS
        #ifdef DURING_DEVELOPMENT
        teefile = fopen( "/users/bill/documents/tarrasch/debuglog.txt", "wt" );
        #endif
        RedirectIOToConsole();
        #endif
    }
    #endif
	int lang = wxLocale::GetSystemLanguage();
	gbl_spelling_us = (lang==wxLANGUAGE_ENGLISH_US);
	gbl_spell_colour = gbl_spelling_us ? "color" : "colour";
	cprintf( "Spelling uses US English? %s\n", gbl_spelling_us?"Yes":"No" );
    wxString error_msg;
    int disp_width, disp_height;
    wxDisplaySize(&disp_width, &disp_height);
    cprintf( "Display size = %d x %d\n", disp_width, disp_height );
    objs.repository = new Repository;

    // Use these as fallbacks/defaults
    bool pos_siz_restored = false;
    int xx = (disp_width * 10) /100;
    int yy = (disp_height * 10) /100;
    int ww = (disp_width * 80) /100;
    int hh = (disp_height * 80) /100;
    wxPoint pt(xx,yy);
    wxSize  sz(ww,hh);

    // Save restore windows logic
    if( objs.repository->nv.m_x>=0 &&
        objs.repository->nv.m_y>=0 &&
        objs.repository->nv.m_w>=0 &&
        objs.repository->nv.m_h>=0
      )
    {
        if( objs.repository->nv.m_x + objs.repository->nv.m_w <= disp_width &&
            objs.repository->nv.m_y + objs.repository->nv.m_h <= disp_height
          )
        {
            pos_siz_restored = true;
            pt.x = objs.repository->nv.m_x;
            pt.y = objs.repository->nv.m_y;
            sz.x = objs.repository->nv.m_w;
            sz.y = objs.repository->nv.m_h;
        }
    }

    // Try to detect and restore a MAXIMISED frame (initial x,y slightly negative, initial w,h slightly larger than display)
    bool  maximize = false;
    if( !pos_siz_restored &&
        objs.repository->nv.m_x>=-8 &&
        objs.repository->nv.m_y>=-8 &&
        objs.repository->nv.m_w>=0 &&
        objs.repository->nv.m_h>=0 &&
        objs.repository->nv.m_x + objs.repository->nv.m_w <= disp_width+8 &&
        objs.repository->nv.m_y + objs.repository->nv.m_h <= disp_height+8
      )
    {
        pos_siz_restored = true;
        maximize = true;
    }
    objs.repository->nv.m_x = pt.x;
    objs.repository->nv.m_y = pt.y;
    objs.repository->nv.m_w = sz.x;
    objs.repository->nv.m_h = sz.y;
    ChessFrame *frame = new ChessFrame (_T("Tarrasch Chess GUI V3 -- Beta version use cautiously"),
                                  pt, sz, pos_siz_restored );
    if( maximize )
        frame->Maximize();
    objs.frame = frame;
    SetTopWindow (frame);
    frame->Show(true);
    if( objs.gl )
        objs.gl->StatusUpdate();
    objs.db         = new Database( objs.repository->database.m_file.c_str() );
    return true;
}


int ChessApp::OnExit()
{
	if (teefile)
	{
		fclose(teefile);
		teefile = NULL;
	}
    cprintf( "ChessApp::OnExit(): May wait for tiny database load here...\n" );
    extern wxMutex *KillWorkerThread();
    wxMutex *ptr_mutex_tiny_database = KillWorkerThread();
    wxMutexLocker lock(*ptr_mutex_tiny_database);
    cprintf( "ChessApp::OnExit() if we did wait, that wait is now over\n" );
    if( objs.uci_interface )
    {
        delete objs.uci_interface;
        objs.uci_interface = NULL;
    }
    JobEnd();
    if( objs.gl )
    {
        delete objs.gl;
        objs.gl = NULL;
    }
    if( objs.tabs )
    {
        delete objs.tabs;
        objs.tabs = NULL;
    }
    if( objs.db )
    {
        delete objs.db;
        objs.db = NULL;
    }
    if( objs.cws )
    {
        delete objs.cws;
        objs.cws = NULL;
    }
    if( objs.repository )
    {
        delete objs.repository;
        objs.repository = NULL;
    }
    if( objs.book )
    {
        delete objs.book;
        objs.book = NULL;
    }
    if( objs.log )
    {
        delete objs.log;
        objs.log = NULL;
    }
    if( objs.session )
    {
        delete objs.session;
        objs.session = NULL;
    }
    return wxApp::OnExit();
}

//-----------------------------------------------------------------------------
// Main frame
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ChessFrame, wxFrame)
    EVT_CLOSE(                     ChessFrame::OnClose )
    EVT_MENU (wxID_EXIT,           ChessFrame::OnQuit)
    EVT_MENU (ID_CMD_ABOUT,        ChessFrame::OnAbout)
    EVT_MENU (ID_HELP_HELP,        ChessFrame::OnHelp)
    EVT_MENU (ID_HELP_CREDITS,     ChessFrame::OnCredits)
    EVT_MENU (ID_CMD_FLIP,         ChessFrame::OnFlip)
    EVT_MENU (ID_CMD_KIBITZ,       ChessFrame::OnKibitz)
        EVT_UPDATE_UI (ID_CMD_KIBITZ,      ChessFrame::OnUpdateKibitz)
    EVT_MENU (ID_CMD_CLEAR_KIBITZ,      ChessFrame::OnClearKibitz)
        EVT_UPDATE_UI (ID_CMD_CLEAR_KIBITZ,      ChessFrame::OnUpdateClearKibitz)
    EVT_MENU (ID_CMD_SET_POSITION, ChessFrame::OnSetPosition)
    EVT_MENU (ID_CMD_NEW_GAME,     ChessFrame::OnNewGame)
    EVT_MENU (wxID_UNDO,           ChessFrame::OnEditUndo)
        EVT_UPDATE_UI (wxID_UNDO,           ChessFrame::OnUpdateEditUndo)
    EVT_MENU (wxID_REDO,           ChessFrame::OnEditRedo)
        EVT_UPDATE_UI (wxID_REDO,           ChessFrame::OnUpdateEditRedo)
    EVT_MENU (ID_CMD_TAKEBACK,     ChessFrame::OnTakeback)
        EVT_UPDATE_UI (ID_CMD_TAKEBACK,     ChessFrame::OnUpdateTakeback)
    EVT_MENU (ID_CMD_MOVENOW,      ChessFrame::OnMoveNow)
        EVT_UPDATE_UI (ID_CMD_MOVENOW,      ChessFrame::OnUpdateMoveNow)
    EVT_MENU (ID_CMD_DRAW,         ChessFrame::OnDraw)
        EVT_UPDATE_UI (ID_CMD_DRAW,         ChessFrame::OnUpdateDraw)
    EVT_MENU (ID_CMD_PLAY_WHITE,   ChessFrame::OnPlayWhite)
        EVT_UPDATE_UI (ID_CMD_PLAY_WHITE,   ChessFrame::OnUpdatePlayWhite)
    EVT_MENU (ID_CMD_PLAY_BLACK,   ChessFrame::OnPlayBlack)
        EVT_UPDATE_UI (ID_CMD_PLAY_BLACK,   ChessFrame::OnUpdatePlayBlack)
    EVT_MENU (ID_CMD_WHITE_RESIGNS,ChessFrame::OnWhiteResigns)
        EVT_UPDATE_UI (ID_CMD_WHITE_RESIGNS,ChessFrame::OnUpdateWhiteResigns)
    EVT_MENU (ID_CMD_BLACK_RESIGNS,ChessFrame::OnBlackResigns)
        EVT_UPDATE_UI (ID_CMD_BLACK_RESIGNS,ChessFrame::OnUpdateBlackResigns)
    EVT_MENU (ID_CMD_SWAP_SIDES,   ChessFrame::OnSwapSides)
        EVT_UPDATE_UI (ID_CMD_SWAP_SIDES,   ChessFrame::OnUpdateSwapSides)
    EVT_MENU (ID_FILE_TAB_NEW,   ChessFrame::OnTabNew)
        EVT_UPDATE_UI (ID_FILE_TAB_NEW,   ChessFrame::OnUpdateTabNew)
    EVT_MENU (ID_FILE_TAB_CLOSE,   ChessFrame::OnTabClose)
        EVT_UPDATE_UI (ID_FILE_TAB_CLOSE,   ChessFrame::OnUpdateTabClose)
    EVT_MENU (ID_FILE_TAB_INCLUDE,   ChessFrame::OnTabInclude)
        EVT_UPDATE_UI (ID_FILE_TAB_INCLUDE,   ChessFrame::OnUpdateTabInclude)

    EVT_MENU (wxID_NEW,                     ChessFrame::OnFileNew)
        EVT_UPDATE_UI (wxID_NEW,                ChessFrame::OnUpdateFileNew)
    EVT_MENU (wxID_OPEN,                    ChessFrame::OnFileOpen)
        EVT_UPDATE_UI (wxID_OPEN,               ChessFrame::OnUpdateFileOpen)
    EVT_MENU (ID_FILE_OPEN_LOG,             ChessFrame::OnFileOpenLog)
        EVT_UPDATE_UI (ID_FILE_OPEN_LOG,        ChessFrame::OnUpdateFileOpenLog)
    EVT_MENU (wxID_SAVE,                    ChessFrame::OnFileSave)
        EVT_UPDATE_UI (wxID_SAVE,               ChessFrame::OnUpdateFileSave)
    EVT_MENU (wxID_SAVEAS,                  ChessFrame::OnFileSaveAs)
        EVT_UPDATE_UI (wxID_SAVEAS,             ChessFrame::OnUpdateFileSaveAs)
    EVT_MENU (ID_FILE_SAVE_GAME_AS,         ChessFrame::OnFileSaveGameAs)
        EVT_UPDATE_UI (ID_FILE_SAVE_GAME_AS,    ChessFrame::OnUpdateFileSaveGameAs)
    EVT_MENU (ID_GAMES_CURRENT,             ChessFrame::OnGamesCurrent)
        EVT_UPDATE_UI (ID_GAMES_CURRENT,        ChessFrame::OnUpdateGamesCurrent)
    EVT_MENU (ID_GAMES_DATABASE,            ChessFrame::OnGamesDatabase)
        EVT_UPDATE_UI (ID_GAMES_DATABASE,       ChessFrame::OnUpdateGamesDatabase)
    EVT_MENU (ID_GAMES_SESSION,             ChessFrame::OnGamesSession)
        EVT_UPDATE_UI (ID_GAMES_SESSION,        ChessFrame::OnUpdateGamesSession)
    EVT_MENU (ID_GAMES_CLIPBOARD,           ChessFrame::OnGamesClipboard)
        EVT_UPDATE_UI (ID_GAMES_CLIPBOARD,      ChessFrame::OnUpdateGamesClipboard)
    EVT_MENU (ID_CMD_NEXT_GAME,             ChessFrame::OnNextGame)
        EVT_UPDATE_UI (ID_CMD_NEXT_GAME,        ChessFrame::OnUpdateNextGame)
    EVT_MENU (ID_CMD_PREVIOUS_GAME,         ChessFrame::OnPreviousGame)
        EVT_UPDATE_UI (ID_CMD_PREVIOUS_GAME,    ChessFrame::OnUpdatePreviousGame)

    EVT_MENU (wxID_COPY,                    ChessFrame::OnEditCopy)
        EVT_UPDATE_UI (wxID_COPY,                       ChessFrame::OnUpdateEditCopy)
    EVT_MENU (wxID_CUT,                     ChessFrame::OnEditCut)
        EVT_UPDATE_UI (wxID_CUT,                        ChessFrame::OnUpdateEditCut)
    EVT_MENU (wxID_PASTE,                   ChessFrame::OnEditPaste)
        EVT_UPDATE_UI (wxID_PASTE,                      ChessFrame::OnUpdateEditPaste)
    EVT_MENU (wxID_DELETE,                  ChessFrame::OnEditDelete)
        EVT_UPDATE_UI (wxID_DELETE,                     ChessFrame::OnUpdateEditDelete)
    EVT_MENU (ID_EDIT_GAME_DETAILS,         ChessFrame::OnEditGameDetails)
        EVT_UPDATE_UI (ID_EDIT_GAME_DETAILS,            ChessFrame::OnUpdateEditGameDetails)
    EVT_MENU (ID_EDIT_GAME_PREFIX,          ChessFrame::OnEditGamePrefix)
        EVT_UPDATE_UI (ID_EDIT_GAME_PREFIX,             ChessFrame::OnUpdateEditGamePrefix)
    EVT_MENU(ID_COPY_GAME_PGN_TO_CLIPBOARD, ChessFrame::OnEditCopyGamePGNToClipboard)
        EVT_UPDATE_UI(ID_COPY_GAME_PGN_TO_CLIPBOARD,    ChessFrame::OnUpdateEditCopyGamePGNToClipboard)
    EVT_MENU (ID_EDIT_DELETE_VARIATION,     ChessFrame::OnEditDeleteVariation)
        EVT_UPDATE_UI (ID_EDIT_DELETE_VARIATION,        ChessFrame::OnUpdateEditDeleteVariation)
    EVT_MENU (ID_EDIT_PROMOTE,              ChessFrame::OnEditPromote)
        EVT_UPDATE_UI (ID_EDIT_PROMOTE,                 ChessFrame::OnUpdateEditPromote)
    EVT_MENU (ID_EDIT_DEMOTE,               ChessFrame::OnEditDemote)
        EVT_UPDATE_UI (ID_EDIT_DEMOTE,                  ChessFrame::OnUpdateEditDemote)
    EVT_MENU (ID_EDIT_DEMOTE_TO_COMMENT,    ChessFrame::OnEditDemoteToComment)
        EVT_UPDATE_UI (ID_EDIT_DEMOTE_TO_COMMENT,       ChessFrame::OnUpdateEditDemoteToComment)
    EVT_MENU (ID_EDIT_PROMOTE_TO_VARIATION, ChessFrame::OnEditPromoteToVariation)
        EVT_UPDATE_UI (ID_EDIT_PROMOTE_TO_VARIATION,    ChessFrame::OnUpdateEditPromoteToVariation)
    EVT_MENU (ID_EDIT_PROMOTE_REST_TO_VARIATION, ChessFrame::OnEditPromoteRestToVariation)
        EVT_UPDATE_UI (ID_EDIT_PROMOTE_REST_TO_VARIATION,    ChessFrame::OnUpdateEditPromoteRestToVariation)

    EVT_MENU (ID_OPTIONS_PLAYERS,  ChessFrame::OnPlayers)
        EVT_UPDATE_UI (ID_OPTIONS_PLAYERS,   ChessFrame::OnUpdatePlayers)
    EVT_MENU (ID_OPTIONS_CLOCKS,   ChessFrame::OnClocks)
        EVT_UPDATE_UI (ID_OPTIONS_CLOCKS,   ChessFrame::OnUpdateClocks)
    EVT_MENU (ID_OPTIONS_BOOK,     ChessFrame::OnBook)
        EVT_UPDATE_UI (ID_OPTIONS_BOOK,     ChessFrame::OnUpdateBook)
    EVT_MENU (ID_OPTIONS_LOG,      ChessFrame::OnLog)
        EVT_UPDATE_UI (ID_OPTIONS_LOG,      ChessFrame::OnUpdateLog)
    EVT_MENU (ID_OPTIONS_ENGINE,   ChessFrame::OnEngine)
        EVT_UPDATE_UI (ID_OPTIONS_ENGINE,   ChessFrame::OnUpdateEngine)
    EVT_MENU (ID_OPTIONS_TRAINING, ChessFrame::OnTraining)
        EVT_UPDATE_UI (ID_OPTIONS_TRAINING, ChessFrame::OnUpdateTraining)
    EVT_MENU (ID_OPTIONS_GENERAL,  ChessFrame::OnGeneral)
        EVT_UPDATE_UI (ID_OPTIONS_GENERAL,  ChessFrame::OnUpdateGeneral)
    EVT_MENU (ID_OPTIONS_RESET,    ChessFrame::OnOptionsReset)
        EVT_UPDATE_UI (ID_OPTIONS_RESET,    ChessFrame::OnUpdateOptionsReset)

    EVT_MENU (ID_DATABASE_SEARCH,                   ChessFrame::OnDatabaseSearch)
        EVT_UPDATE_UI (ID_DATABASE_SEARCH,               ChessFrame::OnUpdateDatabaseSearch)
    EVT_MENU (ID_DATABASE_SHOW_ALL,                 ChessFrame::OnDatabaseShowAll)
        EVT_UPDATE_UI (ID_DATABASE_SHOW_ALL,             ChessFrame::OnUpdateDatabaseShowAll)
    EVT_MENU (ID_DATABASE_PLAYERS,                  ChessFrame::OnDatabasePlayers)
        EVT_UPDATE_UI (ID_DATABASE_PLAYERS,              ChessFrame::OnUpdateDatabasePlayers)
    EVT_MENU (ID_DATABASE_SELECT,                   ChessFrame::OnDatabaseSelect)
        EVT_UPDATE_UI (ID_DATABASE_SELECT,              ChessFrame::OnUpdateDatabaseSelect)
    EVT_MENU (ID_DATABASE_CREATE,                   ChessFrame::OnDatabaseCreate)
        EVT_UPDATE_UI (ID_DATABASE_APPEND,              ChessFrame::OnUpdateDatabaseCreate)
    EVT_MENU (ID_DATABASE_APPEND,                   ChessFrame::OnDatabaseAppend)
        EVT_UPDATE_UI (ID_DATABASE_APPEND,              ChessFrame::OnUpdateDatabaseAppend)
    EVT_MENU (ID_DATABASE_PATTERN,                  ChessFrame::OnDatabasePattern)
        EVT_UPDATE_UI (ID_DATABASE_PATTERN,             ChessFrame::OnUpdateDatabasePattern)
    EVT_MENU (ID_DATABASE_MATERIAL,                 ChessFrame::OnDatabaseMaterial)
        EVT_UPDATE_UI (ID_DATABASE_MATERIAL,            ChessFrame::OnUpdateDatabaseMaterial)
    EVT_MENU (ID_DATABASE_MAINTENANCE,              ChessFrame::OnDatabaseMaintenance)
        EVT_UPDATE_UI (ID_DATABASE_MAINTENANCE,          ChessFrame::OnUpdateDatabaseMaintenance)

    EVT_TOOL (ID_CMD_FLIP,         ChessFrame::OnFlip)
    EVT_TOOL (ID_CMD_NEXT_GAME,    ChessFrame::OnNextGame)
    EVT_TOOL (ID_CMD_PREVIOUS_GAME,ChessFrame::OnPreviousGame)
    EVT_TOOL (ID_HELP_HELP,        ChessFrame::OnHelp)
    EVT_TOOL (ID_BUTTON_UP,        ChessFrame::OnButtonUp)
    EVT_TOOL (ID_BUTTON_DOWN,      ChessFrame::OnButtonDown)
    EVT_TOOL (ID_BUTTON_LEFT,      ChessFrame::OnButtonLeft)
    EVT_TOOL (ID_BUTTON_RIGHT,     ChessFrame::OnButtonRight)
    EVT_IDLE (ChessFrame::OnIdle)
    EVT_TIMER( TIMER_ID, ChessFrame::OnTimeout)
    EVT_MOVE (ChessFrame::OnMove)
    EVT_CHAR( ChessFrame::OnChar )
    EVT_MOUSEWHEEL( ChessFrame::OnMouseWheel )
END_EVENT_TABLE()



ChessFrame::ChessFrame(const wxString& title, const wxPoint& pos, const wxSize& size, bool pos_siz_restored )
    : wxFrame((wxFrame *)NULL, wxID_ANY, title, pos, size ) //, wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN|
                                                            //        wxNO_FULL_REPAINT_ON_RESIZE )
{
    // Set the frame icon
    SetIcon(wxICON(bbbbbbbb));      // for explanation of 'bbbbbbbb' see Tarrasch.rc

    // Timer
    m_timer.SetOwner(this,TIMER_ID);

    // Menu - File
    wxMenu *menu_file     = new wxMenu;
    menu_file->Append (wxID_NEW,                    _T("New\tCtrl+N"));
    menu_file->Append (wxID_OPEN,                   _T("Open\tCtrl+O"));
    menu_file->Append (ID_FILE_OPEN_LOG,            _T("Open log file"));
    menu_file->Append (wxID_SAVE,                   _T("Save\tCtrl+S"));
    menu_file->Append (wxID_SAVEAS,                 _T("Save as"));
    menu_file->Append (ID_FILE_SAVE_GAME_AS,        _T("Save game as"));
    menu_file->AppendSeparator();
    menu_file->Append (ID_FILE_TAB_NEW,              _T("New tab\tCtrl+T"));
    menu_file->Append (ID_FILE_TAB_CLOSE,            _T("Close tab\tCtrl+D"));
    menu_file->Append (ID_FILE_TAB_INCLUDE,          _T("Add tab to file"));
    menu_file->AppendSeparator();
    menu_file->Append (wxID_EXIT,                   _T("E&xit\tAlt-X"));

    // Menu - Edit
    wxMenu *menu_edit     = new wxMenu;
    menu_edit->Append (wxID_COPY,                    _T("Copy\tCtrl+C"));
    menu_edit->Append (wxID_CUT,                     _T("Cut\tCtrl+X"));
    menu_edit->Append (wxID_PASTE,                   _T("Paste\tCtrl+V"));
    menu_edit->Append (wxID_UNDO,                    "Undo\tCtrl+Z");
    menu_edit->Append (wxID_REDO,                    _T("Redo\tCtrl+Y"));
    menu_edit->Append (wxID_DELETE,                  _T("Delete comment text or remainder of variation\tDel"));
    menu_edit->Append (ID_EDIT_GAME_DETAILS,         _T("Edit game details"));
//    menu_edit->Append (ID_EDIT_GAME_PREFIX,          _T("Edit game prefix"));
    menu_edit->Append (ID_COPY_GAME_PGN_TO_CLIPBOARD,_T("Copy game to system clipboard (PGN)"));
    menu_edit->Append (ID_EDIT_DELETE_VARIATION,     _T("Delete variation"));
    menu_edit->Append (ID_EDIT_PROMOTE,              _T("Promote variation"));
    menu_edit->Append (ID_EDIT_DEMOTE,               _T("Demote variation"));
    menu_edit->Append (ID_EDIT_DEMOTE_TO_COMMENT,    _T("Demote rest of variation to comment\tAlt-D"));
    menu_edit->Append (ID_EDIT_PROMOTE_TO_VARIATION, _T("Promote comment to moves"));
    menu_edit->Append (ID_EDIT_PROMOTE_REST_TO_VARIATION, _T("Promote rest of comment to moves\tAlt-P"));

    // Menu - Games
    wxMenu *menu_games   = new wxMenu;
    menu_games->Append (ID_GAMES_CURRENT,        _T("Current file\tCtrl+L"));
    menu_games->Append (ID_GAMES_DATABASE,     _T("Database"));
    menu_games->Append (ID_GAMES_SESSION,        _T("Session"));
    menu_games->Append (ID_GAMES_CLIPBOARD,      _T("Clipboard"));
    menu_games->AppendSeparator();
    menu_games->Append (ID_CMD_NEXT_GAME,       _T("Next game from file"));
    menu_games->Append (ID_CMD_PREVIOUS_GAME,   _T("Previous game from file"));

    // Menu - Commands
    wxMenu *menu_commands = new wxMenu;
    menu_commands->Append (ID_CMD_NEW_GAME,     _T("New Game"));
    menu_commands->Append (ID_CMD_FLIP,         _T("Flip board\tCtrl+F"));
    menu_commands->Append (ID_CMD_TAKEBACK,     _T("Take back\tCtrl+B"));
    menu_commands->Append (ID_CMD_SET_POSITION, _T("Set position\tCtrl+P"));
    menu_commands->Append (ID_CMD_KIBITZ,       _T("Start kibitzer\tCtrl+K"));
    #if 1 // auto_kibitz_clear (if auto kibitz clear, comment out manual clear)
//  menu_commands->Append (ID_CMD_CLEAR_KIBITZ, _T("Clear kibitz text"));
    #else // manual_kibitz_clear
    menu_commands->Append (ID_CMD_CLEAR_KIBITZ, _T("Clear kibitz text"));
    #endif

    menu_commands->Append (ID_CMD_DRAW,             _T("Draw"));
    menu_commands->Append (ID_CMD_WHITE_RESIGNS,    _T("White resigns"));
    menu_commands->Append (ID_CMD_BLACK_RESIGNS,    _T("Black resigns"));
    menu_commands->Append (ID_CMD_PLAY_WHITE,       _T("Play white"));
    menu_commands->Append (ID_CMD_PLAY_BLACK,       _T("Play black"));
    menu_commands->Append (ID_CMD_SWAP_SIDES,       _T("Swap sides\tAlt-S"));
    menu_commands->Append (ID_CMD_MOVENOW,          _T("Move now\tAlt-M"));

    // Options
    wxMenu *menu_options = new wxMenu;
    menu_options->Append (ID_OPTIONS_ENGINE,       _T("Engine"));
    menu_options->Append (ID_OPTIONS_GENERAL,      _T("General"));
    menu_options->Append (ID_OPTIONS_PLAYERS,      _T("Player names"));
    menu_options->Append (ID_OPTIONS_CLOCKS,       _T("Clocks"));
    menu_options->Append (ID_OPTIONS_LOG,          _T("Log file"));
    menu_options->Append (ID_OPTIONS_BOOK,         _T("Opening book"));
    menu_options->Append (ID_OPTIONS_TRAINING,     _T("Training"));
    menu_options->Append (ID_OPTIONS_RESET,        _T("Reset to factory defaults"));

    // Database
    wxMenu *menu_database = new wxMenu;
    menu_database->Append (ID_DATABASE_SEARCH,              _T("Position search"));
    menu_database->Append (ID_DATABASE_PATTERN,             _T("Pattern search"));
    menu_database->Append (ID_DATABASE_MATERIAL,            _T("Material balance search"));
    menu_database->Append (ID_DATABASE_SHOW_ALL,            _T("Show all games"));
    menu_database->Append (ID_DATABASE_PLAYERS,             _T("Show all ordered by player"));
    menu_database->Append (ID_DATABASE_SELECT,              _T("Select current database"));
    menu_database->Append (ID_DATABASE_CREATE,              _T("Create new database"));
    menu_database->Append (ID_DATABASE_APPEND,              _T("Append to database"));
    // menu_database->Append (ID_DATABASE_MAINTENANCE,         _T("INTERNAL TEST - REMOVE SOON - Maintain database"));

    // Help
    wxMenu *menu_help     = new wxMenu;
    menu_help->Append (ID_CMD_ABOUT,                _T("About"));
    menu_help->Append (ID_HELP_HELP,                _T("Help"));
    menu_help->Append (ID_HELP_CREDITS,             _T("Credits"));

    // Menu bar
    wxMenuBar *menu = new wxMenuBar;
    menu->Append (menu_file,     _T("&File"));
    menu->Append (menu_edit,     _T("&Edit"));
    menu->Append (menu_games,    _T("&Games"));
    menu->Append (menu_commands, _T("&Commands"));
    menu->Append (menu_options,  _T("&Options"));
    menu->Append (menu_database, _T("&Database"));
    menu->Append (menu_help,     _T("&Help"));
    SetMenuBar( menu );

    // Create a status bar
    CreateStatusBar(3);
    int widths[3] = {-1,-200,-100};
    SetStatusWidths(3,widths);

#ifdef THC_WINDOWS
    wxToolBar *toolbar = new wxToolBar(this, wxID_ANY,
                               wxDefaultPosition, wxDefaultSize,
                               wxTB_HORIZONTAL|wxNO_BORDER);
#else
    wxToolBar *toolbar = CreateToolBar(); //new wxToolBar(this, wxID_ANY,
                               //wxDefaultPosition, wxDefaultSize,
                               //wxTB_HORIZONTAL|wxNO_BORDER);
#endif

//  wxBitmap bmpNew    (new_xpm);
    wxBitmap bmpOpen   (open_xpm);
    wxBitmap bmpSave   (save_xpm);
    wxBitmap bmpCopy   (copy_xpm);
    wxBitmap bmpCut    (cut_xpm);
    wxBitmap bmpPaste  (paste_xpm);
    wxBitmap bmp_undo  (undo_xpm); //(bill_undo_xpm);
    wxBitmap bmp_redo  (redo_xpm); //(bill_redo_xpm);
    wxBitmap bmp_left  (left_xpm);
    wxBitmap bmp_right (right_xpm);
    wxBitmap bmp_up    (up_xpm);
    wxBitmap bmp_down  (down_xpm);
    wxBitmap bmp_flip  (flip_xpm);
    wxBitmap bmp_database  (database_xpm);
    wxBitmap bmp_help  (bill_help_xpm);
//  wxBitmap bmpNew    (bill_file_new_xpm);
//  wxBitmap bmp_game_new   (bill_game_new_xpm);
    wxBitmap bmpNew         (bill_doc_single_xpm);
    wxBitmap bmp_game_new   (bill_doc_stack_xpm);
    wxBitmap bmp_kibitzer   (bill_kibitzer_xpm);
    wxBitmap bmp_next       (bill_next_game_xpm);
    wxBitmap bmp_previous   (bill_previous_game_xpm);

    // I have modified all the icons to be 16 high (not 15) since a couple
    //  of the Reptor icons I brought across really needed 16.
    wxSize our_size(16,16);
    toolbar->SetToolBitmapSize(our_size);

    // Avoid an annoying deprecation warning
    #define ADD_TOOL(id,bm,txt) toolbar->AddTool( id,  wxEmptyString, bm, wxNullBitmap, wxITEM_NORMAL,txt, wxEmptyString, NULL)
    /*toolbar->AddTool( wxID_NEW,  wxEmptyString, bmpNew,
                        wxNullBitmap, wxITEM_NORMAL,
                        wxT("New file"), wxEmptyString, NULL); */


    ADD_TOOL( wxID_NEW,  bmpNew,  wxT("New file") );
    ADD_TOOL( ID_CMD_NEW_GAME,  bmp_game_new,  wxT("New game") );
    ADD_TOOL( wxID_OPEN, bmpOpen, wxT("Open") );
    ADD_TOOL( wxID_SAVE, bmpSave, wxT("Save") );
    int nbr_separators = 0;
#ifdef THC_WINDOWS
    toolbar->AddSeparator();
    nbr_separators++;
#endif
    toolbar->AddSeparator();
    nbr_separators++;
    ADD_TOOL( wxID_COPY, bmpCopy, _T("Copy") ); //, toolBarBitmaps[Tool_copy], _T("Toggle button 2"), wxITEM_CHECK);
    ADD_TOOL( wxID_CUT,  bmpCut,  _T("Cut")  ); //, toolBarBitmaps[Tool_cut], _T("Toggle/Untoggle help button"));
    ADD_TOOL( wxID_PASTE,bmpPaste,_T("Paste")); // , toolBarBitmaps[Tool_paste], _T("Paste"));
    ADD_TOOL( wxID_UNDO, bmp_undo, "Undo");
    ADD_TOOL( wxID_REDO, bmp_redo, "Redo");
#ifdef THC_WINDOWS
    toolbar->AddSeparator();
    nbr_separators++;
#endif
    toolbar->AddSeparator();
    nbr_separators++;
    ADD_TOOL( ID_CMD_FLIP,      bmp_flip, "Flip board");
    ADD_TOOL( ID_GAMES_DATABASE, bmp_database, "Database");
    ADD_TOOL( ID_CMD_KIBITZ,    bmp_kibitzer,  "Kibitzer start/stop");
    ADD_TOOL( ID_BUTTON_DOWN,   bmp_down,  "Forward (main line)");
    ADD_TOOL( ID_BUTTON_UP,     bmp_up,    "Backward (main line)");
    ADD_TOOL( ID_BUTTON_RIGHT,  bmp_right, "Forward" );
    ADD_TOOL( ID_BUTTON_LEFT,   bmp_left,  "Backward" );
#ifdef THC_WINDOWS
    toolbar->AddSeparator();
    nbr_separators++;
#endif
    toolbar->AddSeparator();
    nbr_separators++;
    ADD_TOOL( ID_CMD_NEXT_GAME,     bmp_next,      "Next game from file"     );
    ADD_TOOL( ID_CMD_PREVIOUS_GAME, bmp_previous,  "Previous game from file" );
#ifdef THC_WINDOWS
    toolbar->AddSeparator();
    nbr_separators++;
#endif
    toolbar->AddSeparator();
    nbr_separators++;
    ADD_TOOL( ID_HELP_HELP,  bmp_help, "Help");

    #if 0 // Wake up this code to generate some icon .xpm data
          //  (the link between original .bmps and generated .xpms
          //   has been muddied and perhaps even lost)
    extern void TempCreateXpm( wxBitmap &bmp, const char *name );
    wxBitmap temp_bmp( "myicons.bmp", wxBITMAP_TYPE_BMP );
    TempCreateXpm( temp_bmp, "myicons_base" );
    wxBitmap temp_bmp4( "extra_icons.bmp", wxBITMAP_TYPE_BMP );
    TempCreateXpm( temp_bmp4, "extra_icons" );
    #endif

    // Position book moves control to right
    // toolbar->SetToolSeparation(100); // note that this approach doesn't work
    //wxSize sz0 = GetSize(); // frame size
    //wxSize sz1 = toolbar->GetMargins();
    //wxSize sz2 = toolbar->GetToolSize();
    //int space1 = toolbar->GetToolPacking();
    //int nbr_tools = toolbar->GetToolsCount() - nbr_separators;
/*    const int SEPARATOR_WIDTH = 8;  // Can only be calculated after Realize(), so done
                                    //  offline below
    const int BOOK_MOVES_WIDTH = 105;
    int x1 = nbr_tools*sz2.x + (nbr_tools-1)*space1 + nbr_separators*SEPARATOR_WIDTH;    // where we are now
    int x2 = (sz0.x - BOOK_MOVES_WIDTH - 20);                                            // where we want to be
    nbr_separators = (x2-x1)/SEPARATOR_WIDTH;
    for( int i=0; i<nbr_separators; i++ )
        toolbar->AddSeparator(); */

    toolbar->Realize();
    SetToolBar(toolbar);

    // Some old Debug only code to calculate SEPARATOR_WIDTH note that
    //  GetPosition() of controls in toolbar is useless until Realize()
    #if 0 
    int space2 = toolbar->GetToolSeparation();
    nbr_tools = toolbar->GetToolsCount();
    int x,y;
    gbl_book_moves->GetPosition( &x, &y );
    int nbr_buttons = nbr_tools - nbr_separators - 1;   // -1 is gbl_book_moves control
    float width_separator = ( x - sz1.x - nbr_buttons*sz2.x -(nbr_tools-1)*space1
                            ) / ((double)nbr_separators);
    dbg_printf( "width of separator = %f (should be integer)\n", width_separator );
    dbg_printf( "margin.x=%d, nbr_tools=%d, nbr_separators=%d, toolsize.x=%d, packing=%d, separation=%d\n",
            sz1.x, nbr_tools, nbr_separators, sz2.x, space1, space2 );
    int w,h;
    int X, Y;
    wxWindow *pw = gbl_book_moves;
    for(int i=0; ; i++ )
    {
        pw->GetSize( &w, &h );
        pw->GetPosition( &x, &y );
        pw->GetScreenPosition( &X, &Y );
        bool top = pw->IsTopLevel();
        dbg_printf( "Book moves(%d,%s) w=%d, h=%d, X=%d, Y=%d, x=%d, y=%d\n", i, top?"top":"child",w, h, X, Y, x, y );
        if( top )
            break;
        pw = pw->GetParent();
        if( !pw )
            break;
    }
    #endif

    wxSize csz = GetClientSize();
    int hh = csz.y;
    int ww = csz.x;

    // notify wxAUI which frame to use
    m_mgr.SetManagedWindow(this);

    // Some dynamic sizing code
    wxButton dummy( this, wxID_ANY, "Dummy" );
    wxFont font_temp( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,   false );
    dummy.SetFont(font_temp);
    wxClientDC dc(&dummy);
    wxCoord width, height, descent, external_leading;
    dc.GetTextExtent( "1000 Book moves", &width, &height, &descent, &external_leading );
    int min_book_moves_width = (width*110) / 100;
    cprintf( "Calculated book moves width (was 105)=%d\n", min_book_moves_width );
    wxSize sz_dummy = dummy.GetSize();
    int min_notebook_height = (sz_dummy.y*115) / 100;
    cprintf( "min_notebook_height (was 29) =%d\n", min_notebook_height );
    int height_context  = 120 < hh/2 ? 120 : hh/2;
    int height_notebook = min_notebook_height  < hh/4 ? min_notebook_height  : hh/4;
    int height_board    = hh - height_context - height_notebook;

    PanelNotebook *skinny = new PanelNotebook( this, -1, wxDefaultPosition, wxSize(ww,height_notebook),min_book_moves_width );

    PanelBoard *pb = new PanelBoard( this,
                            wxID_ANY,
                            wxDefaultPosition, wxSize(ww/2,height_board) );

    CtrlChessTxt *lb = new CtrlChessTxt( this, -1, /*ID_LIST_CTRL*/ wxDefaultPosition, wxSize(ww/2,height_board) );

    context = new PanelContext( this, -1, /*wxID_ANY*/ wxDefaultPosition, wxSize(ww,height_context), pb, lb );
    context->notebook = skinny->notebook;
    objs.canvas = context;

    // Init game logic only after everything setup
    objs.gl         = NULL;
    objs.session    = new Session;
    objs.log        = new Log;
    objs.book       = new Book;
    objs.cws        = new CentralWorkSaver;
    objs.tabs       = new Tabs;
    GameLogic *gl   = new GameLogic( context, lb );

    // Hook up connections to GameLogic
    objs.gl         = gl;
    gl->gd.gl       = gl;
    gl->gd.gv.gl    = gl;
    lb->gl          = gl;
    objs.tabs->gl   = gl;
    GameDocument    blank;
    objs.tabs->TabNew( blank );
    objs.cws->Init( &objs.gl->undo, &objs.gl->gd, &objs.gl->gc_pgn, &objs.gl->gc_clipboard ); 
    context->SetPlayers( "", "" );
    context->resize_ready = true;
 
    // add the panes to the manager Note: Experience shows there is no point trying to change a panel's fundamental characteristics
    //  after adding it to the manager - Things like CaptionVisible(false) etc need to be intantiated with the panel
    m_mgr.AddPane(skinny, //wxBOTTOM);    //, wxT("Pane Number Two"));
                  wxAuiPaneInfo().
                  Name(wxT("test7")).CaptionVisible(false). //(wxT("Tree Pane")).
                  Top().Layer(1).Position(0).DockFixed().
                 // Top().Layer(1).Position(0).Fixed().Resizable(false).
                  CloseButton(false).MaximizeButton(false));
    m_mgr.AddPane(pb, //wxLEFT );         //, wxT("Pane Number One"));
// CreateTreeCtrl(),
                  wxAuiPaneInfo().MinSize(200,200).
                  Name(wxT("test8")).CaptionVisible(false). //(wxT("Tree Pane")).
                  Left().Layer(1).Position(1).
                  CloseButton(false).MaximizeButton(false));
    m_mgr.AddPane(lb, wxCENTER );
    m_mgr.AddPane(context, //wxBOTTOM);    //, wxT("Pane Number Two"));
                  wxAuiPaneInfo().
                  Name(wxT("test9")).CaptionVisible(false). //(wxT("Tree Pane")).
                  Bottom().Layer(1).Position(2).
                  CloseButton(false).MaximizeButton(false));


    // tell the manager to "commit" all the changes just made
    m_mgr.SetDockSizeConstraint(1.0,1.0);
    m_mgr.Update();

    // If we restored a non-volatile window size and location, try to restore non-volatile panel
    //  locations
    if( pos_siz_restored )
    {
        wxString persp = m_mgr.SavePerspective();
        const char *txt = persp.c_str();
        std::string s(txt);
        int panel1 = objs.repository->nv.m_panel1;
        int panel2 = objs.repository->nv.m_panel2;
        int panel3 = objs.repository->nv.m_panel3;
        int panel4 = objs.repository->nv.m_panel4;
        if( panel1>0 && panel2>0 && panel3>0 && panel4>0 )
        {
            size_t idx1 = s.find( "dock_size(1,1,0)=" );
            size_t idx2 = s.find( "dock_size(4,1,0)=" );
            size_t idx3 = s.find( "dock_size(5,0,0)=" );
            size_t idx4 = s.find( "dock_size(3,1,0)=" );
            if( idx1 != std::string::npos &&
                idx2 != std::string::npos &&
                idx3 != std::string::npos &&
                idx4 != std::string::npos &&
                idx2>idx1 && idx3>idx2 && idx4>idx3
              )
            {
                char temp[100];
                sprintf( temp, "dock_size(1,1,0)=%d|dock_size(4,1,0)=%d|dock_size(5,0,0)=%d|dock_size(3,1,0)=%d|", panel1, panel2, panel3, panel4 );
                std::string s2 = s.substr(0,idx1) + std::string(temp);
                wxString persp2(s2.c_str());
                m_mgr.LoadPerspective( persp2, true );
            }
        }
    }
}

void ChessFrame::OnClose( wxCloseEvent& WXUNUSED(event) )
{

    // Save the panel layout for next time
    int panel1=0, panel2=0, panel3=0, panel4=0;
    wxString persp = m_mgr.SavePerspective();
    const char *txt = persp.c_str();
    std::string s(txt);
    int len = strlen("dock_size(1,1,0)=");
    size_t idx = s.find( "dock_size(1,1,0)=" );
    if( idx != std::string::npos )
        panel1 = atoi(txt+idx+len);
    idx = s.find( "dock_size(4,1,0)=" );
    if( idx != std::string::npos )
        panel2 = atoi(txt+idx+len);
    idx = s.find( "dock_size(5,0,0)=" );
    if( idx != std::string::npos )
        panel3 = atoi(txt+idx+len);
    idx = s.find( "dock_size(3,1,0)=" );
    if( idx != std::string::npos )
        panel4 = atoi(txt+idx+len);
    if( panel1>0 && panel2>0 && panel3>0 && panel4>0 )
    {
        objs.repository->nv.m_panel1 = panel1;
        objs.repository->nv.m_panel2 = panel2;
        objs.repository->nv.m_panel3 = panel3;
        objs.repository->nv.m_panel4 = panel4;
    }

    //FILE *f = fopen( "c:/temp/persp.txt", "wb" );
    //fputs(txt,f);
    //fclose(f);
    bool okay = objs.gl->OnExit();
    if( okay )
    {
        wxSize sz = GetSize();
        wxPoint pt = GetPosition();
        objs.repository->nv.m_x = pt.x;
        objs.repository->nv.m_y = pt.y;
        objs.repository->nv.m_w = sz.x;
        objs.repository->nv.m_h = sz.y;
        Destroy();  // only exit if OnExit() worked okay (eg, if it wasn't cancelled)
    }
}

void ChessFrame::OnQuit (wxCommandEvent &)
{
    Close (true);
}

void ChessFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf(
        "\nThis is the Tarrasch Chess GUI V3. Tarrasch is intended to be "
        "an easy to use yet powerful system for playing, analysing and "
        "training with UCI standard chess engines. Tarrasch now supports "
        "useful database features."
        "\n\n"
        "Tarrasch includes the simple Tarrasch toy engine as well "
        "as one or more strong engines. "
        "The Tarrasch toy "
        "engine is particularly suited to Tarrasch's training modes. "
        "(These modes handicap the user, and you don't want to "
        "handicap yourself when you are playing against a strong engine!)."
        "\n\n"
        "Visit the publisher's website www.triplehappy.com for Tarrasch "
        "Chess GUI news and updates."
    );
    wxMessageBox(msg, "About the Tarrasch Chess GUI " MASTER_VERSION, wxOK|wxICON_INFORMATION|wxCENTRE, this);
}

/*
void ChessFrame::OnUnimplemented(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf(
        "Sorry in this pre-beta version of Tarrasch V2, redo is not "
        "not yet implemented, and undo is simply takeback."
        "\n\n"
        "You can check whether a more complete version of the program "
        "is available at the publisher's main site triplehappy.com or "
        "on his blog at triplehappy.wordpress.com."
    );
    wxMessageBox(msg, "Sorry, unimplemented feature", wxOK|wxICON_INFORMATION|wxCENTRE, this);
}  */


void ChessFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf(
        "\nTarrasch is firstly an electronic chessboard. You can start "
        "from the standard starting position or set up any (legal) "
        "position easily. You can move the pieces around freely. A "
        "built in chess clock can be turned on, useful if you want to "
        "play an old fashioned normal game of chess with a friend."
        "\n\n"
        "Tarrasch has an opening book, you can easily see the \"book\" "
        "moves early in a game. Tarrasch uses the %s green when "
        "listing or suggesting book moves. The book can be turned off "
        "using menu \"Options\". That menu allows control over all "
        "optional features, including blindfold and delayed display "
        "training modes."
        "\n\n"
        "Your moves are recorded in a moves window as a main line "
        "plus variations. It is easy to promote or demote variations. "
        "You can freely type (and edit) comments to go with your moves. "
        "Just click in the moves "
        "window and start typing. Comments can be converted to moves "
        "and vice-versa."
        "\n\n"
        "At any time you can save your work in a new or existing .pgn "
        "file (a standard chess file format). You can load games from "
        "any .pgn file. Open a .pgn file in the \"File\" menu and "
        "select a game, later on the \"Games\" menu allows you to pick "
        "another game from the file."
        "\n\n"
        "You can use a UCI chess engine (UCI is another chess "
        "computing standard) either as an opponent, or to provide "
        "expert commentary."
        "\n\n"
        "The Tarrasch package includes one or more engines and one of "
        "these engines is used by default. Other "
        "engines can be separately purchased or downloaded. Use menu "
        "\"Options\" to select an alternative engine if you have one. "
        "You can start a game with the engine at any time, in any "
        "position, as either white or black. By default you will get "
        "a time handicap. You can change the clock settings by "
        "clicking on the clock, or using menu \"Options\". You can "
        "change player names by clicking on them or by using menu "
        "\"Options\"."
        "\n\n"
        "To turn on commentary (kibitzing in chess parlance), use "
        "the \"Commands\" menu or the Robot button or select the Engine Analysis tab at the bottom. You can even "
        "get the chess engine to provide commentary when you are "
        "playing against it.", gbl_spell_colour
    );
    wxMessageBox(msg, "Tarrasch Chess GUI Help", wxOK|wxICON_INFORMATION|wxCENTRE, this);
}

void ChessFrame::OnCredits(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf(
        "\nProgram design and implementation: Bill Forster."
        "\n\n"
        "Foundation supporter: Yves Catineau."
        "\n\n"
        "Foundation donors: Paul Cook, Gary Baydo, Ransith Fernando, "
        "Lukasz Berezowski, Eric Ziegler, Laurence Dayton, Albrecht Schmidt, "
        "Lloyd Standish and David Beagan."
        "\n\n"
        "Testers: Iliya Kristoff, Claude Tamplenizza."
        "\n\n"
        "Thanks to David L Brown and the Good Companions for the chess "
        "graphics."
        "\n\n"
        "Thanks to Pawel Koziol for the icon idea."
        "\n\n"
        "Thanks to Yusuke Kamiyamane for some of the toolbar icons."
        "\n\n"
        "Thanks to Ed Schr�der for Millionbase and Pierre Havard for KingBase. Also to Mark Crowther for TWIC which "
        "is the basis of all modern chess database curation."
        "\n\n"
        "Thanks to the engine authors who provided explicit permission to "
        "include their engines. In chronological order, Vasik Rajlich (Rybka), "
        "Don Dailey and Larry Kaufman (Komodo), and Robert Houdart (Houdini)."
        "\n\n"
        "Thanks to the Stockfish team, Stockfish is now the default engine. "
        "Permission to include Stockfish is inherent in its licence, as long "
        "as the location of the Stockfish source code is provided. The "
        "location is https://stockfishchess.org."
        "\n\n"
        "Thanks to Inno Setup from Jordan Russell (jrsoftware.org), for "
        "the setup program."
        "\n\n"
        "Thanks to Julian Smart, Vadim Zeitlin and the wxWidgets community "
        "for the GUI library."
        "\n\n"
        "Dedicated to the memory of John Victor Forster 1949-2001. We "
        "miss him every day."
    );
    wxMessageBox(msg, "Credits", wxOK|wxICON_INFORMATION|wxCENTRE, this);
}



#if 0
void CustomLog( const char *txt )
{
    static FILE *log;
    if( log == NULL )
        log = fopen( "C:\\Documents and Settings\\Bill\\My Documents\\Chess\\Engines\\custom-log.txt", "wt" );
    if( log )
    {
        static char buf[1024];
        time_t t = time(NULL);
        struct tm *p = localtime(&t);
        const char *s = asctime(p);
        strcpy( buf, s );
        if( strchr(buf,'\n') )
            *strchr(buf,'\n') = '\0';
        fprintf( log, "[%s] %s", buf, txt );
        fflush(log);
    }
}
#endif

void ChessFrame::OnMove( wxMoveEvent &WXUNUSED(event) )
{
 //   if( objs.canvas )
 //       objs.canvas->OnMove();
}

void ChessFrame::OnIdle( wxIdleEvent& event )
{
    //CustomLog( "OnIdle()\n" );
    //if( objs.gl && objs.gl->OnIdleNeeded() )
    //    event.RequestMore();    // more intense idle calls required
    event.Skip();
    if( objs.gl )
    {
        objs.gl->OnIdle();
        int millisecs = 500;
        millisecs = objs.gl->MillisecsToNextSecond();
        if( millisecs < 100 )
            millisecs = 100;
        //char buf[80];
        //sprintf( buf, "Idle: %d milliseconds\n", millisecs );
        //CustomLog( buf );
        m_timer.Start( millisecs, true );
    }
}


void ChessFrame::OnTimeout( wxTimerEvent& WXUNUSED(event) )
{
    //CustomLog( "OnTimeout()\n" );
    if( objs.gl )
    {
        objs.gl->OnIdle();
        int millisecs = 1000;
        millisecs = objs.gl->MillisecsToNextSecond();
        if( millisecs < 100 )
            millisecs = 100;
        //char buf[80];
        //sprintf( buf, "Timeout: %d milliseconds\n", millisecs );
        //CustomLog( buf );
        m_timer.Start( millisecs, true );
    }
}

void ChessFrame::OnFlip (wxCommandEvent &)
{
    objs.gl->CmdFlip();
}

void ChessFrame::OnButtonUp (wxCommandEvent &)
{
    Atomic begin(false);
    objs.canvas->lb->NavigationKey( NK_UP );
}

void ChessFrame::OnButtonDown (wxCommandEvent &)
{
    Atomic begin(false);
    objs.canvas->lb->NavigationKey( NK_DOWN );
}

void ChessFrame::OnButtonLeft (wxCommandEvent &)
{
    Atomic begin(false);
    objs.canvas->lb->NavigationKey( NK_LEFT );
}

void ChessFrame::OnButtonRight (wxCommandEvent &)
{
    Atomic begin(false);
    objs.canvas->lb->NavigationKey( NK_RIGHT );
}

void ChessFrame::OnSetPosition (wxCommandEvent &)
{
    objs.gl->CmdSetPosition();
}

void ChessFrame::OnNewGame (wxCommandEvent &)
{
    objs.gl->CmdNewGame();
}

void ChessFrame::OnEditUndo (wxCommandEvent &)
{
    objs.gl->CmdEditUndo();
}

void ChessFrame::OnEditRedo (wxCommandEvent &)
{
    objs.gl->CmdEditRedo();
}

void ChessFrame::OnTakeback (wxCommandEvent &)
{
    objs.gl->CmdTakeback();
}

void ChessFrame::OnMoveNow (wxCommandEvent &)
{
    objs.gl->CmdMoveNow();
}

void ChessFrame::OnFileNew (wxCommandEvent &)
{
    objs.gl->CmdFileNew();
}

void ChessFrame::OnFileOpen (wxCommandEvent &)
{
    objs.gl->CmdFileOpen();
}

void ChessFrame::OnFileOpenLog (wxCommandEvent &)
{
    objs.gl->CmdFileOpenLog();
}

void ChessFrame::OnFileSave (wxCommandEvent &)
{
    objs.gl->CmdFileSave();
}

void ChessFrame::OnFileSaveAs (wxCommandEvent &)
{
    objs.gl->CmdFileSaveAs();
}

void ChessFrame::OnFileSaveGameAs (wxCommandEvent &)
{
    objs.gl->CmdFileSaveGameAs();
}

void ChessFrame::OnGamesCurrent (wxCommandEvent &)
{
    objs.gl->CmdGamesCurrent();
}

void ChessFrame::OnGamesSession (wxCommandEvent &)
{
    objs.gl->CmdGamesSession();
}

void ChessFrame::OnGamesClipboard (wxCommandEvent &)
{
    objs.gl->CmdGamesClipboard();
}

void ChessFrame::OnNextGame (wxCommandEvent &)
{
    objs.gl->CmdNextGame();
}

void ChessFrame::OnUpdateNextGame( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateNextGame() : false;
    event.Enable(enabled);
}

void ChessFrame::OnPreviousGame (wxCommandEvent &)
{
    objs.gl->CmdPreviousGame();
}

void ChessFrame::OnUpdatePreviousGame( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdatePreviousGame() : false;
    event.Enable(enabled);
}

void ChessFrame::OnEditCopy (wxCommandEvent &)
{
    objs.canvas->lb->Copy();
}

void ChessFrame::OnEditCut (wxCommandEvent &)
{
    objs.canvas->lb->Cut();
}

void ChessFrame::OnEditPaste (wxCommandEvent &)
{
    objs.canvas->lb->Paste();
}

void ChessFrame::OnEditDelete (wxCommandEvent &)
{
    objs.canvas->lb->Delete();
}

void ChessFrame::OnEditGameDetails (wxCommandEvent &)
{
    objs.gl->CmdEditGameDetails();
}

void ChessFrame::OnEditGamePrefix (wxCommandEvent &)
{
    objs.gl->CmdEditGamePrefix();
}

void ChessFrame::OnEditCopyGamePGNToClipboard(wxCommandEvent &)
{
    objs.gl->CmdEditCopyGamePGNToClipboard();
}

void ChessFrame::OnEditDeleteVariation (wxCommandEvent &)
{
    objs.gl->CmdEditDeleteVariation();
}

void ChessFrame::OnEditPromote (wxCommandEvent &)
{
    objs.gl->CmdEditPromote();
}

void ChessFrame::OnEditDemote (wxCommandEvent &)
{
    objs.gl->CmdEditDemote();
}

void ChessFrame::OnEditDemoteToComment (wxCommandEvent &)
{
    objs.gl->CmdEditDemoteToComment();
}

void ChessFrame::OnEditPromoteToVariation (wxCommandEvent &)
{
    objs.gl->CmdEditPromoteToVariation();
}

void ChessFrame::OnEditPromoteRestToVariation (wxCommandEvent &)
{
    objs.gl->CmdEditPromoteRestToVariation();
}

void ChessFrame::OnUpdateFileNew( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateFileOpen( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateFileOpen() : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateFileOpenLog( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateFileOpenLog()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateGamesCurrent( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateGamesCurrent()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateFileSave( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateFileSaveAs( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateFileSaveGameAs( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateGamesDatabase( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateGamesSession( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateGamesClipboard( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateGamesClipboard()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditCopy( wxUpdateUIEvent &event )
{
    bool enabled = objs.canvas ? (objs.canvas->lb->GetStringSelection().Length() > 0) : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditCut( wxUpdateUIEvent &event )
{
    bool enabled = objs.canvas ? (objs.canvas->lb->GetStringSelection().Length() > 0) : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditPaste( wxUpdateUIEvent &event )
{
#ifdef WINDOWS_FIX_LATER
    bool enabled = (wxTheClipboard->Open() && wxTheClipboard->IsSupported( wxDF_TEXT ));
    event.Enable(enabled);
#endif
}

void ChessFrame::OnUpdateEditUndo( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateEditUndo()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditRedo( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateEditRedo()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditDelete( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditGameDetails( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditGamePrefix( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditCopyGamePGNToClipboard(wxUpdateUIEvent &event)
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditDeleteVariation( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditPromote( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditDemote( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditDemoteToComment( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}


void ChessFrame::OnUpdateEditPromoteToVariation( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateEditPromoteRestToVariation( wxUpdateUIEvent &event )
{
    bool enabled = true;
    event.Enable(enabled);
}

void ChessFrame::OnDraw(wxCommandEvent &)
{
    objs.gl->CmdDraw();
}

void ChessFrame::OnWhiteResigns(wxCommandEvent &)
{
    objs.gl->CmdWhiteResigns();
}

void ChessFrame::OnUpdateWhiteResigns( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateWhiteResigns()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnBlackResigns(wxCommandEvent &)
{
    objs.gl->CmdBlackResigns();
}

void ChessFrame::OnUpdateBlackResigns( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateBlackResigns()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateTakeback( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateTakeback()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateMoveNow( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateMoveNow()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateDraw( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateDraw()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdatePlayWhite( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdatePlayWhite()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdatePlayBlack( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdatePlayBlack()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateSwapSides( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateSwapSides()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateTabNew( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateTabNew()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateTabClose( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateTabClose() : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateTabInclude( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateTabInclude() : false;
    event.Enable(enabled);
}

void ChessFrame::OnUpdateKibitz( wxUpdateUIEvent &event )
{
    wxString txt;
    bool enabled = objs.gl ? objs.gl->CmdUpdateKibitz( txt ) : false;
    event.SetText( txt );
    event.Enable(enabled);
}

void ChessFrame::OnUpdateClearKibitz( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->CmdUpdateClearKibitz() : false;
    event.Enable(enabled);
}

void ChessFrame::OnPlayWhite (wxCommandEvent &)
{
    objs.gl->CmdPlayWhite();
}

void ChessFrame::OnPlayBlack (wxCommandEvent &)
{
    objs.gl->CmdPlayBlack();
}

void ChessFrame::OnSwapSides (wxCommandEvent &)
{
    objs.gl->CmdSwapSides();
}

void ChessFrame::OnTabNew (wxCommandEvent &)
{
    objs.gl->CmdTabNew();
}

void ChessFrame::OnTabClose (wxCommandEvent &)
{
    objs.gl->CmdTabClose();
}

void ChessFrame::OnTabInclude (wxCommandEvent &)
{
    objs.gl->CmdTabInclude();
}

void ChessFrame::OnKibitz (wxCommandEvent &)
{
    objs.gl->CmdKibitz();
}

void ChessFrame::OnClearKibitz (wxCommandEvent &)
{
    objs.gl->CmdClearKibitz( true );
}

void ChessFrame::OnPlayers(wxCommandEvent &)
{
    objs.gl->CmdPlayers();
}

void ChessFrame::OnUpdatePlayers(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions() : false;
    event.Enable(enabled);
}

void ChessFrame::OnClocks(wxCommandEvent &)
{
    objs.gl->CmdClocks();
}

void ChessFrame::OnUpdateClocks(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions() : false;
    event.Enable(enabled);
}

void ChessFrame::OnOptionsReset(wxCommandEvent &)
{
    bool ingame=false;
    bool     old_enabled = objs.repository->book.m_enabled;
    wxString old_file    = objs.repository->book.m_file;
    wxString old_engine_file  = objs.repository->engine.m_file;
    const char *from = LangCheckDiffBegin();
    bool before_large_font = objs.repository->general.m_large_font;
    bool before_no_italics = objs.repository->general.m_no_italics;
    delete objs.repository;
    objs.repository = new Repository(true);
    LangSet( objs.repository->general.m_notation_language );
    if( objs.repository->engine.m_file != old_engine_file )
        ingame = objs.gl->EngineChanged();
    if( objs.repository->book.m_enabled != old_enabled ||
        objs.repository->book.m_file    != old_file )
    {
        if( objs.repository->book.m_enabled )
        {
            wxString error_msg;
            bool error = objs.book->Load( error_msg, objs.repository->book.m_file );
            if( error )
                wxMessageBox( error_msg, "Error loading book", wxOK|wxICON_ERROR );
            objs.canvas->BookUpdate( false );
        }
    }
    if( ingame )
    {
        objs.gl->chess_clock.Clocks2Repository();
        objs.gl->chess_clock.GameStart( objs.gl->gd.master_position.WhiteToPlay() );
    }
    else
        objs.gl->chess_clock.Repository2Clocks();
    context->RedrawClocks();
    const char *to = LangCheckDiffEnd();
    bool after_large_font = objs.repository->general.m_large_font;
    bool after_no_italics = objs.repository->general.m_no_italics;
    RefreshLanguageFont( from, before_large_font, before_no_italics,
                           to,  after_large_font,  after_no_italics );
	context->pb->gb->Redraw();
    SetFocusOnList();
}

void ChessFrame::OnUpdateOptionsReset(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions() : false;
    event.Enable(enabled);
}

void ChessFrame::OnBook(wxCommandEvent &)
{
    BookDialog dialog( objs.repository->book, this );
    bool     old_enabled = objs.repository->book.m_enabled;
    wxString old_file    = objs.repository->book.m_file;
    if( wxID_OK == dialog.ShowModal() )
    {
        objs.repository->book = dialog.dat;
        dbg_printf( "file=%s, enabled=%s, limit=%d, percent=%d\n",
            dialog.dat.m_file.c_str(),
            dialog.dat.m_enabled?"yes":"no",
            dialog.dat.m_limit_moves,
            dialog.dat.m_post_limit_percent );
        if( objs.repository->book.m_enabled != old_enabled ||
            objs.repository->book.m_file    != old_file )
        {
            if( objs.repository->book.m_enabled )
            {
                wxString error_msg;
                bool error = objs.book->Load( error_msg, objs.repository->book.m_file );
                if( error )
                    wxMessageBox( error_msg, "Error loading book", wxOK|wxICON_ERROR );
                objs.canvas->BookUpdate( false );
            }
            objs.gl->Refresh();
        }
    }
    SetFocusOnList();
}

void ChessFrame::OnUpdateBook(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions() : false;
    event.Enable(enabled);
}

void ChessFrame::OnLog(wxCommandEvent &)
{
    LogDialog dialog( objs.repository->log, this );
    if( wxID_OK == dialog.ShowModal() )
    {
        objs.repository->log = dialog.dat;
        dbg_printf( "file=%s, enabled=%s\n",
            dialog.dat.m_file.c_str(),
            dialog.dat.m_enabled?"yes":"no" );
    }
    SetFocusOnList();
}

void ChessFrame::OnUpdateLog(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnEngine(wxCommandEvent &)
{
    wxString old_file    = objs.repository->engine.m_file;
    EngineDialog dialog( objs.repository->engine, this );
    if( wxID_OK == dialog.ShowModal() )
    {
        objs.repository->engine = dialog.dat;
        dbg_printf( "file=%s\n",
            dialog.dat.m_file.c_str() );
        if( old_file != objs.repository->engine.m_file )
            objs.gl->EngineChanged();
    }
    SetFocusOnList();
}

void ChessFrame::OnGamesDatabase (wxCommandEvent &)
{
    objs.gl->CmdGamesDatabase();
}

void ChessFrame::OnDatabaseSearch(wxCommandEvent &)
{
    objs.gl->CmdDatabaseSearch();
}

void ChessFrame::OnDatabaseShowAll(wxCommandEvent &)
{
    objs.gl->CmdDatabaseShowAll();
}

void ChessFrame::OnDatabasePlayers(wxCommandEvent &)
{
    objs.gl->CmdDatabasePlayers();
}

void ChessFrame::OnDatabaseSelect(wxCommandEvent &)
{
    objs.gl->CmdDatabaseSelect();
}

void ChessFrame::OnDatabaseCreate(wxCommandEvent &)
{
    objs.gl->CmdDatabaseCreate();
}

void ChessFrame::OnDatabaseAppend(wxCommandEvent &)
{
    objs.gl->CmdDatabaseAppend();
}

void ChessFrame::OnDatabasePattern(wxCommandEvent &)
{
    objs.gl->CmdDatabasePattern();
}

void ChessFrame::OnDatabaseMaterial(wxCommandEvent &)
{
    objs.gl->CmdDatabaseMaterial();
}

void ChessFrame::OnUpdateDatabaseSearch(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabaseShowAll(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabasePlayers(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabaseSelect(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabaseCreate(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabaseAppend(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabasePattern(wxUpdateUIEvent &)
{
}

void ChessFrame::OnUpdateDatabaseMaterial(wxUpdateUIEvent &)
{
}

void ChessFrame::OnDatabaseMaintenance(wxCommandEvent &)
{
    wxString old_file    = objs.repository->engine.m_file;
    MaintenanceDialog dialog( objs.repository->engine, this );
    if( wxID_OK == dialog.ShowModal() )
    {
        objs.repository->engine = dialog.dat;
        dbg_printf( "file=%s\n",
                     dialog.dat.m_file.c_str() );
        if( old_file != objs.repository->engine.m_file )
            objs.gl->EngineChanged();
    }
    SetFocusOnList();
}

void ChessFrame::OnUpdateDatabaseMaintenance( wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions()  : false;
    event.Enable(enabled);
}


void ChessFrame::OnUpdateEngine(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnGeneral(wxCommandEvent &)
{
    const char *from = LangCheckDiffBegin();
    bool before_large_font = objs.repository->general.m_large_font;
    bool before_no_italics = objs.repository->general.m_no_italics;
    GeneralDialog dialog( this, &objs.repository->general );
    if( wxID_OK == dialog.ShowModal() )
    {
        //bool changed = ( 0 != memcmp(&dialog.dat,&objs.repository->training,sizeof(dialog.dat))  );
        objs.repository->general = dialog.dat;
        //if( changed )
        //    objs.gl->SetGroomedPosition();
        const char *s=dialog.dat.m_notation_language.c_str();
        cprintf( "notation language=%s, no italics=%d, straight to game=%d\n",
                                 s, dialog.dat.m_no_italics,
                                 dialog.dat.m_straight_to_game );
        const char *to = LangCheckDiffEnd();
        bool after_large_font = objs.repository->general.m_large_font;
        bool after_no_italics = objs.repository->general.m_no_italics;
        RefreshLanguageFont( from, before_large_font, before_no_italics,
                             to,   after_large_font,  after_no_italics );

		// Change board colours
		objs.canvas->pb->gb->Redraw();
    }
    SetFocusOnList();
}

void ChessFrame::RefreshLanguageFont( const char *UNUSED(from), bool before_large_font, bool before_no_italics,
                                      const char *to,   bool after_large_font,  bool after_no_italics )
{
    if( after_large_font != before_large_font )
        objs.gl->lb->SetLargeFont( after_large_font );
    bool redisplayed = false;
    if( to )
    {

        // If the language has changed, redisplay
        long pos = objs.gl->lb->GetInsertionPoint();
        objs.gl->gd.Rebuild();
        objs.gl->gd.Redisplay(pos);
        redisplayed = true;
    }

    // If the italics setting has changed, redisplay
    else if( before_no_italics != after_no_italics )
    {
        long pos = objs.gl->lb->GetInsertionPoint();
        objs.gl->gd.Rebuild();
        objs.gl->gd.Redisplay(pos);
        redisplayed = true;
    }

    // If no redisplay yet, and font change, redisplay
    if( after_large_font!=before_large_font && !redisplayed )
    {
        long pos = objs.gl->lb->GetInsertionPoint();
        objs.gl->gd.Rebuild();
        objs.gl->gd.Redisplay(pos);
    }
    objs.gl->Refresh();
}

void ChessFrame::OnUpdateGeneral(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnTraining(wxCommandEvent &)
{
    TrainingDialog dialog( this );
    dialog.dat = objs.repository->training;
    if( wxID_OK == dialog.ShowModal() )
    {
        //bool changed = ( 0 != memcmp(&dialog.dat,&objs.repository->training,sizeof(dialog.dat))  );
        objs.repository->training = dialog.dat;
        //if( changed )
        //    objs.gl->SetGroomedPosition();
        dbg_printf( "nbr_half=%d, hide wpawn=%d, hide wpiece=%d,"
                                 " hide bpawn=%d, hide bpiece=%d\n",
                                 dialog.dat.m_nbr_half_moves_behind,
                                 dialog.dat.m_blindfold_hide_white_pawns,
                                 dialog.dat.m_blindfold_hide_white_pieces,
                                 dialog.dat.m_blindfold_hide_black_pawns,
                                 dialog.dat.m_blindfold_hide_black_pieces );
    }
    objs.gl->Refresh();
    SetFocusOnList();
}

void ChessFrame::OnUpdateTraining(wxUpdateUIEvent &event )
{
    bool enabled = objs.gl ? objs.gl->UpdateOptions()  : false;
    event.Enable(enabled);
}

void ChessFrame::OnMouseWheel( wxMouseEvent &event )
{
    if( objs.canvas )
        objs.canvas->lb->GetEventHandler()->ProcessEvent(event);
}

void ChessFrame::OnChar( wxKeyEvent &event )
{
    if( objs.canvas )
        objs.canvas->lb->GetEventHandler()->ProcessEvent(event);
}
