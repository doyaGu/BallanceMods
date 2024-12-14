#ifndef VECTORSTREAM_H
#define VECTORSTREAM_H

#include <cstdint>
#include <cassert>
#include <vector>
#include <streambuf>
#include <iostream>

class VectorStreamBuf : public std::streambuf {
public:
    explicit VectorStreamBuf(std::vector<uint8_t> &vec) : m_vec(vec) {
        // Initially set get area and put area to cover the entire vector
        char *base = reinterpret_cast<char *>(m_vec.data());
        std::size_t size = m_vec.size();
        setg(base, base, base + size);
        setp(base, base + size);
    }

protected:
    // Reading: If we have not reached end of buffer, return current character
    int_type underflow() override {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        return traits_type::eof();
    }

    // Writing: If we try to write beyond the end of the buffer, we need to expand the vector
    int_type overflow(int_type ch) override {
        if (ch == traits_type::eof()) {
            return traits_type::eof();
        }

        // Calculate the current offset from the start for both input and output pointers
        std::ptrdiff_t gOffset = gptr() - eback();
        std::ptrdiff_t pOffset = pptr() - pbase();

        // Expand the vector by one element for the new character
        m_vec.push_back(static_cast<uint8_t>(ch));

        // After expanding, pointers might have changed because reallocation could occur
        char *base = reinterpret_cast<char *>(m_vec.data());
        setg(base, base + gOffset, base + m_vec.size());
        setp(base, base + m_vec.size());

        // Move the put pointer forward by one (the just-written character is at position pOffset)
        pbump(pOffset + 1);

        return ch;
    }

    // Seeking support for random access:
    // seekoff: move by 'off' from a given 'way' for either input or output position
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override {
        return do_seek(off, way, which);
    }

    // seekpos: move to an absolute position 'sp'
    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which) override {
        return do_seek(sp, std::ios_base::beg, which);
    }

private:
    std::streampos do_seek(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
        // Current size
        auto size = static_cast<std::streamoff>(m_vec.size());

        // Determine reference point
        std::streamoff basePos = 0;
        if (way == std::ios_base::cur) {
            if (which & std::ios_base::in) {
                basePos = gptr() - eback();
            } else if (which & std::ios_base::out) {
                basePos = pptr() - pbase();
            }
        } else if (way == std::ios_base::end) {
            basePos = size;
        }

        std::streamoff newPos = basePos + off;
        if (newPos < 0 || newPos > size) {
            // Out of range
            return std::streampos(std::streamoff(-1));
        }

        char *base = reinterpret_cast<char *>(m_vec.data());
        if (which & std::ios_base::in) {
            setg(base, base + newPos, base + size);
        }
        if (which & std::ios_base::out) {
            setp(base, base + size);
            pbump(static_cast<int>(newPos));
        }

        return std::streampos(newPos);
    }

private:
    std::vector<uint8_t> &m_vec;
};

// A read-only vector stream (input)
class VectorInputStream : public std::istream {
public:
    VectorInputStream(const std::vector<uint8_t> &vec)
        : std::istream(nullptr), m_buf(const_cast<std::vector<uint8_t> &>(vec)) {
        rdbuf(&m_buf);
        clear();
    }

private:
    VectorStreamBuf m_buf;
};

// A write-only vector stream (output)
class VectorOutputStream : public std::ostream {
public:
    explicit VectorOutputStream(std::vector<uint8_t> &vec) : std::ostream(nullptr), m_buf(vec) {
        rdbuf(&m_buf);
        clear();
    }

private:
    VectorStreamBuf m_buf;
};

// A read/write vector stream
class VectorStream : public std::iostream {
public:
    explicit VectorStream(std::vector<uint8_t> &vec) : std::iostream(nullptr), m_buf(vec) {
        rdbuf(&m_buf);
        clear();
    }

private:
    VectorStreamBuf m_buf;
};

#endif // VECTORSTREAM_H
