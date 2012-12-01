#ifndef _MEDUSA_MEMORY_AREA_
#define _MEDUSA_MEMORY_AREA_

#include "medusa/namespace.hpp"
#include "medusa/export.hpp"
#include "medusa/cell.hpp"
#include "medusa/value.hpp"
#include "medusa/binary_stream.hpp"
#include "medusa/types.hpp"
#include "medusa/address.hpp"

#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

MEDUSA_NAMESPACE_BEGIN

#define MA_READ    0x00000001
#define MA_WRITE   0x00000002
#define MA_EXEC    0x00000004

//! MemoryArea contains cells for a slice of memory.
class Medusa_EXPORT MemoryArea
{
public:
  typedef std::pair<TOffset, Cell*> TCellPair;
  typedef std::vector<TCellPair>    TCellMap;
  typedef TCellMap::iterator        TIterator;
  typedef TCellMap::const_iterator  TConstIterator;

  virtual ~MemoryArea(void);

  static bool CompareByVirtualBase(MemoryArea const* lhs, MemoryArea const* rhs);

  std::string             ToString(void) const;

  Cell*                   RetrieveCell(TOffset Off);
  Cell const*             RetrieveCell(TOffset Off) const;
  bool                    InsertCell(TOffset Off, Cell* pCell, Address::List& rErasedCell, bool Force = false, bool Safe = true);

  std::string const&      GetName(void)        const   { return m_Name;                       }
  u64                     GetSize(void)        const   { return m_Cells.size();               }
  u32                     GetAccess(void)      const   { return m_Access;                     }

  //! This method translates a virtual offset to physical offset i.e. file offset.
  bool                    Translate(TOffset VirtualOffset, TOffset& rPhysicalOffset) const;

  //! This method converts a virtual offset to a memory area offset.
  bool                    Convert(TOffset VirtualOffset, TOffset& rMemAreaOffset) const;

  bool                    IsPresent(TOffset Off) const
  {
    if (m_VirtualBase.GetAddressingType() == Address::UnknownType)
      return false;

    return m_VirtualBase.IsBetween(GetSize(), Off);
  }

  bool                    IsPresent(Address const& Addr) const
  {
    if (m_VirtualBase.GetAddressingType() == Address::UnknownType)
      return false;

    return m_VirtualBase.IsBetween(GetSize(), Addr);
  }

  bool                    IsPresent(Address::SharedPtr spAddr) const
  {
    if (m_VirtualBase.GetAddressingType() == Address::UnknownType)
      return false;

    return m_VirtualBase.IsBetween(GetSize(), *spAddr.get());
  }

  void                    FormatAddress(Address& rAddr) const
  {
    rAddr = Address(
      m_VirtualBase.GetAddressingType(),
      m_VirtualBase.GetBase(),     rAddr.GetOffset(),
      m_VirtualBase.GetBaseSize(), m_VirtualBase.GetOffsetSize()
      );
  }

  Address                 MakeAddress(TOffset Offset) const
  {
    return Address(
      m_VirtualBase.GetAddressingType(),
      m_VirtualBase.GetBase(),     Offset,
      m_VirtualBase.GetBaseSize(), m_VirtualBase.GetOffsetSize()
      );
  }

  TIterator               Begin(void) { return m_Cells.begin(); }
  TIterator               End(void)   { return m_Cells.end();   }

  TConstIterator          Begin(void) const { return m_Cells.begin(); }
  TConstIterator          End(void)   const { return m_Cells.end();   }

  virtual bool            Read(TOffset Offset, void* pBuffer, u32 Size) const   { return false; }
  virtual bool            Write(TOffset Offset, void const* pBuffer, u32 Size)  { return false; }
  BinaryStream const&     GetBinaryStream(void) const { return m_BinStrm; }

  void                    SetEndianness(EEndianness Endianness) { m_BinStrm.SetEndianness(Endianness); }

  Address const&  GetPhysicalBase(void) const { return m_PhysicalBase; }
  u32             GetPhysicalSize(void) const { return m_PhysicalSize; }

  Address const&  GetVirtualBase(void) const { return m_VirtualBase; }
  u32             GetVirtualSize(void) const { return m_VirtualSize; }

  MemoryArea& operator=(MemoryArea const& rMemoryArea)
  {
    if (this == &rMemoryArea) return *this;
    m_Name         = rMemoryArea.m_Name;
    m_PhysicalBase = rMemoryArea.m_PhysicalBase;
    m_PhysicalSize = rMemoryArea.m_PhysicalSize;
    m_VirtualBase  = rMemoryArea.m_VirtualBase;
    m_VirtualSize  = rMemoryArea.m_VirtualSize;
    m_Access       = rMemoryArea.m_Access;
    m_Cells        = rMemoryArea.m_Cells;
    m_BinStrm      = rMemoryArea.m_BinStrm;
    return *this;
  }

