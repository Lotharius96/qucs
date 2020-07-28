// 2018 Felix
// part of QUCS, GPLv3+
//

#ifndef QUCS_SCHEMATIC_SYMBOL_H
#define QUCS_SCHEMATIC_SYMBOL_H

#include "symbol.h"

class PaintingList;
class WireList;
class NodeMap;
class DiagramList;
class PaintingList;
class ComponentList;

class SchematicSymbol : public Symbol{
private: // hide. don't mess with this.
	SchematicSymbol(SchematicSymbol const&) = delete;
public:
	explicit SchematicSymbol();
	~SchematicSymbol();

private: // Symbol
  virtual void setPort(unsigned, Node*){ incomplete(); }

protected:
public:
  SchematicModel* subckt() {return _subckt;}
  SchematicModel const* subckt() const {return _subckt;}

public:
	WireList const& wires() const;
	NodeMap const& nodes() const;
	DiagramList const& diagrams() const;
	PaintingList const& paintings() const;
	ComponentList const& components() const; // possibly "devices". lets see.

public:
	virtual std::string getParameter(std::string const&) const = 0;
	virtual void setParameter(std::string const&, std::string const&){ unreachable(); }

private: // SchematicSymbol
//   bool portExists(unsigned) const override;
//   QString portName(unsigned) const override;

private:
  virtual Element* clone()const{
	  unreachable();
	  return nullptr; // new SchematicSymbol(*this);
  }

public: //legacy hack
	PaintingList& symbolPaintings();
	PaintingList const& symbolPaintings() const;

private:
	PaintingList* _paint; // BUG
};

#endif
