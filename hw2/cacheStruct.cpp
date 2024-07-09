#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <cmath>

class Cache {
public:
    Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : MemCyc(MemCyc), BSizeBits(BSizeBits), SizeBits(SizeBits), AssocBits(AssocBits), Cyc(Cyc), WrAlloc(WrAlloc), hits(0), misses(0) {
        unsigned numWays = 1 << AssocBits;
        unsigned cacheSize = 1 << SizeBits;
        unsigned blockSize = 1 << BSizeBits;
        numSets = cacheSize / (numWays * blockSize);
        cache.resize(numSets);
    }

    virtual ~Cache() = default;

    double hitMissCalculator() const {
        if (hits + misses == 0) return 0.0;
        return static_cast<double>(misses) / (hits + misses);
    }

    virtual double getAccessTime() const {
        return static_cast<double>(Cyc);
    }

    virtual void read(unsigned long int address) = 0;  // Pure virtual function for read
    virtual void write(unsigned long int address) = 0; // Pure virtual function for write

protected:
    int hits;
    int misses;
    unsigned MemCyc;
    unsigned BSizeBits;
    unsigned SizeBits;
    unsigned AssocBits;
    unsigned Cyc;
    unsigned WrAlloc;
    unsigned numSets;

    struct CacheLine {
        unsigned long int tag;
        bool valid;
    };

    std::vector<std::list<CacheLine>> cache;

    unsigned getIndex(unsigned long int address) {
        return (address >> BSizeBits) & (numSets - 1);
    }

    unsigned long int getTag(unsigned long int address) {
        return address >> (BSizeBits + static_cast<unsigned>(std::log2(numSets)));
    }

    void updateLRU(std::list<CacheLine>& set, std::list<CacheLine>::iterator it) {
        CacheLine line = *it;
        set.erase(it);
        set.push_front(line);
    }

    bool findAndUpdate(std::list<CacheLine>& set, unsigned long int tag) {
        for (auto it = set.begin(); it != set.end(); ++it) {
            if (it->tag == tag && it->valid) {
                updateLRU(set, it);
                return true;
            }
        }
        return false;
    }

    void evictAndAdd(std::list<CacheLine>& set, unsigned long int tag) {
        if (set.size() >= (1u << AssocBits)) {
            set.pop_back();
        }
        CacheLine line = { tag, true };
        set.push_front(line);
    }
};

class L1Cache : public Cache {
public:
    L1Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : Cache(MemCyc, BSizeBits, SizeBits, AssocBits, Cyc, WrAlloc) {}

    void read(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

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

    void write(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L1Cache Write Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L1Cache Write Miss: " << address << std::endl;
            if (l2Cache != nullptr) {
                l2Cache->write(address);
            }
            evictAndAdd(cache[index], tag);
        }
    }

    void setL2Cache(Cache* l2) {
        l2Cache = l2;
    }

private:
    Cache* l2Cache = nullptr;
};

class L2Cache : public Cache {
public:
    L2Cache(unsigned MemCyc, unsigned BSizeBits, unsigned SizeBits, unsigned AssocBits, unsigned Cyc, unsigned WrAlloc)
        : Cache(MemCyc, BSizeBits, SizeBits, AssocBits, Cyc, WrAlloc) {}

    void read(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

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

    void write(unsigned long int address) override {
        unsigned index = getIndex(address);
        unsigned long int tag = getTag(address);

        if (findAndUpdate(cache[index], tag)) {
            hits++;
            std::cout << "L2Cache Write Hit: " << address << std::endl;
        } else {
            misses++;
            std::cout << "L2Cache Write Miss: " << address << std::endl;
            evictAndAdd(cache[index], tag);
        }
    }
};
