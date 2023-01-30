/// \file
/// \brief Implements debug symbol table (from .nl files)

#include "debugsymboltable.h"

#include "types.h"
#include "debug.h"
#include "fceu.h"
#include "cart.h"
#include "Qt/ConsoleUtilities.h"

extern FCEUGI *GameInfo;

debugSymbolTable_t  debugSymbolTable;

//--------------------------------------------------------------
// debugSymbolPage_t
//--------------------------------------------------------------
debugSymbolPage_t::debugSymbolPage_t(void)
{
	pageNum = -1;

}
//--------------------------------------------------------------
debugSymbolPage_t::~debugSymbolPage_t(void)
{
	for (auto it=symMap.begin(); it!=symMap.end(); it++)
	{
		delete it->second;
	}
}
//--------------------------------------------------------------
int debugSymbolPage_t::addSymbol( debugSymbol_t*sym )
{
	if ( symMap.count( sym->ofs ) || symNameMap.count( sym->name ) )
	{
		return -1;
	}

	symMap[ sym->ofs ] = sym;
	symNameMap[ sym->name ] = sym;

	return 0;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolPage_t::getSymbolAtOffset( int ofs )
{
	auto it = symMap.find( ofs );
	return it != symMap.end() ? it->second : NULL;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolPage_t::getSymbol( const std::string &name )
{
	auto it = symNameMap.find( name );
	return it != symNameMap.end() ? it->second : NULL;
}
//--------------------------------------------------------------
int debugSymbolPage_t::deleteSymbolAtOffset( int ofs )
{
	auto it = symMap.find( ofs );

	if ( it != symMap.end() )
	{
		auto itName = symNameMap.find( it->second->name );

		if ( itName != symNameMap.end() )
		{
			delete it->second;

			symMap.erase(it);
			symNameMap.erase(itName);

			return 0;
		}
	}
	return -1;
}
//--------------------------------------------------------------
int debugSymbolPage_t::save(void)
{
	FILE *fp;
	debugSymbol_t *sym;
	std::map <int, debugSymbol_t*>::iterator it;
	const char *romFile;
	std::string filename;
	char stmp[512];
	int i,j;

	if ( symMap.size() == 0 )
	{
		//printf("Skipping Empty Debug Page Save\n");
		return 0;
	}
	if ( pageNum == -2 )
	{
		//printf("Skipping Register Debug Page Save\n");
		return 0;
	}

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			filename.push_back('.');
		}
		else
		{
			filename.push_back(romFile[i]);
		}
		i++;
	}

	if ( pageNum < 0 )
	{
		filename.append(".ram.nl" );
	}
	else
	{
		char suffix[32];

		sprintf( suffix, ".%X.nl", pageNum );

		filename.append( suffix );
	}

	fp = ::fopen( filename.c_str(), "w" );

	if ( fp == NULL )
	{
		printf("Error: Could not open file '%s' for writing\n", filename.c_str() );
		return -1;
	}

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		const char *c;

		sym = it->second;

		i=0; j=0; c = sym->comment.c_str();

		while ( c[i] != 0 )
		{
			if ( c[i] == '\n' )
			{
				i++; break;
			}
			else
			{
				stmp[j] = c[i]; j++; i++;
			}
		}
		stmp[j] = 0;

		fprintf( fp, "$%04X#%s#%s\n", sym->ofs, sym->name.c_str(), stmp );

		j=0;
		while ( c[i] != 0 )
		{
			if ( c[i] == '\n' )
			{
				i++; stmp[j] = 0;

				if ( j > 0 )
				{
					fprintf( fp, "\\%s\n", stmp );
				}
				j=0;
			}
			else
			{
				stmp[j] = c[i]; j++; i++;
			}
		}
	}

	fclose(fp);

	return 0;
}
//--------------------------------------------------------------
void debugSymbolPage_t::print(void)
{
	FILE *fp;
	debugSymbol_t *sym;
	std::map <int, debugSymbol_t*>::iterator it;

	fp = stdout;

	fprintf( fp, "Page: %X \n", pageNum );

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		sym = it->second;

		fprintf( fp, "   Sym: $%04X '%s' \n", sym->ofs, sym->name.c_str() );
	}
}
//--------------------------------------------------------------
// debugSymbolTable_t
//--------------------------------------------------------------
debugSymbolTable_t::debugSymbolTable_t(void)
{

}
//--------------------------------------------------------------
debugSymbolTable_t::~debugSymbolTable_t(void)
{
	this->clear();
}
//--------------------------------------------------------------
void debugSymbolTable_t::clear(void)
{
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		delete it->second;
	}
	pageMap.clear();
}
//--------------------------------------------------------------
static int generateNLFilenameForBank(int bank, std::string &NLfilename)
{
	int i;
	const char *romFile;

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			NLfilename.push_back('.');
		}
		else
		{
			NLfilename.push_back(romFile[i]);
		}
		i++;
	}

	if (bank < 0)
	{
		// The NL file for the RAM addresses has the name nesrom.nes.ram.nl
		NLfilename.append(".ram.nl");
	}
	else
	{
		char stmp[64];
		#ifdef DW3_NL_0F_1F_HACK
		if(bank == 0x0F)
			bank = 0x1F;
		#endif
		sprintf( stmp, ".%X.nl", bank);
		NLfilename.append( stmp );
	}
	return 0;
}
//--------------------------------------------------------------
int generateNLFilenameForAddress(int address, std::string &NLfilename)
{
	int bank;

	if (address < 0x8000)
	{
		bank = -1;
	}
	else
	{
		bank = getBank(address);
		#ifdef DW3_NL_0F_1F_HACK
		if(bank == 0x0F)
			bank = 0x1F;
		#endif
	}
	return generateNLFilenameForBank( bank, NLfilename );
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadFileNL( int bank )
{
	FILE *fp;
	int i, j, ofs, lineNum = 0, literal = 0, array = 0;
	std::string fileName;
	char stmp[512], line[512];
	debugSymbolPage_t *page = NULL;
	debugSymbol_t *sym = NULL;

	//printf("Looking to Load Debug Bank: $%X \n", bank );

	if ( generateNLFilenameForBank( bank, fileName ) )
	{
		return -1;
	}
	//printf("Loading NL File: %s\n", fileName.c_str() );

	fp = ::fopen( fileName.c_str(), "r" );

	if ( fp == NULL )
	{
		return -1;
	}
	page = new debugSymbolPage_t;

	page->pageNum = bank;

	pageMap[ page->pageNum ] = page;

	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		i=0; lineNum++;
		//printf("%4i:%s", lineNum, line );

		if ( line[i] == '\\' )
		{
			// Line is a comment continuation line.
			i++;

			j=0;
			stmp[j] = '\n'; j++;

			while ( line[i] != 0 )
			{
				stmp[j] = line[i]; j++; i++;
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0;
				}
				else
				{
					break;
				}
				j--;
			}
			if ( sym != NULL )
			{
				sym->comment.append( stmp );
			}
		}
		else if ( line[i] == '$' )
		{
			// Line is a new debug offset
			array = 0;

			j=0; i++;
			if ( !isxdigit( line[i] ) )
			{
				printf("Error: Invalid Offset on Line %i of File %s\n", lineNum, fileName.c_str() );
			}
			while ( isxdigit( line[i] ) )
			{
				stmp[j] = line[i]; i++; j++;
			}
			stmp[j] = 0;

			ofs = strtol( stmp, NULL, 16 );

			if ( line[i] == '/' )
			{
				j=0; i++;
				while ( isxdigit( line[i] ) )
				{
					stmp[j] = line[i]; i++; j++;
				}
				stmp[j] = 0;

				array = strtol( stmp, NULL, 16 );
			}

			if ( line[i] != '#' )
			{
				printf("Error: Missing field delimiter following offset $%X on Line %i of File %s\n", ofs, lineNum, fileName.c_str() );
				continue;
			}
			i++;

			while ( isspace(line[i]) ) i++;

			j = 0;
			while ( line[i] != 0 )
			{
				if ( line[i] == '\\' )
				{
					if ( literal )
					{
						switch ( line[i] )
						{
							case 'r':
								stmp[j] = '\r';
							break;
							case 'n':
								stmp[j] = '\n';
							break;
							case 't':
								stmp[j] = '\t';
							break;
							default:
								stmp[j] = line[i];
							break;
						}
						j++; i++;
						literal = 0;
					}
					else
					{
						i++;
						literal = !literal;
					}
				}
				else if ( line[i] == '#' )
				{
					break;
				}
				else
				{
					stmp[j] = line[i]; j++; i++;
				}
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0;
				}
				else
				{
					break;
				}
				j--;
			}

			if ( line[i] != '#' )
			{
				printf("Error: Missing field delimiter following name '%s' on Line %i of File %s\n", stmp, lineNum, fileName.c_str() );
				continue;
			}
			i++;

			sym = new debugSymbol_t();

			if ( sym == NULL )
			{
				printf("Error: Failed to allocate memory for offset $%04X Name '%s' on Line %i of File %s\n", ofs, stmp, lineNum, fileName.c_str() );
				continue;
			}
			sym->ofs = ofs;
			sym->name.assign( stmp );

			while ( isspace( line[i] ) ) i++;

			j=0;
			while ( line[i] != 0 )
			{
				stmp[j] = line[i]; j++; i++;
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0;
				}
				else
				{
					break;
				}
				j--;
			}

			sym->comment.assign( stmp );

			if ( array > 0 )
			{
				debugSymbol_t *arraySym = NULL;

				for (j=0; j<array; j++)
				{
					arraySym = new debugSymbol_t();

					if ( arraySym )
					{
						arraySym->ofs = sym->ofs + j;

						sprintf( stmp, "[%i]", j );
						arraySym->name.assign( sym->name );
						arraySym->name.append( stmp );
						arraySym->comment.assign( sym->comment );

						if ( page->addSymbol( arraySym ) )
						{
							printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, arraySym->name.c_str(), lineNum, fileName.c_str() );
							delete arraySym; arraySym = NULL; // Failed to add symbol
						}
					}
				}
				delete sym; sym = NULL; // Delete temporary symbol
			}
			else
			{
				if ( page->addSymbol( sym ) )
				{
					printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, sym->name.c_str(), lineNum, fileName.c_str() );
					delete sym; sym = NULL; // Failed to add symbol
				}
			}
		}
	}

	::fclose(fp);

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadRegisterMap(void)
{
	debugSymbolPage_t *page;

	page = new debugSymbolPage_t();

	page->pageNum = -2;

	page->addSymbol( new debugSymbol_t( 0x2000, "PPU_CTRL" ) );
	page->addSymbol( new debugSymbol_t( 0x2001, "PPU_MASK" ) );
	page->addSymbol( new debugSymbol_t( 0x2002, "PPU_STATUS" ) );
	page->addSymbol( new debugSymbol_t( 0x2003, "PPU_OAM_ADDR" ) );
	page->addSymbol( new debugSymbol_t( 0x2004, "PPU_OAM_DATA" ) );
	page->addSymbol( new debugSymbol_t( 0x2005, "PPU_SCROLL" ) );
	page->addSymbol( new debugSymbol_t( 0x2006, "PPU_ADDRESS" ) );
	page->addSymbol( new debugSymbol_t( 0x2007, "PPU_DATA" ) );
	page->addSymbol( new debugSymbol_t( 0x4000, "SQ1_VOL" ) );
	page->addSymbol( new debugSymbol_t( 0x4001, "SQ1_SWEEP" ) );
	page->addSymbol( new debugSymbol_t( 0x4002, "SQ1_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x4003, "SQ1_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4004, "SQ2_VOL" ) );
	page->addSymbol( new debugSymbol_t( 0x4005, "SQ2_SWEEP" ) );
	page->addSymbol( new debugSymbol_t( 0x4006, "SQ2_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x4007, "SQ2_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4008, "TRI_LINEAR" ) );
//	page->addSymbol( new debugSymbol_t( 0x4009, "UNUSED" ) );
	page->addSymbol( new debugSymbol_t( 0x400A, "TRI_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x400B, "TRI_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x400C, "NOISE_VOL" ) );
//	page->addSymbol( new debugSymbol_t( 0x400D, "UNUSED" ) );
	page->addSymbol( new debugSymbol_t( 0x400E, "NOISE_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x400F, "NOISE_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4010, "DMC_FREQ" ) );
	page->addSymbol( new debugSymbol_t( 0x4011, "DMC_RAW" ) );
	page->addSymbol( new debugSymbol_t( 0x4012, "DMC_START" ) );
	page->addSymbol( new debugSymbol_t( 0x4013, "DMC_LEN" ) );
	page->addSymbol( new debugSymbol_t( 0x4014, "OAM_DMA" ) );
	page->addSymbol( new debugSymbol_t( 0x4015, "APU_STATUS" ) );
	page->addSymbol( new debugSymbol_t( 0x4016, "JOY1" ) );
	page->addSymbol( new debugSymbol_t( 0x4017, "JOY2_FRAME" ) );

	pageMap[ page->pageNum ] = page;

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadGameSymbols(void)
{
	int nPages, pageSize, romSize = 0x10000;

	this->save();
	this->clear();

	if ( GameInfo != NULL )
	{
		romSize = 16 + CHRsize[0] + PRGsize[0];
	}

	loadFileNL( -1 );

	loadRegisterMap();

	pageSize = (1<<debuggerPageSize);

	//nPages = 1<<(15-debuggerPageSize);
	nPages = romSize / pageSize;

	//printf("RomSize: %i    NumPages: %i \n", romSize, nPages );

	for(int i=0;i<nPages;i++)
	{
		//printf("Loading Page Offset: $%06X\n", pageSize*i );

		loadFileNL( i );
	}

	//print();

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::addSymbolAtBankOffset( int bank, int ofs, debugSymbol_t *sym )
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	it = pageMap.find( bank );

	if ( it == pageMap.end() )
	{
		page = new debugSymbolPage_t();
		page->pageNum = bank;
		pageMap[ bank ] = page;
	}
	else
	{
		page = it->second;
	}
	page->addSymbol( sym );

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::deleteSymbolAtBankOffset( int bank, int ofs )
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	it = pageMap.find( bank );

	if ( it == pageMap.end() )
	{
		return -1;
	}
	else
	{
		page = it->second;
	}

	return page->deleteSymbolAtOffset( ofs );
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbolAtBankOffset( int bank, int ofs )
{
	auto it = pageMap.find( bank );

	return it != pageMap.end() ? it->second->getSymbolAtOffset( ofs ) : NULL;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbol( int bank, const std::string &name )
{
	auto it = pageMap.find( bank );

	return it != pageMap.end() ? it->second->getSymbol( name ) : NULL;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbolAtAnyBank( const std::string &name )
{
	for (auto &page : pageMap)
	{
		auto sym = getSymbol( page.first, name );

		if ( sym )
		{
			return sym;
		}
	}

	return NULL;
}
//--------------------------------------------------------------
void debugSymbolTable_t::save(void)
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		page = it->second;

		page->save();
	}
}
//--------------------------------------------------------------
void debugSymbolTable_t::print(void)
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		page = it->second;

		page->print();
	}
}
