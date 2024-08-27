#pragma once

#include <mutex>
#include <type_traits>
#include <memory>

/**
 * A factory class for creating components.
 */
class IotFactory final
{
public:

    /**
     * Creates and returns a singleton instance of a default-constructible component of type T.
     *
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

    /**
     * Creates and returns a singleton instance of a default-constructible component of type T.
     *
     * @tparam T The type of the component to create.
     * @tparam Args The types of the arguments to pass to the constructor of T.
     *
     * @param args Arguments to pass to the constructor of T.
     * @return A reference to the singleton instance of the component.
     */
    template<typename T, typename... Args>
    static T& create_component(Args&&... args) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        static T instance(std::forward<Args>(args)...);
        return instance;
    }

    /**
     * Creates an object of type T and returns a unique pointer.
     *
     * @tparam T The type of object to create.
     * @tparam Args The types of the arguments to pass to the constructor of T.
     *
     * @param args Arguments to pass to the constructor of T.
     * @return std::unique_ptr<T> A unique pointer to the newly created object of type T.
     *
     * @note The object will be automatically destroyed when the returned unique pointer goes out of scope.
     */
    template <typename T, typename... Args>
    static std::unique_ptr<T> create_scoped(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

};