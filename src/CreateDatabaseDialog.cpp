/****************************************************************************
 * Custom dialog - Maintenance Dialog
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2014, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#include "wx/wx.h"
#include "wx/valtext.h"
#include "wx/valgen.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/filepicker.h"
#include "DebugPrintf.h"
#include "Portability.h"
#include "Appdefs.h"
#include "Repository.h"
#include "PgnRead.h"
#include "ProgressBar.h"
#include "DbMaintenance.h"
#include "DbPrimitives.h"
#include "PackedGameBinDb.h"
#include "BinDb.h"
#include "LegacyDb.h"
#include "CreateDatabaseDialog.h"

// CreateDatabaseDialog type definition
IMPLEMENT_CLASS( CreateDatabaseDialog, wxDialog )

// CreateDatabaseDialog event table definition
BEGIN_EVENT_TABLE( CreateDatabaseDialog, wxDialog )

EVT_BUTTON( wxID_OK, CreateDatabaseDialog::OnOk )

EVT_BUTTON( wxID_HELP, CreateDatabaseDialog::OnHelpClick )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER_DB, CreateDatabaseDialog::OnDbFilePicked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER1,   CreateDatabaseDialog::OnPgnFile1Picked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER2,   CreateDatabaseDialog::OnPgnFile2Picked )
EVT_FILEPICKER_CHANGED( ID_CREATE_DB_PICKER3,   CreateDatabaseDialog::OnPgnFile3Picked )
END_EVENT_TABLE()

CreateDatabaseDialog::CreateDatabaseDialog(
                           wxWindow* parent,
                           wxWindowID id, bool create_mode,
                           const wxPoint& pos, const wxSize& size, long style )
{
    db_created_ok = false;
    this->create_mode = create_mode;
    Create(parent, id, create_mode?"Create Database":"Add Games to Database", pos, size, style);
}

// Dialog create
bool CreateDatabaseDialog::Create( wxWindow* parent,
                          wxWindowID id, const wxString& caption,
                          const wxPoint& pos, const wxSize& size, long style )
{
    bool okay=true;
    
    // We have to set extra styles before creating the dialog
    SetExtraStyle( wxWS_EX_BLOCK_EVENTS/*|wxDIALOG_EX_CONTEXTHELP*/ );
    if( !wxDialog::Create( parent, id, caption, pos, size, style ) )
        okay = false;
    else
    {
        
        CreateControls();
        SetDialogHelp();
        SetDialogValidators();
        
        // This fits the dialog to the minimum size dictated by the sizers
        GetSizer()->Fit(this);
        
        // This ensures that the dialog cannot be sized smaller than the minimum size
        GetSizer()->SetSizeHints(this);
        
        // Centre the dialog on the parent or (if none) screen
        Centre();
    }
    return okay;
}