  MemoryArea(MemoryArea const& rMemoryArea)
    : m_Name(rMemoryArea.m_Name)
    , m_PhysicalBase(rMemoryArea.m_PhysicalBase)
    , m_PhysicalSize(rMemoryArea.m_PhysicalSize)
    , m_VirtualBase(rMemoryArea.m_VirtualBase)
    , m_VirtualSize(rMemoryArea.m_VirtualSize)
    , m_Access(rMemoryArea.m_Access)
    , m_Cells(rMemoryArea.m_Cells)
    , m_BinStrm(rMemoryArea.m_BinStrm)
  {
  }

protected:
  MemoryArea(
    u8* pMemoryArea = NULL,
    std::string const& rName = "",
    Address const& rPhysicalBase = Address(), u32 PhysicalSize = 0x0,
    Address const& rVirtualBase = Address(),  u32 VirtualSize = 0x0,
    u32 Access = 0x0
    )
    : m_BinStrm(pMemoryArea, VirtualSize)
    , m_Name(rName)
    , m_PhysicalBase(rPhysicalBase),  m_PhysicalSize(PhysicalSize)
    , m_VirtualBase(rVirtualBase),    m_VirtualSize(VirtualSize)
    , m_Access(Access)
    , m_Cells()
  {}

  void                    CreateUnitializeCell(u32 DefaultValueType);

  Cell*                   GetCell(TOffset Off);
  Cell const*             GetCell(TOffset Off) const;
  bool                    SetCell(TOffset Off, Cell* pCell);
  bool                    FillCell(TOffset Off);
  bool                    EraseCell(TOffset Off);

  void                    Sanitize(TOffset NewOff, size_t OldCellSize, Address::List& rErasedCell);

  bool                    GetPreviousCell(TOffset& rOff, Cell*& prInfo);
  bool                    GetNextCell(TOffset& rOff, Cell*& prInfo, size_t LimitSize = -1);

  std::string             m_Name;
  Address                 m_PhysicalBase;
  u32                     m_PhysicalSize;
  Address                 m_VirtualBase;
  u32                     m_VirtualSize;
  u32                     m_Access;
  TCellMap                m_Cells;
  MemoryBinaryStream      m_BinStrm;

  typedef boost::mutex    MutexType;
  mutable MutexType       m_Mutex;
};

//! PhysicalMemoryArea is a MemoryArea which contains cells present in file but not in memory.
class Medusa_EXPORT PhysicalMemoryArea : public MemoryArea
{
public:
  PhysicalMemoryArea(void) : MemoryArea() {}

  PhysicalMemoryArea(
    BinaryStream const& rBinStrm,
    std::string const& rName,
    Address const& rPhysicalBase, u32 PhysicalSize,
    u32 Access
    )
    : MemoryArea(NULL, rName, rPhysicalBase, PhysicalSize, Address(), 0x0, Access)
  {
    u8* pMemArea = new u8[PhysicalSize];
    rBinStrm.Read(rPhysicalBase.GetOffset(), pMemArea, PhysicalSize);
    m_BinStrm.Open(pMemArea, PhysicalSize);
    m_BinStrm.SetEndianness(rBinStrm.GetEndianness());

    CreateUnitializeCell(VT_HEX);
  }

  virtual bool                  Read(TOffset Offset, void* pBuffer, u32 Size) const;
  virtual bool                  Write(TOffset Offset, void const* pBuffer, u32 Size);
};

//! MappedMemoryArea is a MemoryArea which contains cells present in both file and memory.
class Medusa_EXPORT MappedMemoryArea : public MemoryArea
{
public:
  MappedMemoryArea(void) : MemoryArea() {}

  MappedMemoryArea(
    BinaryStream const& rBinStrm,
    std::string const& rName,
    Address const& rPhysicalBase, u32 PhysicalSize,
    Address const& rVirtualBase,  u32 VirtualSize,
    u32 Access
    )
    : MemoryArea(NULL, rName
    , rPhysicalBase, PhysicalSize
    , rVirtualBase,  VirtualSize
    , Access)
  {
    u8* pMemArea = new u8[VirtualSize];

    if (VirtualSize <= PhysicalSize)
      rBinStrm.Read(rPhysicalBase.GetOffset(), pMemArea, VirtualSize);
    else
    {
      rBinStrm.Read(rPhysicalBase.GetOffset(), pMemArea, PhysicalSize);
      u32 Diff = VirtualSize - PhysicalSize;
      memset(pMemArea + PhysicalSize, 0x0, Diff);
    }

    m_BinStrm.Open(pMemArea, VirtualSize);
    m_BinStrm.SetEndianness(rBinStrm.GetEndianness());

    CreateUnitializeCell(VT_HEX);
  }

  virtual bool                  Read(TOffset Offset, void* pBuffer, u32 Size) const;
  virtual bool                  Write(TOffset Offset, void const* pBuffer, u32 Size);
};

//! VirtualMemoryArea is a MemoryArea which contains cells present in memory but not in file.
class Medusa_EXPORT VirtualMemoryArea : public MemoryArea
{
public:
  VirtualMemoryArea(void) : MemoryArea() {}

  VirtualMemoryArea(
    EEndianness Endianness,
    std::string const& rName,
    Address const& rVirtualBase, u32 VirtualSize,
    u32 Access
    )
    : MemoryArea(NULL, rName, Address(), 0x0, rVirtualBase, VirtualSize, Access)
  {
    u8* pMemArea = new u8[VirtualSize];
    memset(pMemArea, 0x0, VirtualSize);
    m_BinStrm.Open(pMemArea, VirtualSize);
    m_BinStrm.SetEndianness(Endianness);

    CreateUnitializeCell(VT_UNK);
  }

  virtual bool                  Read(TOffset Offset, void* pBuffer, u32 Size) const;
  virtual bool                  Write(TOffset Offset, void const* pBuffer, u32 Size);

protected:
};

MEDUSA_NAMESPACE_END

#endif // _MEDUSA_MEMORY_AREA_