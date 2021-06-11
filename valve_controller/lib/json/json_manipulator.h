/***
 * json_manipulator.h
 * Copyright [2019-] 
 * Written by EunSeok Kim <es.odysseus@gmail.com>
 * 
 * This file is part of the Common-Communicator framework.
 */
#ifndef _C_JSON_MANIPULATOR_H_
#define _C_JSON_MANIPULATOR_H_

#include <list>
#include <cassert>
#include <memory>
#include <string>
#include <sstream>

#include <json_headers.h>

class CRawMessage;

namespace json_mng
{
    typedef enum E_ERROR {
        E_NO_ERROR = 0,
        E_HAS_NOT_MEMBER = 1,
        E_ITS_NOT_ARRAY = 2,
        E_ITS_NOT_SUPPORTED_TYPE = 3,
        E_INVALID_VALUE = 4
    }E_ERROR;

    typedef enum E_PARSE {
        E_PARSE_NONE = 0,
        E_PARSE_FILE = 1,
        E_PARSE_MESSAGE = 2,
    } E_PARSE;

    class CMjson {
    public:
        CMjson(void);

        CMjson(Object_Type value);

        ~CMjson(void);

        bool is_there(void);

        bool parse(std::string input_data, const E_PARSE arg_type=E_PARSE::E_PARSE_FILE);

        bool parse(const char* input_data, const ssize_t input_size);

        const char* print_buf(void);

        MemberIterator begin(void);

        MemberIterator end(void);

        static std::string get_first(MemberIterator itor);

        template <typename T=std::string>
        static T get_second(MemberIterator itor);

        template <typename T=uint32_t>
        static T get_second_hex(MemberIterator itor) {
            std::string text;
            T result;
            // get data as string.
            text = get_second<std::string>(itor);
            assert( text.empty() != true );
            // convert to T-type.
            std::stringstream convert(text);
            convert >> std::hex >> result;
            return result;
        }

        template <typename T=std::string>
        T get_member(std::string key) {
            validation_check(key);
            return get<T>(key);
        }

        template <typename T=uint32_t>
        T get_member_hex(std::string key) {
            std::string text;
            T result;
            // get data as string.
            validation_check(key);
            text = get<std::string>(key);
            assert( text.empty() != true );
            // convert to T-type.
            std::stringstream convert(text);
            convert >> std::hex >> result;
            return result;
        }

        template <typename T=std::string>
        std::shared_ptr<std::list<std::shared_ptr<T>>> get_array_member(std::string key) {
            validation_check(key);
            return get_array<T>(key);
        }

        std::shared_ptr<CMjson> set_member(std::string key, CMjson *value = NULL) {
            return update(key, value);
        }

        template <typename T=std::string>
        bool set_member(std::string key, T value) {
            return update<T>(key, value);
        }

    private:
        std::shared_ptr<CRawMessage> file_read(std::string &json_file_path);

        void validation_check(std::string& key);

        bool parse(std::shared_ptr<CRawMessage>& msg);

        bool parse(const char* msg_const);

        bool has_member(std::string &key);

        bool is_array(std::string &key);

        void is_array_check(std::string &key);

        std::shared_ptr<Value_Type> get_object(void);

        // get
        template <typename T=std::string>
        std::shared_ptr<std::list<std::shared_ptr<T>>> get_array(std::string &key);

        template <typename T>
        std::shared_ptr<T> get(ValueIterator itr);

        template <typename T>
        T get(std::string &key);

        template <typename T>
        static T get_data(const char* data);

        MemberIterator get_begin_member(void);

        MemberIterator get_end_member(void);

        static std::string get_first_member(MemberIterator itor);

        // set
        std::shared_ptr<CMjson> update(std::string &key, CMjson *value);

        template <typename T>
        bool update(std::string &key, T value);

        bool update_value(std::string &key, Value_Type &value);

    private:
        bool is_parsed;

        static const unsigned int read_bufsize = 1024;

        char read_buf[read_bufsize];

        JsonManipulator manipulator;

        std::shared_ptr<Value_Type> object;

        OutputBuffer object_buf;

    };
}

using Json_DataType = std::shared_ptr<json_mng::CMjson>;

#endif // _C_JSON_MANIPULATOR_H_