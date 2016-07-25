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

union codeview_type
{
    struct
    {
        unsigned short int      len;
        short int               id;
    } generic;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               attribute;
        unsigned short int      type;
    } modifier_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        int                     type;
        short int               attribute;
    } modifier_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               attribute;
        unsigned short int      datatype;
        struct p_string         p_name;
    } pointer_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            datatype;
        unsigned int            attribute;
        struct p_string         p_name;
    } pointer_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      elemtype;
        unsigned short int      idxtype;
        unsigned short int      arrlen;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } array_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            elemtype;
        unsigned int            idxtype;
        unsigned short int      arrlen;    /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } array_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            elemtype;
        unsigned int            idxtype;
        unsigned short int      arrlen;    /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } array_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        unsigned short int      fieldlist;
        short int               property;
        unsigned short int      derived;
        unsigned short int      vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } struct_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        short int               property;
        unsigned int            fieldlist;
        unsigned int            derived;
        unsigned int            vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } struct_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        short int               n_element;
        short int               property;
        unsigned int            fieldlist;
        unsigned int            derived;
        unsigned int            vshape;
        unsigned short int      structlen;  /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } struct_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        unsigned short int      fieldlist;
        short int               property;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } union_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        short int               property;
        unsigned int            fieldlist;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        struct p_string         p_name;
#endif
    } union_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        short int               property;
        unsigned int            fieldlist;
        unsigned short int      un_len;     /* numeric leaf */
#if 0
        char                    name[1];
#endif
    } union_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        unsigned short int      type;
        unsigned short int      fieldlist;
        short int               property;
        struct p_string         p_name;
    } enumeration_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        short int               property;
        unsigned int            type;
        unsigned int            fieldlist;
        struct p_string         p_name;
    } enumeration_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      count;
        short int               property;
        unsigned int            type;
        unsigned int            fieldlist;
        char                    name[1];
    } enumeration_v3;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      rvtype;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned short int      arglist;
    } procedure_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            rvtype;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned int            arglist;
    } procedure_v2;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned short int      rvtype;
        unsigned short int      class_type;
        unsigned short int      this_type;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short int      params;
        unsigned short int      arglist;
        unsigned int            this_adjust;
    } mfunction_v1;

    struct
    {
        unsigned short int      len;
        short int               id;
        unsigned int            rvtype;
        unsigned int            class_type;
        unsigned                this_type;
        unsigned char           call;
        unsigned char           reserved;
        unsigned short          params;
        unsigned int            arglist;
        unsigned int            this_adjust;
    } mfunction_v2;
};

