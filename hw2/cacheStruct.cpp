#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <cmath>
#include <algorithm>

// Global variables to indicate eviction in L2
extern unsigned long int evictedAddressFromL2;
extern bool evictionFlag;

// Base class for cache simulation
class Cache {
public:
    // Constructor to initialize the cache parameters
    Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : MemCyc(MemCyc), BSizeBits(BSizeBits), SizeBits(SizeBits), AssocBits(AssocBits), Cyc(Cyc), WrAlloc(WrAlloc), hits(0), misses(0) {
        unsigned numWays = 1 << AssocBits;
        unsigned cacheSize = 1 << SizeBits;
        unsigned blockSize = 1 << BSizeBits;
        numSets = cacheSize / (numWays * blockSize);
        cache.resize(numSets);
    }

    // Structure to represent a cache line
    struct CacheLine {
        unsigned long int tag;  // Tag of the cache line
        bool valid;  // Validity of the cache line
        bool operator==(const CacheLine& other) const {
            return tag == other.tag && valid == other.valid;
        }
    };

    virtual ~Cache() = default;

    // Calculate the miss rate for the cache
    double hitMissCalculator() const {
        if (hits + misses == 0) return 0.0;
        return static_cast<double>(misses) / (hits + misses);
    }

    // Get the access time for the cache
    virtual double getAccessTime() const {
        return static_cast<double>(Cyc);
    }

    // Pure virtual functions for reading and writing to the cache
    virtual void read(unsigned long int address) = 0;
    virtual void write(unsigned long int address) = 0;
    virtual void evict(unsigned long int address) = 0;  // Pure virtual function for eviction
    virtual void evictAndAdd(std::list<CacheLine>& set, unsigned long int tag) = 0;

protected:
    int hits;  // Number of cache hits
    int misses;  // Number of cache misses
    unsigned MemCyc;  // Memory access cycle time
    unsigned BSizeBits;  // Block size in bits
    unsigned SizeBits;  // Cache size in bits
    unsigned AssocBits;  // Associativity in bits
    unsigned Cyc;  // Cache access cycle time
    unsigned WrAlloc;  // Write allocate policy
    unsigned numSets;  // Number of sets in the cache


    // Vector of lists to represent the cache sets and their lines
    std::vector<std::list<CacheLine>> cache;

    // Calculate the index from the address
    unsigned getIndex(unsigned long int address) {
        return (address >> BSizeBits) & (numSets - 1);
    }

    // Calculate the tag from the address
    unsigned long int getTag(unsigned long int address) {
        return address >> (BSizeBits + static_cast<unsigned>(std::log2(numSets)));
    }

    // Update the LRU order for a cache set
    void updateLRU(std::list<CacheLine>& set, std::list<CacheLine>::iterator it) {
        CacheLine line = *it;
        set.erase(it);
        set.push_front(line);
    }

    // Find a cache line in a set and update the LRU order if found
    bool findAndUpdate(std::list<CacheLine>& set, unsigned long int tag) {
        for (auto it = set.begin(); it != set.end(); ++it) {
            if (it->tag == tag && it->valid) {
                updateLRU(set, it);
                return true;
            }
        }
        return false;
    }
};

// L1 cache class derived from the base Cache class
class L1Cache : public Cache {
public:
    L1Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : Cache(MemCyc, BSizeBits, SizeBits, AssocBits, Cyc, WrAlloc) {}

