/****************************************************************************
 * Control - The graphic chess board that is the centre-piece of the GUI
 *  Still working on making it a more-self contained and reusable control
 ****************************************************************************/
#ifndef GRAPHICBOARD_RESIZABLE_H
#define GRAPHICBOARD_RESIZABLE_H

#include "wx/wx.h"
#include "Portability.h"
#include "thc.h"

class GraphicBoardResizable : public wxControl
{
public:

	// Con/De structor
	GraphicBoardResizable( wxWindow *parent,
                  wxWindowID id,
                  const wxPoint& point,
                  const wxSize& size );
	GraphicBoardResizable( int pix_ );
	~GraphicBoardResizable();
    void Init( const wxSize& size );

	// Find a square within the graphic board
	void HitTest( wxPoint hit, char &file, char &rank );
    void HitTestEx( char &file, char &rank, wxPoint shift );

	// Setup a position	on the graphic board
	void SetChessPosition( char *position_ascii );

	// Draw the graphic board
    void Draw();
    void OnPaint(wxPaintEvent& WXUNUSED(evt));
    void SetBoardSize( wxSize &sz );
	void Redraw() { wxSize sz=GetSize(); SetBoardSize(sz); }

	// Get/Set orientation
	void SetNormalOrientation( bool _normal_orientation )
					{ normal_orientation = _normal_orientation; }
	bool GetNormalOrientation()
					{ return normal_orientation; }

	// Set highlight squares
	void SetHighlight1( char file, char rank ) { highlight_file1=file;
											     highlight_rank1=rank; }
	void SetHighlight2( char file, char rank ) { highlight_file2=file;
	                                             highlight_rank2=rank; }
	void ClearHighlight1()			   { highlight_file1='\0'; }
	void ClearHighlight2()			   { highlight_file2='\0'; }

    // Setup a position	on the graphic board
    void SetPositionEx( thc::ChessPosition pos, bool blank_other_squares, char pickup_file, char pickup_rank, wxPoint shift );

    void OnMouseLeftDown (wxMouseEvent & event);
    void OnMouseLeftUp (wxMouseEvent & event);
    void OnMouseMove (wxMouseEvent & event);
    wxBitmap     my_chess_bmp;

private:

	// Data members
	wxColour	 light_colour;
	wxColour	 dark_colour;
	wxSize       current_size;
    wxBrush      brush;
	wxMemoryDC   dcmem;
    wxPen        pen;
	byte         *buf_board;
	byte         *buf_box;
	unsigned long width_bytes, height, width, xborder, yborder, density;
	bool		 normal_orientation;
	char		 highlight_file1, highlight_rank1;
	char		 highlight_file2, highlight_rank2;
    char         _position_ascii[100];
    std::string  str_white_king_mask;
    const char  *white_king_mask;
    std::string  str_white_queen_mask;
    const char  *white_queen_mask;
    std::string  str_white_knight_mask;
    const char  *white_knight_mask;
    std::string  str_white_bishop_mask;
    const char  *white_bishop_mask;
    std::string  str_white_rook_mask;
    const char  *white_rook_mask;
    std::string  str_white_pawn_mask;
    const char  *white_pawn_mask;
    std::string  str_black_king_mask;
    const char  *black_king_mask;
    std::string  str_black_queen_mask;
    const char  *black_queen_mask;
    std::string  str_black_knight_mask;
    const char  *black_knight_mask;
    std::string  str_black_bishop_mask;
    const char  *black_bishop_mask;
    std::string  str_black_rook_mask;
    const char  *black_rook_mask;
    std::string  str_black_pawn_mask;
    const char  *black_pawn_mask;

	// Helpers
	unsigned long   Offset( char file, char rank );
	void Get( char src_file, char src_rank, char dst_file, char dst_rank, const char *mask = NULL );
	void Put( char src_file, char src_rank, char dst_file, char dst_rank );

    // Put a shifted, masked piece from box onto board
    void PutEx( char piece, char dst_file, char dst_rank, wxPoint shift );

private:
    DECLARE_EVENT_TABLE()
    bool         sliding;
    char         pickup_file;
    char         pickup_rank;
    wxPoint      pickup_point;
    thc::ChessPosition     slide_pos;
    int          PIX;
    int          pix;
};

#endif // GRAPHICBOARD_RESIZABLE_H

