/****************************************************************************
 * BinDb - A non-sql, compact chess database subsystem
 *  Author:  Bill Forster
 *  License: MIT license. Full text of license is in associated file LICENSE
 *  Copyright 2010-2016, Bill Forster <billforsternz at gmail dot com>
 ****************************************************************************/
#ifndef BINDB_H
#define BINDB_H
#include <vector>
#include "Appdefs.h"
#include "ProgressBar.h"
#include "BinaryConversions.h"
#include "ListableGame.h"

bool BinDbOpen( const char *db_file, int &version );
void BinDbClose();
void BinDbLoadAllGames( bool for_append, std::vector< smart_ptr<ListableGame> > &mega_cache, int &background_load_permill, bool &kill_background_load, ProgressBar *pb=NULL  );
std::vector< smart_ptr<ListableGame> > &BinDbLoadAllGamesGetVector();

bool bin_db_append( const char *fen, const char *event, const char *site, const char *date, const char *round,
                  const char *white, const char *black, const char *result, const char *white_elo, const char *black_elo, const char *eco,
                  int nbr_moves, thc::Move *moves );

bool TestBinaryBlock();
uint8_t BinDbReadBegin( bool use_packed_control_block );
uint32_t BinDbGetGamesSize();
void BinDbNormaliseOrder( uint32_t begin, uint32_t end );
bool BinDbDuplicateRemoval( std::string &title, wxWindow *window );
bool BinDbWriteOutToFile( FILE *ofile, ProgressBar *pb=NULL );
bool PgnStateMachine( FILE *pgn_file, int &typ, char *buf, int buflen );

void Pgn2Tdb( const char *infile, const char *outfile );
void Pgn2Tdb( FILE *fin, FILE *fout );
void Tdb2Pgn( const char *infile, const char *outfile );
void Tdb2Pgn( FILE *fin, FILE *fout );

int BitsRequired( int max );
void ReadStrings( FILE *fin, int nbr_strings, std::vector<std::string> &strings );

#endif  // BINDB_H
