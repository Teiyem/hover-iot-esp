#pragma once

#include <mutex>
#include <type_traits>

/**
 * A factory class for creating components.
 */
class iot_factory final
{
public:

    /**
     * Creates and returns a singleton instance of a default-constructible component of type T.
     * @tparam T The type of the component to create.
     * @return A reference to the singleton instance of the component.
     */
    template<typename T>
    static typename std::enable_if<std::is_default_constructible<T>::value, T &>::type create_component() {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        static T instance;
        return instance;
    }
};