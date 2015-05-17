#ifndef REGISTER_H
#define REGISTER_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

/** All of the functions used to expose c++ objects to Lua are in this 
 * file */

#include <string>
#include <iostream>
#include <tuple>
#include <functional>
#include <type_traits>
#include <memory>

extern "C" {
#include "lua5.2/lua.h"
#include "lua5.2/lualib.h"
#include "lua5.2/lauxlib.h"
}

#include "primitives.h"
#include "trait.h"
#include "read_and_write.h"

/**
     * \param 	index index of the stack used to read the value.
     * \return 	tuple filled with the values on the stack.
     * \author 	Stud
     * \brief 	Method used to turn a parameter pack into a tuple of
     * 			variables intialized with the values read on Lua's stack.
     *
     * 			Recursively calls itself until Rest is empty, then it'll call the
     * 			stop function. At each recursion, concatenate the tuple and the
     * 			new value and increment the index to read the correct value on
     * 			the stack.
     * 			SFINAE used, case T1 is not a reference.
     */
template <typename T1, typename T2, typename... Rest>
inline typename std::enable_if<!std::is_reference<T1>::value, std::tuple<T1, T2, Rest...> >::type
getArgs(lua_State* l, const int index)
{
    std::tuple<T1> head = std::make_tuple(read<T1>(l, index));
    return std::tuple_cat(head, getArgs<T2, Rest...>(l, index + 1));
}

/**
    * \param 	index index of the stack used to read the value.
    * \return 	tuple filled with the values on the stack.
    * \author 	Stud
    * \brief 	Method used to turn a parameter pack into a tuple of
    * 			variables intialized with the values read on Lua's stack.
    *
    * 			Recursively calls itself until Rest is empty, then it'll call the
    * 			stop function. At each recursion, concatenate the tuple and the
    * 			new value and increment the index to read the correct value on
    * 			the stack.
    * 			SFINAE used, case T1 is a reference.
    */
template <typename T1, typename T2, typename... Rest>
inline typename std::enable_if<std::is_reference<T1>::value, std::tuple<T1&, T2, Rest...> >::type
getArgs(lua_State* l, const int index)
{
    std::tuple<T1&> head = std::tie(read<T1&>(l, index));
    return std::tuple_cat(head, getArgs<T2, Rest...>(l, index + 1));
}

/**
     * \param 	index index of the stack used to read the value.
     * \return 	tuple filled with the value read on the stack.
     * \author 	Stud
     * \brief 	Build a tuple with a single variable intialized with the
     * 			value on the stack. Used to concatenate several tuple in
     * 			one tuple with several parameter.
     * 			SFINAE used, case T1 is not a reference.
     */
template <typename T>
inline typename std::enable_if<!std::is_reference<T>::value, std::tuple<T> >::type
getArgs(lua_State* l, const int index)
{
    return std::make_tuple(read<T>(l, index));
}

/**
     * \param 	index index of the stack used to read the value.
     * \return 	tuple filled with the value read on the stack.
     * \author 	Stud
     * \brief 	Build a tuple with a single variable intialized with the
     * 			value on the stack. Used to concatenate several tuple in
     * 			one tuple with several parameter.
     * 			SFINAE used, case T1 is a reference.
     */
template <typename T>
inline typename std::enable_if<std::is_reference<T>::value, std::tuple<T> >::type
getArgs(lua_State* l, const int index)
{
    return std::tie(read<T&>(l, index));
}

/**
     * 	\param 		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\return 	instantiated object from class ClassName
     * 	\author 	Stud
     * 	\brief 		Function used to instantiate an object using a tuple
     * 				that hold all of the arguments.
     * */
template <typename ClassName, typename... Args, std::size_t... N>
inline ClassName* instantiate(lua_State* l, std::tuple<Args...> args, _indices<N...>)
{
    void* data = lua_newuserdata(l, sizeof(ClassName));
    return new(data) ClassName(std::get<N>(args)...);
}

/**
     * 	\param 		args tuple filled with initialized arguments
     * 	\return 	instantiated object from class ClassName
     * 	\author 	Stud
     * 	\brief 		Starts the process of expanding the tuple to instantiate
     * 				an object of class ClassName.
     * */
