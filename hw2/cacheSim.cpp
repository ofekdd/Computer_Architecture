#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cachestruct.cpp"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

unsigned long int evictedAddressFromL2 = 0;
bool evictionFlag = false;

// Function to calculate average access time
double avgAccTimeCalculator(const L1Cache& l1Cache, const L2Cache& l2Cache, unsigned memCyc) {
    double L1MissRate = l1Cache.hitMissCalculator();
    double L2MissRate = l2Cache.hitMissCalculator();
    double avgAccTime = l1Cache.getAccessTime() + (L1MissRate * (l2Cache.getAccessTime() + (L2MissRate * memCyc)));
    return avgAccTime;
}

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

    L1Cache l1Cache(MemCyc, BSize, L1Size, L1Assoc, L1Cyc, WrAlloc);
    L2Cache l2Cache(MemCyc, BSize, L2Size, L2Assoc, L2Cyc, WrAlloc);
	// Set L2 cache in L1 cache
    l1Cache.setL2Cache(&l2Cache);

	
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

        if (operation == 'r') {
            l1Cache.read(num);
        } else if (operation == 'w') {
            l1Cache.write(num);
        } else {
            cerr << "Unknown operation: " << operation << endl;
            return 0;
        }
	}

    double L1MissRate = l1Cache.hitMissCalculator();
    double L2MissRate = l2Cache.hitMissCalculator();
    double avgAccTime = avgAccTimeCalculator(l1Cache, l2Cache, MemCyc);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
