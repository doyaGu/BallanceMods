#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <memory>
#include <array>
#include <tuple>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <deque>
#include <optional>
#include <variant>
#include <stack>
#include <queue>

class Serializable {
public:
    virtual ~Serializable() = default;

    virtual bool Serialize(std::ostream &out) const = 0;
    virtual bool Deserialize(std::istream &in) = 0;

    template<typename T>
    static bool Write(std::ostream &out, const T &data) {
        return WriteImpl(out, data);
    }

    template<typename T>
    static bool Read(std::istream &in, T &data) {
        return ReadImpl(in, data);
    }

    static bool WriteString(std::ostream &out, const std::string &str) {
        size_t size = str.size();
        if (!Write(out, size)) return false;
        out.write(str.data(), size);
        return !out.fail();
    }

    static bool ReadString(std::istream &in, std::string &str) {
        size_t size;
        if (!Read(in, size)) return false;
        str.resize(size);
        in.read(&str[0], size);
        return !in.fail();
    }

    static bool WriteBytes(std::ostream &out, const uint8_t *data, size_t size) {
        out.write(reinterpret_cast<const char *>(data), size);
        return !out.fail();
    }

    static bool ReadBytes(std::istream &in, uint8_t *data, size_t size) {
        in.read(reinterpret_cast<char *>(data), size);
        return !in.fail();
    }

    template<typename T, size_t N>
    static bool WriteArray(std::ostream &out, const std::array<T, N> &arr) {
        return WriteBytes(out, reinterpret_cast<const uint8_t *>(arr.data()), N * sizeof(T));
    }

    template<typename T, size_t N>
    static bool ReadArray(std::istream &in, std::array<T, N> &arr) {
        return ReadBytes(in, reinterpret_cast<uint8_t *>(arr.data()), N * sizeof(T));
    }

    static bool WriteDynamicBuffer(std::ostream &out, const std::unique_ptr<uint8_t[]> &buffer, size_t size) {
        return WriteBytes(out, buffer.get(), size);
    }

    static bool ReadDynamicBuffer(std::istream &in, std::unique_ptr<uint8_t[]> &buffer, size_t size) {
        buffer = std::make_unique<uint8_t[]>(size);
        return ReadBytes(in, buffer.get(), size);
    }

    template<typename... Args>
    static bool WriteTuple(std::ostream &out, const std::tuple<Args...> &tuple) {
        return WriteTupleImpl(out, tuple, std::index_sequence_for<Args...>{});
    }

    template<typename... Args>
    static bool ReadTuple(std::istream &in, std::tuple<Args...> &tuple) {
        return ReadTupleImpl(in, tuple, std::index_sequence_for<Args...>{});
    }

    template<typename K, typename V>
    static bool WriteMap(std::ostream &out, const std::map<K, V> &map) {
        const size_t size = map.size();
        if (!Write(out, size))
            return false;
        for (const auto &[key, value]: map) {
            if (!Write(out, key) || !Write(out, value)) {
                return false;
            }
        }
        return true;
    }

    template<typename K, typename V>
    static bool WriteMap(std::ostream &out, const std::unordered_map<K, V> &map) {
        const size_t size = map.size();
        if (!Write(out, size))
            return false;
        for (const auto &[key, value]: map) {
            if (!Write(out, key) || !Write(out, value)) {
                return false;
            }
        }
        return true;
    }

    template<typename K, typename V>
    static bool ReadMap(std::istream &in, std::map<K, V> &map) {
        size_t size;
        if (!Read(in, size))
            return false;
        map.clear();
        for (size_t i = 0; i < size; ++i) {
            K key;
            V value;
            if (!Read(in, key) || !Read(in, value)) {
                return false;
            }
            map.emplace(std::move(key), std::move(value));
        }
        return true;
    }

    template<typename K, typename V>
    static bool ReadMap(std::istream &in, std::unordered_map<K, V> &map) {
        size_t size;
        if (!Read(in, size))
            return false;
        map.clear();
        for (size_t i = 0; i < size; ++i) {
            K key;
            V value;
            if (!Read(in, key) || !Read(in, value)) {
                return false;
            }
            map.emplace(std::move(key), std::move(value));
        }
        return true;
    }

