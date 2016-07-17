// EpochPDB.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct p_string
{
    unsigned char               namelen;
    char                        name[1];
};


#include <pshpack1.h>
	struct Relocation
	{
		uint32_t address;
		uint32_t symbolindex;
		uint16_t type;
	};
#include <poppack.h>

typedef struct OMFDirHeader
{
    WORD        cbDirHeader;
    WORD        cbDirEntry;
    DWORD       cDir;
    DWORD       lfoNextDir;
    DWORD       flags;
} OMFDirHeader;


union codeview_symbol
{
    struct
    {
        short int	        len;
        short int	        id;
    } generic;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        symtype;
        struct p_string         p_name;
    } data_v1;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        symtype;
	unsigned int	        offset;
	unsigned short	        segment;
        struct p_string         p_name;
    } data_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } data_v3;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        thunk_len;
	unsigned char	        thtype;
        struct p_string         p_name;
    } thunk_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            pparent;
        unsigned int            pend;
        unsigned int            next;
        unsigned int            offset;
        unsigned short          segment;
        unsigned short          thunk_len;
        unsigned char           thtype;
        char                    name[1];
    } thunk_v3;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        proc_len;
	unsigned int	        debug_start;
	unsigned int	        debug_end;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned short	        proctype;
	unsigned char	        flags;
        struct p_string         p_name;
    } proc_v1;

    struct
    {
	short int	        len;
	short int	        id;
	unsigned int	        pparent;
	unsigned int	        pend;
	unsigned int	        next;
	unsigned int	        proc_len;
	unsigned int	        debug_start;
	unsigned int	        debug_end;
	unsigned int	        proctype;
	unsigned int	        offset;
	unsigned short	        segment;
	unsigned char	        flags;
        struct p_string         p_name;
    } proc_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            pparent;
        unsigned int            pend;
        unsigned int            next;
        unsigned int            proc_len;
        unsigned int            debug_start;
        unsigned int            debug_end;
        unsigned int            proctype;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        char                    name[1];
    } proc_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        struct p_string         p_name;
    } public_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } public_v3;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_BPREL_V1 */
	unsigned int	        offset;	        /* Stack offset relative to BP */
	unsigned short	        symtype;
        struct p_string         p_name;
    } stack_v1;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_BPREL_V2 */
	unsigned int	        offset;	        /* Stack offset relative to EBP */
	unsigned int	        symtype;
        struct p_string         p_name;
    } stack_v2;

    struct
    {
        short int               len;            /* Total length of this entry */
        short int               id;             /* Always S_BPREL_V3 */
        int                     offset;         /* Stack offset relative to BP */
        unsigned int            symtype;
        char                    name[1];
    } stack_v3;

    struct
    {
        short int               len;            /* Total length of this entry */
        short int               id;             /* Always S_BPREL_V3 */
        int                     offset;         /* Stack offset relative to BP */
        unsigned int            symtype;
        unsigned short          reg;
        char                    name[1];
    } regrel_v3;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_REGISTER */
        unsigned short          type;
        unsigned short          reg;
        struct p_string         p_name;
        /* don't handle register tracking */
    } register_v1;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_REGISTER_V2 */
        unsigned int            type;           /* check whether type & reg are correct */
        unsigned short          reg;
        struct p_string         p_name;
        /* don't handle register tracking */
    } register_v2;

    struct
    {
	short int	        len;	        /* Total length of this entry */
	short int	        id;		/* Always S_REGISTER_V3 */
        unsigned int            type;           /* check whether type & reg are correct */
        unsigned short          reg;
        char                    name[1];
        /* don't handle register tracking */
    } register_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            parent;
        unsigned int            end;
        unsigned int            length;
        unsigned int            offset;
        unsigned short          segment;
        struct p_string         p_name;
    } block_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            parent;
        unsigned int            end;
        unsigned int            length;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } block_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        struct p_string         p_name;
    } label_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
        unsigned char           flags;
        char                    name[1];
    } label_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned short          type;
        unsigned short          cvalue;         /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } constant_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        unsigned short          cvalue;         /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } constant_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        unsigned short          cvalue;
#if 0
        char                    name[1];
