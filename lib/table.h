#ifndef TABLE_H
#define TABLE_H

/**
 * Copyright (c) 2014 Domora
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

extern "C" {
#include "lua5.2/lua.h"
#include "lua5.2/lualib.h"
#include "lua5.2/lauxlib.h"
}

/** This class defines a container similar to the Lua tables. The C++ 
 *  is only an interface used to reach the real table that exists in Lua.
 *  */

#include "read_and_write.h"

 class Table
 {
  public:    

     Table(lua_State* l, bool use_stack=false);

     Table(lua_State* l, const int ref);

     Table(const std::string name, lua_State* l);

     Table(const Table& t);

     Table& operator=(const Table& t);

     ~Table();

     void unref();

     /**
      * \param 	key key used to reach the value in the table, use "." to
      *          concatenate the path into nested tables.
      * \param 	value value to write.
      * \author 	Stud
      * \brief 	Recursively look for the place of "key" in the table and
      *          write the value. If some nested table are missing, it adds
      *          them.
      */

     template<typename T>
     void set(const std::string& key, T value);

     /**
      * \param 	key key used to reach the value in the table, use "." to
      *          concatenate the path into nested tables.
      * \author 	Stud
      * \brief 	Recursively look for the place of "key" in the table and
      *          return the value.
      */
     template<typename U>
     U get(const std::string& key);

     /**
      * \param 	key index of the value in the current table.
      * \author 	Stud
      * \brief 	Return the value at the given index.
      */
     template<typename U>
     U get(const int key);

     /**
      * \param 	key index of the value in the current table.
      * \param 	value the value.
      * \author 	Stud
      * \brief 	Modify or add the value at the given index
      */
     template<typename U>
     void set(const int key, U value);

     int get_size() const;

     void load_table() const;

     template<typename T>
     bool is_number(const T key);

     template<typename T>
     bool is_string(const T key);

     template<typename T>
     bool is_nil(const T key);

     template<typename T>
     bool is_table(const T key);

     template<typename T>
     bool is_function(const T key);

     template<typename T>
     bool is_userdata(const T key);

 private:

	 const int KEY;
	 const int VAL;

     void load_value(const std::string& key);

     void load_value(const int key);

     void copy(const Table& t);

     /**
      * \param 	key key used to reach the value in the table, use "." to
      *          concatenate the path into nested tables.
      * \author 	Stud
      * \brief 	Recursively look for the place of "key" in the table and
      *          return the value.
      */
     template<typename U>
     U get(const std::string& key, const std::string& table_name);

     /**
      * \param 	key key used to reach the value in the table.
      * \author 	Stud
      * \brief 	The last recursion when looking for a value in the table.
      *          Returns the value.
      */
     template<typename U>
     U get_value(const std::string& key);

     /**
      * \param 	key key used to reach the value in the table, use "." to
      *          concatenate the path into nested tables.
      * \param 	value value to write.
      * \author 	Stud
      * \brief 	Recursively look for the place of "key" in the table and
      *          write the value. If some nested table are missing, it adds
      *          them.
      */
     template<typename T>
     void set(const std::string& key, T value, const std::string& table_name);

     /**
      * \param 	key key used to reach the value in the table.
      * \param 	value value to write.
      * \author 	Stud
      * \brief 	The last recursion when looking for a key in the table.
      *          Writes the value.
      */
     template<typename U>
     void set_value(const std::string& key, U value);

     int get_size_loaded() const;

     void walk();

     void copy_value();

     std::string name_;
     lua_State* l_state_;
     int ref_;
     bool global_;
 };

#include "table.tpp"
#endif
