#ifndef __MEDUSA_CONTROL_FLOW_GRAPH_HPP__
#define __MEDUSA_CONTROL_FLOW_GRAPH_HPP__

#include "medusa/namespace.hpp"
#include "medusa/export.hpp"
#include "medusa/types.hpp"
#include "medusa/basic_block.hpp"
#include "medusa/database.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topology.hpp>

MEDUSA_NAMESPACE_BEGIN

//! ControlFlowGraph is a graph which contains BasicBlock.
class Medusa_EXPORT ControlFlowGraph
{
  struct ControlFlowGraphProperties{};

public:
  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::directedS,
    BasicBlockVertexProperties,
    BasicBlockEdgeProperties,
    ControlFlowGraphProperties
  > Type;

  typedef boost::rectangle_topology<> TopologyType;
  typedef TopologyType::point_type PointType;
  typedef std::vector<PointType> PointVector;
  typedef boost::iterator_property_map<
    PointVector::iterator, boost::property_map<Type, boost::vertex_index_t>::type
  > PositionMap;

  typedef boost::graph_traits<Type>::vertex_descriptor BasicBlockVertexDescriptor;
  typedef boost::graph_traits<Type>::edge_descriptor   BasicBlockEdgeDescriptor;

  typedef boost::graph_traits<Type>::vertex_iterator   BasicBlockIterator;

  ControlFlowGraph(void);

  //! AddBasicBlockVertex adds a new basic block.
  void AddBasicBlockVertex(BasicBlockVertexProperties const& rVertex);
  //! AddBasicBlockEdge adds a new connection (i.e. branch/jump) between basic block.
  bool AddBasicBlockEdge(BasicBlockEdgeProperties const& rEdge, Address const& rSrcAddr, Address const& rDstAddr);
  // FindBasicBlock finds a basic block by \param rAddr.
  bool FindBasicBlock(Address const& rAddr, BasicBlockVertexDescriptor& BasicBlckDesc);
  // SplitBasicBlock splits a basic block using both \param rDstAddr and \param rSrcAddr.
  bool SplitBasicBlock(Address const& rDstAddr, Address const& rSrcAddr, BasicBlockEdgeProperties::Type Type);
  // Finalize allows to connect orphan basic block.
  void Finalize(Database const& rDb);

  // Dump graph layout in dot language format (only address)
  void Dump(std::string const& rFilename);
  // Dump graph layout in dot language format (with instructions)
  void Dump(std::string const& rFilename, Database const& rDb);

  bool Layout(PositionMap& rPosMap);

private:
  typedef std::map<Address, BasicBlockVertexDescriptor> VertexMap;

  Type      m_Graph;
  VertexMap m_VertexMap;
};

MEDUSA_NAMESPACE_END

#endif // !__MEDUSA_CONTROL_FLOW_GRAPH_HPP__