template <typename ClassName, typename... Args>
inline ClassName* instantiate(lua_State* l, std::tuple<Args...> args)
{
    return instantiate<ClassName>(l, args,
    typename _indices_builder<sizeof...(Args)>::type());
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\return 	What the pointed function returns
     * 	\author 	Stud
     * 	\brief 		Expands the tuple to call
     * 				the function pointed by the first parameter.
     * 				SFINAE used, case non void return (what is returned
     * 				is pushed on the Lua stack).
     * */
template <typename Ret, typename ClassName, typename... Args, std::size_t... N>
inline typename std::enable_if<!std::is_void<Ret>::value>::type
callFunctionWithTuple(lua_State* l, Ret (ClassName::*fun)(Args...),
          std::tuple<Args...> args,
          _indices<N...>)
{
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    push(l, (obj->*fun)(std::get<N>(args)...));
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\author 	Stud
     * 	\brief 		Starts the process of expanding the tuple to call
     * 				the function pointed by the first parameter.
     * 				SFINAE used, case void return (there is nothing to
     * 				push on the Lua stack).
     * */
template <typename Ret, typename ClassName, typename... Args, std::size_t... N>
inline typename std::enable_if<std::is_void<Ret>::value>::type
callFunctionWithTuple(lua_State* l, Ret (ClassName::*fun)(Args...),
          std::tuple<Args...> args,
          _indices<N...>)
{
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    (obj->*fun)(std::get<N>(args)...);
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\author 	Stud
     * 	\brief 		Calls a member function that only needs a lua_State*
     * */
template <typename Ret, typename ClassName>
inline typename std::enable_if<std::is_void<Ret>::value>::type
callFunctionWithLua(lua_State* l, Ret (ClassName::*fun)(lua_State*))
{
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    (obj->*fun)(l);
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\author 	Stud
     * 	\brief 		Calls a member function that only needs a lua_State*
     * */
template <typename Ret, typename ClassName>
inline typename std::enable_if<!std::is_void<Ret>::value>::type
callFunctionWithLua(lua_State* l, Ret (ClassName::*fun)(lua_State*))
{
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    push(l, (obj->*fun)(l));
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\author 	Stud
     * 	\brief 		Starts the process of expanding the tuple to call
     * 				the function pointed by the first parameter.
     * */
template <typename Ret, typename ClassName, typename... Args>
inline void callFunctionWithTuple(lua_State* l, Ret (ClassName::*fun)(Args...),
          std::tuple<Args...> args) {
    callFunctionWithTuple(l, fun, args, typename _indices_builder<sizeof...(Args)>::type());
}

/**
     * 	\param 		Ret (fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\return 	What the pointed function returns
     * 	\author 	Stud
     * 	\brief 		Expands the tuple to call
     * 				the function pointed by the first parameter.
     * 				SFINAE used, case non void return (what is returned
     * 				is pushed on the Lua stack).
     * */
template <typename Ret, typename... Args, std::size_t... N>
inline typename std::enable_if<!std::is_void<Ret>::value>::type
callFunctionWithTuple(lua_State* l, Ret (fun)(Args...),
          std::tuple<Args...> args,
          _indices<N...>)
{
    push(l, (fun)(std::get<N>(args)...));
}

/**
     * 	\param 		Ret (fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\param 		_indices trait used to unpack the tuple
     * 	\author 	Stud
     * 	\brief 		Starts the process of expanding the tuple to call
     * 				the function pointed by the first parameter.
     * 				SFINAE used, case void return (there is nothing to
     * 				push on the Lua stack).
     * */
template <typename Ret, typename... Args, std::size_t... N>
inline typename std::enable_if<std::is_void<Ret>::value>::type
callFunctionWithTuple(lua_State* , Ret (fun)(Args...),
          std::tuple<Args...> args,
          _indices<N...>)
{
    (fun)(std::get<N>(args)...);
}

/**
     * 	\param 		Ret (fun)(Args...) pointer to the function
     * 	\param		args tuple filled with initialized arguments
     * 	\author 	Stud
     * 	\brief 		Starts the process of expanding the tuple to call
     * 				the function pointed by the first parameter.
     * */
template <typename Ret, typename... Args>
inline void callFunctionWithTuple(lua_State* l, Ret(fun)(Args...),
          std::tuple<Args...> args) {
    callFunctionWithTuple(l, fun, args, typename _indices_builder<sizeof...(Args)>::type());
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\author 	Stud
     * 	\brief 		Calls a function with 0 argument and push the result
     * 				on the stack.
     * 				SFINAE used, case non void return (what is returned
     * 				is pushed on the Lua stack).
     * */
template <typename Ret, typename ClassName>
inline typename std::enable_if<!std::is_void<Ret>::value>::type
callFunction(lua_State* l, Ret (ClassName::*fun)())
{
    //Get the instance on the stack and call the function
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    push(l, (obj->*fun)());
}

/**
     * 	\param 		Ret (ClassName::*fun)(Args...) pointer to the function
     * 	\author 	Stud
     * 	\brief 		Calls a function with 0 argument and that return void.
     * 				SFINAE used, case void return (there is nothing to
     * 				push on the Lua stack).
     * */
template <typename Ret, typename ClassName>
inline typename std::enable_if<std::is_void<Ret>::value>::type
callFunction(lua_State* l, Ret (ClassName::*fun)())
{
    //Get the instance on the stack and call the function
    ClassName * obj = l_checkClass<ClassName>(l, 1);
    (obj->*fun)();
}

/**
     * 	\param 		Ret (fun)() pointer to the function
     * 	\author 	Stud
     * 	\brief 		Calls a function with 0 argument and that return void.
     * 				SFINAE used, case void return (there is nothing to
     * 				push on the Lua stack).
     * */
template <typename Ret>
inline typename std::enable_if<std::is_void<Ret>::value>::type
callFunction(lua_State* , Ret(fun)())
{
    fun();
}

/**
     * 	\param 		Ret (fun)() pointer to the function
     * 	\author 	Stud
     * 	\brief 		Calls a function with 0 argument and push the result
     * 				on the stack.
     * 				SFINAE used, case non void return (what is returned
     * 				is pushed on the Lua stack).
     * */
template <typename Ret>
inline typename std::enable_if<!std::is_void<Ret>::value>::type
callFunction(lua_State* l, Ret (fun)())
{
    push(l, fun());
}


/**	\brief Struct used to expose member function 
 *
 * */
template<typename MethodType, MethodType> struct registerMemberFunction;

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C++ member function to Lua.
 * 			A structure is used to pass a function member pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 */
template<typename ClassName, typename ReturnType, typename ...Args, ReturnType (ClassName::*method)(Args...)>
struct registerMemberFunction<ReturnType (ClassName::*)(Args...), method>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushstring(l, name.c_str());
        lua_pushcfunction(l, [](lua_State* l) {
            lua_pushcfunction(l, [](lua_State* l) {
                //Create a tuple from the variadic template and initialize
                //The variables with the values on the stack
                std::tuple<Args...> args = getArgs<Args...>(l, 2);
                //Clean the stack
                lua_settop(l, - (sizeof...(Args) + 1));
                //Unpack the tuple, calls the function and push the result
                callFunctionWithTuple(l, method, args);
                return 1;
            });
            return 1;
        });
        //Link the lambda function with the name
        lua_rawset(l, -3);
    }
};

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose specific member function
 *          that need to manage the stack themselves.
 */
template<typename ClassName, typename ReturnType, ReturnType (ClassName::*method)(lua_State*)>
struct registerMemberFunction<ReturnType (ClassName::*)(lua_State*), method>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushstring(l, name.c_str());
        lua_pushcfunction(l, [](lua_State* l) {
            lua_pushcfunction(l, [](lua_State* l) {
                callFunctionWithLua(l, method);
                return 1;
            });
            return 1;
        });
        //Link the lambda function with the name
        lua_rawset(l, -3);
    }
};

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C++ member function to Lua.
 * 			A structure is used to pass a member function pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 * 			Template specialization for no argument.
 */
template<typename ClassName, typename ReturnType, ReturnType (ClassName::*method)(void)>
struct registerMemberFunction<ReturnType (ClassName::*)(void), method>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushstring(l, name.c_str());
        lua_pushcfunction(l, [](lua_State* l) {
            //Simply register a lambda that calls the function
            lua_pushcfunction(l, [](lua_State* l) {
                //Clean the stack
                lua_settop(l, -1);
                //Call the function without arguments
                callFunction(l, method);

                return 1;
            });
            return 1;
        });
        //Link the lambda function with the name
        lua_rawset(l, -3);
    }
};

