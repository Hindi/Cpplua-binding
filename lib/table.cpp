/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include "table.h"

inline Table::TablePair::TablePair(std::string key, Table &table):
    table_(table),
	KEY(-2),
	VAL(-1)
{
    key_ = key;
}


inline Table::Table(lua_State *l, bool use_stack):
    name_(""),
    l_state_(l),
    global_(true)
{
    if(!use_stack)
        lua_settop(l_state_, 0);
    lua_newtable(l_state_);
    lua_insert(l_state_, 1);
    walk();
    lua_pushvalue(l_state_, 1);
    ref_ = luaL_ref(l_state_ ,LUA_REGISTRYINDEX);
    lua_remove(l_state_, 1);
}

inline Table::Table(lua_State *l, const int ref):
    name_(""),
    l_state_(l),
    ref_(ref),
    global_(false)
{

}

inline Table::Table(const std::string name, lua_State *l):
    name_(name),
    l_state_(l),
    global_(true)
{
    lua_getglobal(l_state_, name_.c_str());
    ref_ = luaL_ref(l_state_ ,LUA_REGISTRYINDEX);

}

inline Table::Table(const Table &t):
    name_(t.name_),
    l_state_(t.l_state_),
    global_(true)
{
    copy(t);
}

inline Table &Table::operator=(const Table &t)
{
    name_ = t.name_;
    l_state_ = t.l_state_;
    global_ = true;
    copy(t);
    return *this;
}

inline Table::~Table()
{
    unref();
}

inline void Table::unref()
{
    luaL_unref(l_state_, LUA_REGISTRYINDEX, ref_);
}

inline int Table::get_size() const
{
    load_table();
    lua_len(l_state_, 1);
    return read<int>(l_state_, -1);
}

inline void Table::load_table() const
{
    if(global_)
        lua_rawgeti(l_state_, LUA_REGISTRYINDEX, ref_);
    else
        lua_pushvalue(l_state_, ref_);
}

inline void Table::load_value(const std::string &key)
{
    load_table();
    lua_pushstring(l_state_, key.c_str());
    lua_gettable(l_state_, -2);
}

inline void Table::load_value(const int key)
{
    load_table();
    lua_pushnumber(l_state_, key);
    lua_gettable(l_state_, -2);
}

inline void Table::copy(const Table &t)
{
    t.load_table();
    lua_newtable(l_state_);
    lua_insert(l_state_, 1);
    walk();
    lua_pushvalue(l_state_, 1);
    lua_remove(l_state_, 1);
    ref_ = luaL_ref(l_state_ ,LUA_REGISTRYINDEX);
    lua_remove(l_state_, -1);
}


template<typename T>
inline void Table::set(const std::string& key, T value)
{
    load_table();
    luaL_checktype(l_state_, -1, LUA_TTABLE);
    std::size_t found = key.find(".");

    if(found == std::string::npos)
    {
        set_value(key, value);
    }
    else
    {
        std::string k = key.substr(0, found);           //first key
        std::string nextKey = key.substr(found + 1);    //rest of the path without the separator

        set(nextKey, value, k);
    }
}


template<typename U>
inline U Table::get(const std::string& key)
{
    load_table();
    luaL_checktype(l_state_, -1, LUA_TTABLE);

    std::size_t found = key.find(".");
    if(found == std::string::npos)
    {
        return get_value<U>(key);
    }
    else
    {
        std::string k = key.substr(0, found);           //first key
        std::string nextKey = key.substr(found + 1);    //rest of the path without the separator
        return get<U>(nextKey, k);
    }
}

template<typename U>
inline U Table::get(const int key)
{
    load_table();
    luaL_checktype(l_state_, -1, LUA_TTABLE);
    lua_pushnumber(l_state_, key);
    lua_gettable(l_state_, -2);
    lua_remove(l_state_, -2);
    return read<U>(l_state_, -1);
}


