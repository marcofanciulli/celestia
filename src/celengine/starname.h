//
// C++ Interface: starname
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _STARNAME_H_
#define _STARNAME_H_

#include <celengine/name.h>
#include <celengine/star.h>


class StarNameDatabase: public NameDatabase<Star>
{
 public:
    StarNameDatabase() {};


    uint32 findCatalogNumberByName(const std::string&) const;

    static StarNameDatabase* readNames(std::istream&);
};

#endif // _STARNAME_H_