/**	\brief struct used to expose C++ static function to lua
 *
 * */
template<typename F, F f> struct registerStaticFunction;

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C++ static function to Lua.
 * 			A structure is used to pass a member function pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 */
template<typename Ret, typename ...Args, Ret(*f)(Args...)>
struct registerStaticFunction<Ret(*)(Args...), f>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushcfunction(l, [](lua_State* l) {
            //Create a tuple from the variadic template and initialize
            //The variables with the values on the stack
            std::tuple<Args...> args = getArgs<Args...>(l, 1);
            //Clean the stack
            lua_settop(l, - (sizeof...(Args) + 1));
            //Unpack the tuple, calls the function and push the result
            callFunctionWithTuple(l, f, args);
            return 1;
        });
        //Link the lambda function with the name
        lua_setfield(l, -2, name.c_str());
    }
};

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C++ static function to Lua.
 * 			A structure is used to pass a member function pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 * 			Template specialization for no argument.
 */
template<typename Ret, Ret(*f)(void)>
struct registerStaticFunction<Ret(*)(void), f>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushcfunction(l, [](lua_State* l) {
            //Calls the function and push the result
            callFunction(l, f);
            return 1;
        });
        //Link the lambda function with the name
        lua_setfield(l, -2, name.c_str());
    }
};