template<typename U>
inline void Table::set(const int key, U value)
{
    load_table();
    luaL_checktype(l_state_, -1, LUA_TTABLE);
    push(l_state_, key, value);
    lua_settable(l_state_, -3);
}

template<typename T>
inline bool Table::is_number(const T key)
{
    load_value(key);
    bool temp = lua_isnumber(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename T>
inline bool Table::is_string(const T key)
{
    load_value(key);
    bool temp = lua_isstring(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename T>
inline bool Table::is_nil(const T key)
{
    load_value(key);
    bool temp = lua_isnil(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename T>
inline bool Table::is_table(const T key)
{
    load_value(key);
    bool temp = lua_istable(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename T>
inline bool Table::is_function(const T key)
{
    load_value(key);
    bool temp = lua_isfunction(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename T>
inline bool Table::is_userdata(const T key)
{
    load_value(key);
    bool temp = lua_isuserdata(l_state_, -1);
    lua_settop(l_state_, -3);
    return temp;
}

template<typename U>
inline U Table::get(const std::string& key, const std::string& table_name)
{
    lua_getfield(l_state_, -1, table_name.c_str());
    luaL_checktype(l_state_, 1, LUA_TTABLE);
    std::size_t found = key.find(".");
    if(found == std::string::npos)
    {
        return get_value<U>(key);
    }
    else
    {
        std::string k = key.substr(0, found);           //first key
        std::string nextKey = key.substr(found + 1);    //rest of the path without the separator
        return get<U>(nextKey, k);
    }
}


template<typename U>
inline U Table::get_value(const std::string& key)
{
    lua_pushstring(l_state_, key.c_str());
    lua_gettable(l_state_, -2);
    U u = read<U>(l_state_, -1);
    lua_settop(l_state_, 0);
    return u;
}

template<typename U>
inline void Table::set_value(const std::string& key, U value)
{
    push(l_state_, value);
    lua_setfield(l_state_, -2, key.c_str());
    lua_settop(l_state_, 0);
}

template<typename T>
inline void Table::set(const std::string& key, T value, const std::string& table_name)
{
    if(!lua_istable(l_state_, 0))
    {
        lua_newtable(l_state_);
        lua_setfield(l_state_, -2, table_name.c_str());
    }
    lua_getfield(l_state_, -1, table_name.c_str());
    luaL_checktype(l_state_, 1, LUA_TTABLE);
    std::size_t found = key.find(".");

    if(found == std::string::npos)
    {
        set_value(key, value);
    }
    else
    {
        std::string k = key.substr(0, found);           //first key
        std::string nextKey = key.substr(found + 1);    //rest of the path without the separator
        set(nextKey, value, k);
    }
}

inline int Table::get_size_loaded() const
{
    lua_len(l_state_, 1);
    return read<int>(l_state_, -1);
}

inline void Table::walk()
{

    lua_pushnil(l_state_);
    while(lua_next(l_state_, -2) != 0)
    {
        copy_value();
    }
}

inline void Table::copy_value()
{
    switch (lua_type(l_state_, KEY)) {
    case LUA_TNUMBER:
        lua_pushvalue(l_state_, -2);
        lua_insert(l_state_, -3);
        lua_settable(l_state_, 1);
        break;
    case LUA_TSTRING:
        lua_setfield(l_state_, 1, lua_tostring(l_state_, KEY));
        break;
    }
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	t Table
  * \brief 	Push a table on the Lua stack
  */
template <>
inline void _push<Table>(lua_State *, Table t) {
    t.load_table();
}

/**
  * \author 	Stud
  * \param 	_id tag that gives the type
  * \param 	l lua_State*
  * \param 	index index on the stack
  * \param 	_id<T> default type (userdata)
  * \brief 	Read reference on the Lua stack.
  */
inline Table _get(_id<Table>, lua_State *l, const int index)
{
    return Table(l, index);
}
