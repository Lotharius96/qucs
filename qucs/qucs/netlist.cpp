/***************************************************************************
    copyright            : (C) 2020 Felix Salfelder
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "netlist.h"
#include "net.h"

Net* NetList::newNet()
{
	unsigned idx;
	if(_g.empty()){
		idx = _l.size();
		_l.push_back(new Net());
		trace4("newNet", _l.back(), idx, _l.size(), this);
		_l[idx]->setPos(idx);
	}else{
		idx = _g.top();
		_g.pop();
		assert(_l[idx]->size() == -1);
		_l[idx]->setSize(0);
		assert(_l[idx]->getPos() == idx);
	}

	assert(_l[idx]->size() == 0);
	assert(_l[idx]);
	_l[idx]->setLabel("_net" + std::to_string(idx));
	return _l[idx];
}

void NetList::delNet(Net* n)
{
	trace1("delNet", n);
	assert(n);
	trace1("delNet", n->size());
	assert(n->size() == 0);
	n->setSize(-1);

	size_t idx = n->getPos();
	trace3("delNet", idx, _l.size(), this);
	assert(n==_l[idx]);
	_g.push(idx);
}