/**	\brief struct used to expose C function to lua
 *
 * */
template<typename F, F f> struct registerCFunction;

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C function to Lua.
 * 			A structure is used to pass a member function pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 */
template<typename Ret, typename ...Args, Ret(*f)(Args...)>
struct registerCFunction<Ret(*)(Args...), f>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushcfunction(l , [](lua_State* l) {
            //Create a tuple from the variadic template and initialize
            //The variables with the values on the stack
            std::tuple<Args...> args = getArgs<Args...>(l, 1 + 1);//There is always a this from js
            //Clean the stack
            lua_settop(l, - (sizeof...(Args) + 1 + 1));//There is always a this from js
            //Unpack the tuple, calls the function and push the result
            callFunctionWithTuple(l, f, args);
            return 1;
        });
        //Link the lambda function with the name
        lua_setfield(l, -2, name.c_str());
    }
};

/**
 * \param 	s State that carries lua_State
 * \param 	name name of the function in the Lua environment
 * \author 	Stud, Marc
 * \brief 	Structure used to expose C function to Lua.
 * 			A structure is used to pass a member function pointer as template
 * 			parameter and access it in the lambda function while respecting
 * 			the prototype required by Lua (int func(lua_State*) ).
 * 			Template specialization for no argument.
 */
template<typename Ret, Ret(*f)(void)>
struct registerCFunction<Ret(*)(void), f>
{
    static void push(lua_State* l, std::string name)
    {
        lua_pushcfunction(l, [](lua_State* l) {
            //Calls the function and push the result
            callFunction(l, f);
            return 1;
        });
        //Link the lambda function with the name
        lua_setfield(l, -2, name.c_str());
    }
};

template<typename Type, typename ClassName, Type ClassName::* t>
void registerAttribute(lua_State* l, std::string name)
{
    lua_pushcfunction(l, [](lua_State* l) {
        ClassName *obj = l_checkClass<ClassName>(l, 1);
        if(lua_gettop(l) == 1)
        {
            if(obj != nullptr)
                push(l, obj->*t);
        }
        else
            obj->*t = read<Type>(l, 2);
        return 1;
    });
    //Link the lambda function with the name
    lua_setfield(l, -2, name.c_str());
}

/**
 * \param 	Ret (*)(Args...)) the function pointer
 * \author 	Stud
 * \brief 	Avoids the user to write the templates
 */
template<typename Ret, typename... Args>
auto deduceFunction(Ret (*)(Args...)) -> Ret(*)(Args...)
{}

/**
 * \param 	ReturnType (ClassName::*)(Args...) the function pointer
 * \author 	Marc
 * \brief 	Avoids the user to write the templates
 */