    // Read from the L1 cache
    void read(unsigned long int address) override {
        // Check for L2 eviction flag
        if (evictionFlag) {
            evict(evictedAddressFromL2);
            evictionFlag = false; // Reset the flag
        }

        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);
        std::cout << "l1,r, Set number: " << index << std::endl;
        std::cout << "l1,r, Tag number: " << tag << std::endl;
        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L1Cache Read Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L1Cache Read Miss: " << address << std::endl;
            if (l2Cache != nullptr) {
                l2Cache->read(address);
            }
            evictAndAdd(cache[index], tag);
        }
    }

    // Write to the L1 cache
    void write(unsigned long int address) override {
        // Check for L2 eviction flag
        if (evictionFlag) {
            evict(evictedAddressFromL2);
            evictionFlag = false; // Reset the flag
        }

        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);
        std::cout << "l1,w, Set number: " << index << std::endl;
        std::cout << "l1,w, Tag number: " << tag << std::endl;
        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L1Cache Write Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L1Cache Write Miss: " << address << std::endl;
            if (l2Cache != nullptr) {
                l2Cache->write(address);
            }
            if (WrAlloc) { // WrAlloc == 1 (Write allocate)
                evictAndAdd(cache[index], tag);
            }
        }
    }

    // Evict a cache line from L1
    void evict(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

        auto it = std::find_if(cache[index].begin(), cache[index].end(), [tag](const CacheLine& line) {
            return line.tag == tag && line.valid;
        });

        if (it != cache[index].end()) {
            cache[index].erase(it);
            std::cout << "L1Cache Evict: " << address << std::endl;
        }
    }

    // Evict and add a new cache line in L1
    void evictAndAdd(std::list<CacheLine>& set, unsigned long int tag) override {
        if (set.size() >= (1u << AssocBits)) {
            std::cout << "Evicted" << std::endl;
            set.pop_back();
        }
        CacheLine line = { tag, true };
        set.push_front(line);
    }

    // Set the L2 cache for inclusion policy
    void setL2Cache(Cache* l2) {
        l2Cache = l2;
    }

private:
    Cache* l2Cache = nullptr;  // Pointer to the L2 cache
};

// L2 cache class derived from the base Cache class
class L2Cache : public Cache {
public:
    L2Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : Cache(MemCyc, BSizeBits, SizeBits, AssocBits, Cyc, WrAlloc) {}

    // Read from the L2 cache
    void read(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);
        std::cout << "l2,r, Set number: " << index << std::endl;
        std::cout << "l2,r, Tag number: " << tag << std::endl;

        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L2Cache Read Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L2Cache Read Miss: " << address << std::endl;
            std::cout << "Fetch from main memory: " << address << std::endl;
            evictAndAdd(cache[index], tag);
        }
    }

    // Write to the L2 cache
    void write(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);
        std::cout << "l2,w, Set number: " << index << std::endl;
        std::cout << "l2,w, Tag number: " << tag << std::endl;

        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L2Cache Write Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L2Cache Write Miss: " << address << std::endl;
            if (WrAlloc) { // WrAlloc == 1 (Write allocate)
                evictAndAdd(cache[index], tag);
            }
        }
    }

    // Evict a cache line from L2
    void evict(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

        auto it = std::find_if(cache[index].begin(), cache[index].end(), [tag](const CacheLine& line) {
            return line.tag == tag && line.valid;
        });

        if (it != cache[index].end()) {
            cache[index].erase(it);
            std::cout << "L2Cache Evict: " << address << std::endl;
        }
    }

    // Evict and add a new cache line in L2
    void evictAndAdd(std::list<CacheLine>& set, unsigned long int tag) override {
        if (set.size() >= (1u << AssocBits)) {
            auto evictedLine = set.back();
            if (evictedLine.valid) {
                evictionFlag = true;
                evictedAddressFromL2 = (evictedLine.tag << (BSizeBits + static_cast<unsigned>(std::log2(numSets)))) + (std::distance(cache.begin(), std::find(cache.begin(), cache.end(), set)));
            }
            set.pop_back();
        }
        CacheLine line = { tag, true };
        set.push_front(line);
    }

    // Set the L1 cache for inclusion policy
    void setL1Cache(Cache* l1) {
        l1Cache = l1;
    }

private:
    Cache* l1Cache = nullptr;  // Pointer to the L1 cache for inclusion policy
};