    template<typename T>
    static bool WriteSet(std::ostream &out, const std::set<T> &set) {
        size_t size = set.size();
        if (!Write(out, size)) return false;
        for (const auto &value: set) {
            if (!Write(out, value)) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    static bool ReadSet(std::istream &in, std::set<T> &set) {
        size_t size;
        if (!Read(in, size)) return false;
        set.clear();
        for (size_t i = 0; i < size; ++i) {
            T value;
            if (!Read(in, value)) {
                return false;
            }
            set.emplace(std::move(value));
        }
        return true;
    }

    template<typename T>
    static bool WriteSet(std::ostream &out, const std::unordered_set<T> &set) {
        size_t size = set.size();
        if (!Write(out, size)) return false;
        for (const auto &value: set) {
            if (!Write(out, value)) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    static bool ReadSet(std::istream &in, std::unordered_set<T> &set) {
        size_t size;
        if (!Read(in, size)) return false;
        set.clear();
        for (size_t i = 0; i < size; ++i) {
            T value;
            if (!Read(in, value)) {
                return false;
            }
            set.emplace(std::move(value));
        }
        return true;
    }

    template<typename T>
    static bool WriteVector(std::ostream &out, const std::vector<T> &vec) {
        const size_t size = vec.size();
        return Write(out, size) && WriteBytes(out, reinterpret_cast<const uint8_t *>(vec.data()), size * sizeof(T));
    }

    template<typename T>
    static bool ReadVector(std::istream &in, std::vector<T> &vec) {
        size_t size;
        if (!Read(in, size))
            return false;
        vec.resize(size);
        return ReadBytes(in, reinterpret_cast<uint8_t *>(vec.data()), size * sizeof(T));
    }

    template<typename T>
    static bool WriteDeque(std::ostream &out, const std::deque<T> &deq) {
        const size_t size = deq.size();
        if (!Write(out, size))
            return false;
        for (const auto &item: deq) {
            if (!Write(out, item))
                return false;
        }
        return true;
    }

    template<typename T>
    static bool ReadDeque(std::istream &in, std::deque<T> &deq) {
        size_t size;
        if (!Read(in, size))
            return false;
        deq.clear();
        for (size_t i = 0; i < size; ++i) {
            T item;
            if (!Read(in, item))
                return false;
            deq.push_back(std::move(item));
        }
        return true;
    }

    template<typename T>
    static bool WriteStack(std::ostream &out, const std::stack<T> &stack) {
        std::stack<T> temp = stack; // Copy stack for serialization
        std::vector<T> reversed;
        while (!temp.empty()) {
            reversed.push_back(temp.top());
            temp.pop();
        }
        return WriteVector(out, reversed);
    }

    template<typename T>
    static bool ReadStack(std::istream &in, std::stack<T> &stack) {
        std::vector<T> reversed;
        if (!ReadVector(in, reversed))
            return false;
        for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
            stack.push(*it);
        }
        return true;
    }

    template<typename T>
    static bool WriteQueue(std::ostream &out, const std::queue<T> &queue) {
        std::queue<T> temp = queue; // Copy queue for serialization
        std::vector<T> items;
        while (!temp.empty()) {
            items.push_back(temp.front());
            temp.pop();
        }
        return WriteVector(out, items);
    }

    template<typename T>
    static bool ReadQueue(std::istream &in, std::queue<T> &queue) {
        std::vector<T> items;
        if (!ReadVector(in, items))
            return false;
        for (const auto &item: items) {
            queue.push(item);
        }
        return true;
    }

    template<typename T>
    static bool WriteOptional(std::ostream &out, const std::optional<T> &opt) {
        const bool has_value = opt.has_value();
        if (!Write(out, has_value))
            return false;
        if (has_value)
            return Write(out, *opt);
        return true;
    }

    template<typename T>
    static bool ReadOptional(std::istream &in, std::optional<T> &opt) {
        bool has_value;
        if (!Read(in, has_value))
            return false;
        if (has_value) {
            T value;
            if (!Read(in, value))
                return false;
            opt = std::move(value);
        } else {
            opt.reset();
        }
        return true;
    }

    template<typename... Ts>
    static bool WriteVariant(std::ostream &out, const std::variant<Ts...> &var) {
        const size_t index = var.index();
        if (!Write(out, index))
            return false;
        return std::visit([&](const auto &val) { return Write(out, val); }, var);
    }

    template<typename... Ts>
    static bool ReadVariant(std::istream &in, std::variant<Ts...> &var) {
        size_t index;
        if (!Read(in, index))
            return false;
        var = std::variant<Ts...>{};
        if (index >= sizeof...(Ts))
            return false;
        return ReadVariantImpl(in, var, index, std::make_index_sequence<sizeof...(Ts)>{});
    }

protected:
    // Partial specialization for trivial types
    template<typename T>
    struct SerializationTraits {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable for serialization.");

        static bool Write(std::ostream &out, const T &data) {
            out.write(reinterpret_cast<const char *>(&data), sizeof(T));
            return !out.fail();
        }

        static bool Read(std::istream &in, T &data) {
            in.read(reinterpret_cast<char *>(&data), sizeof(T));
            return !in.fail();
        }
    };

    template<typename T>
    static bool WriteImpl(std::ostream &out, const T &data) {
        return SerializationTraits<T>::Write(out, data);
    }

    template<typename T>
    static bool ReadImpl(std::istream &in, T &data) {
        return SerializationTraits<T>::Read(in, data);
    }

    template<typename... Args, size_t... Index>
    static bool WriteTupleImpl(std::ostream &out, const std::tuple<Args...> &tuple, std::index_sequence<Index...>) {
        return (... && Write(out, std::get<Index>(tuple)));
    }

    template<typename... Args, size_t... Index>
    static bool ReadTupleImpl(std::istream &in, std::tuple<Args...> &tuple, std::index_sequence<Index...>) {
        return (... && Read(in, std::get<Index>(tuple)));
    }

    template<typename... Ts, size_t... Is>
    static bool ReadVariantImpl(std::istream &in, std::variant<Ts...> &var, size_t index, std::index_sequence<Is...>) {
        return ((index == Is && Read(in, var.template emplace<Is>())) || ...);
    }
};

#endif // SERIALIZABLE_H