// Control creation for CreateDatabaseDialog
void CreateDatabaseDialog::CreateControls()
{
    
    // A top-level sizer
    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(top_sizer);
    
    // A second box sizer to give more space around the controls
    wxBoxSizer* box_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->Add(box_sizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // A friendly message
    wxStaticText* descr = new wxStaticText( this, wxID_STATIC,
           create_mode?
           "To create a new database from scratch, name the new database and select\n"
           "one or more .pgn files with the games to go into the database.\n"
           :
           "To add games to an existing database, select the database and one or\n"
           "more .pgn files with the additional games to go into the database.\n"
           , wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(descr, 0, wxALIGN_LEFT|wxALL, 5);
    
    // Spacer
    box_sizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // Label for file
    wxStaticText* db_file_label = new wxStaticText ( this, wxID_STATIC,
                                                        create_mode ?
                                                            wxT("&Name a new database file") :
                                                            wxT("&Choose an existing database file"),
                                                        wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(db_file_label, 0, wxALIGN_LEFT|wxALL, 5);
    
    // File picker controls
    wxFilePickerCtrl *picker_db = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER_DB, db_filename,
                                                        create_mode ?
                                                            wxT("Locate and name a database to create") :
                                                            wxT("Select database to add games to"),
                                                        "*.tdb", wxDefaultPosition, wxDefaultSize,
                                                        create_mode ?
                                                            wxFLP_USE_TEXTCTRL|wxFLP_SAVE :
                                                            wxFLP_USE_TEXTCTRL|wxFLP_FILE_MUST_EXIST
                                                       );
    box_sizer->Add(picker_db, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    // Label for file
    wxStaticText* pgn_file_label = new wxStaticText ( this, wxID_STATIC,
                                                 wxT("&Select one or more .pgn files to add to the database"), wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(pgn_file_label, 0, wxALIGN_LEFT|wxALL, 5);
    wxFilePickerCtrl *picker1 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER1, pgn_filename1, wxT("Select 1st .pgn"),
                                                    "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                    wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
    box_sizer->Add(picker1, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    wxFilePickerCtrl *picker2 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER2, pgn_filename2, wxT("Select 2nd .pgn"),
                                                    "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                    wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
    box_sizer->Add(picker2, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    if( create_mode )
    {
        wxFilePickerCtrl *picker3 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER3, pgn_filename3, wxT("Select next .pgn"),
                                                        "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                        wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
        box_sizer->Add(picker3, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

        wxFilePickerCtrl *picker4 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER4, pgn_filename4, wxT("Select next .pgn"),
                                                        "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                        wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
        box_sizer->Add(picker4, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

        wxFilePickerCtrl *picker5 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER5, pgn_filename5, wxT("Select next .pgn"),
                                                        "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                        wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
        box_sizer->Add(picker5, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

        wxFilePickerCtrl *picker6 = new wxFilePickerCtrl( this, ID_CREATE_DB_PICKER6, pgn_filename6, wxT("Select next .pgn"),
                                                        "*.pgn", wxDefaultPosition, wxDefaultSize,
                                                        wxFLP_USE_TEXTCTRL|wxFLP_OPEN|wxFLP_FILE_MUST_EXIST ); //|wxFLP_CHANGE_DIR );
        box_sizer->Add(picker6, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);
    }

    // Label for elo cutoff
    wxStaticText* elo_cutoff_label = new wxStaticText ( this, wxID_STATIC,
        wxT("Elo cutoff (if Elo values present, at least one player must meet this threshold)"), wxDefaultPosition, wxDefaultSize, 0 );
    box_sizer->Add(elo_cutoff_label, 0, wxALL, 5);

    // A spin control for the elo cutoff
    elo_cutoff_spin = new wxSpinCtrl ( this, ID_CREATE_ELO_CUTOFF,
        wxEmptyString, wxDefaultPosition, wxSize(60, -1),
        wxSP_ARROW_KEYS, 0, 4000, 2000 );
    elo_cutoff_spin->SetValue( objs.repository->database.m_elo_cutoff );
    box_sizer->Add(elo_cutoff_spin, 0, wxALL, 5);

    // A dividing line before the OK and Cancel buttons
    wxStaticLine* line2 = new wxStaticLine ( this, wxID_STATIC,
                                           wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    box_sizer->Add(line2, 0, wxGROW|wxALL, 5);
    
    // A horizontal box sizer to contain Reset, OK, Cancel and Help
    wxBoxSizer* okCancelBox = new wxBoxSizer(wxHORIZONTAL);
    box_sizer->Add(okCancelBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 15);
  
   
    
    // The OK button
    wxButton* ok = new wxButton ( this, wxID_OK, wxT("&OK"),
                                 wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(ok, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    
    // The Cancel button
    wxButton* cancel = new wxButton ( this, wxID_CANCEL,
                                     wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(cancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    
    // The Help button
    wxButton* help = new wxButton( this, wxID_HELP, wxT("&Help"),
                                  wxDefaultPosition, wxDefaultSize, 0 );
    okCancelBox->Add(help, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
}

// Set the validators for the dialog controls
void CreateDatabaseDialog::SetDialogValidators()
{
}

// Sets the help text for the dialog controls
void CreateDatabaseDialog::SetDialogHelp()
{
}

// wxID_OK handler
void CreateDatabaseDialog::OnOk( wxCommandEvent& WXUNUSED(event) )
{
    int elo_cutoff = elo_cutoff_spin->GetValue();
    extern int gbl_elo_cutoff;
    gbl_elo_cutoff = elo_cutoff;
    objs.repository->database.m_elo_cutoff = elo_cutoff;
    if( create_mode )
        OnCreateDatabase();
    else
        OnAppendDatabase();
}

// Create
void CreateDatabaseDialog::OnCreateDatabase()
{
    bool ok=true;
    std::string files[6];
    int cnt=0;
    std::string error_msg;
    db_primitive_error_msg();   // clear error reporting mechanism
    
    // Collect files
    for( int i=1; i<=6; i++ )
    {
        bool exists = false;
        std::string filename;
        switch(i)
        {
            case 1: exists = wxFile::Exists(pgn_filename1);
                    filename = std::string(pgn_filename1.c_str());  break;
            case 2: exists = wxFile::Exists(pgn_filename2);
                    filename = std::string(pgn_filename2.c_str());  break;
            case 3: exists = wxFile::Exists(pgn_filename3);
                    filename = std::string(pgn_filename3.c_str());  break;
            case 4: exists = wxFile::Exists(pgn_filename4);
                    filename = std::string(pgn_filename4.c_str());  break;
            case 5: exists = wxFile::Exists(pgn_filename5);
                    filename = std::string(pgn_filename5.c_str());  break;
            case 6: exists = wxFile::Exists(pgn_filename6);
                    filename = std::string(pgn_filename6.c_str());  break;
        }
        if( exists )
        {
            files[cnt++] = filename;
        }
    }
    bool exists = wxFile::Exists(db_filename);
    db_name = std::string( db_filename.c_str() );
    if( exists )
    {
        error_msg = "Tarrasch database file " + db_name + " already exists";
        ok = false;
    }
    else if( db_name.length() == 0 )
    {
        error_msg = "No Tarrasch database file specified";
        ok = false;
    }
    else if( cnt == 0 )
    {
        error_msg = "No usable pgn files specified";
        ok = false;
    }
    // TODO DatabaseClear();
    FILE *ofile=NULL;
    if( ok )
    {
        BinDbReadBegin( true );
        wxString fullpath = db_filename;
        wxFileName wfn(db_filename);
        if( wfn.IsOk() && wfn.HasName() )
        {
            wxString cwd  = wxGetCwd();
            wfn.SetExt("tdb");
            wxString path = wxPathOnly(db_filename);
            bool no_path = (path=="");
            if( no_path )
                wfn.SetPath(cwd);
            fullpath = wfn.GetFullPath();
        }
        ofile = fopen( fullpath.c_str(), "wb" );
        if( !ofile )
        {
            error_msg = "Cannot create ";
            error_msg += fullpath;
            ok = false;
        }
    }
    for( int i=0; ok && i<cnt; i++ )
    {
        FILE *ifile = fopen( files[i].c_str(), "rt" );
        if( !ifile )
        {
            error_msg = "Cannot open ";
            error_msg += files[i];
            ok = false;
        }
        else
        {
            std::string title( "Creating database, step 1 of 3");
            std::string desc("Reading file #");
            char buf[80];
            sprintf( buf, "%d of %d", i+1, cnt );
            desc += buf;
            ProgressBar progress_bar( title, desc, true, this, ifile );
            uint32_t begin = BinDbGetGamesSize();
            PgnRead pgn('B',&progress_bar);
            bool aborted = pgn.Process(ifile);
            uint32_t end = BinDbGetGamesSize();
            BinDbNormaliseOrder( begin, end );
            if( aborted )
            {
                error_msg = "cancel";
                ok = false;
            }
            fclose(ifile);
        }
    }
    if( ok )
    {
        std::string title( "Creating database, step 2 of 3");
        ok = BinDbDuplicateRemoval(title,this);
    }
    if( ok )
    {
        std::string title( "Creating database, step 3 of 3");
        std::string desc("Writing file");
        ProgressBar progress_bar( title, desc, true, this );
        ok = BinDbWriteOutToFile(ofile,&progress_bar);
    }
    if( ofile )
    {
        wxSafeYield();
        fclose(ofile);
        ofile = NULL;
    }
    if( ok )
    {
        wxSafeYield();
        AcceptAndClose();
        db_created_ok = true;
    }
    else
    {
        if( error_msg == "" )
            error_msg = db_primitive_error_msg();
        if( error_msg == "cancel" )
            error_msg = "Database creation cancelled";
        wxMessageBox( error_msg.c_str(), "Database creation failed", wxOK|wxICON_ERROR );
        _unlink(db_filename.c_str());
    }
    // TODO DatabaseReload();
}

void CreateDatabaseDialog::OnAppendDatabase()
{
    bool ok=true;
    std::string files[6];
    int cnt=0;
    std::string error_msg;
    db_primitive_error_msg();   // clear error reporting mechanism

    // Collect files
    for( int i=1; i<=6; i++ )
    {
        bool exists = false;
        std::string filename;
        switch(i)
        {
            case 1: exists = wxFile::Exists(pgn_filename1);
                    filename = std::string(pgn_filename1.c_str());  break;
            case 2: exists = wxFile::Exists(pgn_filename2);
                    filename = std::string(pgn_filename2.c_str());  break;
            case 3: exists = wxFile::Exists(pgn_filename3);
                    filename = std::string(pgn_filename3.c_str());  break;
            case 4: exists = wxFile::Exists(pgn_filename4);
                    filename = std::string(pgn_filename4.c_str());  break;
            case 5: exists = wxFile::Exists(pgn_filename5);
                    filename = std::string(pgn_filename5.c_str());  break;
            case 6: exists = wxFile::Exists(pgn_filename6);
                    filename = std::string(pgn_filename6.c_str());  break;
        }
        if( exists )
        {
            files[cnt++] = filename;
        }
    }
    if( cnt == 0 )
    {
        error_msg = "No usable pgn files specified";
        ok = false;
    }
    if( ok )
    {
        bool exists = wxFile::Exists(db_filename);
        db_name = std::string( db_filename.c_str() );
        if( !exists )
        {
            error_msg = "Tarrasch database file " + db_name + " doesn't exist";
            ok = false;
        }
    }
    int version=0;
    if( ok )
    {
        ok = BinDbOpen( db_filename.c_str(), version );
        if( version==0 || version>DATABASE_VERSION_NUMBER_BIN_DB )
        {
            error_msg = "Tarrasch database file " + db_name + " is incompatible with this version of Tarrasch";
            if( ok )
                BinDbClose();
            ok = false;
        }
        else if( !ok )
            error_msg = "Cannot open  " + db_name;
    }
    if( ok )
    {
        bool dummyb=false;
        int dummyi=false;
        std::string title( "Appending to database, step 1 of 4");
        std::string desc("Reading existing database");
        ProgressBar progress_bar( title, desc, true, this );
        
        std::vector< smart_ptr<ListableGame> > &mega_cache = BinDbLoadAllGamesGetVector();
        if( version < DATABASE_VERSION_NUMBER_BIN_DB )
        {
            BinDbClose();
            ok = LegacyDbLoadAllGames( db_filename.c_str(), true, mega_cache, dummyi, dummyb, &progress_bar );
        }
        else
        {
            BinDbLoadAllGames( true, mega_cache, dummyi, dummyb, &progress_bar );
            BinDbClose();
        }
        FILE *ofile=NULL;
        if( ok )
        {
            ofile = fopen( db_name.c_str(), "wb" );
            if( !ofile )
            {
                error_msg = "Cannot open ";
                error_msg += db_name;
                ok = false;
            }
        }
        for( int i=0; ok && i<cnt; i++ )
        {
            FILE *ifile = fopen( files[i].c_str(), "rt" );
            if( !ifile )
            {
                error_msg = "Cannot open ";
                error_msg += files[i];
                ok = false;
            }
            else
            {
                std::string title2( "Appending to database, step 2 of 4");
                std::string desc2("Reading file #");
                char buf[80];
                sprintf( buf, "%d of %d", i+1, cnt );
                desc2 += buf;
                ProgressBar progress_bar2( title2, desc2, true, this, ifile );
                uint32_t begin = BinDbGetGamesSize();
                PgnRead pgn('B',&progress_bar2);
                bool aborted = pgn.Process(ifile);
                uint32_t end = BinDbGetGamesSize();
                BinDbNormaliseOrder( begin, end );
                if( aborted )
                {
                    error_msg = "cancel";
                    ok = false;
                }
                fclose(ifile);
            }
        }
        if( ok )
        {
            std::string title3( "Appending to database, step 3 of 4");
            ok = BinDbDuplicateRemoval(title3,this);
        }
        if( ok )
        {
            std::string title4( "Creating database, step 4 of 4");
            std::string desc4("Writing file");
            ProgressBar progress_bar4( title4, desc4, true, this );
            ok = BinDbWriteOutToFile(ofile,&progress_bar4);
            fclose(ofile);
        }
        if( ok )
        {
            wxSafeYield();
            AcceptAndClose();
            db_created_ok = true;
        }
        else
        {
            if( error_msg == "" )
                error_msg = db_primitive_error_msg();
            if( error_msg == "cancel" )
                error_msg = "Database creation cancelled";
            wxMessageBox( error_msg.c_str(), "Database creation failed", wxOK|wxICON_ERROR );
            _unlink(db_filename.c_str());
        }
        // TODO DatabaseReload();
    }
}

void CreateDatabaseDialog::OnDbFilePicked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    db_filename = file;
}

void CreateDatabaseDialog::OnPgnFile1Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename1 = file;
}

void CreateDatabaseDialog::OnPgnFile2Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename2 = file;
}

void CreateDatabaseDialog::OnPgnFile3Picked( wxFileDirPickerEvent& event )
{
    wxString file = event.GetPath();
    pgn_filename3 = file;
}

void CreateDatabaseDialog::OnHelpClick( wxCommandEvent& WXUNUSED(event) )
{
    // Normally we would wish to display proper online help.
    // For this example, we're just using a message box.
    /*
     wxGetApp().GetHelpController().DisplaySection(wxT("Personal record dialog"));
     */
    
    wxString helpText =
    wxT("\nTarrasch database files (.tdb files) are compact game collections. ")
    wxT("At the moment only complete games are supported (i.e. no game fragments). ")
    wxT("Similarly no comments or variations are supported at this time. ")
    wxT("Tarrasch uses one database file at a time (the current database). ")
    wxT("Tarrasch can search the current database quickly and efficiently ")
    wxT("for any position or for piece patterns or material balances. ")
    wxT("Tarrasch databases are created from .pgn files.")
    wxT("\n\n")
    wxT("The only way Tarrasch databases can be modified (at this time) is by ")
    wxT("appending more games to them with the Append to database command. ")
    wxT("Tarrasch automatically rejects duplicate games and games played by ")
    wxT("players with insufficiently high Elo ratings. ")
    wxT("Rejected duplicate games are saved in file TarraschDbDuplicatesFile.pgn. ")
    wxT("Since most historical games don't have rating information, games with ")
    wxT("no rating information at all are not rejected. ")
    wxT("For best results, create databases with older .pgn files first, and ")
    wxT("append newer games as you collect them. ")
    wxT("Tarrasch will try to present most recent games first." );
    wxMessageBox(helpText,
                 wxT("Create database / Append to database help"),
                 wxOK|wxICON_INFORMATION, this);
}
