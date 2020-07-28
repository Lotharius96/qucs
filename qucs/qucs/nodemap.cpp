// (C) 2020 Felix Salfelder
//
#include "nodemap.h"
#include "node.h"
#include "trace.h"
#include "netlist.h"
#include "net.h"
// #include "schematic_model.h"

#include "dynamic_cc.h"

template<>
struct graph_traits<NodeMap>{
	typedef Conductor* vertex_descriptor;
	typedef AdjConductorIterator adjacency_iterator;
	typedef Net* cc_descriptor;

	static std::pair<AdjConductorIterator,AdjConductorIterator>
	adjacent_vertices(vertex_descriptor t, NodeMap const&) {
		auto n = t->neighbours();
		return std::make_pair(n.begin(), n.end());
	}

	// connected components parameters
	static cc_descriptor invalid_cc() { return nullptr; }
	static void set_cc(Conductor* t, Net* n) {
		// assert(n); // no. could as well detach
		trace2("set_cc", t, n);
		t->setNet(n);
	}
	static cc_descriptor get_cc(vertex_descriptor t) {
		return t->getNet();
	}
	static Net* new_cc(NodeMap& s);
	static void del_cc(Net*, NodeMap& s);
	static size_t& cc_size(Net* n, NodeMap const&);
	// needed for searching
	static void visit(Conductor* t, unsigned level) {
		t->visit(level);
	}
	static bool visited(Conductor const* t, unsigned level) {
		return t->visited(level);
	}
};

NodeMap::NodeMap(NetList& n)
		: _nets(n),
		   _cc(new_ccs())
{
}
NodeMap::~NodeMap()
{
	delete _cc;
}
		
ConnectedComponents<NodeMap>* NodeMap::new_ccs()
{
	return new ConnectedComponents<NodeMap>(*this);
}

// Qucs schematic node.
// at most one node in one place,
// may have strange side effects.
int NodeMap::erase(Node* tt)
{
	assert(tt->hasNet());
	Conductor* c = tt;
	_cc->deregisterVertex(c);
	assert(!tt->hasNet());

	_nodes.erase(tt);
	return 1;
}

Node* NodeMap::find_at(std::pair<int, int> key)
{
	auto f = _nodes.find(key);
	if(f != _nodes.end()){
		return *f;
	}else{
		return nullptr;
	}
}
Node* NodeMap::find_at(int x, int y)
{
	auto key=std::make_pair(x,y);
	return find_at(key);
}

// TODO: is this really needed with a proper map??
Node& NodeMap::new_at(int x, int y)
{

	auto key=std::make_pair(x,y);
	assert(!find_at(key)); // (!)

	trace2("new_at", x, y);

//	auto status = _nodes.emplace(key);
	auto status = _nodes.emplace(new Node(key));
	assert(status.second);
	Node* pn = *status.first;
	Conductor* c = pn;

	_cc->registerVertex(c);
   assert(c->hasNet());

	return *pn;
}

Node& NodeMap::at(int x, int y)
{
	Node* pn = find_at(x,y);

	// create new node, if no existing one lies at this position
	if(!pn) {
		pn = &new_at(x, y);
	}else{
	}

	assert(pn);
	assert(pn->hasNet());
	return *pn;
}

Net* NodeMap::newNet()
{
	return _nets.newNet();
}
void NodeMap::delNet(Net* n)
{
	assert(n);
	return _nets.delNet(n);
}

void NodeMap::postRemoveEdge(Conductor* a, Conductor* b)
{
  _cc->postRemoveEdge(a, b);
}
void NodeMap::addEdge(Conductor* a, Conductor* b)
{
  _cc->addEdge(a, b);
}
void NodeMap::registerVertex(Conductor*c)
{
  _cc->registerVertex(c);
}
void NodeMap::deregisterVertex(Conductor*c)
{
  _cc->deregisterVertex(c);
}

/* -------------------------------------------------------------- */
size_t& graph_traits<NodeMap>::cc_size(Net* n, NodeMap const&)
{
	assert(n);
	return n->size_hack();
}
Net* graph_traits<NodeMap>::new_cc(NodeMap& s)
{
	return s.newNet();
}
void graph_traits<NodeMap>::del_cc(Net* n, NodeMap& s)
{
	return s.delNet(n);
}

