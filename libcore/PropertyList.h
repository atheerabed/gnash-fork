// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
//   Foundation, Inc
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef GNASH_PROPERTYLIST_H
#define GNASH_PROPERTYLIST_H

#include "Property.h" // for templated functions

#include <set> 
#include <map> 
#include <string> // for use within map 
#include <cassert> // for inlines
#include <utility> // for std::pair
#include <boost/cstdint.hpp> 
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/noncopyable.hpp>

// Forward declaration
namespace gnash {
    class as_object;
    class as_environment;
    class as_function;
    class ObjectURI;
    class as_value;
}

namespace gnash {

/// Set of properties associated with an ActionScript object.
//
/// The PropertyList container is the sole owner of the Property
/// elements in it contained and has full responsibility of their
/// construction and destruction.
//
/// A PropertyList holds a reference to the as_object whose properties it
/// contains. This reference will always be valid if the PropertyList
/// is a member of as_object.
//
/// It is theoretically possible for a PropertyList to be used with any
/// as_object, not just original as_object it was use with. Currently (as
/// there is no use for this scenario) it is not possible to change the
/// owner.
class PropertyList : boost::noncopyable
{
public:

    typedef std::set<ObjectURI> PropertyTracker;

    /// A tag for identifying an index in the container.
    struct OrderTag {};

    /// The actual container
    /// index 0 is the fully indexed name/namespace pairs, which are unique
    /// Because of the way searching works, this index can also be
    /// used to search for the names alone (composite keys are sorted
    /// lexographically, beginning with the first element specified)
    ///
    /// index 1 is an ordered sequence, and it is used for the AS3 style
    /// enumeration (which requires an order number for each property),
    /// for slot access, and for array access.
    typedef boost::multi_index_container<
        Property,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::const_mem_fun<Property,const ObjectURI&,&Property::uri>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<OrderTag>,
                boost::multi_index::const_mem_fun<Property,int,&Property::getOrder>
            >
        >
    > container;

    /// Construct the PropertyList 
    //
    /// @param obj      The as_object to which this PropertyList belongs.
    ///                 This object is not fully constructed at this stage,
    ///                 so this constructor should not do anything with it!
    PropertyList(as_object& obj)
        :
        _props(),
        _defaultOrder(0),
        _owner(obj)
    {
    }

    /// Visit properties 
    //
    /// The method will invoke the given visitor method
    /// passing it two arguments: name of the property and
    /// value of it.
    //
    /// @tparam V       The type of the visitor.
    /// @tparam U       An object that may check property values. The object's
    ///                 operator() should return false if the property is not
    ///                 acceptable.
    //
    /// @param visitor  The visitor function. It must implement the function:
    ///                     bool accept(const ObjectURI&, const as_value&);
    ///                 Scan is by enumeration order and stops when accept()
    ///                 returns false.
    template <class U, class V>
    void visitValues(V& visitor, U cmp = U()) const
    {
        typedef container::nth_index<1>::type ContainerByOrder;

        // The template keyword is not required by the Standard here, but the
        // OpenBSD compiler needs it. Use of the template keyword where it is
        // not necessary is not an error.
        for (ContainerByOrder::const_reverse_iterator
                it = _props.template get<1>().rbegin(),
                ie = _props.template get<1>().rend(); it != ie; ++it)
        {
            if (!cmp(*it)) continue;
            as_value val = it->getValue(_owner);
            if (!visitor.accept(it->uri(), val)) return;
        }
    }

    /// Enumerate all non-hidden properties to the given as_environment.
    //
    /// Follows enumeration order. Note that this enumeration does not
    /// access the values. Accessing the values can result in changes to
    /// the object if the value is a getter-setter, and key enumeration must
    /// avoid this.
    ///
    /// @param donelist     Don't enumerate properties in donelist.
    ///                     Enumerated properties are added to donelist.
    void enumerateKeys(as_environment& env, PropertyTracker& donelist) const;

    /// Get the order number just after the passed order number.
    ///
    /// @param order    0 is a special value indicating the first order
    ///                 should be returned, otherwise, this should be the
    ///                 result of a previous call to getOrderAfter
    /// @return         A value which can be used for ordered access. 
    const Property* getOrderAfter(int order);

    /// Set the value of a property, creating a new one if it doesn't exist.
    //
    /// If the named property is a getter/setter one it's setter
    /// will be invoked using the given as_object as 'this' pointer.
    /// If the property is not found a SimpleProperty will be created.
    ///
    /// @param key
    ///    Name of the property. Search is case-*sensitive*
    /// @param value
    ///    a const reference to the as_value to use for setting
    ///    or creating the property. 
    /// @param namespaceId
    ///    The namespace in which this should be entered. If 0 is given,
    ///    this will use the first value found, if it exists.
    /// @param flagsIfMissing
    ///    Flags to associate to the property if a new one is created.
    /// @return true if the value was successfully set, false
    ///         otherwise (found a read-only property, most likely).
    bool setValue(const ObjectURI& uri, const as_value& value,
            const PropFlags& flagsIfMissing = 0);