#endif
    } constant_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned short          type;
        struct p_string         p_name;
    } udt_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                type;
        struct p_string         p_name;
    } udt_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            type;
        char                    name[1];
    } udt_v3;

    struct
    {
        short int               len;
        short int               id;
        char                    signature[4];
        struct p_string         p_name;
    } objname_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            unknown;
        struct p_string         p_name;
    } compiland_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned                unknown1[4];
        unsigned short          unknown2;
        struct p_string         p_name;
    } compiland_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            unknown;
        char                    name[1];
    } compiland_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
        unsigned short          symtype;
        struct p_string         p_name;
    } thread_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        struct p_string         p_name;
    } thread_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            symtype;
        unsigned int            offset;
        unsigned short          segment;
        char                    name[1];
    } thread_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned short          segment;
    } ssearch_v1;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            offset;
        unsigned int            unknown;
    } security_cookie_v3;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            sz_frame;       /* size of frame */
        unsigned int            unknown2;
        unsigned int            unknown3;
        unsigned int            sz_saved_regs;  /* size of saved registers from callee */
        unsigned int            eh_offset;      /* offset for exception handler */
        unsigned short          eh_sect;        /* section for exception handler */
        unsigned int            flags;
    } frame_info_v2;

    struct
    {
        short int               len;
        short int               id;
        unsigned int            checksum;
        unsigned int            offset;
        unsigned int            module;
        struct p_string         p_name;  // not included in len
    } procref_v1;

	struct
	{
        short int               len;
        short int               id;
		unsigned short int      sizeLocals;  // sum of size of locals and arguments
		unsigned short int      unknown[10];
		unsigned short int      info;        // hasAlloca,hasSetjmp,hasLongjmp,hasInlAsm,hasEH,inl_specified,hasSEH,naked,hasGsChecks,hasEHa,noStackOrdering,wasInlined,strictGsCheck
		// return UDT,instance constructor,instance constructor with virtual base
		unsigned int            unknown2;
	} funcinfo_32;
};


typedef struct OMFDirEntry
{
    WORD        SubSection;
    WORD        iMod;
    DWORD       lfo;
    DWORD       cb;
} OMFDirEntry;

typedef struct OMFSymHash
{
    unsigned short  symhash;
    unsigned short  addrhash;
    unsigned long   cbSymbol;
    unsigned long   cbHSym;
    unsigned long   cbHAddr;
} OMFSymHash;


struct Mod {
public: virtual unsigned long Mod::QueryInterfaceVersion(void);
public: virtual unsigned long Mod::QueryImplementationVersion(void);
public: virtual int Mod::AddTypes(unsigned char *pTypeData,long cbTypeData);
public: virtual int Mod::AddSymbols(unsigned char *pSymbolData,long cbSymbolData);
public: virtual int Mod::AddPublic(char const *,unsigned short,long); // forwards to AddPublic2(...,0)
public: virtual int Mod::AddLines(char const *fname,unsigned short sec,long off,long size,long off2,unsigned short firstline,unsigned char *pLineInfo,long cbLineInfo); // forwards to AddLinesW
public: virtual int Mod::AddSecContrib(unsigned short sec,long off,long size,unsigned long secflags); // forwards to Mod::AddSecContribEx(..., 0, 0)
public: virtual int Mod::QueryCBName(long *);
public: virtual int Mod::QueryName(char * const,long *);
public: virtual int Mod::QuerySymbols(unsigned char *,long *);
public: virtual int Mod::QueryLines(unsigned char *,long *);
public: virtual int Mod::SetPvClient(void *);
public: virtual int Mod::GetPvClient(void * *);
public: virtual int Mod::QueryFirstCodeSecContrib(unsigned short *,long *,long *,unsigned long *);
public: virtual int Mod::QueryImod(unsigned short *);
public: virtual int Mod::QueryDBI(class DBI * *);
public: virtual int Mod::Close(void);
public: virtual int Mod::QueryCBFile(long *);
public: virtual int Mod::QueryFile(char * const,long *);
public: virtual int Mod::QueryTpi(struct TPI * *);
public: virtual int Mod::AddSecContribEx(unsigned short sec,long off,long size,unsigned long secflags,unsigned long crc/*???*/,unsigned long);
public: virtual int Mod::QueryItsm(unsigned short *);
public: virtual int Mod::QuerySrcFile(char * const,long *);
public: virtual int Mod::QuerySupportsEC(void);
public: virtual int Mod::QueryPdbFile(char * const,long *);
public: virtual int Mod::ReplaceLines(unsigned char *,long);
public: virtual bool Mod::GetEnumLines(struct EnumLines * *);
public: virtual bool Mod::QueryLineFlags(unsigned long *);
public: virtual bool Mod::QueryFileNameInfo(unsigned long,unsigned short *,unsigned long *,unsigned long *,unsigned char *,unsigned long *);
public: virtual int Mod::AddPublicW(unsigned short const *,unsigned short,long,unsigned long);
public: virtual int Mod::AddLinesW(unsigned short const *fname,unsigned short sec,long off,long size,long off2,unsigned long firstline,unsigned char *plineInfo,long cbLineInfo);
public: virtual int Mod::QueryNameW(unsigned short * const,long *);
public: virtual int Mod::QueryFileW(unsigned short * const,long *);
public: virtual int Mod::QuerySrcFileW(unsigned short * const,long *);
public: virtual int Mod::QueryPdbFileW(unsigned short * const,long *);
public: virtual int Mod::AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
public: virtual int Mod::InsertLines(unsigned char *,long);
public: virtual int Mod::QueryLines2(long,unsigned char *,long *);
};


