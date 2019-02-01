#include <iostream>
#include <utility>
#include <chrono>
#include <thread>
#include <random>
#include <string>
using namespace std;

template <typename T,typename U>                                                   
pair<T,U> operator+(const pair<T,U> &l,const pair<T,U> &r) {   
    return { l.first + r.first, l.second + r.second };                                    
}

const int FRAME_DELAY_MS = 200;

/************************
 * Actions and Percepts *
 ************************/
enum Action { FORWARD, TURN_LEFT, TURN_RIGHT, SUCK, TURN_OFF };
enum Status { DIRTY, CLEAN, WALL };  
const char WALL_SENSOR = 1 << 0;
const char DIRT_SENSOR = 1 << 1;
const char HOME_SENSOR = 1 << 2;


/******************
 * Generic Agents *
 ******************/
class Agent {
public:
    virtual Action get_action(char percepts) = 0;
};

class SimpleReflexAgent : public Agent {
public:
    virtual Action get_action(char percepts) = 0;
};

class StochasticReflexAgent : public Agent {
protected:
    mt19937 randgen;
public:
    StochasticReflexAgent()
        : randgen(mt19937(chrono::system_clock::now().time_since_epoch().count()))
        {}
    virtual Action get_action(char percepts) = 0;
};

class ModelBasedAgent : public Agent {
protected:
    char state;
public:
    virtual Action get_action(char percepts) = 0;
};

/*************************
 * Agent Implementations *
 *************************/
class ReflexAgent1 : public SimpleReflexAgent {
public:
    Action get_action(char percepts) {
        if (percepts & DIRT_SENSOR)
            return Action::SUCK;
        if (percepts & WALL_SENSOR)
            return Action::TURN_RIGHT;
        return Action::FORWARD;
    }
};

class StochasticAgent1 : public StochasticReflexAgent {
public:
    Action get_action(char percepts) {
        int r = randgen() % 100;
        if (percepts & DIRT_SENSOR)
            return Action::SUCK;
        if (percepts & WALL_SENSOR) {
            if (r < 50) return Action::TURN_LEFT;
            return Action::TURN_RIGHT;
        }
        if (r < 80) return Action::FORWARD;
        if (r < 90) return Action::TURN_LEFT;
        return Action::TURN_RIGHT;
    }
};

