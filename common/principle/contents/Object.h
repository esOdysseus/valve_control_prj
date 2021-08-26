#ifndef _H_INTERFACE_TYPE_AND_OBJECT_CLASS_
#define _H_INTERFACE_TYPE_AND_OBJECT_CLASS_

#include <map>
#include <string>
#include <memory>
#include <type_traits>


namespace base {


/*****************
 * IType Virtual-Class for mapper with multi-type value.
 */
class IType {
public:
    IType(void) = default;

    virtual ~IType(void) = default;

};


/*****************
 * Object Class
 */
template<typename T>
class Object: public IType {
public:
    Object(void) = default;

    ~Object(void) = default;

    static T& get(std::shared_ptr<IType>& inf) {
        if( inf.get() == NULL ) {
            throw std::out_of_range("Object is not created.");
        }

        return std::dynamic_pointer_cast<Object<T>>(inf)->object;
    }

public:
    T object;

};


}   // base

#endif // _H_INTERFACE_TYPE_AND_OBJECT_CLASS_