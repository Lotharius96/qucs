// Felix 2018, GPLv3+
// Part of QUCS

#include "qucsdoc.h"
#include "schematic_model.h"
#include "schematic_lang.h"
#include "schematic_symbol.h"
#include "schematic_model.h" /// hmm
#include "globals.h"
#include "wire.h"
#include "command.h"
#include "paintings/painting.h"
#include "diagram.h" // BUG
#include "subcircuit.h"

#ifdef DO_TRACE
#include <typeinfo>
#endif

bool PaintingList::load(QTextStream& str)
{
	auto stream=&str;
	auto List=this;
  incomplete();
# if 1
  Painting *p=0;
  QString Line, cstr;
  while(!stream->atEnd()) {
    Line = stream->readLine();
    if(Line.at(0) == '<') if(Line.at(1) == '/') return true;

    Line = Line.trimmed();
    if(Line.isEmpty()) continue;
    if( (Line.at(0) != '<') || (Line.at(Line.length()-1) != '>')) { untested();
		 incomplete();
		 // QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Format Error:\nWrong 'painting' line delimiter!"));
      return false;
    }
    Line = Line.mid(1, Line.length()-2);  // cut off start and end character

    cstr = Line.section(' ',0,0);    // painting type
    qDebug() << cstr;
    if(Painting const* pp=painting_dispatcher[cstr.toStdString()]){
      p=prechecked_cast<Painting*>(pp->clone());
      assert(p);
    }else{ untested();
		 // incomplete();
      // QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Format Error:\nUnknown painting " + cstr));
      return false;
    }

    if(!p->load(Line)) { untested();
		 incomplete();
      // QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Format Error:\nWrong 'painting' line format!"));
      delete p;
      return false;
    }
	 qDebug() << "got painting" << cstr << p->name();
    List->append(p);
  }

  incomplete();
  // QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Format Error:\n'Painting' field is not closed!"));
  return false;
#endif
}