    /// Reserves a slot number for a property
    ///
    /// @param slotId
    /// The slot id to use. (Note that getOrder() on this property will return
    /// this slot number + 1 if the assignment was successful.)
    /// @param key      Name of the property.
    /// @param nsId     The namespace in which the property should be found.
    /// @return         true if the slot did not previously exist.
    bool reserveSlot(const ObjectURI& uri, boost::uint16_t slotId);

    /// Get a property if it exists.
    //
    /// @param key  Name of the property. Search is case-*sensitive*
    /// @param nsId The id of the namespace to search
    /// @return     A Property or 0, if no such property exists.
    ///             All Property objects are owned by this PropertyList. Do
    ///             not delete them.
    Property* getProperty(const ObjectURI& uri)
        const;

    /// Get a property, if existing, by order
    //
    /// @param order    The ordering id
    const Property* getPropertyByOrder(int order);
    
    /// Delete a Property, if existing and not protected from deletion.
    //
    ///
    /// @param key      Name of the property.
    /// @param nsId     Name of the namespace
    /// @return         a pair of boolean values expressing whether the property
    ///                 was found (first) and whether it was deleted (second).
    ///                 Of course a pair(false, true) would be invalid (deleted
    ///                 a non-found property!?). Valid returns are:
    ///                     - (false, false) : property not found
    ///                     - (true, false) : property protected from deletion
    ///                     - (true, true) : property successfully deleted
    std::pair<bool,bool> delProperty(const ObjectURI& uri);

    /// Add a getter/setter property, if not already existing
    //
    /// TODO: this function has far too many arguments.
    //
    /// @param key      Name of the property. Search is case-*sensitive*
    /// @param getter   A function to invoke when this property value is
    ///                 requested. 
    /// @param setter   A function to invoke when setting this property's value.
    /// @param cacheVal The value to use as a cache. If null uses any cache
    ///                 from pre-existing property with same name.
    /// @param flagsIfMissing Flags to associate to the property if a new one
    ///                       is created.
    /// @return         true if the property was successfully added, false
    ///                 otherwise.
    bool addGetterSetter(const ObjectURI& uri, as_function& getter,
        as_function* setter, const as_value& cacheVal,
        const PropFlags& flagsIfMissing = 0);

    /// Add a getter/setter property, if not already existing
    //
    /// @param key      Name of the property.
    /// @param getter   A function to invoke when this property value is
    ///                 requested.
    /// @param setter   A function to invoke when setting this property's value.
    /// @return         true if the property was successfully added, false
    ///                 otherwise.
    bool addGetterSetter(const ObjectURI& uri, as_c_function_ptr getter,
        as_c_function_ptr setter, const PropFlags& flagsIfMissing);

    /// Add a destructive getter property, if not already existant.
    //
    /// @param key      Name of the property.
    /// @param getter   A function to invoke when this property value is
    ///                 requested.
    /// @param flagsIfMissing Flags to associate to the property if a new
    ///                             one is created.
    /// @return         true if the property was successfully added.
    bool addDestructiveGetter(const ObjectURI& uri, as_function& getter,
        const PropFlags& flagsIfMissing = 0);

    /// Add a destructive getter property, if not already existant.
    ///
    /// @param key      Name of the property. Case-sensitive search.
    /// @param getter   A function to invoke when this property value is
    ///                 requested.
    ///
    /// @param flagsIfMissing   Flags to associate to the property if a new
    //                          one is created.
    /// @return         true if the property was successfully added, false
    ///                 otherwise.
    bool addDestructiveGetter(const ObjectURI& uri, as_c_function_ptr getter, 
        const PropFlags& flagsIfMissing = 0);

    /// Set the flags of a property.
    //
    /// @param key      Name of the property. Search is case-*sensitive*
    /// @param setTrue  The set of flags to set
    /// @param setFalse The set of flags to clear
    /// @return         true if the value was successfully set, false
    ///                 otherwise (either not found or protected)
    bool setFlags(const ObjectURI& uri, int setTrue, int setFalse);

    /// Set the flags of all properties.
    //
    /// @param setTrue      The set of flags to set
    /// @param setFalse     The set of flags to clear
    void setFlagsAll(int setTrue, int setFalse);

    /// Remove all entries in the container
    void clear();

    /// Return number of properties in this list
    size_t size() const {
        return _props.size();
    }

    /// Dump all members (using log_debug)
    //
    /// This does not reflect the normal enumeration order. It is sorted
    /// lexicographically by property.
    void dump();

    /// Dump all members into the given map
    //
    /// This does not reflect the normal enumeration order. It is sorted
    /// lexicographically by property.
    ///
    void dump(std::map<std::string, as_value>& to);

    /// Mark all simple properties, getters and setters
    /// as being reachable (for the GC)
    void setReachable() const;

private:

    container _props;

    boost::uint32_t _defaultOrder;
    
    as_object& _owner;

};


} // namespace gnash

#endif // GNASH_PROPERTYLIST_H
