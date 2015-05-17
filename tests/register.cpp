#include <gtest/gtest.h>

extern "C" {
#include "lua5.2/lua.h"
#include "lua5.2/lualib.h"
#include "lua5.2/lauxlib.h"
}

#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

#include "table.h"
#include "lua_register.h"

//Load LUA_C API
static const luaL_Reg loadedlibs[] = {
    {"_G", luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_IOLIBNAME, luaopen_io},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_BITLIBNAME, luaopen_bit32},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_DBLIBNAME, luaopen_debug},
    {NULL, NULL}
};

LUALIB_API void openlib (lua_State *l);

/*
#############################################
A bunch of methods and variables to tests the values after the test.
#############################################
*/
int index_constructor, void_CFunction, void_CFunctionA, void_CFuncParam, c_table_param, table_param, table_param_ref;
int parent_count, derive_count, parent_module_count, derive_module_count, read_on_lua_value, register_optional_int;
std::string name_constructor, name_empty_constructor;
bool destructor_called = false;
lua_State * l_;

void Cfunc_with_table(Table t)
{
    c_table_param = t.get<int>("foo");
}

int test_CFunctionA(int a)
{
    return a;
}

void test_void_CFuncParam()
{
    void_CFuncParam =5;
}

int test_CFunction()
{
    return 10;
}

void test_void_CFunction()
{
    void_CFunction = 10;
}

void test_void_CFunctionA(int a)
{
    void_CFunctionA = a;
}

/*
#############################################
Several classes used to test the registration
#############################################
*/
class RegisterTest: public ::testing::Test
{
public:    
    virtual void SetUp()
    {
        l_ = luaL_newstate();
        openlib(l_);
    }

    virtual void TearDown()
    {
        lua_close(l_);
    }
};

class Base
{
public:
    Base() {}

    void countBase()
    {
        parent_count = 10;
    }
};

class BaseModule
{
public:
    BaseModule(): ten(10)
    {}

    void countBase()
    {
        parent_module_count = 10;
    }

    int ten;
};

class Derive : public Base
{
public:
    Derive() {}

    void countDerive()
    {
        derive_count = 7;
    }
};

class DeriveModule : public BaseModule
{
public:
    DeriveModule():
        BaseModule()
    {}

    void countDerive()
    {
        derive_count = 7;
    }
};

class StaticClass
{
public:
    StaticClass() {}

    int sum(int a, int b)
    {
        return (a + b);
    }

    static int static_sum(int a, int b)
    {
        return (a + b);
    }

    static Table static_table(std::string s)
    {
        Table t(l_);
        t.set("key", s);
        return t;
    }

    static int get_ten()
    {
        return 10;
    }

private:
};

class Class
{
public:    
    Class(const std::string & n, const int i) :
        _name(n),
        _index(i)
    {
        ten = 10;
        index_constructor = i;
        name_constructor = n;
    }

    void func_with_table(Table t)
    {
        table_param = t.get<int>("foo");
    }

    void func_with_table_ref(const Table &)
    {
        fprintf(stderr, "Dummy\n");
        //table_param_ref = t.get<int>("foo");
    }
    
    void add(int a, int b)
    {
        _index = (a+b);
    }
    
    std::string get_name()
    {
        return _name;
    }

    int get_index()
    {
        return _index;
    }


    int call_func(std::function<int()> f)
    {
        return f();
    }

    int call_func_sum(std::function<int(int)> f)
    {
        return f(5);
    }

    void call_func_increment(std::function<void()> f)
    {
        f();
    }

    Table ret_table(std::string s)
    {
        Table t(l_);
        t.set("key", s);
        return t;
    }

    int return_to_lua(lua_State* )
    {
        return 10;
    }

    void read_on_lua(lua_State* l)
    {
        read_on_lua_value = read<int>(l, 2);
    }

    void register_optional(optional<int> a)
    {
        if(a)
        {
            register_optional_int = (*a);
        }
        else
            register_optional_int = 1;
    }


    ~Class()
    {

    }

    int ten;
private:
    std::string _name;
    int _index;
};


class ClassEmpty
{
public:
    ClassEmpty() : _name("NAME")
    {
        name_empty_constructor = "NAME";
    }
    
    void add()
    {
        _index = (5);
    }
    
    std::string get_name()
    {
        return _name;
    }

    int get_index()
    {
        return _index;
    }

    std::string get_class_ref(Class &c)
    {
        return c.get_name();
    }

    std::string get_class_copy(Class c)
    {
        return c.get_name();
    }

    std::string get_class_point(Class *c)
    {
        return c->get_name();
    }

