#pragma once

#include <algorithm>
#include <string>
#include <map>

namespace particle {

// Infinite buffer for sparse data
class SparseBuffer {
public:
    typedef std::map<size_t, std::string> Segments;

    explicit SparseBuffer(int fill = 0) :
            fill_(fill) {
    }

    std::string read(size_t offs, size_t size) {
        std::string dest;
        dest.reserve(size);
        // Check if there's a segment at a lower offset that overlaps with the requested range
        auto it = seg_.lower_bound(offs);
        if (it != seg_.begin()) {
            auto prev = std::prev(it);
            if (offs < prev->first + prev->second.size()) {
                auto s = std::string_view(prev->second).substr(offs - prev->first);
                append(dest, offs, s, offs);
            }
        }
        // Read the data of the subsequent segments that overlap with the requested range
        auto end = offs + size;
        while (it != seg_.end() && end > it->first) {
            if (end < it->first + it->second.size()) {
                auto s = std::string_view(it->second).substr(0, end - it->first);
                append(dest, offs, s, it->first);
                break;
            }
            append(dest, offs, it->second, it->first);
            ++it;
        }
        // Add trailing filler bytes if necessary
        if (dest.size() < size) {
            append(dest, offs, std::string_view(), end);
        }
        return dest;
    }

    void write(size_t offs, std::string_view data) {
        if (data.empty()) {
            return;
        }
        erase(offs, data.size());
        auto it = seg_.insert({ offs, std::string(data) }).first;
        // Check if the new segment has adjacent segments it can be merged with
        if (it != seg_.begin()) {
            auto prev = std::prev(it);
            if (prev->first + prev->second.size() == it->first) {
                prev->second.append(it->second);
                seg_.erase(it);
                it = prev;
            }
        }
        auto next = std::next(it);
        if (next != seg_.end() && it->first + it->second.size() == next->first) {
            it->second.append(next->second);
            seg_.erase(next);
        }
    }

    void erase(size_t offs, size_t size) {
        if (!size) {
            return;
        }
        auto end = offs + size;
        // Check if there's a segment at a lower offset that overlaps with the requested range
        auto it = seg_.lower_bound(offs);
        if (it != seg_.begin()) {
            auto prev = std::prev(it);
            if (offs < prev->first + prev->second.size()) {
                auto s = std::move(prev->second);
                prev->second = s.substr(0, offs - prev->first);
                if (end < prev->first + s.size()) {
                    // Split the segment into two
                    seg_.insert({ end, s.substr(end - prev->first) });
                }
            }
        }
        // Update the subsequent segments that overlap with the requested range
        while (it != seg_.end() && end > it->first) {
            if (end < it->first + it->second.size()) {
                auto s = it->second.substr(end - it->first);
                seg_.erase(it);
                seg_.insert({ end, std::move(s) });
                break;
            }
            auto next = std::next(it);
            seg_.erase(it);
            it = next;
        }
    }

    size_t size() const {
        if (seg_.empty()) {
            return 0;
        }
        auto it = --seg_.end();
        return it->first + it->second.size();
    }

    const Segments& segments() const {
        return seg_;
    }

private:
    std::map<size_t, std::string> seg_;
    int fill_;

    void append(std::string& dest, size_t destOffs, std::string_view src, size_t srcOffs) const {
        size_t n = dest.size();
        if (srcOffs - destOffs > n) {
            n += srcOffs - destOffs; // Add space for filler bytes
        }
        dest.resize(src.size() + n, fill_);
        std::memcpy(dest.data() + n, src.data(), src.size());
    }
};

} // namespace particle