template<typename ClassName, typename ReturnType, typename... Args>
auto deduceMethod(ReturnType (ClassName::*)(Args...)) -> ReturnType (ClassName::*)(Args...)
{}

/**
 * \author 	Stud
 * \brief 	use MODULEFUNCTION(Func)::push(State, "name"); to register a module function
 */
#define MODULEFUNCTION(m) registerCFunction<decltype(deduceFunction(&m)), &m>

/**
 * \author 	Stud
 * \brief 	use STATICFUNCTION(Class::Func)::push(State, "name"); to register a static function
 */
#define STATICMETHOD(m) registerStaticFunction<decltype(deduceFunction(&m)), &m>

/**
 * \author 	Marc
 * \brief 	use METHOD(Class::Func)::push(State, "name"); to register a function
 */
#define METHOD(m) registerMemberFunction<decltype(deduceMethod(&m)), &m>

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function used to register class constructor.
 * 			SFINAE, case where constructor needs arguments.
 */
template <typename ClassName, typename... Args>
typename std::enable_if< (sizeof ...(Args) != 0) >::type
generateConstructorFunction(lua_State * l)
{
    lua_pushcfunction(l, [](lua_State* l) {
        //Convert the variadic template into a tuple of arguments
        //initialized on the Lua stack
        std::tuple<Args...> args = getArgs<Args...>(l, 2);
        //Unpack the tuple to instantiate an object
        ClassName* elem = instantiate<ClassName>(l, args);
        //Hide the "warning: unused variable" at compile time.
        (void)elem;
        //Push the metatable with the name of the class
        luaL_getmetatable(l, getClassName<ClassName>().c_str());
        //Pops a table from the stack and set it as the metatable of the object
        lua_setmetatable(l, -2);
        return 1;
    });
}

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function used to register class constructor.
 * 			SFINAE, case where constructor does not need arguments.
 */
template <typename ClassName, typename... Args>
typename std::enable_if< (sizeof ...(Args) == 0) >::type
generateConstructorFunction(lua_State * l)
{
    lua_pushcfunction(l, [](lua_State* l) {
        //Placement new and instantiation of the object
        void* data = lua_newuserdata(l, sizeof(ClassName));
        ClassName* elem = new(data) ClassName();
        //Hide the "warning: unused variable" at compile time.
        (void)elem;
        //Push the metatable with the name of the class
        luaL_getmetatable(l, getClassName<ClassName>().c_str());
        //Pops a table from the stack and set it as the metatable of the object
        lua_setmetatable(l, -2);
        return 1;
    });
}

template <typename ClassName, typename... Args>
void registerConstructor(lua_State * l)
{
    //The table that will hold the constructor
    lua_newtable(l);
    //The metatable used to store __call
    lua_newtable(l);
    //Push the constructor
    generateConstructorFunction<ClassName, Args...>(l);
    //Add __call with the previous lambda (the constructor)
    lua_setfield(l, -2, "__call");
    //Set the metatable on the "constructor" table
    lua_setmetatable(l, -2);
    //Add the .prototype key with the metatable of the class as value
    luaL_getmetatable(l, getClassName<ClassName>().c_str());
    lua_setfield(l, -2, "prototype");
}

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function used to register class destructor.
 */
template <typename ClassName>
void registerDestructor(lua_State * l) 
{
    lua_pushstring(l, "__gc");
    lua_pushcfunction(l, [](lua_State* l) {
        //Get the intance on the stack
        ClassName * cl = l_checkClass<ClassName>(l, 1);
        //Explicit call to the destructor as we used placementnew to instantiate
        cl->~ClassName();
        return 1;
    });
    lua_rawset(l, -3);
}

/**
 * \param 	l lua_State*
 * \param 	int (*f)(State&) free function that register the member functions
 * \author 	Stud
 * \brief 	function used to register a class. The function pointer passed
 * 			as template parameter should point to a function that registers
 * 			all the member function of the class.
 */