    ~ClassEmpty()
    {
        destructor_called = true;
    }

private:
    std::string _name;
    int _index;
};

/*
#############################################
The methods used to register the classes to Lua.
Each class can define two function : one for member functions and another one for static member functions.
#############################################
*/
int load_base(lua_State* l)
{
	//Macro that register a member function
    METHOD(Base::countBase)::push(l, "countBase");
    return 0;
}

int load_base_two(lua_State* l)
{
	METHOD(BaseModule::countBase)::push(l, "countBase");
	//Macro that register a public attribute
    registerAttribute<int, BaseModule, &BaseModule::ten>(l_, "a");
    return 0;
}

int load_derive(lua_State* l)
{
    METHOD(Derive::countDerive)::push(l, "countDerive");
    return 0;
}

int load_derive_two(lua_State* l)
{
    METHOD(DeriveModule::countDerive)::push(l, "countDerive");
    return 0;
}

int load_Class(lua_State* l)
{
    METHOD(Class::func_with_table)::push(l, "func_with_table");
    METHOD(Class::func_with_table_ref)::push(l, "func_with_table_ref");
    METHOD(Class::call_func)::push(l, "call_func");
    METHOD(Class::call_func)::push(l, "call_func_increment");
    METHOD(Class::call_func_sum)::push(l, "call_func_sum");
    METHOD(Class::add)::push(l, "add");
    METHOD(Class::get_name)::push(l, "get_name");
    METHOD(Class::get_index)::push(l, "get_index");
    METHOD(Class::ret_table)::push(l, "ret_table");
    METHOD(Class::return_to_lua)::push(l, "return_to_lua");
    METHOD(Class::read_on_lua)::push(l, "read_on_lua");
    METHOD(Class::register_optional)::push(l, "register_optional");
    registerAttribute<int, Class, &Class::ten>(l_, "a");
    return 0;
}

int load_EmptyClass(lua_State* l)
{
    METHOD(ClassEmpty::get_class_ref)::push(l, "get_class_ref");
    METHOD(ClassEmpty::get_class_copy)::push(l, "get_class_copy");
    METHOD(ClassEmpty::get_class_point)::push(l, "get_class_point");
    METHOD(ClassEmpty::add)::push(l, "add");
    METHOD(ClassEmpty::get_name)::push(l, "get_name");
    METHOD(ClassEmpty::get_index)::push(l, "get_index");
    return 0;
}

int load_StaticClass(lua_State* l)
{
	//Macro that register a static member function
    STATICMETHOD(StaticClass::get_ten)::push(l, "get_ten");
    STATICMETHOD(StaticClass::static_sum)::push(l, "static_sum");
    STATICMETHOD(StaticClass::static_table)::push(l, "static_table");
    return 0;
}

int load_S_Class(lua_State* l)
{	
    METHOD(StaticClass::sum)::push(l, "sum");
    return 0;
}

/*
#############################################
The functions that register the classes into modules (This allows the user to load modules in Lua like NodeJS modules)
#############################################
*/
int load_module(lua_State* l)
{
	//Function that register classes
    registerClass<Class, std::string, int>(l, load_Class,"Class");
    registerClass<ClassEmpty>(l, load_EmptyClass,"ClassEmpty");
    registerClass<StaticClass>(l, load_S_Class, load_StaticClass, "StaticClass");
    registerClass<Base>(l, load_base, "Base");
    registerClassInherit<Derive>(l, load_derive, "Derive", "Base");
	registerClassInherit<DeriveModule>(l, load_derive_two, "DeriveModule", "Module2.BaseModule");
	//Macro that register a function into the module (C function available after the module is loaded in lua)
    MODULEFUNCTION(test_void_CFuncParam)::push(l_, "test_void_CFuncParam");
    MODULEFUNCTION(test_CFunctionA)::push(l_, "test_CFunctionA");
    MODULEFUNCTION(test_CFunction)::push(l_, "test_CFunction");
    MODULEFUNCTION(test_void_CFunctionA)::push(l_, "test_void_CFunctionA");
    MODULEFUNCTION(test_void_CFunction)::push(l_, "test_void_CFunction");
    MODULEFUNCTION(Cfunc_with_table)::push(l_, "Cfunc_with_table");
}

int load_module_two(lua_State* l)
{
    registerClass<BaseModule>(l, load_base_two, "BaseModule");
}