struct DBI_part1 {
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual int OpenMod(char const *objName,char const *libName,struct Mod * *);
public: virtual int DeleteMod(char const *);
public: virtual int QueryNextMod(struct Mod *,struct Mod * *);
public: virtual int OpenGlobals(struct GSI * *);
public: virtual int OpenPublics(struct GSI * *);
public: virtual int AddSec(unsigned short sec,unsigned short flags,long offset,long cbseg);
public: virtual int QueryModFromAddr(unsigned short,long,struct Mod * *,unsigned short *,long *,long *);
public: virtual int QuerySecMap(unsigned char *,long *);
public: virtual int QueryFileInfo(unsigned char *,long *);
public: virtual void DumpMods(void);
public: virtual void DumpSecContribs(void);
public: virtual void DumpSecMap(void);
public: virtual int Close(void);
public: virtual int AddThunkMap(long *,unsigned int,long,struct SO *,unsigned int,unsigned short,long);
public: virtual int AddPublic(char const *,unsigned short,long);
public: virtual int getEnumContrib(struct Enum * *);
public: virtual int QueryTypeServer(unsigned char,struct TPI * *);
public: virtual int QueryItsmForTi(unsigned long,unsigned char *);
public: virtual int QueryNextItsm(unsigned char,unsigned char *);
public: virtual int reinitialize(void); // returns 0 (QueryLazyTypes in 10.0)
public: virtual int SetLazyTypes(int);
public: virtual int FindTypeServers(long *,char *);
public: virtual void noop(void); // noop (_Reserved_was_QueryMreLog in 10.0)
public: virtual int OpenDbg(enum DBGTYPE,struct Dbg * *);
public: virtual int QueryDbgTypes(enum DBGTYPE *,long *);
public: virtual int QueryAddrForSec(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};
struct DBI_part2 : public DBI_part1 {
// in mspdb100.dll:
public: virtual int QueryAddrForSecEx(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};

template<class BASE> 
struct DBI_BASE : public BASE {
public: virtual int QuerySupportsEC(void);
public: virtual int QueryPdb(class PDB * *);
public: virtual int AddLinkInfo(struct LinkInfo *);
public: virtual int QueryLinkInfo(struct LinkInfo *,long *);
public: virtual unsigned long QueryAge(void)const ;
public: virtual int reinitialize2(void);  // returns 0 (QueryLazyTypes in 10.0)
public: virtual void FlushTypeServers(void);
public: virtual int QueryTypeServerByPdb(char const *,unsigned char *);
public: virtual int OpenModW(unsigned short const *objName,unsigned short const *libName,struct Mod * *);
public: virtual int DeleteModW(unsigned short const *);
public: virtual int AddPublicW(unsigned short const *name,unsigned short sec,long off,unsigned long type);
public: virtual int QueryTypeServerByPdbW(unsigned short const *,unsigned char *);
public: virtual int AddLinkInfoW(struct LinkInfoW *);
public: virtual int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
public: virtual unsigned short QueryMachineType(void)const ;
public: virtual void SetMachineType(unsigned short);
public: virtual void RemoveDataForRva(unsigned long,unsigned long);
public: virtual int FStripped(void);
public: virtual int QueryModFromAddr2(unsigned short,long,struct Mod * *,unsigned short *,long *,long *,unsigned long *);
public: virtual int QueryNoOfMods(long *);
public: virtual int QueryMods(struct Mod * *,long);
public: virtual int QueryImodFromAddr(unsigned short,long,unsigned short *,unsigned short *,long *,long *,unsigned long *);
public: virtual int OpenModFromImod(unsigned short,struct Mod * *);
public: virtual int QueryHeader2(long,unsigned char *,long *);
public: virtual int FAddSourceMappingItem(unsigned short const *,unsigned short const *,unsigned long);
public: virtual int FSetPfnNotePdbUsed(void *,void (__cdecl*)(void *,unsigned short const *,int,int));
public: virtual int FCTypes(void);
public: virtual int QueryFileInfo2(unsigned char *,long *);
public: virtual int FSetPfnQueryCallback(void *,int (__cdecl*(__cdecl*)(void *,enum DOVC))(void));
};

struct DBI_VS9  : public DBI_BASE<DBI_part1> {};
struct DBI_VS10 : public DBI_BASE<DBI_part2> {};

class DBI
{
public:
	unsigned long QueryImplementationVersion() { return vs9.QueryImplementationVersion(); }
    unsigned long QueryInterfaceVersion() { return vs9.QueryInterfaceVersion(); }
    int Close() { return vs9.Close(); }