template <int (*f)(lua_State*), typename... Args>
void registerModule(lua_State* l, const char*  name)
{
    auto lambda = [](lua_State* l) {
        //Create a table for the module
        lua_newtable(l);

        //Register all the class and function that we want to expose to Lua
        (*f)(l);
        return 1;
    };
    //Register the previous lambda function that'll be called on "require" in Lua
    lua_pushcfunction(l, lambda);
    //Link the previous lambda with the name of the class.
    lua_setfield(l, -2, name);
}

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function called when the application tries to access to an
 *          element (function or attribute) in the userdata.
 *          If it is a function, we must return a function, we return a
 *          lambda that return the function so that the application can
 *          call it.
 */
inline int indexFunction(lua_State* l)
{
    lua_insert(l, 1);
    do
    {
        //If it's not the first iteration, nil is on the stack
        if(lua_isnil(l, -1))
            lua_remove(l, -1);
        //Get the userdata's or the table's metatable
        if(!lua_getmetatable(l, -1))
        {
            //If it does not have a metatable, it means the object does not have that member
            fprintf(stderr, "The object does not have the requested member\n");
            exit (EXIT_FAILURE);
        }
        //Move the table to the valid index for gettable
        lua_pushvalue(l, 1);
        lua_rawget(l, -2);
    } while(lua_isnil(l, -1));
    //Set all the value on the right position for lua_call (function userdata)
    lua_pushvalue(l, 2);
    //Call the lambda that will return the attribute or the method. __index -> get()
    lua_call(l, 1, 1);
    //Clean the stack
    while(lua_gettop(l) > 1)
        lua_remove(l, 1);
    return 1;
}

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function called when the application tries to update the value
 *          of a userdata's element. The value will only be updated if the
 *          element leads to an attribute (if the function is a setter).
 */
inline int newIndexFunction(lua_State* l)
{
    lua_insert(l, 1);
    lua_insert(l, 1);
    do
    {
        //If it's not the first iteration, nil is on the stack
        if(lua_isnil(l, -1))
            lua_remove(l, -1);
        //Get the userdata's or the table's metatable
        if(!lua_getmetatable(l, -1))
        {
            //If it does not have a metatable, it means the object does not have that member
            fprintf(stderr, "The object does not have the requested member\n");
            exit (EXIT_FAILURE);
        }
        //Move the table to the valid index for gettable
        lua_pushvalue(l, 1);
        lua_rawget(l, -2);
    } while(lua_isnil(l, -1));
    //Set all the value on the right position for lua_call (function userdata)
    lua_pushvalue(l, 3);
    lua_pushvalue(l, 2);
    lua_pushvalue(l, 1);
    //Call the lambda that will return the attribute or the method. __index -> get()
    lua_call(l, 3, 0);
    //Clean the stack
    lua_settop(l, 0);
    return 1;
}

/**
 * \param 	l lua_State*
 * \author 	Stud
 * \brief 	function that create the metatable when we register classes.
 *          It also adds the metamethods __index, __newindex and _prototype.
 */
template <typename ClassName>
void create_metatable(lua_State* l)
{
    //Create a metatable for this class
    luaL_newmetatable(l, getClassName<ClassName>().c_str());

    //Add the index
    lua_pushcfunction(l, indexFunction);
    lua_setfield(l, -2, "__index");

    lua_pushvalue(l, -1);
    lua_setfield(l, -2, "_prototype");

    //Add the newindex
    lua_pushcfunction(l, newIndexFunction);
    lua_setfield(l, -2, "__newindex");
}

/**
 * \param 	l lua_State*
 * \param 	inhClassName std::string The name of the parent's class in the module.
 *          If the parent is in another module, use "moduleName.className"
 * \author 	Stud
 * \brief 	function that set a parent's metatable as the metatable of the
 *          class that inherits it.
 */
inline void load_inherited_values(lua_State* l, std::string inhClassName)
{
    //Check if the parent class is in another module, if so load it.
    std::size_t found = inhClassName.find(".");
    if(found != std::string::npos)
    {
        std::string module = inhClassName.substr(0, found);
        inhClassName = inhClassName.substr(found + 1);
        luaL_dostring(l, ("require(\"" + module + "\")").c_str());
    }
    luaL_getmetatable(l, inhClassName.c_str());
    lua_setmetatable(l, -2);
}

/**
 * \param 	l lua_State*
 * \param 	name the name of the class in the module.
 * \author 	Stud
 * \brief 	function used to ends a class registration.
 */
