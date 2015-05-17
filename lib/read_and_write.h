#ifndef READ_AND_WRITE_H
#define READ_AND_WRITE_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <memory>

extern "C" {
#include "lua5.2/lua.h"
#include "lua5.2/lualib.h"
#include "lua5.2/lauxlib.h"
}

#include "trait.h"
#include "primitives.h"
#include "luaref_tracker.h"
#include "optional.hpp"

template <typename Ret, typename... Args>
typename std::enable_if<std::is_void<Ret>::value, std::function<Ret(Args...)> >::type
_get(_id<std::function<Ret(Args...)> >, lua_State *l, const int index);

template <typename Ret, typename... Args>
typename std::enable_if<!std::is_void<Ret>::value, std::function<Ret(Args...)> >::type
_get(_id<std::function<Ret(Args...)> >, lua_State *l, const int index);

template <typename T>
inline T* _get(_id<T*>, lua_State *l, const int index);

template <typename T>
inline T& _get(_id<T&>, lua_State *l, const int index);

inline bool _get(_id<bool>, lua_State *l, const int index);
inline int _get(_id<int>, lua_State *l, const int index);
inline unsigned int _get(_id<unsigned int>, lua_State *l, const int index);
inline lua_Number _get(_id<lua_Number>, lua_State *l, const int index);
inline std::string _get(_id<std::string>, lua_State *l, const int index);

using namespace std::experimental;

/**
* \param 	l_ the lua_state*
* \author 	Stud
* \brief 	Used for debug, prints the lua stack
*/
inline void stack_dump (lua_State* l_)
{
    int i;
    int top = lua_gettop(l_);
    for (i = 1; i <= top; i++)
    {
        int t = lua_type(l_, i);
        switch (t) {

        case LUA_TSTRING:
            fprintf(stderr, "`%s'", lua_tostring(l_, i));
            break;

        case LUA_TBOOLEAN:
            fprintf(stderr, lua_toboolean(l_, i) ? "true" : "false");
            break;

        case LUA_TNUMBER:
            fprintf(stderr, "%g", lua_tonumber(l_, i));
            break;

        default:
            fprintf(stderr, "%s", lua_typename(l_, t));
            break;

        }
        fprintf(stderr, "  ");
    }
    fprintf(stderr, "\n");
}

struct nil {};

inline void push(lua_State* )
{}

/**
     * \param 	value the value pushed during this iteration.
     * \param 	values values pushed in the next recursions.
     * \author 	Stud
     * \brief 	Recursively push values on Lua's stack.
     */
template <typename T, typename... Ts>
inline void push(lua_State* l, T &&value, Ts&&... values)
{
    //Call _push to add the value on the stack
    _push(l, std::forward<T>(value));
    //Recursive call to push
    push(l, std::forward<Ts>(values)...);
}

/**
     * \param 	index specify where to read the value on the stack.
     * \return 	the value read on the stack.
     * \author 	Stud
     * \brief 	Read a value on Lua's stack at the specify index.
     */