/*
#############################################
Load the lua lib and the modules
#############################################
*/
LUALIB_API void openlib (lua_State *l) {
    const luaL_Reg *lib;
    /* call open functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++)
    {
        luaL_requiref(l, lib->name, lib->func, 1);
        lua_pop(l, 1);  /* remove lib */
    }


    //Add  C++ classes to the preload table
    luaL_getsubtable(l, LUA_REGISTRYINDEX, "_PRELOAD");

	//Function that register the module
    registerModule<load_module_two>(l, "Module2");
    registerModule<load_module>(l, "Module");

    lua_pop(l, 1);  /* remove _PRELOAD table */
}

/*
#############################################
Here start the tests
#############################################
*/
TEST_F(RegisterTest, constructor_empty_param)
{ 
    //Test constructor
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "classEmpty = Module.ClassEmpty()");
    ASSERT_EQ(std::string("NAME"), name_empty_constructor);
}

TEST_F(RegisterTest, constructor_variadic_param)
{ 
    //Test constructor with several arguments
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    ASSERT_EQ(10, index_constructor);
    ASSERT_TRUE("Dummy" == name_constructor);
}

TEST_F(RegisterTest, register_class_with_static)
{ 
    //Test to register a class with a static member function
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "test = Module.StaticClass.get_ten()");
    luaL_dostring(l_, "test2 = Module.StaticClass.static_sum(3, 5)");
    lua_getglobal(l_, "test");
    lua_getglobal(l_, "test2");

    ASSERT_EQ(10, read<int>(l_, 1));
    ASSERT_EQ(8, read<int>(l_, 2));
}

TEST_F(RegisterTest, return_nonvoid_empty_param)
{ 
    //Test function that return something and take no argument

    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "classEmpty = Module.ClassEmpty()");
    luaL_dostring(l_, "test = classEmpty:get_name()");
    lua_getglobal(l_, "test");

    ASSERT_EQ(std::string("NAME"), read<std::string>(l_, 1));
}

TEST_F(RegisterTest, return_nonvoid_variadic_param)
{ 
    //Test function that return something and take several argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "test = class:get_index()");
    luaL_dostring(l_, "test2 = class:get_name()");

    lua_getglobal(l_, "test");
    lua_getglobal(l_, "test2");

    ASSERT_EQ(10, read<int>(l_, 1));
    ASSERT_EQ(std::string("Dummy"), read<std::string>(l_, 2));
}

TEST_F(RegisterTest, return_void_variadic_param)
{ 
    //Test function that return void and take several argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "class:add(5,4)");
    luaL_dostring(l_, "test = class:get_index()");
    lua_getglobal(l_, "test");

    ASSERT_EQ(9, read<int>(l_, 1));
}

TEST_F(RegisterTest, return_void_empty_param)
{ 
    //Test function that return void and need no argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "classEmpty = Module.ClassEmpty()");
    luaL_dostring(l_, "classEmpty:add()");
    luaL_dostring(l_, "test = classEmpty:get_index()");

    lua_getglobal(l_, "test");

    ASSERT_EQ(5, read<int>(l_, 1));
}

TEST_F(RegisterTest, Objects_parameter)
{ 
    //Test function that take userdatas as arguments
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "classEmpty = Module.ClassEmpty()");
    luaL_dostring(l_, "test_udRef = classEmpty:get_class_ref(class)");
    luaL_dostring(l_, "test_udCpy = classEmpty:get_class_copy(class)");
    luaL_dostring(l_, "test_udPt = classEmpty:get_class_point(class)");

    lua_getglobal(l_, "test_udRef");
    lua_getglobal(l_, "test_udCpy");
    lua_getglobal(l_, "test_udPt");

    ASSERT_EQ(std::string("Dummy"), read<std::string>(l_, 1));
    ASSERT_EQ(std::string("Dummy"), read<std::string>(l_, 2));
    ASSERT_EQ(std::string("Dummy"), read<std::string>(l_, 3));
}

TEST_F(RegisterTest, Function_pointer_parameter)
{
    //Test function that take userdatas as arguments
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");

    luaL_dostring(l_, "function returnFive(this) "
                  "return 5 "
                  "end");

    luaL_dostring(l_, "function sumFive(this, a) "
                  "return (5 + a) "
                  "end");
    luaL_dostring(l_, "test = class:call_func(returnFive)");
    luaL_dostring(l_, "test2 = class:call_func_sum(sumFive)");
    luaL_dostring(l_, "class:call_func_increment(Module.test_void_CFuncParam)");

    lua_getglobal(l_, "test");
    lua_getglobal(l_, "test2");

    ASSERT_EQ(5, read<int>(l_, 1));
    ASSERT_EQ(10, read<int>(l_, 2));
    ASSERT_EQ(5, void_CFuncParam);
}