inline void set_field(lua_State* l, std::string name)
{
    //Add the constructor table to the module
    lua_setfield(l, -3, name.c_str());
    //Clean the stack
    lua_remove(l, -1);
}

/**
 * \param 	l lua_State*
 * \param 	f  int(*)(lua_State*) the function that calls the macros used to register
 *          the non static class elements.
 * \param 	name the name of the class in the module.
 * \author 	Stud
 * \brief 	function used to register a class within a module. It needs to be called
 *          inside the function that is passed as a callback in registerModule.
 */
template <typename ClassName, typename... Args>
void registerClass(lua_State* l, int (*f)(lua_State*), std::string  name)
{
    //Create the metatable and init the values of the metamethods
    create_metatable<ClassName>(l);
    //Register all the member function that we want to expose to Lua
    (*f)(l);
    //Register the destructor
    registerDestructor<ClassName>(l);
    //Register the destructor
    registerConstructor<ClassName, Args...>(l);

    set_field(l, name);
}

/**
 * \param 	l lua_State*
 * \param 	f  int(*)(lua_State*) the function that calls the macros used to register
 *          the non static class elements.
 * \param 	g  int(*)(lua_State*) the function that calls the macros used to register
 *          the static class elements.
 * \param 	name the name of the class in the module.
 * \author 	Stud
 * \brief 	function used to register a class within a module. It needs to be called
 *          inside the function that is passed as a callback in registerModule.
 */
template <typename ClassName, typename... Args>
void registerClass(lua_State* l, int (*f)(lua_State*), int (*g)(lua_State*), const char*  name)
{
    //Create the metatable and init the values of the metamethods
    create_metatable<ClassName>(l);

    //Register all the member function that we want to expose to Lua
    (*f)(l);
    //Register the destructor
    registerDestructor<ClassName>(l);
    //Register the destructor
    registerConstructor<ClassName, Args...>(l);
    //Register all the static member function that we want to expose to Lua
    (*g)(l);

    set_field(l, name);
}

/**
 * \param 	l lua_State*
 * \param 	f  int(*)(lua_State*) the function that calls the macros used to register
 *          the non static class elements.
 * \param 	name the name of the class in the module.
 * \param 	inhClassName the name of the parent class. If the parent is in another module,
 *          use "moduleName.className"
 * \author 	Stud
 * \brief 	function used to register a class that inherit from another class within a module. It needs to be called
 *          inside the function that is passed as a callback in registerModule.
 */
template <typename ClassName, typename... Args>
void registerClassInherit(lua_State* l, int (*f)(lua_State*), const char*  name, std::string inhClassName)
{
    //Create the metatable and init the values of the metamethods
    create_metatable<ClassName>(l);

    //Call the parent module if needed and take the parent metamethod
    load_inherited_values(l, inhClassName);

    //Register all the member function that we want to expose to Lua
    (*f)(l);
    //Register the destructor
    registerDestructor<ClassName>(l);
    //Register the destructor
    registerConstructor<ClassName, Args...>(l);

    set_field(l, name);
}

/**
 * \param 	l lua_State*
 * \param 	f  int(*)(lua_State*) the function that calls the macros used to register
 *          the non static class elements.
 * \param 	g  int(*)(lua_State*) the function that calls the macros used to register
 *          the static class elements.
 * \param 	inhClassName the name of the parent class. If the parent is in another module,
 *          use "moduleName.className"
 * \author 	Stud
 * \brief 	function used to register a class that inherit from another class within a module. It needs to be called
 *          inside the function that is passed as a callback in registerModule.
 */
template <typename ClassName, typename... Args>
void registerClassInherit(lua_State* l, int (*f)(lua_State*), int (*g)(lua_State*), const char*  name, std::string inhClassName)
{
    //Create the metatable and init the values of the metamethods
    create_metatable<ClassName>(l);

    //Call the parent's module and take the parent's metamethod
    load_inherited_values(l, inhClassName);

    //Register all the member function that we want to expose to Lua
    (*f)(l);
    //Register the destructor
    registerDestructor<ClassName>(l);
    //Register the destructor
    registerConstructor<ClassName, Args...>(l);
    //Register all the static member function that we want to expose to Lua
    (*g)(l);

    set_field(l, name);
}

#endif
