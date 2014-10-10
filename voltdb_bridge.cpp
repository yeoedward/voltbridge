#include <iostream>

#include "execution/VoltDBEngine.h"

#include "voltdb_bridge.hpp"


using namespace std;

struct duck {};

class Duck : public duck {
    public:
        Duck(int feet);
        ~Duck();
        void quack(float volume);
    private:
        int feet;
};

inline Duck* real(duck* d) { return static_cast<Duck*>(d); }
duck* new_duck(int feet) { return new Duck(feet); }
void delete_duck(duck* d) { delete real(d); }
void duck_quack(duck* d, float volume) { real(d)->quack(volume); }

Duck::Duck(int feet) {
    this->feet = feet;
}

Duck::~Duck() {
    // Do nothing.
}

void Duck::quack(float volume) {
    cout << "Quacked with volume: " << volume << endl;
    voltdb::VoltDBEngine engine;
    engine.initialize(0, 0, 0, 0, "eddy", 10);
    cout << engine.getLogManager() << endl;
    cout << engine.getTable("PERSONS") << endl;
    cout << engine.debug() << endl;
}
