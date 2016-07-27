//
// The Epoch Language Project
// Epoch Development Tools - PDB generation library
//
// EPOCHPDB.CPP
// Primary collecting point for PDB experimentation code
//
// This module is a dumping ground for all the experimentation going on
// with generation of PDB debug information files. PDBs enable us to debug
// Epoch programs using tools like Visual Studio and WinDbg. They will also
// eventually be useful for powering our own debugging tools should it be
// deemed worthwhile to roll our own (e.g. as part of some idealized IDE
// and/or REPL-type experience).
//
// Most of this should be documented as thoroughly as possible, not just
// for its own sake, but because it should ideally be contributed back to
// the open-source compiler community in some form or another.
//
// Please be aware that the final goal for this project is for it to go
// away entirely. Epoch's self-hosting compiler/linker should internalize
// the logic necessary for emitting PDB/MSF files in an ideal world. Once
// all of the code here is ported into Epoch, all that will be preserved
// is the documentation artifacts. We will try to extract all the juicy
// tidbits of knowledge into the Epoch Wiki or a similar resource so that
// other language/toolchain implementers can benefit without having to
// read the source for a full-blown compiler and linker.
//
// Notable inspiration for this module comes from CV2PDB of the D language
// ecosystem, from CVDUMP in the Microsoft GitHub repo, and from DIA2DUMP
// from the DIA SDK. Additional learnings gleaned from llvm-pdbdump will
// hopefully make the emission of MSF files possible, so we can break the
// dependency on MSPDB140.dll.
//

#include "stdafx.h"



//
// This is an interface (i.e. empty struct with a vtable of implementation
// pointers) exposed by MSPDB140.dll. It loosely represents the concept of
// a module within a PDB file; there can be multiple modules but they must
// agree on certain things like where bits of code and global data live in
// the debuggee. An IPDBModule may also represent an arbitrary number of
// actual code files in the original source, so there isn't really a strict
// 1:1 mapping between this concept and, well, anything else I can think of
// offhand. Epoch prefers to operate in a single module for simplicity,
// although it occurs to me that separate compilation might make it more
// useful to do a mod per some number of stable translation units....
//
// Anyways, the order of member functions in this struct is VERY important.
// It must align with the exported interface from MSPDB140.dll EXACTLY, as
// each function is basically called via the vtable by index, not name.
//
struct IPDBModule
{
	// Typical COM-style "what version are you?" queries.
	// Should be old hat to any native-code Windows developer
	virtual unsigned long QueryInterfaceVersion() = 0;
	virtual unsigned long QueryImplementationVersion() = 0;

	// These guys add blobs of CodeView data to the module.
	// AddTypes is still undergoing examination and experimentation
	// but AddSymbols is relatively well-understood now.
	virtual int AddTypes(unsigned char* pdata, long databytes) = 0;
	virtual int AddSymbols(unsigned char* pdata, long databytes) = 0;

	// TODO - cleanup below
	virtual int AddPublic(char const *,unsigned short,long); // forwards to AddPublic2(...,0)
	virtual int AddLines(char const *fname,unsigned short sec,long off,long size,long off2,unsigned short firstline,unsigned char *pLineInfo,long cbLineInfo); // forwards to AddLinesW
	virtual int AddSecContrib(unsigned short sec,long off,long size,unsigned long secflags); // forwards to Mod::AddSecContribEx(..., 0, 0)
	virtual int QueryCBName(long *);
	virtual int QueryName(char * const,long *);
	virtual int QuerySymbols(unsigned char *,long *);
	virtual int QueryLines(unsigned char *,long *);
	virtual int SetPvClient(void *);
	virtual int GetPvClient(void * *);
	virtual int QueryFirstCodeSecContrib(unsigned short *,long *,long *,unsigned long *);
	virtual int QueryImod(unsigned short *);
	virtual int QueryDBI(class IUnknown * *);
	virtual int Close();
	virtual int QueryCBFile(long *);
	virtual int QueryFile(char * const,long *);
	virtual int QueryTpi(class IUnknown * *);
	virtual int AddSecContribEx(unsigned short sec,long off,long size,unsigned long secflags,unsigned long crc/*???*/,unsigned long);
	virtual int QueryItsm(unsigned short *);
	virtual int QuerySrcFile(char * const,long *);
	virtual int QuerySupportsEC();
	virtual int QueryPdbFile(char * const,long *);
	virtual int ReplaceLines(unsigned char *,long);
	virtual bool GetEnumLines(struct EnumLines * *);
	virtual bool QueryLineFlags(unsigned long *);
	virtual bool QueryFileNameInfo(unsigned long,unsigned short *,unsigned long *,unsigned long *,unsigned char *,unsigned long *);
	virtual int AddPublicW(unsigned short const *,unsigned short,long,unsigned long);
	virtual int AddLinesW(unsigned short const *fname,unsigned short sec,long off,long size,long off2,unsigned long firstline,unsigned char *plineInfo,long cbLineInfo);
	virtual int QueryNameW(unsigned short * const,long *);
	virtual int QueryFileW(unsigned short * const,long *);
	virtual int QuerySrcFileW(unsigned short * const,long *);
	virtual int QueryPdbFileW(unsigned short * const,long *);
	virtual int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
	virtual int InsertLines(unsigned char *,long);
	virtual int QueryLines2(long,unsigned char *,long *);
};


