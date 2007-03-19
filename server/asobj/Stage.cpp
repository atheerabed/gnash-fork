// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Stage.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "smart_ptr.h" // for boost intrusive_ptr
#include "builtin_function.h" // need builtin_function
#include "VM.h"

namespace gnash {

as_value stage_addlistener(const fn_call& fn);
as_value stage_removelistener(const fn_call& fn);
as_value stage_ctor(const fn_call& fn);

static void
attachStageInterface(as_object& o)
{
	if ( VM::get().getSWFVersion() > 5 )
	{
		o.init_member("addListener", new builtin_function(stage_addlistener));
		o.init_member("removeListener", new builtin_function(stage_removelistener));
	}
}

static as_object*
getStageInterface()
{
	static boost::intrusive_ptr<as_object> o;
	if ( ! o )
	{
		o = new as_object();
		attachStageInterface(*o);
	}
	return o.get();
}

class stage_as_object: public as_object
{

public:

	stage_as_object()
		:
		as_object(getStageInterface())
	{}

	// override from as_object ?
	//const char* get_text_value() const { return "Stage"; }

	// override from as_object ?
	//double get_numeric_value() const { return 0; }
};

as_value stage_addlistener(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
    return as_value();
}
as_value stage_removelistener(const fn_call& /*fn*/) {
    log_warning("%s: unimplemented \n", __FUNCTION__);
    return as_value();
}

as_value
stage_ctor(const fn_call& /* fn */)
{
	boost::intrusive_ptr<as_object> obj = new stage_as_object;
	
	return as_value(obj.get()); // will keep alive
}

// extern (used by Global.cpp)
void stage_class_init(as_object& global)
{

	static boost::intrusive_ptr<as_object> obj = new as_object();
	attachStageInterface(*obj);
	global.init_member("Stage", obj.get());

}


} // end of gnash namespace

