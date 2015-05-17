#ifndef LUAREFTRACKER_H
#define LUAREFTRACKER_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

/** This class is used to be sure that a registered elements in lua is
 *  deleted.
 * 
 *  For example when a lua function is given as a callback to a C++ 
 *  function, it is necessary to keep a reference to the function as 
 *  it'll not stay on the stack forever. 
 *  It is then necesseray to be able to delete it to avoid memory leaks. */

class LuarefTracker
{
public:
    LuarefTracker(lua_State* l, int index) :
        l_(l)
    {

        lua_pushvalue(l, index);
        ref_ = luaL_ref(l, LUA_REGISTRYINDEX);
    }

    ~LuarefTracker()
    {
        luaL_unref(l_, LUA_REGISTRYINDEX, ref_);
    }

    int get_ref()
    {
        return ref_;
    }

private:
    int ref_;
    lua_State* l_;
};
#endif
