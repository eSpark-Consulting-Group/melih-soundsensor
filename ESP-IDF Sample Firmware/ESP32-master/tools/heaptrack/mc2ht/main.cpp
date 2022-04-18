#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_map>

//////////////////// FORM HEAPTRACK SRC ///////////////////
/// Trace::ip_t -> ip_t -> uint32_t
/// Trace -> std::vector<ip_t>
using ip_t = uint32_t;

struct TraceEdge
{
    ip_t instructionPointer;
    // index associated to the backtrace up to this instruction pointer
    // the evaluation process can then reverse-map the index to the parent ip
    // to rebuild the backtrace from the bottom-up
    uint32_t index;
    // sorted list of children, assumed to be small
    std::vector<TraceEdge> children;
};

class TraceTree
{
public:
    void clear()
    {
        m_root.children.clear();
        m_index = 1;
    }

    /**
     * Index the data in @p trace and return the index of the last instruction
     * pointer.
     *
     * Unknown instruction pointers will be handled by the @p callback
     */
    template <typename Fun>
    uint32_t index(const std::vector<ip_t>& trace, Fun callback)
    {
        uint32_t index = 0;
        TraceEdge* parent = &m_root;
        for (int i = trace.size() - 1; i >= 0; --i) {
            const auto ip = trace[i];
            if (!ip) {
                continue;
            }
            auto it =
                std::lower_bound(parent->children.begin(), parent->children.end(), ip,
                                 [](const TraceEdge& l, const ip_t ip) { return l.instructionPointer < ip; });
            if (it == parent->children.end() || it->instructionPointer != ip) {
                index = m_index++;
                it = parent->children.insert(it, {ip, index, {}});
                callback(ip, parent->index);
            }
            index = it->index;
            parent = &(*it);
        }
        return index;
    }

private:
    TraceEdge m_root = {0, 0, {}};
    uint32_t m_index = 1;
};

///////////////////////////////////////////////////////////

void stringSplit(const std::string &s, std::vector<std::string> &parts, char delim)
{
    std::istringstream fs(s);
    std::string line;
    while (std::getline(fs, line, delim))
    {
        parts.push_back(line);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: mc2ht <in memcheck log> <out heaptrack log> <elf file>\n");
        return -1;
    }

    std::fstream fi(argv[1], std::fstream::in);
    std::fstream fo(argv[2], std::fstream::out | std::fstream::trunc);
    TraceTree tree;

    fo << "v 10000 2" << std::endl; // version
    fo << "x " << argv[3] << std::endl; // elf
    fo << "X " << argv[3] << std::endl; // cmd line
    fo << "I 1000 1000" << std::endl; // page size
    fo << "m -" << std::endl; // drop modules list
    fo << "m x 0 0 100000000" << std::endl; // add main module with all 32bit address space (temp)

    std::string line;
    uint32_t elapsedTime = 0;
    uint32_t rssCount = 0;
    uint32_t rssTime = 0;
    uint32_t rss = 0;
    std::unordered_map<uint32_t, uint32_t> rssAllocs;
    while (std::getline(fi, line))
    {
        std::vector<std::string> parts;
        stringSplit(line, parts, ' ');
        const size_t partsSize = parts.size();
        if (partsSize < 3 || parts[0].compare("MC"))
        {
            continue; // skip line
        }

        uint32_t ptr = 0;
        uint32_t ptr_new = 0;
        uint32_t size = 0;
        std::vector<uint32_t> bt;

        if (parts[2].compare("+") == 0)
        {
            if (partsSize < 5 ||
                sscanf(parts[3].c_str(), "%x", &ptr) < 1 ||
                sscanf(parts[4].c_str(), "%x", &size) < 1)
            {
                continue;
            }
            for (size_t i = 5; i < partsSize; ++i)
            {
                uint32_t btAddr;
                if (sscanf(parts[i].c_str(), "%x", &btAddr) > 0)
                {
                    bt.push_back(btAddr);
                }
            }
            uint32_t index = tree.index(bt, [&fo](ip_t ip, uint32_t index) {
                fo << "t " << std::hex << ip << ' ' << index << std::endl;
            });
            fo << "+ " << std::hex << size << ' ' << index << ' ' << ptr << std::endl;
            rssAllocs[ptr] = size;
            rss += size;
        } else
        if (parts[2].compare("-") == 0)
        {
            if (partsSize < 4 ||
                sscanf(parts[3].c_str(), "%x", &ptr) < 1)
            {
                continue;
            }
            fo << "- " << std::hex << ptr << std::endl;
            auto it = rssAllocs.find(ptr);
            if (it != rssAllocs.end())
            {
                const uint32_t size = it->second;
                rssAllocs.erase(it);
                if (rss >= size)
                {
                    rss -= size;
                }
            }
        } else
        if (parts[2].compare("*") == 0)
        {
            if (partsSize < 6 ||
                sscanf(parts[3].c_str(), "%x", &ptr) < 1 ||
                sscanf(parts[4].c_str(), "%x", &ptr_new) < 1 ||
                sscanf(parts[5].c_str(), "%x", &size) < 1)
            {
                continue;
            }
            for (size_t i = 6; i < partsSize; ++i)
            {
                uint32_t btAddr;
                if (sscanf(parts[i].c_str(), "%x", &btAddr) > 0)
                {
                    bt.push_back(btAddr);
                }
            }
            if (ptr)
            {
                fo << "- " << std::hex << ptr << std::endl;
                auto it = rssAllocs.find(ptr);
                if (it != rssAllocs.end())
                {
                    const uint32_t size = it->second;
                    rssAllocs.erase(it);
                    if (rss >= size)
                    {
                        rss -= size;
                    }
                }
            }
            uint32_t index = tree.index(bt, [&fo](ip_t ip, uint32_t index) {
                fo << "t " << std::hex << ip << ' ' << index << std::endl;
            });
            fo << "+ " << std::hex << size << ' ' << index << ' ' << ptr_new << std::endl;
            rssAllocs[ptr_new] = size;
            rss += size;
        }

        sscanf(parts[1].c_str(), "%x", &elapsedTime); // update elapsedTime
        if (rssTime + 100 < elapsedTime)
        {
            rssTime = elapsedTime;
            fo << "c " << std::dec << rssCount++ << std::endl;
            fo << "R " << std::hex << rss << std::endl;
        }
    }

    return 0;
}
