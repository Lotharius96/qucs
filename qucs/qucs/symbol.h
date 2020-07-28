/***************************************************************************
                                 symbol.h
                                -----------
    copyright            : (C) 2016, 2019 Felix Salfelder
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** \file symbol.h
  * \brief Defines symbols for components, to be used in schematics.
  *
  * a symbol mostly takes care of ports, parameters and connectivity.
  * (this is under construction)
  *
  */
#ifndef QUCS_SYMBOL_H
#define QUCS_SYMBOL_H

#include "element.h"
#include "exception.h"

/** \class Symbol
  * \brief Superclass of circuit components
  *     (basically a Component,
  *     but "Component" already exists.
  *     It should be an implemenation only. maybe rename later.)
  */

class QPainter;
//class Schematic;
class NodeMap;

// a component symbol. not necessarily one of the legacy components
class Symbol : public Element{
public: // construct
  explicit Symbol();
  virtual ~Symbol();

private: // Element
	void paint(ViewPainter*) const{unreachable();}	

public: // interface
  // what is this?
  // some kind of "init"??!
  virtual void recreate(); // SchematicModel const& ctx);

  virtual std::string /* const& */ type() const{
	  return _type;
  }
  void setType(std::string const& x){
	  _type = x;
  }
  virtual void build() {} // what does it do?
  virtual unsigned paramCount()const {return 0;}

public:
  SchematicModel const* scope() const;

protected: // needed in netlister
  virtual SchematicModel* scope();

public: // Parameters
  virtual void setParameter(QString const&, QString const&){ throw ExceptionCantFind();}
  virtual void setParameter(unsigned, QString const&){ throw ExceptionCantFind();}
  virtual QString getParameter(unsigned){ throw ExceptionCantFind(); }

  // yikes there's param{Name,Value} already..
  virtual std::string paramValue(unsigned i) const;
  virtual std::string paramName(unsigned i) const;

public: // non-virtual (on purpose)
  QString const& netLabel(unsigned i) const;

public: // Node stuff
  virtual Node* connectNode(unsigned idx, NodeMap&);
  virtual Node* disconnectNode(unsigned idx, NodeMap&);

public: // Port access
  virtual std::pair<int, int> const& portPosition(unsigned) const;
  QString const& portValue(unsigned) const;
  // TODO: rethink Port/Node semantics
  virtual unsigned numPorts() const = 0;
  Port const& port(unsigned) const;
  virtual bool portExists(unsigned) const{ return false; }
  virtual QString portName(unsigned) const{return "invalid"; }

  SchematicModel const* subckt() const{ return _subckt; }
  SchematicModel* subckt(){ return _subckt; }
  void new_subckt();

private: // internal port access
  virtual Port& port(unsigned){ unreachable(); return *new Port(0,0);}

public: // graphics
        // hmm, maybe just dispatch a gfx object.
		  // leave it like that, for now.
  virtual unsigned height()const {return 0;}
  virtual unsigned width()const {return 0;}
//  virtual void draw(QPainter&)const=0;
  //...  more to come
protected: // maybe not here. but need to rebase MultiViewComponent to ScktProto first.
  SchematicModel* _subckt; // stuff contained in this symbol.
                           // such as subckt components. meta data or symbol gfx
private:
  std::string _type;
}; // symbol




#endif