struct IDBIVersion9
{
public: virtual unsigned long QueryImplementationVersion();
public: virtual unsigned long QueryInterfaceVersion();
public: virtual int OpenMod(char const *objName,char const *libName,struct IPDBModule * *);
public: virtual int DeleteMod(char const *);
public: virtual int QueryNextMod(struct IPDBModule *,struct IPDBModule * *);
public: virtual int OpenGlobals(struct GSI * *);
public: virtual int OpenPublics(struct GSI * *);
public: virtual int AddSec(unsigned short sec,unsigned short flags,long offset,long cbseg);
public: virtual int QueryModFromAddr(unsigned short,long,struct IPDBModule * *,unsigned short *,long *,long *);
public: virtual int QuerySecMap(unsigned char *,long *);
public: virtual int QueryFileInfo(unsigned char *,long *);
public: virtual void DumpMods();
public: virtual void DumpSecContribs();
public: virtual void DumpSecMap();
public: virtual int Close();
public: virtual int AddThunkMap(long *,unsigned int,long,struct SO *,unsigned int,unsigned short,long);
public: virtual int AddPublic(char const *,unsigned short,long);
public: virtual int getEnumContrib(struct Enum * *);
public: virtual int QueryTypeServer(unsigned char, IUnknown * *);
public: virtual int QueryItsmForTi(unsigned long,unsigned char *);
public: virtual int QueryNextItsm(unsigned char,unsigned char *);
public: virtual int reinitialize(); // returns 0 (QueryLazyTypes in 10.0)
public: virtual int SetLazyTypes(int);
public: virtual int FindTypeServers(long *,char *);
public: virtual void noop(); // noop (_Reserved_was_QueryMreLog in 10.0)
public: virtual int OpenDbg(enum DBGTYPE,struct Dbg * *);
public: virtual int QueryDbgTypes(enum DBGTYPE *,long *);
public: virtual int QueryAddrForSec(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};

struct IDBIVersion10 : public IDBIVersion9
{
// in mspdb100.dll:
public: virtual int QueryAddrForSecEx(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};

template<class IDBIBaseVersion> 
struct IDBIBase : public IDBIBaseVersion
{
public: virtual int QuerySupportsEC();
public: virtual int QueryPdb(class PDB * *);
public: virtual int AddLinkInfo(struct LinkInfo *);
public: virtual int QueryLinkInfo(struct LinkInfo *,long *);
public: virtual unsigned long QueryAge()const ;
public: virtual int reinitialize2();  // returns 0 (QueryLazyTypes in 10.0)
public: virtual void FlushTypeServers();
public: virtual int QueryTypeServerByPdb(char const *,unsigned char *);
public: virtual int OpenModW(unsigned short const *objName,unsigned short const *libName,struct IPDBModule * *);
public: virtual int DeleteModW(unsigned short const *);
public: virtual int AddPublicW(unsigned short const *name,unsigned short sec,long off,unsigned long type);
public: virtual int QueryTypeServerByPdbW(unsigned short const *,unsigned char *);
public: virtual int AddLinkInfoW(struct LinkInfoW *);
public: virtual int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
public: virtual unsigned short QueryMachineType()const ;
public: virtual void SetMachineType(unsigned short);
public: virtual void RemoveDataForRva(unsigned long,unsigned long);
public: virtual int FStripped();
public: virtual int QueryModFromAddr2(unsigned short,long,struct IPDBModule * *,unsigned short *,long *,long *,unsigned long *);
public: virtual int QueryNoOfMods(long *);
public: virtual int QueryMods(struct IPDBModule * *,long);
public: virtual int QueryImodFromAddr(unsigned short,long,unsigned short *,unsigned short *,long *,long *,unsigned long *);
public: virtual int OpenModFromImod(unsigned short,struct IPDBModule * *);
public: virtual int QueryHeader2(long,unsigned char *,long *);
public: virtual int FAddSourceMappingItem(unsigned short const *,unsigned short const *,unsigned long);
public: virtual int FSetPfnNotePdbUsed(void *,void (__cdecl*)(void *,unsigned short const *,int,int));
public: virtual int FCTypes();
public: virtual int QueryFileInfo2(unsigned char *,long *);
public: virtual int FSetPfnQueryCallback(void *,int (__cdecl*(__cdecl*)(void *,enum DOVC))());
};

class DBI
{
public:
	unsigned long QueryImplementationVersion()
	{
		return DBIVersion9.QueryImplementationVersion();
	}