TEST_F(RegisterTest, CFunction_return_nonvoid)
{
	//C function that returns something
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "test = Module.test_CFunction(nil)");
    luaL_dostring(l_, "test2 = Module.test_CFunctionA(nil, 10)");

    lua_getglobal(l_, "test");
    lua_getglobal(l_, "test2");

    ASSERT_EQ(10, read<int>(l_, 1));
    ASSERT_EQ(10, read<int>(l_, 2));
}

TEST_F(RegisterTest, CFunction_return_void)
{
	//C functio nthat returns void
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "Module.test_void_CFunction(nil)");
    luaL_dostring(l_, "Module.test_void_CFunctionA(nil,10)");
    ASSERT_EQ(10, void_CFunction);
    ASSERT_EQ(10, void_CFunctionA);
}

TEST(RegisterT, destructor)
{ 
    //Test that the destructor is called
    l_ = luaL_newstate();
    openlib(l_);
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "classEmpty = Module.ClassEmpty()");
    lua_close(l_);
    ASSERT_TRUE(destructor_called);
}

TEST_F(RegisterTest, table_parameter)
{
	//Function that takes a Table parameter
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "t = { foo = 10 } ");
    luaL_dostring(l_, "Module.Cfunc_with_table(nil, { foo = 10 } ) ");

    ASSERT_EQ(10, c_table_param);
}

TEST_F(RegisterTest, func_with_table)
{
    //Test function that return void and take several argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "class:func_with_table( { foo = 3 } )");

    ASSERT_EQ(3, table_param);
}

TEST_F(RegisterTest, DISABLED_func_with_table_ref)
{
    //Test function that return void and take several argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "class:func_with_table_ref( { foo = 3 } )");

    ASSERT_EQ(3, table_param_ref);
}

TEST_F(RegisterTest, func_return_table)
{
    //Test function that return void and take several argument
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "table = Module.StaticClass.static_table(\"static_Dummy\")");
    Table table("table", l_);
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "t = class:ret_table(\"Dummy\")");
    Table table2("t", l_);
    ASSERT_EQ(std::string("Dummy"), table2.get<std::string>("key"));
    ASSERT_EQ(std::string("static_Dummy"), table.get<std::string>("key"));

}

TEST_F(RegisterTest, inheritance)
{
	//Test the inheritance with two classes in the same module
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "derive = Module.Derive()");
    luaL_dostring(l_, "derive:countDerive()");
    luaL_dostring(l_, "derive:countBase()");

    ASSERT_EQ(10, parent_count);
    ASSERT_EQ(7, derive_count);
}

TEST_F(RegisterTest, inheritance_through_module)
{
	//Test the inheritance with two classes in two differents modules
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "derive = Module.DeriveModule()");
    luaL_dostring(l_, "derive:countDerive()");
    luaL_dostring(l_, "derive:countBase()");

    ASSERT_EQ(10, parent_count);
    ASSERT_EQ(7, derive_count);
}

TEST_F(RegisterTest, callFunctionWithLua)
{
	//Test a simple member function from lua.
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "result = class:return_to_lua()");

    lua_getglobal(l_, "result");
    ASSERT_EQ(10, read<int>(l_, 1));
    luaL_dostring(l_, "class:read_on_lua(5)");
    ASSERT_EQ(5, read_on_lua_value);
}

TEST_F(RegisterTest, register_optional)
{
	//Test the function overload
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");
    luaL_dostring(l_, "class:register_optional(9)");
    ASSERT_EQ(9, register_optional_int);
    luaL_dostring(l_, "class:register_optional()");
    ASSERT_EQ(1, register_optional_int);
}

TEST_F(RegisterTest, attribute)
{
	//Test the public attribute
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "class = Module.Class(\"Dummy\", 10)");

    luaL_dostring(l_, "a = class.a");
    lua_getglobal(l_, "a");
    ASSERT_EQ(10, read<int>(l_, 1));

    luaL_dostring(l_, "class.a = 8");
    luaL_dostring(l_, "a = class.a");
    lua_getglobal(l_, "a");
    ASSERT_EQ(8, read<int>(l_, 2));
}


TEST_F(RegisterTest, attribute_inherited)
{
    //Test the public attribute inheritance
    luaL_dostring(l_, "Module = require(\"Module\")");
    luaL_dostring(l_, "derive = Module.DeriveModule()");
    luaL_dostring(l_, "a = derive.a");
    luaL_dostring(l_, "derive.a = 8");
    luaL_dostring(l_, "b = derive.a");
    lua_getglobal(l_, "a");
    lua_getglobal(l_, "b");
    ASSERT_EQ(10, read<int>(l_, 1));
    ASSERT_EQ(8, read<int>(l_, 2));
}