#include <poppack.h>


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
		if(basev10.CreateDBI(nullptr, &ret) <= 0 || !ret)
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

	std::string QueryLastError()
	{
		char buffer[256];
		long ret = basev10.QueryLastError(buffer);

		std::ostringstream format;
		format << buffer << " (" << ret << ")";

		return format.str();
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


#include <pshpack1.h>

	struct LineInfoEntry
	{
		unsigned int offset;
		unsigned short line;
	};

#include <poppack.h>

	typedef unsigned long   CV_pubsymflag_t;
	typedef union CV_PUBSYMFLAGS {
		CV_pubsymflag_t grfFlags;
		struct {
			CV_pubsymflag_t fCode       :  1;    // set if public symbol refers to a code address
			CV_pubsymflag_t fFunction   :  1;    // set if public symbol is a function
			CV_pubsymflag_t fManaged    :  1;    // set if managed code (native or IL)
			CV_pubsymflag_t fMSIL       :  1;    // set if managed IL code
			CV_pubsymflag_t __unused    : 28;    // must be zero
		};
	} CV_PUBSYMFLAGS;




	void RelocateRawCVData(std::vector<unsigned char>* cvbuffer, const std::vector<unsigned char>& relocbuffer, const std::vector<unsigned char>& symbuffer)
	{
		const IMAGE_SYMBOL* psyms = reinterpret_cast<const IMAGE_SYMBOL*>(symbuffer.data());
		const unsigned numsyms = 0x39;		// TODO
		
		size_t numrelocs = relocbuffer.size() / sizeof(Relocation);
		const Relocation* prelocs = reinterpret_cast<const Relocation*>(relocbuffer.data());
		
		for(size_t i = 0; i < numrelocs; ++i)
		{
			if(prelocs[i].type == IMAGE_REL_AMD64_SECREL)
			{
				assert(prelocs[i].address <= cvbuffer->size() - sizeof(DWORD));
				assert(psyms[prelocs[i].symbolindex].SectionNumber == 8);

				unsigned adjustment = psyms[prelocs[i].symbolindex].Value;

				DWORD* target = reinterpret_cast<DWORD*>(cvbuffer->data() + prelocs[i].address);
				assert(*target == 0);

				*target = adjustment;
			}
			else if(prelocs[i].type == IMAGE_REL_AMD64_SECTION)
			{
				assert(prelocs[i].address <= cvbuffer->size() - sizeof(WORD));
				assert(psyms[prelocs[i].symbolindex].SectionNumber == 8);

				WORD* target = reinterpret_cast<WORD*>(cvbuffer->data() + prelocs[i].address);
				assert(*target == 0);

				*target = 8;
			}
			else
			{
				assert(false);
			}
		}
	}


	void AssembleCorrectedCVData(Mod* mod, const std::vector<unsigned char>& inbuffer, std::vector<unsigned char>* outbuffer)
	{
		const unsigned char* input = inbuffer.data();

		DWORD versionheader = ConsumeFromBuffer<DWORD>(input);
		assert(versionheader == 4);


		Utility::AppendToBuffer(outbuffer, versionheader);

		Utility::AppendToBuffer(outbuffer, DWORD(0xf1));
		Utility::AppendToBuffer(outbuffer, DWORD(26));
		Utility::AppendToBuffer(outbuffer, WORD(24));
		Utility::AppendToBuffer(outbuffer, WORD(0x1136));		// S_SECTION
		Utility::AppendToBuffer(outbuffer, WORD(8));
		Utility::AppendToBuffer(outbuffer, unsigned char(0));
		Utility::AppendToBuffer(outbuffer, unsigned char(0));
		Utility::AppendToBuffer(outbuffer, DWORD(0x8000));
		Utility::AppendToBuffer(outbuffer, DWORD(0x1000));
		Utility::AppendToBuffer(outbuffer, DWORD(0x06000020));
		Utility::AppendToBuffer(outbuffer, ".text");
		Utility::AppendToBuffer(outbuffer, unsigned char('\0'));

		while(outbuffer->size() % 4)
			outbuffer->push_back(0);



		while(input < inbuffer.data() + inbuffer.size())
		{
			DWORD sectionheader = ConsumeFromBuffer<DWORD>(input);
			DWORD sectionsize = ConsumeFromBuffer<DWORD>(input);

			if(sectionheader == 0xf1)
			{
				WORD procbytes = ConsumeFromBuffer<WORD>(input);
				WORD procstartmagic = ConsumeFromBuffer<WORD>(input);
				assert(procstartmagic == 0x1147);

				ConsumeFromBuffer<DWORD>(input);
				ConsumeFromBuffer<DWORD>(input);
				ConsumeFromBuffer<DWORD>(input);

				DWORD funcsize = ConsumeFromBuffer<DWORD>(input);

				ConsumeFromBuffer<DWORD>(input);
				ConsumeFromBuffer<DWORD>(input);
				ConsumeFromBuffer<DWORD>(input);

				DWORD sectionrelative = ConsumeFromBuffer<DWORD>(input);
				WORD sectionindex = ConsumeFromBuffer<WORD>(input);

				ConsumeFromBuffer<unsigned char>(input);

				std::string name;
				char b;

				while((b = ConsumeFromBuffer<char>(input)) != '\0')
					name += b;

				WORD procterm = ConsumeFromBuffer<WORD>(input);
				assert(procterm == 0x02);

				WORD procend = ConsumeFromBuffer<WORD>(input);
				assert(procend == 0x114F);

				Utility::AppendToBuffer(outbuffer, sectionheader);
				Utility::AppendToBuffer(outbuffer, sectionsize);
				Utility::AppendToBuffer(outbuffer, procbytes);
				Utility::AppendToBuffer(outbuffer, procstartmagic);

				// pad 1
				Utility::AppendToBuffer(outbuffer, 0);
				Utility::AppendToBuffer(outbuffer, 0);
				Utility::AppendToBuffer(outbuffer, 0);

				Utility::AppendToBuffer(outbuffer, funcsize);

				// pad 2
				Utility::AppendToBuffer(outbuffer, 0);
				Utility::AppendToBuffer(outbuffer, 0);
				Utility::AppendToBuffer(outbuffer, 0);

				Utility::AppendToBuffer(outbuffer, sectionrelative);
				Utility::AppendToBuffer(outbuffer, sectionindex);

				Utility::AppendToBuffer(outbuffer, unsigned char(0));
				Utility::AppendToBuffer(outbuffer, name.c_str());
				Utility::AppendToBuffer(outbuffer, unsigned char('\0'));

				Utility::AppendToBuffer(outbuffer, procterm);
				Utility::AppendToBuffer(outbuffer, procend);

				while(outbuffer->size() % 4)
					outbuffer->push_back(0);

				if(mod->AddSecContrib(sectionindex, sectionrelative, funcsize, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_LNK_COMDAT) <= 0)
					std::cout << "Failure with section contribution!" << std::endl;
			}
			else
			{
				assert(sectionheader == 0xf2 || sectionheader == 0xf3 || sectionheader == 0xf4);

				Utility::AppendToBuffer(outbuffer, sectionheader);
				Utility::AppendToBuffer(outbuffer, sectionsize);

				const unsigned char* p = input;
				while(p < input + sectionsize)
				{
					outbuffer->push_back(*p);
					++p;
				}
				
				input += sectionsize;
			}

			while((input - inbuffer.data()) % 4)
				ConsumeFromBuffer<char>(input);
		}
	}

	void PopulateModuleV2(Mod* mod)
	{
		// Open compiler-generated data files
		std::basic_ifstream<unsigned char> symfile("TestSuite.sym", std::ios::binary);
		std::basic_ifstream<unsigned char> relocfile("TestSuite.reloc", std::ios::binary);
		std::basic_ifstream<unsigned char> cvfile("TestSuite.cv", std::ios::binary);

		// Read file contents
		std::vector<unsigned char> symbuffer((std::istreambuf_iterator<unsigned char>(symfile)), (std::istreambuf_iterator<unsigned char>()));;
		std::vector<unsigned char> relocbuffer((std::istreambuf_iterator<unsigned char>(relocfile)), (std::istreambuf_iterator<unsigned char>()));;
		std::vector<unsigned char> cvbuffer((std::istreambuf_iterator<unsigned char>(cvfile)), (std::istreambuf_iterator<unsigned char>()));
		std::vector<unsigned char> codeviewdata;

		RelocateRawCVData(&cvbuffer, relocbuffer, symbuffer);
		AssembleCorrectedCVData(mod, cvbuffer, &codeviewdata);


		if(mod->AddSymbols(codeviewdata.data(), static_cast<long>(codeviewdata.size())) <= 0)
			std::cout << "Failure adding symbols!" << std::endl;

		// Set up public mappings for each function
		const IMAGE_SYMBOL* psyms = reinterpret_cast<const IMAGE_SYMBOL*>(symbuffer.data());
		const unsigned numsyms = 0x39;		// TODO
		const char* stringstart = reinterpret_cast<const char*>(symbuffer.data()) + (numsyms * sizeof(IMAGE_SYMBOL));

		for(size_t i = 0; i < numsyms; ++i)
		{
			if(!ISFCN(psyms[i].Type))
				continue;

			const char* symname = stringstart + psyms[i].N.LongName[1];
			short section = psyms[i].SectionNumber;
			DWORD value = psyms[i].Value;

			CV_PUBSYMFLAGS flags;
			flags.grfFlags = 0;
			flags.fFunction = 1;
			if(mod->AddPublic2(symname, section, value, flags.grfFlags) <= 0)
				std::cout << "Failure adding public symbol!" << std::endl;
		}

		// TODO - do we need to populate type data or something? (not likely)
	}

	void GeneratePDB(PDB* pdb)
	{
		std::cout << "PDB opened!" << std::endl;

		GUID guid;
		pdb->QuerySignature2(&guid);

		std::cout << "PDB age is " << pdb->QueryAge() << std::endl;

		for(size_t i = 0; i < sizeof(guid); ++i)
			printf("%02x ", (reinterpret_cast<const unsigned char*>(&guid))[i]);

		printf("\n");


		DBI* dbi = pdb->CreateDBI();
		TPI* tpi = pdb->OpenTPI();
		if(dbi && tpi)
		{
			if(dbi->AddSec(1, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;
			
			if(dbi->AddSec(2, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(3, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(4, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(5, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(6, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(7, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(8, 0x10d, 0x0, 0x1000) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSec(9, 0x208, 0x0, 0xffffffff) <= 0)
				std::cout << "Error adding section..." << std::endl;

			Mod* mod = dbi->OpenMod("D:\\epoch\\epoch-language\\x64\\debug\\TestSuite.obj", "D:\\epoch\\epoch-language\\x64\\debug\\TestSuite.lib");
			if(mod)
			{
				/*
				std::vector<char> typebuffer;

				DWORD typeheader = 4;
				Utility::AppendToBuffer(&typebuffer, typeheader);

				/ *
				{
					codeview_type t;
					t.pointer_v1.len = 8;//sizeof(t.pointer_v1) - sizeof(t.pointer_v1.len);
					t.pointer_v1.id = 0x1002;
					t.pointer_v1.p_name.namelen = 1;
					t.pointer_v1.p_name.name[0] = 'U';
					t.pointer_v1.attribute = 0x403;
					t.pointer_v1.datatype = 0x74;
					Utility::AppendToBuffer(&typebuffer, t.pointer_v1);
				}
				* /
				
				{
					codeview_type t;
					t.pointer_v2.len = 10;
					t.pointer_v2.id = 0x1002;
					t.pointer_v2.attribute = 0;
					t.pointer_v2.datatype = 0;
					Utility::AppendToBuffer<decltype(t.pointer_v2), 12>(&typebuffer, t.pointer_v2);
				}

				while(typebuffer.size() & 3)
					typebuffer.push_back(0xf4 - (typebuffer.size() & 3));

				int result = mod->AddTypes(reinterpret_cast<unsigned char*>(typebuffer.data()), (long)typebuffer.size());
				std::cout << result << std::endl;
				if(result <= 0)
					std::cout << "Failure with type registration!" << std::endl;

				std::cout << pdb->QueryLastError() << std::endl;
				*/

				PopulateModuleV2(mod);

				if(mod->Close() <= 0)
					std::cout << "Commit module failure" << std::endl;
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

