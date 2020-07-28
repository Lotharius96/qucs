// (C) 2020 Felix Salfelder
//
#ifndef QUCS_NODEMAP_H
#define QUCS_NODEMAP_H

#include "qt_compat.h"
#include "conductor.h"
#include "node.h"
#include <set>

template<class G>
struct graph_traits{};

class Node;
class NodeMap;
class NetList;
// describe the conductor graph.
// Conductors are the vertices.
// a connected component in this graph is a Net
template<class M>
class ConnectedComponents;

class NodeMap {
public:
  	typedef std::pair<int, int> key_type;
private:
	class NodeCompare{
	public:
		using is_transparent = void;
		bool operator()(Node const* n1, Node const* n2) const{
			return c(n1->position(), n2->position());
		}
		bool operator()(Node const* n1, key_type const& n2) const{
			return c(n1->position(), n2);
		}
		bool operator()(key_type const& n1, Node const* n2) const{
			return c(n1, n2->position());
		}
//		bool operator()(key_type const& n1, key_type const& n2) const{ untested();
//			return c(n1, n2);
//		}
	private:
		std::less< std::pair<int, int> > c;
	};
public:
	// need *set*, because map splits key from payload.
	// using a set of *pointers* because nodes must be mutable.
	// adds another indirection, but it is not exposed.
  	typedef std::set<Node*, NodeCompare > container_type;
	typedef container_type::iterator iterator;
	typedef container_type::const_iterator const_iterator;
private:
	// NodeList(NodeList const& x) : _model(x._model){unreachable();}
	NodeMap(NodeMap const& x) = delete;
public:
	explicit NodeMap(NetList& n);
	~NodeMap();

public:
// 	iterator begin(){return _nodes.begin();}
// 	iterator end(){return _nodes.end();}
// 	const_iterator begin() const{return _nodes.begin();}
// 	const_iterator end() const{return _nodes.end();}

	void clear(){ return _nodes.clear(); }
	size_t size() const{ return _nodes.size(); }

public: //obsolete interface
// 	void append(Node* n){
// 		return _nodes.push_back(n);
// 	}
#if 1
	int erase(Node* tt);
#endif

public: // friend Symbol?
	void addEdge(Conductor* a, Conductor* b);
	void postRemoveEdge(Conductor* a, Conductor* b);

public:
	Node* find_at(int x, int y);
	Node* find_at(key_type);
	Node& at(int x, int y);
	Node& new_at(int x, int y); // same as at. but don't search

public: // net access
	Net* newNet();
	void delNet(Net*);

	void registerVertex(Conductor*);
	void deregisterVertex(Conductor*);

private:
	ConnectedComponents<NodeMap>* new_ccs();

private:
	NetList& _nets;
	container_type _nodes;
private:
	ConnectedComponents<NodeMap>* _cc;
};


#endif
