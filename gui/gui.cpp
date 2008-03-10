// gui.cpp:  Top level GUI for flash player, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
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
#include "gnashconfig.h"
#endif

#include "sprite_instance.h"
#include "gui.h"
#include "render.h"  // debug
#include "render_handler.h"
#include "sound_handler.h"
#include "gnash.h" // for get_sound_handler
#include "movie_root.h"
#include "VM.h"

#ifdef GNASH_FPS_DEBUG
#include "tu_timer.h"
#endif

#include <cstdio>
#include <cstring>

#ifdef SKIP_RENDERING_IF_LATE
#include "WallClockTimer.h"
#endif

/// Define this to make sure each frame is fully rendered from ground up
/// even if no motion has been detected in the movie.
//#define FORCE_REDRAW 1

/// Define this to have updated regions enclosed in a red rectangle.
/// In the future, enabling this might actually use a runtime flag
/// as an additional conditional.
/// This has the side effect that all frames will be re-rendered completely
/// but in contrast to FORCE_REDRAW it won't re-render when no motion
/// has been detected in the movie (for example when the movie is stopped).
///
//#define ENABLE_REGION_UPDATES_DEBUGGING 1

/// Define this if you want to debug the *detection* of region updates only.
/// This will disable region updates for the backend (GUI+renderer) completely 
/// so that only the last region (red frame) will be visible. However, this 
/// slows down rendering as each frame is fully re-rendered. If you want to 
/// debug the GUI part, however (see if blitting the region works), then you 
/// probably won't define this.
#ifdef ENABLE_REGION_UPDATES_DEBUGGING 
//#define REGION_UPDATES_DEBUGGING_FULL_REDRAW 1
#endif 

#ifdef ENABLE_REGION_UPDATES_DEBUGGING
// a runtime check would make the { x; } block conditionally executed
#define IF_DEBUG_REGION_UPDATES(x) { x; }
#else
#define IF_DEBUG_REGION_UPDATES(x) 
#endif

// Define this to have gnash print the mouse pointer coordinates
// as the mouse moves. See also ENABLE_KEYBOARD_MOUSE_MOVEMENTS
// to have more control over mouse pointer.
//
//#define DEBUG_MOUSE_COORDINATES 1


