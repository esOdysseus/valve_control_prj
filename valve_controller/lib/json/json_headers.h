#ifndef _C_JSON_HEADERS_SETTING_H_
#define _C_JSON_HEADERS_SETTING_H_

#ifdef JSON_LIB_RAPIDJSON
    // mode-flag for annotation-allow json-format.
    #define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseCommentsFlag

    #include <rapidjson/document.h>
    #include <rapidjson/stringbuffer.h>
    #include <rapidjson/writer.h>

    namespace json_mng
    {
        /** for Json Parser. */
        using JsonManipulator = rapidjson::Document;
        /** for Object. */
        using Object_Type = rapidjson::Value::Object;
        /** for Value. */
        using Value_Type = rapidjson::Value;
        /** for Array-values. */
        using ValueIterator = rapidjson::Value::ValueIterator;
        /** for Object-members. */
        using MemberIterator = rapidjson::Value::MemberIterator;

        using OutputBuffer = rapidjson::StringBuffer;

        #define ValueDef_Flag rapidjson::kObjectType

        // template <typename Encoding, typename Allocator> 
        // using first = rapidjson::GenericMember<Encoding, Allocator>::GenericValue<Encoding, Allocator> name;
        // template <typename Encoding, typename Allocator> 
        // using second = rapidjson::GenericMember<Encoding, Allocator>::GenericValue<Encoding, Allocator> value;
    }
    
#elif JSON_LIB_HLOHMANN
    // TODO
#else
    #error "Select Json-library: \'JSON_LIB_RAPIDJSON\', \'JSON_LIB_HLOHMANN\'."
#endif

#endif // _C_JSON_HEADERS_SETTING_H_