    unsigned long QueryInterfaceVersion()
	{
		return DBIVersion9.QueryInterfaceVersion();
	}
    
	int Close()
	{
		return DBIVersion9.Close();
	}

    IPDBModule* OpenMod(const char* objname, const char* libname)
	{
		IPDBModule* ret = nullptr;
		if(DBIVersion9.OpenMod(objname, libname, &ret) <= 0 || !ret)
			return nullptr;

		return ret;
	}

    int AddSection(unsigned short sectionindex, unsigned short flags, long offset, long bytesinsection)
	{
		return DBIVersion9.AddSec(sectionindex, flags, offset, bytesinsection);
	}

    int AddPublic2(const char* name, unsigned short sectionindex, long offset, unsigned long type)
    {
        return ((IDBIBase<IDBIVersion10>*)&DBIVersion9)->AddPublic2(name, sectionindex, offset, type);
    }

    void SetMachineType(unsigned short type)
    {
        return ((IDBIBase<IDBIVersion10>*)&DBIVersion9)->SetMachineType(type);
    }

private:
    IDBIBase<IDBIVersion9> DBIVersion9;			// MUST BE the first member variable so it can snag the interface vtable at offset 0
};




struct IPDBBaseVersion10 {
	virtual unsigned long QueryInterfaceVersion();
	virtual unsigned long QueryImplementationVersion();
	virtual long QueryLastError(char* const);
	virtual char* QueryPDBName(char* const);
	virtual unsigned long QuerySignature();
	virtual unsigned long QueryAge();
	virtual int CreateDBI(const char*, DBI**);
	virtual int OpenDBI(const char*, const char *, DBI**);
	virtual int OpenTpi(const char*, IUnknown**);
};

struct IPDBBaseVersion11 : public IPDBBaseVersion10
{
	virtual int OpenIpi(const char*, IUnknown**); // VS11
};

template<class IPDBBaseVersion>
struct IPDB : public IPDBBaseVersion
{
	virtual int Commit();
	virtual int Close();
	virtual int OpenStreamW(const unsigned short*, IUnknown**);
	virtual int GetEnumStreamNameMap(IUnknown**);
	virtual int GetRawBytes(int (__cdecl*)(const void*, long));
	virtual unsigned long QueryPdbImplementationVersion();
	virtual int OpenDBIEx(const char*, const char*, DBI**, int (__stdcall*)(IUnknown*));
	virtual int CopyTo(const char*, unsigned long, unsigned long);
	virtual int OpenSrc(IUnknown**);
	virtual long QueryLastErrorExW(unsigned short*, unsigned int);
	virtual unsigned short* QueryPDBNameExW(unsigned short*, unsigned int);
	virtual int QuerySignature2(GUID* outguid);
	virtual int CopyToW(const unsigned short*, unsigned long, unsigned long);
	virtual int fIsSZPDB() const;
	virtual int containsW(const unsigned short*, unsigned long*);
	virtual int CopyToW2(const unsigned short*, unsigned long, int (__cdecl*(__cdecl*)(void*, enum EUnknown))(), void*);
	virtual int OpenStreamEx(const char*, const char*, IUnknown**);
};


class PDB
{
public:
	int Commit()
	{
		return ((IPDB<IPDBBaseVersion11>*)&PDBVersion10)->Commit();
	}
	int Close()
	{
		return ((IPDB<IPDBBaseVersion11>*)&PDBVersion10)->Close();
	}
	int QuerySignature2(GUID* guid)
	{
		return ((IPDB<IPDBBaseVersion11>*)&PDBVersion10)->QuerySignature2(guid);
	}