namespace gnash {

Gui::Gui() :
    _loop(true),
    _xid(0),
    _width(1),
    _height(1),
    _xscale(1.0f),
    _yscale(1.0f),
    _depth(16),
    _interval(0),
    _renderer(NULL),
    _redraw_flag(true),
    _fullscreen(false),
    _mouseShown(true)
#ifdef GNASH_FPS_DEBUG
    ,fps_counter(0)
    ,fps_counter_total(0)
    ,fps_timer(0)
    ,fps_timer_interval(0.0)
#endif
#ifdef SKIP_RENDERING_IF_LATE
    ,estimatedDisplayTime(0) // milliseconds (will grow later..)
#endif // SKIP_RENDERING_IF_LATE
    ,_movieDef(0)
    ,_stage(0)
    ,_stopped(false)
    ,_started(false)
#ifdef ENABLE_KEYBOARD_MOUSE_MOVEMENTS 
    ,_xpointer(0)
    ,_ypointer(0)
    ,_keyboardMouseMovements(true) // TODO: base default on gnashrc or always false and provide menu item to toggle
    ,_keyboardMouseMovementsStep(1)
#endif
{
//    GNASH_REPORT_FUNCTION;
}

Gui::Gui(unsigned long xid, float scale, bool loop, unsigned int depth)
	:
    _loop(loop),
    _xid(xid),
    _width(1),
    _height(1),
    _xscale(scale),
    _yscale(scale),
    _depth(depth),
    _interval(0),
    _renderer(NULL),
    _redraw_flag(true),
    _fullscreen(false)
#ifdef GNASH_FPS_DEBUG
    ,fps_counter(0)    
    ,fps_counter_total(0)    
    ,fps_timer(0)
    ,fps_timer_interval(0.0)
#endif        
#ifdef SKIP_RENDERING_IF_LATE
    ,estimatedDisplayTime(0) // milliseconds (will grow later..)
#endif // SKIP_RENDERING_IF_LATE
    ,_movieDef(0)
    ,_stage(0)
    ,_stopped(false)
    ,_started(false)
#ifdef ENABLE_KEYBOARD_MOUSE_MOVEMENTS 
    ,_xpointer(0)
    ,_ypointer(0)
    ,_keyboardMouseMovements(true) // TODO: base default on gnashrc or always false and provide menu item to toggle
    ,_keyboardMouseMovementsStep(1)
#endif
{
}

Gui::~Gui()
{
//    GNASH_REPORT_FUNCTION;
    delete _renderer;
#ifdef GNASH_FPS_DEBUG
    printf("Total frame advances: %u\n", fps_counter_total);
#endif
}

void
Gui::setFullscreen()
{
    log_unimpl(_("Fullscreen not yet supported in this GUI"));
}

void
Gui::unsetFullscreen()
{
    log_unimpl(_("Fullscreen not yet supported in this GUI"));
}

void
Gui::showMouse(bool /* show */)
{
	static bool warned = false;
	if (!warned)
	{
    	log_unimpl(_("Mouse show/hide not yet supported in this GUI"));
    	warned = true;
   	}
}

void
Gui::toggleFullscreen()
{
	if (_fullscreen) {
		unsetFullscreen();
	}
	else {
		setFullscreen();
	} 
}

void
Gui::menu_restart()
{
//    GNASH_REPORT_FUNCTION;

	_stage->reset();
	_started = false;
	start();

	//_stage->restart();

}

void
Gui::resize_view(int width, int height)
{
//	GNASH_REPORT_FUNCTION;

	assert(width>0);
	assert(height>0);

	if ( VM::isInitialized() )
	{

		float swfwidth = _movieDef->get_width_pixels();
		float swfheight = _movieDef->get_height_pixels();

		if ( _stage && _started )
		{
			_stage->set_display_viewport(0, 0, width, height);
		}

		if ( _stage && _stage->isRescalingAllowed() )
		{
			// set new scale value
			_xscale = width / swfwidth;
			_yscale = height / swfheight;
			
			// always scale proportionally
			if (_xscale < _yscale) _yscale = _xscale;
			if (_yscale < _xscale) _xscale = _yscale;
			
			_renderer->set_scale(_xscale, _yscale);
		}

	}
	else
	{
		//log_debug(_("Resize request received while there's still"
		//	" no movie loaded, can't correctly set movie scale"));
	}
	
	// trigger redraw
	//_redraw_flag |= (_width!=width) || (_height!=height);
	_redraw_flag = true; // this fixes bug #21971

	// set new size ?
	_width = width;
	_height = height;
	_validbounds.setTo(0, 0, _width-1, _height-1);
	//log_debug(_("new size (in twips) is: %dx%d"), _width*20, _height*20); 
}

void
Gui::menu_quit()
{
//    GNASH_REPORT_FUNCTION;
    quit();
}

void
Gui::menu_play()
{
//    GNASH_REPORT_FUNCTION;
    //get_current_root()->set_play_state(gnash::sprite_instance::PLAY);
    play();
}

void
Gui::menu_pause()
{
//    GNASH_REPORT_FUNCTION;

    pause();
//    movie_root* m = get_current_root();
//    if (m->get_play_state() == gnash::sprite_instance::STOP) {
//      m->set_play_state(gnash::sprite_instance::PLAY);
//    } else {
//      m->set_play_state(gnash::sprite_instance::STOP);
//    }

}

void
Gui::menu_stop()
{
//    GNASH_REPORT_FUNCTION;
    //get_current_root()->set_play_state(gnash::sprite_instance::STOP);
    stop();
}

void
Gui::menu_step_forward()
{
//    GNASH_REPORT_FUNCTION;
	movie_root* m = get_current_root();
	m->goto_frame(m->get_current_frame()+1);
}

void
Gui::menu_step_backward()
{
//    GNASH_REPORT_FUNCTION;

	movie_root* m = get_current_root();
	m->goto_frame(m->get_current_frame()-1);
}

void
Gui::menu_jump_forward()
{
//    GNASH_REPORT_FUNCTION;

	movie_root* m = get_current_root();
	m->goto_frame(m->get_current_frame()+10);
}

void
Gui::menu_jump_backward()
{
//    GNASH_REPORT_FUNCTION;

	movie_root* m = get_current_root();
	m->goto_frame(m->get_current_frame()-10);
}

void
Gui::menu_toggle_sound()
{
//    GNASH_REPORT_FUNCTION;
    media::sound_handler* snd_handler = get_sound_handler();

    if (!snd_handler)
       return;

    if (snd_handler->is_muted()) {
       snd_handler->unmute();
    } else {
       snd_handler->mute();
    }
}


void
Gui::notify_mouse_moved(int x, int y) 
{
	movie_root* m = _stage;

    if ( ! _started ) return;

    if ( _stopped ) return;

#ifdef DEBUG_MOUSE_COORDINATES
	log_debug(_("mouse @ %d,%d"), x, y);
#endif
	if ( m->notify_mouse_moved(x, y) )
	{
		// any action triggered by the
		// event required screen refresh
		display(m);
	}
    
	character* activeEntity = m->getActiveEntityUnderPointer();
	if ( activeEntity )
	{
		if ( activeEntity->isSelectableTextField() )
		{
			setCursor(CURSOR_INPUT);
		}
		else
		{
			setCursor(CURSOR_HAND);
		}
	}
	else
	{
		setCursor(CURSOR_NORMAL);
	}

#ifdef ENABLE_KEYBOARD_MOUSE_MOVEMENTS
	_xpointer = x;
	_ypointer = y;
#endif


}

void
Gui::notify_mouse_clicked(bool mouse_pressed, int mask) 
{
	movie_root* m = _stage;
	assert(m);

    if ( ! _started ) return;

    if ( _stopped ) return;

	if ( m->notify_mouse_clicked(mouse_pressed, mask) )
	{
		// any action triggered by the
		// event required screen refresh
		display(m);
	}
}

void
Gui::refreshView()
{
    movie_root* m = _stage;

    if ( ! _started ) return;

    assert(m);
    _redraw_flag=true;
    display(m);
}


void
Gui::notify_key_event(gnash::key::code k, int modifier, bool pressed) 
{
	movie_root* m = _stage;

	/* Handle GUI shortcuts */
	if (pressed)
	{
		if (k == gnash::key::ESCAPE)
		{
			if (isFullscreen())
			{
				unsetFullscreen();
			}
		}
		
		if (modifier & gnash::key::MOD_CONTROL)
		{
			switch(k)
			{
				case gnash::key::r:
				case gnash::key::R:
					menu_restart();
					break;
				case gnash::key::p:
				case gnash::key::P:
					menu_pause();
					break;
				case gnash::key::l:
				case gnash::key::L:
					refreshView();
					break;
				case gnash::key::q:
				case gnash::key::Q:
				case gnash::key::w:
				case gnash::key::W:
					menu_quit();
					break;
				case gnash::key::f:
				case gnash::key::F:
					toggleFullscreen();
					break;
				case gnash::key::RIGHT_BRACKET:
					menu_step_forward();
					break;
				case gnash::key::LEFT_BRACKET:
					menu_step_backward();
					break;
				default:
					break;
			}

#ifdef ENABLE_KEYBOARD_MOUSE_MOVEMENTS
			if ( _keyboardMouseMovements )
			{
				int step = _keyboardMouseMovementsStep; 
				if (modifier & gnash::key::MOD_SHIFT) step*=5; // x5 if SHIFT is pressed
				switch(k)
				{
					case gnash::key::UP:
					{
						int newx = _xpointer;
						int newy = _ypointer-step;
						//log_debug("(theoretically) From %d,%d to %d,%d (step %d)", _xpointer, _ypointer, newx, newy, step);
						if ( newy < 0 ) newy=0;
						//log_debug("From %d,%d to %d,%d", _xpointer, _ypointer, newx, newy);
						notify_mouse_moved(newx, newy);
						break;
					}
					case gnash::key::DOWN:
					{
						int newx = _xpointer;
						int newy = _ypointer+step;
						//log_debug("(theoretically) From %d,%d to %d,%d (step %d)", _xpointer, _ypointer, newx, newy, step);
						if ( newy >= _height ) newy = _height-1;
						//log_debug("From %d,%d to %d,%d", _xpointer, _ypointer, newx, newy);
						notify_mouse_moved(newx, newy);
						break;
					}
					case gnash::key::LEFT:
					{
						int newx = _xpointer-step;
						int newy = _ypointer;
						//log_debug("(theoretically) From %d,%d to %d,%d (step %d)", _xpointer, _ypointer, newx, newy, step);
						if ( newx < 0 ) newx = 0;
						//log_debug("From %d,%d to %d,%d", _xpointer, _ypointer, newx, newy);
						notify_mouse_moved(newx, newy);
						break;
					}
					case gnash::key::RIGHT:
					{
						int newy = _ypointer;
						int newx = _xpointer+step;
						//log_debug("(theoretically) From %d,%d to %d,%d (step %d)", _xpointer, _ypointer, newx, newy, step);
						if ( newx >= _width ) newx = _width-1;
						//log_debug("From %d,%d to %d,%d", _xpointer, _ypointer, newx, newy);
						notify_mouse_moved(newx, newy);
						break;
					}
					default:
						break;
				}
			}
#endif // ENABLE_KEYBOARD_MOUSE_MOVEMENTS
		}
	}

    if ( ! _started ) return;

    if ( _stopped ) return;

	if ( m->notify_key_event(k, pressed) )
	{
		// any action triggered by the
		// event required screen refresh
		display(m);
	}

}

bool
Gui::display(movie_root* m)
{
    assert(m == _stage); // why taking this arg ??

    assert(_started);

	InvalidatedRanges changed_ranges;
	bool redraw_flag;

	// Should the frame be rendered completely, even if it did not change?
#ifdef FORCE_REDRAW
  redraw_flag = true;
#else	
	redraw_flag = _redraw_flag || want_redraw();
#endif	
	
	// reset class member if we do a redraw now
	if (redraw_flag) _redraw_flag=false;

	// Find out the surrounding frame of all characters which
	// have been updated. This just checks what region of the stage has changed
	// due to ActionScript code, the timeline or user events. The GUI can still
	// choose to render a different part of the stage. 
	//
	if (!redraw_flag) {
		
		// choose snapping ranges factor 
		changed_ranges.setSnapFactor(1.3f);  
			
		// Use multi ranges only when GUI/Renderer supports it
		// (Useless CPU overhead, otherwise)
		changed_ranges.setSingleMode(!want_multiple_regions());

		// scan through all sprites to compute invalidated bounds  
		m->add_invalidated_bounds(changed_ranges, false);
		
		// grow ranges by a 2 pixels to avoid anti-aliasing issues		
		changed_ranges.growBy(40.0f / _xscale);
		
		// optimize ranges
		changed_ranges.combine_ranges();
		
	}

	if (redraw_flag)     // TODO: Remove this and want_redraw to avoid confusion!?
	{
		changed_ranges.setWorld();
	}
  
	// Avoid drawing of stopped movies

	if ( ! changed_ranges.isNull() ) // use 'else'?
	{
		// Tell the GUI(!) that we only need to update this
		// region. Note the GUI can do whatever it wants with
		// this information. It may simply ignore the bounds
		// (which will normally lead into a complete redraw),
		// or it may extend or shrink the bounds as it likes. So,
		// by calling set_invalidated_bounds we have no guarantee
		// that only this part of the stage is rendered again.
#ifdef REGION_UPDATES_DEBUGGING_FULL_REDRAW
		// redraw the full screen so that only the
		// *new* invalidated region is visible
		// (helps debugging)
		InvalidatedRanges world_ranges;
		world_ranges.setWorld();
		setInvalidatedRegions(world_ranges);
#else
		setInvalidatedRegions(changed_ranges);
#endif

		// TODO: should this be called even if we're late ?
		beforeRendering();

		// Render the frame, if not late.
		// It's up to the GUI/renderer combination
		// to do any clipping, if desired.     
		m->display();
  
		// show invalidated region using a red rectangle
		// (Flash debug style)
		IF_DEBUG_REGION_UPDATES (
		if ( ! changed_ranges.isWorld() )
		{
		
			for (int rno=0; rno<changed_ranges.size(); rno++) {
			
				geometry::Range2d<float> bounds = changed_ranges.getRange(rno);

				point corners[4];
				float xmin = bounds.getMinX();
				float xmax = bounds.getMaxX();
				float ymin = bounds.getMinY();
				float ymax = bounds.getMaxY();
				
				corners[0].x = xmin;
				corners[0].y = ymin;
				corners[1].x = xmax;
				corners[1].y = ymin;
				corners[2].x = xmax;
				corners[2].y = ymax;
				corners[3].x = xmin;
				corners[3].y = ymax;
				gnash::render::draw_poly(&corners[0], 4,
					rgba(0,0,0,0), rgba(255,0,0,255), matrix::identity, false);
					
			}
		}
		);

		// show frame on screen
		renderBuffer();
   	
	};
  
	return true;
}

void
Gui::play()
{
    if ( ! _stopped ) return;

    _stopped = false;
    if ( ! _started ) start();
}

void
Gui::stop()
{
    if ( _stopped ) return;

    _stopped = true;
}

void
Gui::pause()
{
    if ( _stopped )
    {
        play();
    }
    else _stopped = true;
}

void
Gui::start()
{
    assert ( ! _started );
    if ( _stopped )
    {
        log_debug("Gui is in stop mode, won't start application");
        return;
    }

    std::auto_ptr<movie_instance> mr ( _movieDef->create_movie_instance() );
    mr->setVariables(_flashVars);

    _stage->setRootMovie( mr.release() ); // will construct the instance
    _stage->set_display_viewport(0, 0, _width, _height);

    bool background = true; // ??
    _stage->set_background_alpha(background ? 1.0f : 0.05f);


    _started = true;
}

bool
Gui::advanceMovie()
{
	if ( isStopped() ) return true;

    if ( ! _started ) start();
  
//	GNASH_REPORT_FUNCTION;

#ifdef SKIP_RENDERING_IF_LATE
	WallClockTimer advanceTimer;
#endif // SKIP_RENDERING_IF_LATE

	gnash::movie_root* m = _stage;
	
#ifdef GNASH_FPS_DEBUG
	fpsCounterTick(); // will be a no-op if fps_timer_interval is zero
#endif

// Define REVIEW_ALL_FRAMES to have *all* frames
// consequencially displaied. Useful for debugging.
//#define REVIEW_ALL_FRAMES 1

#ifndef REVIEW_ALL_FRAMES
	// Advance movie by one frame
	m->advance();
#else
	size_t cur_frame = m->getRootMovie()->get_current_frame();
	size_t tot_frames = m->getRootMovie()->get_frame_count();
	m->advance();
	m->get_movie_definition()->ensure_frame_loaded(tot_frames);
	m->goto_frame(cur_frame+1);
    	m->set_play_state(gnash::sprite_instance::PLAY);
	log_debug(_("Frame %d"), m->get_current_frame());
#endif


#ifdef SKIP_RENDERING_IF_LATE

	boost::uint32_t advanceTime = advanceTimer.elapsed(); // in milliseconds !

	boost::uint32_t timeSlot = _interval; // milliseconds between advance calls 

	if ( advanceTime+gui->estimatedDisplayTime < timeSlot )
	{
		advanceTimer.restart();
		display(m);
		boost::uint32_t displayTime = advanceTimer.elapsed();

		if ( displayTime > estimatedDisplayTime)
		{
			//log_debug("Display took %6.6g seconds over %6.6g available for each frame", displayTime, timeSlot);

			// Don't update estimatedDisplayTime if it's bigger then timeSlot*0.8
			if (  displayTime < timeSlot*0.8 )
			{
				// TODO: check for absurdly high values, like we can't set
				//       estimatedDisplayTime to a value higher then FPS, or
				//       we'll simply never display...
				estimatedDisplayTime = displayTime;
			}
		}
	}
	else
	{
		log_debug("We're unable to keep up with FPS speed: "
			"advanceTime was %u + estimatedDisplayTime (%u) "
			"== %u, over a timeSlot of %u",
			advanceTime, estimatedDisplayTime,
			advanceTime+estimatedDisplayTime, timeSlot);
		// TODO: increment a counter, we don't want to skip too many frames
	}
#else // ndef SKIP_RENDERING_IF_LATE

	display(m);

#endif // ndef SKIP_RENDERING_IF_LATE
	
	if ( ! loops() )
	{
		size_t curframe = m->get_current_frame(); // can be 0 on malformed SWF
		gnash::sprite_instance* si = m->getRootMovie();
		if (curframe + 1 >= si->get_frame_count())
		{
			quit(); 
		}
	}

	return true;
}

void
Gui::setCursor(gnash_cursor_type /*newcursor*/)
{
}

bool
Gui::want_redraw()
{
    return false;
}

float
Gui::getXScale()
{
    return _xscale;
}

float
Gui::getYScale()
{
    return _yscale;
}

bool
Gui::loops()
{
    return _loop;
}

void
Gui::setInvalidatedRegion(const rect& /*bounds*/)
{
}

void
Gui::setInvalidatedRegions(const InvalidatedRanges& ranges)
{
	// fallback to single regions
	geometry::Range2d<float> full = ranges.getFullArea();
	
	rect bounds;
	
	if (full.isFinite())
		bounds = rect(full.getMinX(), full.getMinY(), full.getMaxX(), full.getMaxY());
	else
	if (full.isWorld())
		bounds.set_world();
	
	setInvalidatedRegion(bounds);
}

std::auto_ptr<Gui::InfoTree>
Gui::getMovieInfo() const
{
    std::auto_ptr<InfoTree> tr;

    if ( ! VM::isInitialized() )
    {
        return tr;
    }

    tr.reset(new InfoTree());

    // Top nodes for the tree:
    // 1. VM information
    // 2. Movie information
    // 3. ...

    InfoTree::iterator_base topIter = tr->begin();
    InfoTree::iterator_base firstLevelIter;

    VM& vm = VM::get();

    std::ostringstream os;

    // VM top level
    os << "SWF" << vm.getSWFVersion();
    topIter = tr->insert(topIter, StringPair("VM", os.str()));

    // VM children
    movie_root& stage = vm.getRoot();
    boost::intrusive_ptr<movie_instance> level0 = stage.getRootMovie();
    movie_definition* def0 = level0->get_movie_definition();
    assert(def0);

    os.str("");
    os << "SWF " << def0->get_version();
    firstLevelIter = tr->append_child(topIter, StringPair("level0 SWF version", os.str()));
    firstLevelIter = tr->append_child(topIter, StringPair("level0 URL", def0->get_url()));


    // Print info about scripts state (enabled/disabled)
    topIter = tr->insert(topIter, StringPair("Stage scripts",
                stage.scriptsDisabled() ? " disabled" : "enabled"));

    // Print info about mouse entities
    topIter = tr->insert(topIter, StringPair("Mouse Entities", ""));

    const character* ch;
    ch = stage.getActiveEntityUnderPointer();
    if ( ch )
    {
	    std::stringstream ss;
	    ss << ch->getTarget() << " (" + typeName(*ch)
            << " - id:" << ch->get_id() << " depth:"
            << ch->get_depth();
    	firstLevelIter = tr->append_child(topIter, StringPair("Active mouse entity", ss.str()));
    }

    ch = stage.getEntityUnderPointer();
    if ( ch )
    {
	    std::stringstream ss;
	    ss << ch->getTarget() << " (" + typeName(*ch) <<
            " - id:" << ch->get_id() << " depth:" << ch->get_depth();
    	firstLevelIter = tr->append_child(topIter, StringPair("Topmost mouse entity", ss.str()));
    }

    ch = stage.getDraggingCharacter();
    if ( ch ) 
    {
	    std::stringstream ss;
	    ss << ch->getTarget() << " (" + typeName(*ch) <<
            " - id:" << ch->get_id() << " depth:" << ch->get_depth();
    	firstLevelIter = tr->append_child(topIter, StringPair("Dragging character: ", ss.str()));
    }

    // GC row
    topIter = tr->insert(topIter, StringPair("GC Statistics", ""));
    GC::CollectablesCount cc;
    GC::get().countCollectables(cc);
    
    const std::string lbl = "GC managed ";
    for (GC::CollectablesCount::iterator i=cc.begin(), e=cc.end(); i!=e; ++i)
    {
        const std::string& typ = i->first;
        std::ostringstream ss;
        ss << i->second;
        firstLevelIter = tr->append_child(topIter,
                            StringPair(lbl + typ, ss.str()));
    }

    tr->sort(firstLevelIter.begin(), firstLevelIter.end());

//    for (InfoTree::pre_order_iterator it = tr->begin(); it != tr->end(); it++)
//    {
//        int j = 0;
//        while (j++ < tr->depth(it)) std::cout << " ";
//        StringPair& p = *it;
//        std::cout << p.first << ": " << p.second << std::endl;
//    }

    return tr;
}

#ifdef GNASH_FPS_DEBUG
void 
Gui::fpsCounterTick()
{
  // GNASH_REPORT_FUNCTION;

  // increment this *before* the early return so that
  // frame count on exit is still valid
  ++fps_counter_total;

  if ( ! fps_timer_interval )
  {
	  return;
  }

  boost::uint64_t current_timer = tu_timer::get_ticks();

  // TODO: keep fps_timer_interval in milliseconds to avoid the multiplication
  //       at each fpsCounterTick call...
  boost::uint64_t interval_ms = (boost::uint64_t)(fps_timer_interval * 1000.0);

  if (fps_counter_total==1) {
    fps_timer = current_timer;
    fps_start_timer = current_timer;
  }
  
  ++fps_counter;
  
  if (current_timer - fps_timer >= interval_ms) {
  
    float secs = (current_timer - fps_timer) / 1000.0;
    float secs_total = (current_timer - fps_start_timer)/1000.0;
        
    float rate = fps_counter/secs;
    
    if (secs > 10000000) {
      // the timers are unsigned, so when the clock runs "backwards" it leads
      // to a very high difference value. In theory, this should never happen
      // with ticks, but it does on my machine (which may have a hw problem?).
      printf("Time glich detected, need to restart FPS counters, sorry...\n");
      
      fps_timer = current_timer;
      fps_start_timer = current_timer;
      fps_counter_total = 0;
      fps_counter = 0;
      return;
    } 
     
    // first FPS message?
    if (fps_timer == fps_start_timer) {     // they're ints, so we can compare
      fps_rate_min = rate;
      fps_rate_max = rate; 
    } else {
      fps_rate_min = fmin(fps_rate_min, rate);
      fps_rate_max = fmax(fps_rate_max, rate);
    }
    
    float avg = fps_counter_total / secs_total; 
  
    //log_debug("Effective frame rate: %0.2f fps", (float)(fps_counter/secs));
    printf("Effective frame rate: %0.2f fps (min %0.2f, avg %0.2f, max %0.2f, "
      "%u frames in %0.1f secs total)\n", rate, fps_rate_min, avg, fps_rate_max,
      fps_counter_total, secs_total);
      
    fps_counter = 0;
    fps_timer = current_timer;
    
  }
   
}
#endif

void
Gui::addFlashVars(Gui::VariableMap& from)
{
    for (VariableMap::iterator i=from.begin(), ie=from.end(); i!=ie; ++i)
    {
        _flashVars[i->first] = i->second;
    }
}

void
Gui::setMovieDefinition(movie_definition* md)
{
    assert(!_movieDef);
    _movieDef = md;
}

void
Gui::setStage(movie_root* stage)
{
    assert(stage);
    assert(!_stage);
    _stage = stage;
}

// end of namespace
}