class ModelAgent6 : public ModelBasedAgent {
public:
    Action get_action(char percepts) {
        bool dirty = percepts & DIRT_SENSOR;
        bool wall  = percepts & WALL_SENSOR;
        bool home  = percepts & HOME_SENSOR;
        if (dirty) return Action::SUCK;
        switch (state) {
            case 0:
                if (wall) {
                    state = 1;
                    return Action::TURN_RIGHT;
                }
                return Action::FORWARD;
            case 1:
                if (wall) return Action::TURN_RIGHT;
                state = 2;
                return Action::FORWARD;
            case 2:
                state = 3;
                return Action::TURN_RIGHT;
            case 3:
                if (wall) {
                    state = 4;
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 4:
                if (wall) return Action::TURN_LEFT;
                state = 5;
                return Action::FORWARD;
            case 5:
                state = 0;
                return Action::TURN_LEFT;
        }
    }
};

class ModelAgent12 : public ModelBasedAgent {
public:
    Action get_action(char percepts) {
        bool dirty = percepts & DIRT_SENSOR;
        bool wall  = percepts & WALL_SENSOR;
        bool home  = percepts & HOME_SENSOR;
        if (dirty) return Action::SUCK;
        switch (state) {
            case 0:
                if (wall) {
                    state = 1;
                    return Action::TURN_RIGHT;
                }
                return Action::FORWARD;
            case 1:
                if (wall) {
                    cout << "Error" << endl;
                    return Action::TURN_OFF;
                }
                state = 2;
                return Action::FORWARD;
            case 2:
                if (wall) state = 3;
                else state = 6;
                return Action::TURN_RIGHT;
            case 3:
                if (wall) {
                    state = 4;
                    return Action::TURN_RIGHT;
                }
                return Action::FORWARD;
            case 4:
                if (wall) {
                    if (home) return Action::TURN_OFF;
                    state = 5;
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 5:
                if (wall) {
                    state = 6;
                    return Action::TURN_LEFT;
                }
                cout << "Error" << endl;
                return Action::TURN_OFF;
            case 6:
                if (wall) {
                    state = 7;
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 7:
                if (wall) {
                    cout << "Error" << endl;
                    return Action::TURN_OFF;
                }
                state = 8;
                return Action::FORWARD;
            case 8:
                if (wall) state = 9;
                else state = 0;
                return Action::TURN_LEFT;
            case 9:
                if (wall) {
                    state = 10;
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 10:
                if (wall) {
                    if (home) {
                        state = 6;
                        return Action::TURN_LEFT;
                    }
                    state = 11;
                    return Action::TURN_RIGHT;
                }
                return Action::FORWARD;
            case 11:
                if (wall) {
                    state = 0;
                    return Action::TURN_RIGHT;
                }
                cout << "Error" << endl;
                return Action::TURN_OFF;
        }
    }
};

class ModelAgent7 : public ModelBasedAgent {
public:
    Action get_action(char percepts) {
        bool dirty = percepts & DIRT_SENSOR;
        bool wall  = percepts & WALL_SENSOR;
        bool home  = percepts & HOME_SENSOR;
        if (dirty) return Action::SUCK;
        switch (state) {
            case 0: //combing forward with right turn
                if (wall) {
                    state = 1;
                    return Action::TURN_RIGHT;
                }
                return Action::FORWARD;
            case 1: //RT1
                state = 2;
                if (wall) {
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 2: //RT2
                state = 3;
                if (wall) {
                    return Action::TURN_LEFT;
                }
                return Action::TURN_RIGHT;
            case 3: //combing with left turn
                if (wall) {
                    state = 4;
                    return Action::TURN_LEFT;
                }
                return Action::FORWARD;
            case 4: //LT1
                if (wall) {
                    state = 3;
                    return Action::TURN_LEFT;
                }
                state = 5;
                return Action::FORWARD;
            case 5: //LT2
                if (wall) {
                    state = 6;
                    return Action::TURN_RIGHT;
                }
                state = 0;
                return Action::TURN_LEFT;
            case 6: //LT3
                if (wall) {
                    state = 0;
                    return Action::TURN_RIGHT;
                }
                cout << "Error!!! Self destruct sequence initiated!" << endl;
                return Action::TURN_OFF;
        }
    }
};

/*************************
 * Environment/Simulator *
 *************************/
class Environment;

class Critic {
public:
    void rate_performance(const Environment &env);
};

class Environment {
    friend class Critic;
    static const int GRID_SIZE = 10;
    static const int TOTAL_SIZE = GRID_SIZE + 3;

    pair<int, int> location, orientation;
    Status room[TOTAL_SIZE][TOTAL_SIZE]{};

    void init(bool walls) {
        location = make_pair(1, 1);
        orientation = make_pair(0, 1);
        for (int i = 0; i < TOTAL_SIZE; ++i)
            for (int j = 0; j < TOTAL_SIZE; ++j)
                room[i][j] = DIRTY;
        for (int i = 0; i < TOTAL_SIZE; ++i) {
            room[0][i] = room[i][0] = room[TOTAL_SIZE - 1][i] = room[i][TOTAL_SIZE - 1] = WALL;
            if (walls) {
                if (i != 3 && i != 9) {
                    room[6][i] = room[i][6] = WALL;
                }
            }
            else {
                room[TOTAL_SIZE - 2][i] = room[i][TOTAL_SIZE - 2] = WALL;
            }
        }
    }

    char get_percepts() {
        pair<int, int> forward = location + orientation;
        char percepts = 0;
        if (room[forward.first][forward.second] == WALL)
            percepts |= WALL_SENSOR;
        if (room[location.first][location.second] == DIRTY)
            percepts |= DIRT_SENSOR;
        if (location.first == 1 && location.second == 1)
            percepts |= HOME_SENSOR;
        return percepts;
    }

    void print_env(Critic &C) {
        for (int i = 0; i < TOTAL_SIZE; ++i) {
            for (int j = 0; j < TOTAL_SIZE; ++j) {
                if (i == location.first && j == location.second) {
                    if (orientation.first == 0) {
                        if (orientation.second == 1)
                            cout << "> ";
                        else cout << "< ";
                    }
                    else if (orientation.first == 1)
                        cout << "v ";
                    else cout << "^ ";
                }
                else {
                    switch (room[i][j]) {
                        case WALL:
                            cout << "\xfe ";
                            break;
                        case CLEAN:
                            cout << "- ";
                            break;
                        case DIRTY:
                            cout << "O ";
                            break;
                    }
                }
            }
            cout << endl;
        }
        C.rate_performance(*this);
        cout << endl;
    }

public:
    void execute(Agent &A, Critic &C, bool walls) {
        init(walls);
        print_env(C);
        Action action;
        do {
            char percepts = get_percepts();
            action = A.get_action(percepts);
            pair<int, int> temp;
            int factor;
            switch(action) {
                case FORWARD:
                    temp = location + orientation;
                    if (room[temp.first][temp.second] != WALL)
                        location = temp;
                    break;
                case TURN_LEFT:
                    temp = orientation;
                    factor = 1 - 2 * (orientation.first == 0);
                    orientation.first = factor * temp.second;
                    orientation.second = factor * temp.first;
                    break;
                case TURN_RIGHT:
                    temp = orientation;
                    factor = 1 - 2 * (orientation.second == 0);
                    orientation.first = factor * temp.second;
                    orientation.second = factor * temp.first;
                    break;
                case SUCK:
                    room[location.first][location.second] = CLEAN;
                    break;
            }
            print_env(C);
            this_thread::sleep_for(chrono::milliseconds(FRAME_DELAY_MS));
        }while (action != TURN_OFF);
    }
};


/**********************
 * Performance Critic *
 **********************/
void Critic::rate_performance(const Environment &env) {
    int clean = 0;
    int total = 0;
    for (int i = 0; i < Environment::TOTAL_SIZE; ++i)
        for (int j = 0; j < Environment::TOTAL_SIZE; ++j)
            switch (env.room[i][j]) {
                case Status::CLEAN:
                    ++clean;
                case Status::DIRTY:
                    ++total;
            }
    double clean_percent = 100. * clean / total;
    cout << "Floorspace cleaned: " << clean_percent << '%' << endl;
}


/*****************
 * Main Function *
 *****************/
int main() {
    Environment env;
    Critic critic;
    ReflexAgent1 reflex1;
    StochasticAgent1 stochastic1;
    ModelAgent6 model6;
    ModelAgent12 model12;
    ModelAgent7 model7;
    env.execute(reflex1, critic, false);
    return 0;
}