    Mod* OpenMod(char const *objName, char const *libName)
	{
		Mod* ret = nullptr;
		if(vs9.OpenMod(objName, libName, &ret) <= 0 || !ret)
			return nullptr;

		return ret;
	}

    int AddSec(unsigned short sec,unsigned short flags,long offset,long cbseg) { return vs9.AddSec(sec,flags,offset,cbseg); }

    int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type)
    {
        return ((DBI_VS10*) &vs9)->AddPublic2(name, sec, off, type);
    }
    void SetMachineType(unsigned short type)
    {
        return ((DBI_VS10*) &vs9)->SetMachineType(type);
    }

private:
    DBI_VS9 vs9;
};


struct TPI {
public: virtual unsigned long TPI::QueryInterfaceVersion(void);
public: virtual unsigned long TPI::QueryImplementationVersion(void);
public: virtual int TPI::QueryTi16ForCVRecord(unsigned char *,unsigned short *);
public: virtual int TPI::QueryCVRecordForTi16(unsigned short,unsigned char *,long *);
public: virtual int TPI::QueryPbCVRecordForTi16(unsigned short,unsigned char * *);
public: virtual unsigned short TPI::QueryTi16Min(void);
public: virtual unsigned short TPI::QueryTi16Mac(void);
public: virtual long TPI::QueryCb(void);
public: virtual int TPI::Close(void);
public: virtual int TPI::Commit(void);
public: virtual int TPI::QueryTi16ForUDT(char const *,int,unsigned short *);
public: virtual int TPI::SupportQueryTiForUDT(void);
public: virtual int TPI::fIs16bitTypePool(void);
public: virtual int TPI::QueryTiForUDT(char const *,int,unsigned long *);
public: virtual int TPI::QueryTiForCVRecord(unsigned char *,unsigned long *);
public: virtual int TPI::QueryCVRecordForTi(unsigned long,unsigned char *,long *);
public: virtual int TPI::QueryPbCVRecordForTi(unsigned long,unsigned char * *);
public: virtual unsigned long TPI::QueryTiMin(void);
public: virtual unsigned long TPI::QueryTiMac(void);
public: virtual int TPI::AreTypesEqual(unsigned long,unsigned long);
public: virtual int TPI::IsTypeServed(unsigned long);
public: virtual int TPI::QueryTiForUDTW(unsigned short const *,int,unsigned long *);
};


std::ostream& operator<<(std::ostream& os, const GUID& guid){

    os << std::uppercase;
    os.width(8);
    os << std::hex << guid.Data1 << '-';

    os.width(4);
    os << std::hex << guid.Data2 << '-';

    os.width(4);
    os << std::hex << guid.Data3 << '-';

    os.width(2);
    os << std::hex
        << static_cast<short>(guid.Data4[0])
        << static_cast<short>(guid.Data4[1])
        << '-'
        << static_cast<short>(guid.Data4[2])
        << static_cast<short>(guid.Data4[3])
        << static_cast<short>(guid.Data4[4])
        << static_cast<short>(guid.Data4[5])
        << static_cast<short>(guid.Data4[6])
        << static_cast<short>(guid.Data4[7]);
    os << std::nouppercase;
    return os;
}


struct PDB_part1 {
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual long QueryLastError(char * const);
public: virtual char * QueryPDBName(char * const);
public: virtual unsigned long QuerySignature(void);
public: virtual unsigned long QueryAge(void);
public: virtual int CreateDBI(const char *, DBI**);
public: virtual int OpenDBI(const char *, const char *, DBI**);
public: virtual int OpenTpi(char const *,struct TPI * *);
};

struct PDB_part_vs11 : public PDB_part1 {
public: virtual int OpenIpi(char const *,struct IPI * *); // VS11
};

template<class BASE>
struct PDB_part2 : public BASE {
public: virtual int Commit(void);
public: virtual int Close(void);
public: virtual int OpenStreamW(unsigned short const *,struct Stream * *);
public: virtual int GetEnumStreamNameMap(struct Enum * *);
public: virtual int GetRawBytes(int (__cdecl*)(void const *,long));
public: virtual unsigned long QueryPdbImplementationVersion(void);
public: virtual int OpenDBIEx(char const *,char const *, DBI * *,int (__stdcall*)(struct _tagSEARCHDEBUGINFO *));
public: virtual int CopyTo(char const *,unsigned long,unsigned long);
public: virtual int OpenSrc(struct Src * *);
public: virtual long QueryLastErrorExW(unsigned short *,unsigned int);
public: virtual unsigned short * QueryPDBNameExW(unsigned short *,unsigned int);
public: virtual int QuerySignature2(struct _GUID *);
public: virtual int CopyToW(unsigned short const *,unsigned long,unsigned long);
public: virtual int fIsSZPDB(void)const ;
public: virtual int containsW(unsigned short const *,unsigned long *);
public: virtual int CopyToW2(unsigned short const *,unsigned long,int (__cdecl*(__cdecl*)(void *,enum PCC))(void),void *);
public: virtual int OpenStreamEx(char const *,char const *,struct Stream * *);
};

struct PDB_VS10 : public PDB_part2<PDB_part1> {};
struct PDB_VS11 : public PDB_part2<PDB_part_vs11> {};



class PDB
{
public:
	int Commit()
	{
		return ((PDB_VS11*)&basev10)->Commit();
	}
	int Close()
	{
		return ((PDB_VS11*)&basev10)->Close();
	}
	int QuerySignature2(struct _GUID* guid)
	{
		return ((PDB_VS11*)&basev10)->QuerySignature2(guid);
	}

