#ifndef PRIMITIVE_H
#define PRIMITIVE_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <string>
#include <algorithm>

extern "C" {
#include "lua5.2/lua.h"
#include "lua5.2/lualib.h"
#include "lua5.2/lauxlib.h"
}

#include "trait.h"
 	
 /**
  * \brief 	Default type is not a primitive
  */
template <typename T>
struct is_primitive {
    static constexpr bool value = false;
};
 /**
  * \brief 	Int type is a primitive
  */
template <>
struct is_primitive<int> {
    static constexpr bool value = true;
};
 /**
  * \brief 	Unsigned int type is a primitive
  */
template <>
struct is_primitive<unsigned int> {
    static constexpr bool value = true;
};
 /**
  * \brief 	Bool type is a primitive
  */
template <>
struct is_primitive<bool> {
    static constexpr bool value = true;
};
 /**
  * \brief 	lua_Number type is a primitive
  */
template <>
struct is_primitive<lua_Number> {
    static constexpr bool result = true;
};
 /**
  * \brief 	std::string type is a primitive
  */
template <>
struct is_primitive<std::string> {
    static constexpr bool value = true;
};

/**
 * 	\param 		std::string&& the std::string that'll be cleaned.
 * 	\return 	the cleaned std::string
 * 	\author 	Stud
 * 	\brief 		Remove the alphanumeric values from the std::string.
 * */
inline std::string&& stripString(std::string&& s)
{
    s.erase(std::remove_if(s.begin(), s.end(),
    [](std::string::value_type ch) {
        return !isalpha(ch);
        }), s.end());
    return std::move(s);
}

/**
 * 	\return 	the name used to expose the class to Lua
 * 	\author 	Stud
 * 	\brief 		converts the class name to an std::string
 * */
template <typename T>
std::string getClassName()
{
    return stripString(typeid(T).name());
}

inline void sdump (lua_State* l_)
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

/**
 * 	\param 		l the lua_state*
 * 	\param 		ud the index on the Lua stack where the instance is.
 * 	\return 	an instance of ClassName
 * 	\author 	Stud
 * 	\brief 		Check whether the userdata on the stack is of type
 * 				ClassName and return it. Recursively check for
 *              possible parents metatables.
 * */
inline LUALIB_API void *checkudata (lua_State *L, int ud) {
    //Check for parents
    if (lua_getmetatable(L, -1))
    {
        lua_getmetatable(L, -1);
        if (!lua_rawequal(L, -1, -2))
        {
            checkudata(L, ud);
        }
        lua_pop(L, 1);
    }
    else
        return NULL;
}

/**
 * 	\param 		l the lua_state*
 * 	\param 		ud the index on the Lua stack where the instance is.
 * 	\param 		tname the name of the mematable in the registry
 * 	\return 	an instance of ClassName
 * 	\author 	Stud
 * 	\brief 		Check whether the userdata on the stack is of type
 * 				ClassName and return it.
 * */
inline LUALIB_API void *checkudata (lua_State *L, int ud, const char *tname) {
    //fprintf(stderr, "try : ");
    //sdump(L);
    void *p = lua_touserdata(L, ud);
    if (p != NULL)
    {  /* value is a userdata? */
        if (lua_getmetatable(L, ud))
        {  /* does it have a metatable? */
            luaL_getmetatable(L, tname);  /* get correct metatable */
            if (!lua_rawequal(L, -1, -2))  /* not the same? */
            {
                checkudata(L, ud);
            }
            lua_pop(L, 2);  /* remove both metatables */
            return p;
        }
    }
    //fprintf(stderr, "fail\n");
    return NULL;  /* value is not a userdata with a metatable */
}

/**
 * 	\param 		l the lua_state*
 * 	\param 		n the index on the Lua stack where the instance is.
 * 	\return 	an instance of ClassName
 * 	\author 	Stud
 * 	\brief 		Check whether the userdata on the stack is of type
 * 				ClassName and return it.
 * */
template <typename ClassName>
ClassName * l_checkClass(lua_State *l, int n)
{
    return static_cast<ClassName*>(checkudata(l, n, getClassName<ClassName>().c_str() ));
}




#endif