template <typename T>
inline T read(lua_State* l, const int index) {
    //Call for a templated function to read values
    return _get(_id<T>{}, l, index);
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<T*> generic pointer type
 * \brief 	Reads a pointer on the Lua stack
 */
template <typename T>
T* _get(_id<T*>, lua_State *l, const int index) {
    return static_cast<T*>(lua_touserdata(l, index));
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<T&> generic reference type
 * \brief 	Read reference on the Lua stack.
 */
template <typename T>
T& _get(_id<T&>, lua_State *l, const int index) {
    static_assert(!is_primitive<T>::value,
                  "Reference types must not be primitives.");
    return (*l_checkClass<T>(l, index));
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<T> default type (userdata)
 * \brief 	Read reference on the Lua stack.
 */
template <typename T>
T _get(_id<T>, lua_State *l, const int index) {
    return (*l_checkClass<T>(l, index));
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 *
 * \param 	_id<T> default type (userdata)
 * \brief 	Read reference on the Lua stack.
 */
template <typename Ret, typename... Args>
typename std::enable_if<std::is_void<Ret>::value, std::function<Ret(Args...)> >::type
_get(_id<std::function<Ret(Args...)> >, lua_State *l, const int index)
{
    std::shared_ptr<LuarefTracker> luarefTracker(new LuarefTracker(l, index));
    auto lambda = [l, luarefTracker](Args... args) {
        //Get the reference
        lua_rawgeti(l, LUA_REGISTRYINDEX, luarefTracker->get_ref());
        //Push the arguments on the stack
        push(l, nil(), args...);
        constexpr int num_args = sizeof...(Args) + 1;
        //Call the function
        if (lua_pcall(l, num_args, 0, 0) != 0)
            printf("error running function `f': %s\n",lua_tostring(l, -1));

        lua_settop(l, 0);
    };
    return lambda;
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<std::function<Ret(Args...)> > dummy struct indicate a function pointer
 * \brief 	Read reference on the Lua stack.
 */
template <typename Ret, typename... Args>
typename std::enable_if<!std::is_void<Ret>::value, std::function<Ret(Args...)> >::type
_get(_id<std::function<Ret(Args...)> >, lua_State *l, const int index)
{
    lua_pushvalue(l, index);
    int ref = luaL_ref(l, LUA_REGISTRYINDEX);
    std::shared_ptr<LuarefTracker> luarefTracker(new LuarefTracker(l, ref));
    auto lambda = [l, ref, luarefTracker](Args... args) {
        //Get the reference
        lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
        //Push the arguments on the stack
        push(l, nil(), args...);
        constexpr int num_args = sizeof...(Args) + 1;
        //Call the function
        lua_call(l, num_args, 1);
        Ret ret = read<Ret>(l, -1);
        lua_settop(l, 0);
        return ret;
    };
    return lambda;
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<bool> dummy struct indicate a bool
 * \brief 	Reads a bool on the Lua stack
 */
inline bool _get(_id<bool>, lua_State *l, const int index) {
    return lua_toboolean(l, index) != 0;
}
/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<int> dummy struct indicate a int
 * \brief 	Reads an int on the Lua stack
 */
inline int _get(_id<int>, lua_State *l, const int index) {
    int a = lua_tointeger(l, index);
    return a;
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<unsigned int> dummy struct indicate a unsigned int
 * \brief 	Reads an unsigned int on the Lua stack
 */
inline unsigned int _get(_id<unsigned int>, lua_State *l, const int index) {
    return lua_tounsigned(l, index);
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<lua_Number> dummy struct indicate a number
 * \brief 	Reads a number on the Lua stack
 */
template<typename T>
optional<T> _get(_id< optional<T> >, lua_State *l, const int index) {
    if(lua_isuserdata(l, index))
        return l_checkClass<T>(l, index);
    return optional<T>();
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<lua_Number> dummy struct indicate a number
 * \brief 	Reads a number on the Lua stack
 */
lua_Number _get(_id<lua_Number>, lua_State *l, const int index) {
    return lua_tonumber(l, index);
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id<std::string> dummy struct indicate a string
 * \brief 	Reads an std::string on the Lua stack
 */
std::string _get(_id<std::string>, lua_State *l, const int index) {
    size_t size;
    const char *buff = lua_tolstring(l, index, &size);
    return std::string{buff, size};
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id< optional<bool> > optional parameter
 * \brief 	Check if the optional parameter is on the stack
 *          and returns it.
 *
 */
template <>
inline optional<lua_Number> _get(_id< optional<lua_Number> >, lua_State *l, const int index) {
    if(lua_isnumber(l, index))
        return lua_tonumber(l, index);
    return optional<lua_Number>();
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id< optional<bool> > optional parameter
 * \brief 	Check if the optional parameter is on the stack
 *          and returns it.
 *
 */
template <>
inline optional<unsigned int> _get(_id< optional<unsigned int> >, lua_State *l, const int index) {
    if(lua_isnumber(l, index))
        return lua_tounsigned(l, index);
    return optional<unsigned int>();
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id< optional<bool> > optional parameter
 * \brief 	Check if the optional parameter is on the stack
 *          and returns it.
 *
 */
template <>
inline optional<bool> _get(_id< optional<bool> >, lua_State *l, const int index) {
    if(lua_isboolean(l, index))
        return lua_toboolean(l, index);
    return optional<bool>();
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id< optional<std::string> > optional parameter
 * \brief 	Check if the optional parameter is on the stack
 *          and returns it.
 *
 */
template <>
inline optional<std::string> _get(_id< optional<std::string> >, lua_State *l, const int index) {
    if(lua_isstring(l, index))
    {
        size_t size;
        const char *buff = lua_tolstring(l, index, &size);
        return std::string{buff, size};
    }
    return optional<std::string>();
}

/**
 * \author 	Stud
 * \param 	_id tag that gives the type
 * \param 	l lua_State*
 * \param 	index index on the stack
 * \param 	_id< optional<int> > optional parameter
 * \brief 	Check if the optional parameter is on the stack
 *          and returns it.
 *
 */
template <>
inline optional<int> _get(_id< optional<int> >, lua_State *l, const int index) {
    if(lua_isnumber(l, index))
        return lua_tointeger(l, index);
    return optional<int>();
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \brief 	Empty push, do nothing
  */
template <typename T>
void _push(lua_State *l) {}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \brief 	Push a pointer
  */
template <typename T>
void _push(lua_State *l, T data )
{
    lua_pushlightuserdata(l, data);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	b boolean
  * \brief 	Push a boolean on the Lua stack
  */
template <>
inline void _push<bool>(lua_State *l, bool b) {
    lua_pushboolean(l, b);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	i int
  * \brief 	Push an int on the Lua stack
  */
template <>
inline void _push<int>(lua_State *l, int i) {
    lua_pushinteger(l, i);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	u unsigned int
  * \brief 	Push an unsigned int on the Lua stack
  */
template <>
inline void _push<unsigned int>(lua_State *l, unsigned int u) {
    lua_pushunsigned(l, u);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	f lua_Number
  * \brief 	Push other types of number on the Lua stack
  */
template <>
inline void _push<lua_Number>(lua_State *l, lua_Number f) {
    lua_pushnumber(l, f);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	s std::string
  * \brief 	Push an std::string on the Lua stack
  */
template <>
inline void _push<std::string>(lua_State *l, std::string s) {
    lua_pushlstring(l, s.c_str(), s.size());
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param 	s char*
  * \brief 	Push a char* on the Lua stack
  */
template <>
inline void _push<const char*>(lua_State *l, const char *s) {
    lua_pushstring(l, s);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param  n nil value
  * \brief 	Push a nil on the Lua stack
  */
template <>
inline void _push<nil>(lua_State *l, nil ) {
    lua_pushnil(l);
}

/**
  * \author 	Stud
  * \param 	l lua_State*
  * \param  n nil value
  * \brief 	Push a nil on the Lua stack
  */
template <typename T>
inline void _push(lua_State *l, optional<T> o) {
    if(o)
        push(l, *o);
    else
        push(l, nil());
}

#endif