namespace{

class LegacySchematicLanguage : public SchematicLanguage {
public:
	LegacySchematicLanguage() : SchematicLanguage(){
	}
private: // stuff saved from schematic_file.cpp
	Diagram* loadDiagram(QString const& Line, DocumentStream& /*, DiagramList *List */) const;
private: // stuff from component.cc
	void loadProperties(QTextStream& stream, SchematicSymbol& m) const;
	Component* parseComponentObsoleteCallback(const QString& _s, Component* c) const;
	Command* loadCommand(const QString& _s, Command* c) const;
	Element* loadElement(const QString& _s, Element* e) const;
	Element* getComponentFromName(QString& Line) const;
private: // overrides
	// BUG: need parseItem
	void parse(DocumentStream& stream, SchematicSymbol& s) const;

private:
	void printSymbol(Symbol const*, stream_t&) const;
}defaultSchematicLanguage_;
static Dispatcher<DocumentLanguage>::INSTALL
    p(&doclang_dispatcher, "leg_sch", &defaultSchematicLanguage_);
// ------------------------------------------------------
// // "parseElement"?
Element* LegacySchematicLanguage::loadElement(const QString& _s, Element* e) const
{
	if(Command* c=dynamic_cast<Command*>(e)){
		c = loadCommand(_s, c);
		// incomplete();
	}else if(Component* c=dynamic_cast<Component*>(e)){
		// legacy components
		// will not work non-qucs-.sch languages
		incomplete();
		c = parseComponentObsoleteCallback(_s, c);

		QString cstr = c->name();   // is perhaps changed in "recreate" (e.g. subcircuit)
		int x = c->tx;
		int y = c->ty;
		// c->setSchematic (p);
	//	s->recreate(); // half-expand subcircuits? why not "precalc"?
	//	               // why not in constructor? it needs parameters.
		c->setLabel(cstr);
		c->tx = x;  c->ty = y;
	}else{ untested();
		incomplete();
		return e;
	}
	return nullptr;
}

static std::list<Element*> implicit_hack;

static bool obsolete_load(Wire* w, const QString& sc)
{
	QString s(sc);
	trace1("obsolete_load", s);
	bool ok;

	if(s.at(0) != '<'){
		throw ExceptionCantParse();
	}else if(s.at(s.length()-1) != '>'){
		throw ExceptionCantParse();
	}
	s = s.mid(1, s.length()-2);   // cut off start and end character

	QString n;
	n  = s.section(' ',0,0);    // x1
	w->x1__() = n.toInt(&ok);
	if(!ok) return false; // BUG: throw

	n  = s.section(' ',1,1);    // y1
	w->y1__() = n.toInt(&ok);
	if(!ok) return false; // BUG: throw

	n  = s.section(' ',2,2);    // x2
	w->x2__() = n.toInt(&ok);
	if(!ok) return false; // BUG: throw

	n  = s.section(' ',3,3);    // y2
	w->y2__() = n.toInt(&ok);
	if(!ok) return false; // BUG: throw

	n = s.section('"',1,1);
	trace1("parse node label", n);
	if(!n.isEmpty()) {
		Symbol* sym=w;
		// there is a label hidden in this wire. go get it.
		//
		// a label has a position and is connected to a node.
		// nodelabel #(.$xposition(x), .$yposition(y)) name(node);
		// name may be used to identify the net connected to node in a flat netlist.
		auto nx = s.section(' ',5,5);
		auto ny = s.section(' ',6,6);
		auto delta = s.section(' ',7,7);
		auto sth = s.section('"',3,3);

		// nl->setLabel(n, s.section('"',3,3), delta, nx, ny);  // Wire Label

		// not sure what the parameters mean, need to compute label position and
		// node position from it.
		auto nn = s.section('"',3,3);
		trace7("hack push", s, sth, delta, nx, ny, nn, n);
		sym->setParameter("netname", n); // what is this?
		sym->setParameter("delta", delta); // what is this?
		sym->setParameter("nx", nx); // not the node position, maybe the label position?
		sym->setParameter("ny", ny); // not the node position, maybe the label position?
	}else{
	}

	return true;
}

// BUG: this is schematicFormat
void LegacySchematicLanguage::parse(DocumentStream& stream, SchematicSymbol& s) const
{
	assert(!implicit_hack.size());
	QString Line;

	// mode: a throwback to the legacy format:
	//       connect legacy "parsers".
	// this is not needed in a proper SchematicLanguage
	char mode='\0';
	while(!stream.atEnd()) {
		Line = stream.readLine();
		Line = Line.trimmed();
		if(Line.size()<2){ untested();
		}else if(Line.at(0) == '<'
				&& Line.at(1) == '/'){
			qDebug() << "endtag?" << Line;
		}else if(Line.isEmpty()){ untested();
		}else if(Line == "<Components>") {
			mode='C';
		}else if(Line == "<Symbol>") {
			mode='S';
		}else if(Line == "<Wires>") {
			mode='W';
		}else if(Line == "<Diagrams>") {
			mode='D';
		}else if(Line == "<Properties>") {
			mode='Q';
		}else if(Line == "<Paintings>") {
			mode='P';
		}else{

			/// \todo enable user to load partial schematic, skip unknown components
			Element*c=nullptr;
			if(mode=='C'){
				c = getComponentFromName(Line);
				c->setOwner(&s);
				if(Symbol* sym=dynamic_cast<Symbol*>(c) ){
					//always do this?

//					assert(s.scope());
					// what are those?!
					sym->recreate(); // re? create symbol gfx and random other things. needs owner
					sym->build(); // what's this?!
				}else{
				}
			}else if(mode=='S'){
				incomplete();
				try{
					qDebug() << "symbol Paintings";
					s.symbolPaintings().load(stream);
					c = nullptr;
				}catch(...){ untested();
					incomplete();
				}
			}else if(mode=='W'){
				qDebug() << "wire parse?" << Line;
				// (Node*)4 =  move all ports (later on)
				// Wire* w = new Wire(0,0,0,0, (Node*)4,(Node*)4); // this is entirely nuts.
				Wire* w = new Wire();
				w->setOwner(&s);
				incomplete(); // qt5 branch...
				bool err = obsolete_load(w, Line);
				if(!err){ untested();
					qDebug() << "ERROR" << Line;
					delete(w);
				}else{
					c = w;
				}
			}else if(mode=='D'){
				qDebug() << "diagram parse?" << Line;

				Diagram* d=loadDiagram(Line, stream);
				if(d){
					c = d;
					c->setOwner(&s);
				}else{ untested();
					incomplete();
				}

			}else if(mode=='Q'){

			}else{
				qDebug() << "LSL::parse" <<  Line;
				incomplete();
			}

			assert(s.subckt());
			if(c){
				trace2("pushing back", c->label(), typeid(*c).name());
				Element const* cc = c;
				assert(cc->owner() == &s);
				s.subckt()->pushBack(c);
			}else{
			}

		}
	}


	/// some components are implicit, collect them here
	assert(s.subckt());
	while(implicit_hack.size()){ untested();
		auto cc = implicit_hack.front();
		cc->setOwner(&s);
		implicit_hack.pop_front();
		s.subckt()->pushBack(cc);
	}
}

Diagram* LegacySchematicLanguage::loadDiagram(QString const& line_in,
		DocumentStream& stream /*, DiagramList *List */)const
{
	Diagram *d;
	QString Line=line_in;
	QString cstr;
	if(!stream.atEnd()) {
		qDebug() << "diagram?" << Line;
		if(Line.at(0) == '<') if(Line.at(1) == '/') return nullptr;
		Line = Line.trimmed();
		if(Line.isEmpty()){ untested();
			return nullptr;
		}else{
		}
		cstr = Line.section(' ',0,0);    // diagram type
		std::string what=cstr.toStdString();

		if(auto x=diagram_dispatcher[what.c_str()+1]){
			d=prechecked_cast<Diagram*>(x->clone());
			assert(d);
			qDebug() << "gotit" << what.c_str();
		}else{ untested();
			incomplete();
			// throw ...
			return nullptr;
		}

		if(!d->load(Line, stream)) { untested();
			incomplete();
			delete d;
			return nullptr;
		}else{
		}
		return d;
	}

	return nullptr;
}
//
//
// -------------------------------------------------------
// was: void Schematic::saveComponent(QTextStream& s, Component const* c) const
void LegacySchematicLanguage::printSymbol(Symbol const* sym, stream_t& s) const
{
	Component const* c=dynamic_cast<Component const*>(sym);
	if(!c){ untested();
		incomplete();
		return;
	}else{
	}
	s << "  <";
	if(dynamic_cast<Subcircuit const*>(c)){
		// print "Sub" instead of type.
		s << "Sub " << c->label();
	}else{
		s << c->obsolete_model_hack(); // c->type()

		s << " ";
		if(c->name().isEmpty()){ untested();
			s << "*";
		}else{
			s << c->name(); // label??
		}
	}
	s << " ";

	int i=0;
	if(!c->showName){
		i = 4;
	}
	i |= c->isActive;
	s << QString::number(i);
	s << " "+QString::number(c->cx())+" "+QString::number(c->cy());
	s << " "+QString::number(c->tx)+" "+QString::number(c->ty);
	s << " ";
	if(c->mirroredX){
		s << "1";
	}else{
		s << "0";
	}
	s << " " << QString::number(c->rotated);

	// write all properties
	// FIXME: ask component for properties, not for dictionary
	Component* cc=const_cast<Component*>(c); // BUGBUGBUGBUG
	// cannot access Props without this hack
	for(Property *p1 = cc->Props.first(); p1 != 0; p1 = cc->Props.next()) {
		if(p1->Description.isEmpty()){
			s << " \""+p1->Name+"="+p1->Value+"\"";   // e.g. for equations
		}else{
			s << " \""+p1->Value+"\"";
		}
		s << " ";
		if(p1->display){
			s << "1";
		}else{
			s << "0";
		}
	}

	s << ">";
} // printSymbol

Command* LegacySchematicLanguage::loadCommand(const QString& _s, Command* c) const
{
	bool ok;
	int  ttx, tty, tmp;
	QString s = _s;

	if(s.at(0) != '<'){ untested();
		return NULL;
	}else if(s.at(s.length()-1) != '>'){ untested();
		return NULL;
	}
	s = s.mid(1, s.length()-2);   // cut off start and end character

	QString label=s.section(' ',1,1);
	trace1("NAME", label);
	c->setName(label);

	QString n;
	n  = s.section(' ',2,2);      // isActive
	tmp = n.toInt(&ok);
	if(!ok){ untested();
		return NULL;
	}
	c->isActive = tmp & 3;

	if(tmp & 4){ untested();
		c->showName = false;
	}else{
		// use default, e.g. never show name for GND (bug?)
	}

	n  = s.section(' ',3,3);    // cx
	int cx = n.toInt(&ok);
	if(!ok) return NULL;

	n  = s.section(' ',4,4);    // cy
	c->setCenter(cx, n.toInt(&ok));
	if(!ok) return NULL;

	n  = s.section(' ',5,5);    // tx
	ttx = n.toInt(&ok);
	if(!ok) return NULL;

	n  = s.section(' ',6,6);    // ty
	tty = n.toInt(&ok);
	if(!ok) return NULL;

	assert(c);

	c->tx = ttx;
	c->ty = tty; // restore text position (was changed by rotate/mirror)

	unsigned int z=0, counts = s.count('"');
	// FIXME. use c->paramCount()

	/// BUG FIXME. dont use Component parameter dictionary.
	for(; tmp<=(int)counts/2; tmp++)
		c->Props.append(new Property("p", "", true, " "));

	// load all properties
	Property *p1;
	qDebug() << "load command props" << s;
	for(p1 = c->Props.first(); p1 != 0; p1 = c->Props.next()) {
		qDebug() << "load command props" << z;
		z++;
		n = s.section('"',z,z);    // property value
		z++;
		//qDebug() << "LOAD: " << p1->Description;

		// not all properties have to be mentioned (backward compatible)
		if(z > counts) {
			if(p1->Description.isEmpty()){ untested();
				c->Props.remove();    // remove if allocated in vain
			}

			return c;
		}

		p1->Value = n;

		n  = s.section('"',z,z);    // display
		p1->display = (n.at(1) == '1');
	}

	return c;
}

// BUG raise exceptions if something goes wrong.
Component* LegacySchematicLanguage::parseComponentObsoleteCallback(const QString& _s, Component* c) const
{
	qDebug() << "parseComponentObsoleteCallback" << _s;
	bool ok;
	int  ttx, tty, tmp;
	QString s = _s;

	if(s.at(0) != '<'){ untested();
		return NULL;
	}else if(s.at(s.length()-1) != '>'){ untested();
		return NULL;
	}
	s = s.mid(1, s.length()-2);   // cut off start and end character

	QString label=s.section(' ',1,1);
	c->obsolete_name_override_hack(label); //??
	trace1("loadComp", label);
	c->setLabel(label);

	QString n;
	n  = s.section(' ',2,2);      // isActive
	tmp = n.toInt(&ok);
	if(!ok){ untested();
		return NULL;
	}
	c->isActive = tmp & 3;

	if(tmp & 4){ untested();
		c->showName = false;
	}else{
		// use default, e.g. never show name for GND (bug?)
	}

	n  = s.section(' ',3,3);    // cx
	int cx=n.toInt(&ok);
	qDebug() << "cx" << cx;
	c->obsolete_set("cx", cx);
	if(!ok) return NULL;

	n  = s.section(' ',4,4);    // cy
	c->obsolete_set("cy", n.toInt(&ok));
	if(!ok) return NULL;

	n  = s.section(' ',5,5);    // tx
	ttx = n.toInt(&ok);
	if(!ok) return NULL;

	n  = s.section(' ',6,6);    // ty
	tty = n.toInt(&ok);
	if(!ok) return NULL;

	assert(c);
	assert(c->obsolete_model_hack().at(0) != '.');

	{

		n  = s.section(' ',7,7);    // mirroredX
		if(n.toInt(&ok) == 1){
			c->mirrorX();
		}
		if(!ok) return NULL;

		n  = s.section(' ',8,8);    // rotated
		tmp = n.toInt(&ok);
		if(!ok) return NULL;
		if(c->rotated > tmp)  // neccessary because of historical flaw in ...
			tmp += 4;        // ... components like "volt_dc"
		for(int z=c->rotated; z<tmp; z++){
			c->rotate();
		}
	}

	c->tx = ttx;
	c->ty = tty; // restore text position (was changed by rotate/mirror)

	QString Model = c->obsolete_model_hack(); // BUG: don't use names

	unsigned int z=0, counts = s.count('"');
	// FIXME. use c->paramCount()
	if(Model == "Sub"){
		tmp = 2;   // first property (File) already exists
	}else if(Model == "Lib"){ untested();
		tmp = 3;
	}else if(Model == "EDD"){ untested();
		tmp = 5;
	}else if(Model == "RFEDD"){ untested();
		tmp = 8;
	}else if(Model == "VHDL"){ untested();
		tmp = 2;
	}else if(Model == "MUTX"){ untested();
		tmp = 5; // number of properties for the default MUTX (2 inductors)
	}else{
		// "+1" because "counts" could be zero
		tmp = counts + 1;
	}

	/// BUG FIXME. dont use Component parameter dictionary.
	for(; tmp<=(int)counts/2; tmp++){ untested();
		c->Props.append(new Property("p", "", true, " "));
	}

	// set parameters.
	Property *p1;
	unsigned position=0;
	for(p1 = c->Props.first(); p1 != 0; p1 = c->Props.next()) {
		z++;
		n = s.section('"',z,z);    // property value. gaah parse over and over again?
		z++;
		//qDebug() << "LOAD: " << p1->Description;

		// not all properties have to be mentioned (backward compatible)
		if(z > counts) {
			if(p1->Description.isEmpty()){ untested();
				c->Props.remove();    // remove if allocated in vain
			}else{
			}

			if(Model == "Diode") { // BUG: don't use names
				if(counts < 56) {  // backward compatible
					counts >>= 1;
					p1 = c->Props.at(counts-1);
					for(; p1 != 0; p1 = c->Props.current()) { untested();
						if(counts-- < 19){ untested();
							break;
						}

						n = c->Props.prev()->Value;
						p1->Value = n;
					}

					p1 = c->Props.at(17);
					p1->Value = c->Props.at(11)->Value;
					c->Props.current()->Value = "0";
				}
			}else if(Model == "AND" || Model == "NAND" || Model == "NOR" ||
					Model == "OR" ||  Model == "XNOR"|| Model == "XOR") { untested();
				if(counts < 10) {   // backward compatible
					counts >>= 1;
					p1 = c->Props.at(counts);
					for(; p1 != 0; p1 = c->Props.current()) { untested();
						if(counts-- < 4)
							break;
						n = c->Props.prev()->Value;
						p1->Value = n;
					}
					c->Props.current()->Value = "10";
				}
			}else if(Model == "Buf" || Model == "Inv") { untested();
				if(counts < 8) {   // backward compatible
					counts >>= 1;
					p1 = c->Props.at(counts);
					for(; p1 != 0; p1 = c->Props.current()) { untested();
						if(counts-- < 3)
							break;
						n = c->Props.prev()->Value;
						p1->Value = n;
					}
					c->Props.current()->Value = "10";
				}
			}else{
			}

			return c;
		}else{
			// z <= counts
		}

		// for equations
		qDebug() << "Model" << Model;
#if 1
		if(Model != "EDD" && Model != "RFEDD" && Model != "RFEDD2P")
			if(p1->Description.isEmpty()) {  // unknown number of properties ?
				p1->Name = n.section('=',0,0);
				n = n.section('=',1);
				// allocate memory for a new property (e.g. for equations)
				if(c->Props.count() < (counts>>1)) {
					c->Props.insert(z >> 1, new Property("y", "1", true));
					c->Props.prev();
				}
			}
#endif
		if(z == 6)  if(counts == 6)     // backward compatible
			if(Model == "R") {
				c->Props.getLast()->Value = n;
				return c;
			}
		p1->Value = n; // TODO: remove

		Symbol* sym = c;
		sym->setParameter(position++, n); // BUG: also do for non-Components.

		n  = s.section('"',z,z);    // display
		p1->display = (n.at(1) == '1');
	}

	trace1("done legacy load", c->label());
	return c;
}

Element* LegacySchematicLanguage::getComponentFromName(QString& Line) const
{
	qDebug() << "component" << Line;
	Element *e = 0;

	Line = Line.trimmed();
	if(Line.at(0) != '<') { untested();
		throw "notyet_exception"
			"Format Error:\nWrong line start!";
	}

	QString cstr = Line.section (' ',0,0); // component type
	cstr.remove (0,1);    // remove leading "<"

	// TODO: get rid of the exceptional cases.
	if (cstr == "Lib"){ untested();
		incomplete();
		// c = new LibComp ();
	}else if (cstr == "Eqn"){
		incomplete();
		// c = new Equation ();
	}else if (cstr == "SPICE"){ untested();
		incomplete();
		// c = new SpiceFile();
	}else if (cstr.left (6) == "SPfile" && cstr != "SPfile"){ untested();
		incomplete();
		// backward compatible
		//c = new SParamFile ();
		//c->Props.getLast()->Value = cstr.mid (6);
	}

	// fetch proto from dictionary.
	Element const* s=symbol_dispatcher[cstr.toStdString()];

	if(Component const* sc=dynamic_cast<Component const*>(s)){
		// legacy component
		Element* k=sc->clone(); // memory leak?
		e = prechecked_cast<Element*>(k);
	}else if(Command const* sc=dynamic_cast<Command const*>(s)){ untested();
		// legacy component
		Element* k = sc->clone(); // memory leak?
		e = prechecked_cast<Element*>(k);
	}else{
		e = command_dispatcher.clone(cstr.toStdString());
		// don't know what this is (yet);
		incomplete();
	}

	if(e) {
		incomplete();
		loadElement(Line, e);
	}else{ untested();
		qDebug() << "error with" << cstr;
		message(QucsWarningMsg,
			"Format Error:\nUnknown component!\n"
			"%1\n\n"
			"Do you want to load schematic anyway?\n"
			"Unknown components will be replaced \n"
			"by dummy subcircuit placeholders.");

		incomplete();
		// c = new Subcircuit();
		// // Hack: insert dummy File property before the first property
		// int pos1 = Line.indexOf('"');
		// QString filestr = QString("\"%1.sch\" 1 ").arg(cstr);
		// Line.insert(pos1,filestr);
	}


#if 0 // legacy cruft?
	// BUG: don't use schematic.
	if(Command* cmd=command(e)){ untested();
		p->loadCommand(Line, cmd);
	}else if(Component* c=component(e)){ untested();
		if(!p->parseComponentObsoleteCallback(Line, c)) { untested();
			QMessageBox::critical(0, QObject::tr("Error"),
					QObject::tr("Format Error:\nWrong 'component' line format!"));
			delete e;
			return 0;
		}else{ untested();
		}
		cstr = c->name();   // is perhaps changed in "recreate" (e.g. subcircuit)
		int x = c->tx, y = c->ty;
		c->setSchematic (p);
		c->recreate(0);
		c->obsolete_name_override_hack(cstr);
		c->tx = x;  c->ty = y;
	}
#endif

	return e;
}

// was Schematic::loadProperties
void LegacySchematicLanguage::loadProperties(QTextStream& s_in,
		SchematicSymbol& m) const
{ untested();
	auto stream=&s_in;
	bool ok = true;
	QString Line, cstr, nstr;
	while(!stream->atEnd()) { untested();
		Line = stream->readLine();
		if(Line.at(0) == '<') if(Line.at(1) == '/') return; //?!
		Line = Line.trimmed();
		if(Line.isEmpty()) continue;

		// move up.
		if(Line.size() < 2){ untested();
			throw "incomplete_exception1";
		}else if(Line.at(0) != '<') { untested();
			throw "incomplete_exception1";
		}else if(Line.at(Line.length()-1) != '>') { untested();
			throw "incomplete_exception2";
		}
		Line = Line.mid(1, Line.length()-2);   // cut off start and end character

		cstr = Line.section('=',0,0);    // property type
		nstr = Line.section('=',1,1);    // property value
		if(cstr == "View") { untested();
			incomplete(); // setParameter
		 	QString tmp;
			tmp=nstr.section(',',0,0).toInt(&ok);
			m.setParameter("ViewX1", tmp.toStdString());
			tmp=nstr.section(',',1,1).toInt(&ok);
			m.setParameter("ViewY1", tmp.toStdString());
			tmp=nstr.section(',',2,2).toInt(&ok);
			m.setParameter("ViewX2", tmp.toStdString());
			tmp=nstr.section(',',2,2).toInt(&ok);
			m.setParameter("ViewY2", tmp.toStdString());

		// 					Scale  = nstr.section(',',4,4).toDouble(&ok); if(ok) { untested();
		// 						tmpViewX1 = nstr.section(',',5,5).toInt(&ok); if(ok)
		// 							tmpViewY1 = nstr.section(',',6,6).toInt(&ok); }
		} else if(cstr == "Grid") { untested();
			m.setParameter("GridX", nstr.section(',',0,0).toStdString());
			m.setParameter("GridY", nstr.section(',',1,1).toStdString());
			m.setParameter("GridOn",nstr.section(',',2,2).toStdString());
		}else if(cstr == "DataSet"
		       ||cstr == "Script"
		       ||cstr == "FrameText0"
		       ||cstr == "FrameText1"
		       ||cstr == "FrameText2"
		       ||cstr == "FrameText3"
		       ||cstr == "showFrame"
		       ||cstr == "DataDisplay"
		       ||cstr == "OpenDisplay"){ untested();
			m.setParameter(cstr.toStdString(), nstr.toStdString());
		}
		// else if(cstr == "OpenDisplay") // bool SimOpenDpl
		// else if(cstr == "RunScript") // bool simRunScript
		// else if(cstr == "showFrame") // bool showFrame
		//else if(cstr == "FrameText0") misc::convert2Unicode(Frame_Text0 = nstr);
		//else if(cstr == "FrameText1") misc::convert2Unicode(Frame_Text1 = nstr);
		//else if(cstr == "FrameText2") misc::convert2Unicode(Frame_Text2 = nstr);
		//else if(cstr == "FrameText3") misc::convert2Unicode(Frame_Text3 = nstr);
		else { untested();
			throw "incomplete_exception" + cstr.toStdString();
		}
		if(!ok) { untested();
			incomplete();
					//QObject::tr("Format Error:\nNumber expected in property field!"));
			throw "something wrong";
		}else{ untested();
		}
	}

	throw "QObject::tr(\"Format Error:\n'Property' field is not closed!\")";
}

} // namespace