	DBI* CreateDBI()
	{
		DBI* ret = nullptr;
		if(basev10.CreateDBI("", &ret) <= 0 || !ret)
			return nullptr;

		return ret;
	}

	TPI* OpenTPI()
	{
		TPI* ret = nullptr;
		if(basev10.OpenTpi("", &ret) <= 0 || !ret)
			return nullptr;

		return ret;
	}

	unsigned long QueryAge()
	{
		return basev10.QueryAge();
	}

private:
	PDB_VS10 basev10;
};


typedef int __cdecl FPdbOpen2W(const wchar_t* path, char const* mode, long* p, wchar_t* ext, uint64_t flags, PDB** pPDB);



template<typename FuncPtrT>
FuncPtrT GetProcPtr(HMODULE mod, const char* funcname)
{
	return reinterpret_cast<FuncPtrT>(::GetProcAddress(mod, funcname));
}



namespace
{


	template<typename T, typename BufferElemT>
	T ConsumeFromBuffer(BufferElemT*& buffer)
	{
		T ret = *reinterpret_cast<const T*>(buffer);
		buffer += sizeof(T);
		return ret;
	}


	struct LineInfoEntry
	{
		unsigned int offset;
		unsigned short line;
	};


	void GeneratePDB(PDB* pdb)
	{
		std::cout << "PDB opened!" << std::endl;

		GUID guid;
		pdb->QuerySignature2(&guid);

		std::cout << "PDB age is " << pdb->QueryAge() << " on GUID " << guid << std::endl;


		DBI* dbi = pdb->CreateDBI();
		TPI* tpi = pdb->OpenTPI();
		if(dbi && tpi)
		{
			if(dbi->AddSec(1, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;
			
			if(dbi->AddSec(2, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(3, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(4, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(5, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(6, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(7, 0x1, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(8, 0x5, 0x0, 0x1000) <= 0)
				std::cout << "Error adding section..." << std::endl;

			Mod* mod = dbi->OpenMod("TestSuite.obj", "TestSuite.lib");
			if(mod)
			{
				if(mod->AddSecContrib(8, 0, 0x1000, 0x5) <= 0)
					std::cout << "Failure with section contribution!" << std::endl;
					
				LineInfoEntry lineinfo;
				lineinfo.offset = 0;
				lineinfo.line = 1;

				if(mod->AddLines("LinkedProgram.epoch", 8, 0, 0x6, 0x6, 1, reinterpret_cast<unsigned char*>(&lineinfo), sizeof(lineinfo)) <= 0)
					std::cout << "Failed to add line info!" << std::endl;

				std::basic_ifstream<char> infile("TestSuite.sym", std::ios::binary);
				std::vector<char> symbolBuffer((std::istreambuf_iterator<char>(infile)), (std::istreambuf_iterator<char>()));

				const IMAGE_SYMBOL* psyms = reinterpret_cast<const IMAGE_SYMBOL*>(symbolBuffer.data());
				unsigned numsyms = 0x39;			// TODO
				const char* stringstart = symbolBuffer.data() + (numsyms * sizeof(IMAGE_SYMBOL));
				for(unsigned i = 0; i < numsyms; ++i)
				{
					const char* name = stringstart + psyms[i].N.LongName[1];

					//if(!ISFCN(psyms[i].Type))
					//	continue;

					if(mod->AddPublic2(name, 8, psyms[i].Value, psyms[i].Type) <= 0)
						std::cout << "Failure to add public symbol!" << std::endl;
				}



				std::basic_ifstream<char> relocfile("TestSuite.reloc", std::ios::binary);
				std::vector<char> relocbuffer((std::istreambuf_iterator<char>(relocfile)), (std::istreambuf_iterator<char>()));

				std::basic_ifstream<unsigned char> cvfile("TestSuite.cv", std::ios::binary);
				std::vector<unsigned char> cvbuffer((std::istreambuf_iterator<unsigned char>(cvfile)), (std::istreambuf_iterator<unsigned char>()));




				size_t numrelocs = relocbuffer.size() / sizeof(Relocation);
				Relocation* reloc = reinterpret_cast<Relocation*>(relocbuffer.data());

				for(size_t i = 0; i < numrelocs; ++i, ++reloc)
				{
					if(reloc->type == IMAGE_REL_AMD64_SECREL)
					{
						if(psyms[reloc->symbolindex].SectionNumber == 8)
						{
							unsigned adjustment = psyms[reloc->symbolindex].Value + 0x8000;

							DWORD* adjusttarget = (DWORD*)(cvbuffer.data() + reloc->address);
							*adjusttarget = adjustment;
						}
					}
					else if(reloc->type == IMAGE_REL_AMD64_SECTION)
					{
						assert(psyms[reloc->symbolindex].SectionNumber == 8);

						WORD* adjusttarget = (WORD*)(cvbuffer.data() + reloc->address);
						*adjusttarget = 8;
					}
				}



				std::vector<unsigned char> compactedcv;

				unsigned char* cvdata = cvbuffer.data();
				DWORD magic = ConsumeFromBuffer<DWORD>(cvdata);
				assert(magic == 4);

				std::copy(cvbuffer.data(), cvdata, std::back_inserter(compactedcv));

				while(size_t(cvdata - cvbuffer.data()) < cvbuffer.size())
				{
					unsigned char* bookmark = cvdata;
					DWORD sectiontype = ConsumeFromBuffer<DWORD>(cvdata);
					DWORD sectionsize = ConsumeFromBuffer<DWORD>(cvdata);

					cvdata += sectionsize;

					if(sectiontype == 0xf1)
					{
						std::copy(bookmark, cvdata, std::back_inserter(compactedcv));
					}
				}
				
				if(mod->AddSymbols(compactedcv.data(), (long)compactedcv.size()) <= 0)
					std::cout << "Failed to add symbols!" << std::endl;

				mod->Close();
			}
			else
			{
				std::cout << "Error grabbing module" << std::endl;
			}

			dbi->SetMachineType(IMAGE_FILE_MACHINE_AMD64);
			dbi->Close();

			tpi->Commit();
			tpi->Close();
		}
		else
		{
			std::cout << "DBI/TPI failure" << std::endl;
		}

		pdb->Commit();
		pdb->Close();
	}


}



int main()
{

	HMODULE mod = ::LoadLibraryA("mspdb140.dll");
	if(!mod)
	{
		std::cout << "Failed to load mspdb140.dll" << std::endl;
		return -1;
	}

	PDB* pdb = nullptr;

	long dummybuffer[194] = {193, 0};
	wchar_t ext[256] = L".exe";
	
	auto pdbopen = GetProcPtr<FPdbOpen2W*>(mod, "PDBOpen2W");
	int result = pdbopen(L"TestSuite.pdb", "wf", dummybuffer, ext, 0x400, &pdb);
	if(result)
	{
		GeneratePDB(pdb);
	}
	else
	{
		std::cout << "Barfed." << std::endl;
	}


	::FreeLibrary(mod);

    return 0;
}

