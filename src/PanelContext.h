/****************************************************************************
 * Top level UI panel for providing context, kibitzer info etc.
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#ifndef PANEL_CONTEXT_H
#define PANEL_CONTEXT_H
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/richtext/richtextctrl.h"
#include "Appdefs.h"
#include "CtrlChessTxt.h"
#include "CtrlBox.h"
#include "CtrlBox2.h"
#include "PanelBoard.h"
#include "thc.h"
class GraphicBoard;
class GraphicBoardResizable;
class PopupControl;
class CtrlBox;
class CtrlBoxBookMoves;

class wxStaticTextSub: public wxStaticText
{
    DECLARE_CLASS( wxStaticTextSub )
    DECLARE_EVENT_TABLE()
public:
    wxStaticTextSub( wxWindow *parent, wxWindowID id, const wxString &label, const wxPoint &pos, const wxSize &size, long style )
        : wxStaticText( parent, id, label, pos, size, style )
    {
    }
    void OnMouseLeftDown( wxMouseEvent &event );
};

class PanelContext: public wxPanel
{
public:
    PanelContext( wxWindow *parent, wxWindowID, const wxPoint &pos, const wxSize &size, PanelBoard *pb_, GraphicBoardResizable *gb_, CtrlChessTxt *lb_ );
    ~PanelContext();
    void SetPlayers( const char *white, const char *black ) { pb->SetPlayers(white,black); }
    bool GetNormalOrientation()                             { return pb->GetNormalOrientation(); }
    void SetNormalOrientation( bool normal )                { pb->SetNormalOrientation(normal); }
    void SetChessPosition( thc::ChessPosition &pos )        { pb->SetChessPosition(pos); }
    void SetChessPosition()                                 { pb->SetChessPosition(); }
    void ClocksVisible()                                    { pb->ClocksVisible(); }
    void WhiteClock( const wxString &txt )                  { pb->WhiteClock(txt); }
    void BlackClock( const wxString &txt )                  { pb->BlackClock(txt); }
    void RedrawClocks()                                     { pb->RedrawClocks(); }
    void PositionButtons();
    void SetSmallBox( bool is_small );
    void BookUpdate( bool suppress );
    void OnChar( wxKeyEvent &event );
    void OnMouseLeftDown (wxMouseEvent & event);
    void OnMouseMove (wxMouseEvent & event);
    void OnMouseWheel( wxMouseEvent &event );
    void OnPaint (wxPaintEvent & event);
    void OnEraseBackground( wxEraseEvent &event );
    void OnMouseCaptureLost( wxMouseCaptureLostEvent &event );
    void OnTabSelected( wxBookCtrlEvent &event );
    void OnButton1( wxCommandEvent &event );
    void OnButton2( wxCommandEvent &event );
    void OnButton3( wxCommandEvent &event );
    void OnButton4( wxCommandEvent &event );
    void OnKibitzButton1( wxCommandEvent &event );
    void OnKibitzButton2( wxCommandEvent &event );
    void Kibitz( int idx, const wxString &txt );
    void KibitzScroll( const wxString &txt );
    void SetFocusOnList() { if(lb) lb->SetFocus(); }
    void OnSize( wxSizeEvent &event );
    void OnMove();
    void SetMinimumPlaySize();
    void SetMinimumKibitzSize();

public:
    bool            resize_ready;
    thc::ChessPosition   save_position;
    PanelBoard              *pb;
    GraphicBoardResizable   *gb;
    CtrlChessTxt    *lb;
    PopupControl    *popup;
    wxStaticText    *status;
    wxStaticText    *kibitz0;
    wxStaticText    *kibitz1;
    wxStaticText    *kibitz2;
    wxStaticText    *kibitz3;
    wxStaticText    *kibitz4;
    wxString        str_kibitz0;
    wxString        str_kibitz1;
    wxString        str_kibitz2;
    wxString        str_kibitz3;
    wxString        str_kibitz4;
    wxButton        *button1;
    wxButton        *button2;
    wxButton        *button3;
    wxButton        *button4;
    wxListCtrl      *kibitz_ctrl;
    wxButton        *kibitz_button1;
    wxButton        *kibitz_button2;
    int              button1_cmd;
    int              button2_cmd;
    int              button3_cmd;
    int              button4_cmd;
    CtrlBoxBookMoves *book_moves;
    wxNotebook      *notebook;
    //wxPanel         *notebook_page1;
    wxPanel         *notebook_page2;
private:
    wxPoint       kbut1_base;
    wxPoint       kbut2_base;
    wxSize        lb_sz_base;
    wxSize        k_sz_base;
    wxSize        parent_sz_base;
    wxFont       *font1;
    wxFont       *font2;
    wxFont       *font3;
    wxFont       *font_book;
    wxFont       *font_clock;
    wxStaticBox  *box;
    wxStaticBox  *kibitz_box;
    //wxRichTextCtrl *kibitz_ctrl;

    wxRect       board_rect;
    int          box_width;
    bool         captured;
    void         ButtonCmd( int cmd );
    DECLARE_EVENT_TABLE()
};

#endif // PANEL_CONTEXT_H