	DBI* CreateDBI()
	{
		DBI* ret = nullptr;
		if(PDBVersion10.CreateDBI(nullptr, &ret) <= 0 || !ret)
			return nullptr;

		return ret;
	}

	unsigned long QueryAge()
	{
		return PDBVersion10.QueryAge();
	}

	std::string QueryLastError()
	{
		char buffer[512];
		long ret = PDBVersion10.QueryLastError(buffer);

		std::ostringstream format;
		format << buffer << " (" << ret << ")";

		return format.str();
	}

private:
	IPDB<IPDBBaseVersion10> PDBVersion10;			// MUST BE the first member variable so it can snag the interface vtable at offset 0
};


namespace
{

	typedef int __cdecl FPdbOpen2W(const wchar_t* path, char const* mode, long* p, wchar_t* ext, uint64_t flags, PDB** pPDB);

	template<typename FuncPtrT>
	FuncPtrT GetProcPtr(HMODULE mod, const char* funcname)
	{
		return reinterpret_cast<FuncPtrT>(::GetProcAddress(mod, funcname));
	}



	template<typename T, typename BufferElemT>
	T ConsumeFromBuffer(BufferElemT*& buffer)
	{
		T ret = *reinterpret_cast<const T*>(buffer);
		buffer += sizeof(T);
		return ret;
	}


#include <pshpack1.h>

	struct Relocation
	{
		uint32_t address;
		uint32_t symbolindex;
		uint16_t type;
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


	void AssembleCorrectedCVData(IPDBModule* mod, const std::vector<unsigned char>& inbuffer, std::vector<unsigned char>* outbuffer)
	{
		const unsigned char* input = inbuffer.data();

		DWORD versionheader = ConsumeFromBuffer<DWORD>(input);
		assert(versionheader == 4);


		Utility::AppendToBuffer(outbuffer, versionheader);

		Utility::AppendToBuffer(outbuffer, DWORD(0xf1));					// symbol header
		Utility::AppendToBuffer(outbuffer, DWORD(26));						// length of section
		Utility::AppendToBuffer(outbuffer, WORD(24));						// length of subsequent record
		Utility::AppendToBuffer(outbuffer, WORD(0x1136));					// S_SECTION
		Utility::AppendToBuffer(outbuffer, WORD(8));						// image section index
		Utility::AppendToBuffer(outbuffer, unsigned char(0));				// ?
		Utility::AppendToBuffer(outbuffer, unsigned char(0));				// ?
		Utility::AppendToBuffer(outbuffer, DWORD(0x8000));					// RVA of image section
		Utility::AppendToBuffer(outbuffer, DWORD(0x1000));					// size of image section
		Utility::AppendToBuffer(outbuffer, DWORD(0x06000020));				// characteristics (code + read + execute)
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

	void PopulateModuleV2(IPDBModule* mod)
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
		if(dbi)
		{
			if(dbi->AddSection(1, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;
			
			if(dbi->AddSection(2, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(3, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(4, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(5, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(6, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(7, 0x109, 0x0, 0x100) <= 0)
				std::cout << "Error adding section..." << std::endl;

			if(dbi->AddSection(8, 0x10d, 0x0, 0x1000) <= 0)
				std::cout << "Error adding section..." << std::endl;

			// Not strictly sure if this is necessary... but the Microsoft
			// toolchain adds a dummy section that pads out the remainder of
			// the 32-bit address space, presumably as some kind of poisoning
			// strategy to catch out-of-bounds access?? Anyways it doesn't
			// seem to hurt anything so why not.
			if(dbi->AddSection(9, 0x208, 0x0, 0xffffffff) <= 0)
				std::cout << "Error adding section..." << std::endl;

			IPDBModule* mod = dbi->OpenMod("D:\\epoch\\epoch-language\\x64\\debug\\TestSuite.obj", "D:\\epoch\\epoch-language\\x64\\debug\\TestSuite.lib");
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
		}
		else
		{
			std::cout << "DBI failure" << std::endl;
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

