#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <utility>
#include <unordered_set>
#include <climits>
#include "MurmurHash3.h"

using namespace std;


/**************************************
 * Board State class
 *
 * Be wary, action functions do not check
 * whether the action is applicable
 * to the board state.
 **************************************/
class Board {
public:
    static const uint32_t HASHSEED = 0x9747b28c;
    static const int ROWS = 4;
    static const int COLS = 4;
    int board[ROWS][COLS];
    int i_cord;
    int j_cord;
    int F;

    enum Action { UP, DOWN, LEFT, RIGHT };

    Board() : i_cord(3), j_cord(3) {
        for (int i = 0; i < 15; ++i)
            board[i / 4][i % 4] = i + 1;
        board[3][3] = 0;
    }

    Board(const Board &obj)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                board[i][j] = obj.board[i][j];
        i_cord = obj.i_cord;
        j_cord = obj.j_cord;
        F = obj.F;
    }


    Board(Board &b, Action a) : i_cord(b.i_cord), j_cord(b.j_cord) {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                board[i][j] = b.board[i][j];

        switch (a) {
        case UP:    up();    break;
        case DOWN:  down();  break;
        case LEFT:  left();  break;
        case RIGHT: right(); break;
        }
    }

    bool operator==(const Board &b) const {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (board[i][j] != b.board[i][j])
                    return false;
        return true;
    }

    Board& up() {
        swap(board[i_cord][j_cord], board[i_cord - 1][j_cord]);
        --i_cord;
        return *this;
    }

    Board& down() {
        swap(board[i_cord][j_cord], board[i_cord + 1][j_cord]);
        ++i_cord;
        return *this;
    }

    Board& left() {
        swap(board[i_cord][j_cord], board[i_cord][j_cord - 1]);
        --j_cord;
        return *this;
    }

    Board& right() {
        swap(board[i_cord][j_cord], board[i_cord][j_cord + 1]);
        ++j_cord;
        return *this;
    }

    void print() {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                int v = board[i][j];
                if (v < 10) cout << ' ';
                cout << ' ' << v;
            }
            cout << endl;
        }
    }
};

// Implement std::hash<Board> so we can use std::unordered_set<Board>
namespace std
{
    template<>
    struct hash<Board> {
        size_t operator()(const Board &b) const {
            uint32_t hash;
            MurmurHash3_x86_32(&b.board, sizeof(b.board), Board::HASHSEED, &hash);
            return hash;
        }
    };
}


/****************************
 * Abstract Heuristic class
 ****************************/
class Heuristic {
public:
    virtual int operator()(Board &b) = 0;
    virtual string get_name() = 0;
};


/*******************************
 * Heuristic Implementations
 *******************************/
class ManhattanDistance : public Heuristic {
public:
    virtual int operator()(Board &b) {
        int MD = 0;
        for (int i = 0; i < 16; ++i) {
            int x = i / 4;
            int y = i % 4;
            int v = b.board[x][y];
            if (v > 0) {
                int x_dest = (v - 1) / 4;
                int y_dest = (v - 1) % 4;
                MD += abs(x - x_dest) + abs(y - y_dest);
            }
        }
        return MD;
    }

    virtual string get_name() {
        return "Manhattan Distance";
    }
};

//
// Linear conflict correction:
// Look at every line of the puzzle. If you find two tiles there which are supposed to end up in this line,
// but which are currently in the wrong order, then you know that the Manhattan distance is too optimistic
// and you actually need at least 2 more moves to get the two tiles past each other.  One can prove that the
// heuristic function remains admissible (in fact monotone) even if you add 2 for every pair with this problem
// in any row. The same applies to every pair with the analogous problem in any column.
//
class LinearConflictMD : public ManhattanDistance {
    Board solved = Board();

    inline bool isValidForRow(int row, int x)
    {
        return (x >= solved.board[row][0] && x <= solved.board[row][Board::COLS - 1]);
    }

    int getRowCount(Board &b)
    {
        int count = 0;
        for (int row = 0; row < Board::ROWS; row++)
        {
            for (int column = 0; column < Board::COLS - 2; column++)
            {
                int left = b.board[row][column];
                int right = b.board[row][column + 1];
                int correct_right = solved.board[row][column + 1];
                if (left == solved.board[row][column] && isValidForRow(row, right))
                {
                    if (right != correct_right)
                        count++;
                }
            }
        }
        return count;
    }

public:
    virtual int operator()(Board &b) {
        return ManhattanDistance::operator()(b) + getRowCount(b) * 2;
    }

    virtual string get_name() {
        return "MD + Linear Conflict Correction";
    }
};


class InversionDistance : public Heuristic {
public:
    virtual int operator()(Board &b) {
        int h_inv = 0;
        int v_inv = 0;
        for (int i = 0; i < 16; ++i) {
            for (int j = i + 1; j < 16; ++j) {
                int x = b.board[i / 4][i % 4];
                int y = b.board[j / 4][j % 4];
                if (x * y > 0) {    // ignore inversions with empty square
                    if (x > y) ++h_inv;

                    // map to column major ordering
                    int vi = 4 * (i % 4) + i / 4;
                    int vj = 4 * (j % 4) + j / 4;
                    int vx = 4 * ((x - 1) % 4) + (x - 1) / 4;
                    int vy = 4 * ((y - 1) % 4) + (y - 1) / 4;
                    if ((vx > vy) != (vi > vj)) ++v_inv;
                }
            }
        }
        return ceil(h_inv / 3.0) + ceil(v_inv / 3.0);
    }

    virtual string get_name() {
        return "Inversion Distance";
    }
};


/*****************************************
 * Problem class
 *
 * Contains the heuristic, successor, and
 * goal_test functions. Also contains a
 * scramble function, that generates
 * solvable starting states, and print
 * function that prints a sequence of board
 * states.
 *****************************************/
class Problem {
    mt19937 randgen;
public:
    Heuristic &h;

    Problem(Heuristic &h) : h(h), randgen(mt19937(chrono::system_clock::now().time_since_epoch().count())) {}

    vector<Board> successors(Board &b) {
        vector<Board> succ;
        if (b.i_cord > 0) succ.emplace_back(b, Board::UP);
        if (b.i_cord < 3) succ.emplace_back(b, Board::DOWN);
        if (b.j_cord > 0) succ.emplace_back(b, Board::LEFT);
        if (b.j_cord < 3) succ.emplace_back(b, Board::RIGHT);
        return succ;
    }

    bool goal_test(Board &b) {
        return h(b) == 0;
    }

    Board scramble(int m) {
        Board b;
        for (int i = 0; i < m; ++i) {
            vector<Board> succ = successors(b);
            int r = randgen() % succ.size();
            b = succ[r];
        }
        return b;
    }

    void print(vector<Board> &path) {
        for (Board &b : path) {
            b.print();
            cout << endl;
        }
    }
};

// Debugging helper function
void pause(Board &b, Problem &p, int f) {
    b.print();
    cout << "h: " << p.h(b) << ", f: " << f << endl << "paused..." << endl << endl;
    string s;
    getline(cin, s);
}

/*************************
 * IDA* Search Algorithm *
 *************************/
int DL_A_star(vector<Board> &path, unordered_set<Board> &pathSet, Problem &p, int g, int f_limit, int &nodes_expanded) {
    Board &b = path.back();
    int f = g + p.h(b);
    ++nodes_expanded;
    //pause(b, p, f);
    if (f > f_limit) return f;
    if (p.goal_test(b)) return f;
    int f_min = INT_MAX;
    for (Board &s : p.successors(b)) {
        if (pathSet.find(s) == pathSet.end()) {
            path.push_back(s);
            pathSet.insert(s);
            f = DL_A_star(path, pathSet, p, g + 1, f_limit, nodes_expanded);
            if (f <= f_limit) return f; // if goal is found, return length
            if (f < f_min) f_min = f;   // if smallest over limit, update f_min
            path.pop_back();
            pathSet.erase(s);
        }
    }
    return f_min;                       // return smallest over limit
}

vector<Board> ID_A_star(Board &start, Problem &p, int &nodes_expanded) {
    int f_limit = p.h(start);
    vector<Board> path{ start };
    unordered_set<Board> pathSet{ start };
    while (1) {
        int f_min = DL_A_star(path, pathSet, p, 0, f_limit, nodes_expanded);
        if (f_min <= f_limit) return path;              // if goal is found, return path
        if (f_min == INT_MAX) return vector<Board>();   // if failure, return empty path
        f_limit = f_min;
    }
}

/**********************
 *   RBFS Algorithm   *
 **********************/
bool ascF(Board &b1, Board &b2) { return b1.F < b2.F; }

int RBFS(vector<Board> &path, Problem &p, int g, int f_limit, int &nodes_expanded) {
    Board &b = path.back();
    ++nodes_expanded;
    if (p.goal_test(b)) return b.F;
    vector<Board> successors = p.successors(b);
    if (successors.empty()) return INT_MAX;
    for (Board &s : successors)
        s.F = max(g + p.h(s), b.F);
    while (1) {
        sort(successors.begin(), successors.end(), ascF);
        Board &best = successors[0];
        if (best.F > f_limit) return best.F;
        int new_f_limit = min(f_limit, successors[1].F);
        path.push_back(best);
        best.F = RBFS(path, p, g + 1, new_f_limit, nodes_expanded);
        if (best.F <= new_f_limit) return best.F;
        path.pop_back();
    }
}

vector<Board> RecursiveBestFirst(Board &start, Problem &p, int &nodes_expanded) {
    start.F = p.h(start);
    vector<Board> path{ start };
    RBFS(path, p, 1, INT_MAX, nodes_expanded);
    return path;
}


void csv_write_headers(std::ofstream& f) {
    f << "Board_ID, Scramble_Number, Algorithm, Heuristic, Moves, Nodes_Expanded, Computation_Time(us)" << endl;
}

void csv_write_row(std::ofstream& f, int board_id, int scramble_num, string algo, string heuristic, size_t moves, int nodes_exp, long microseconds) {
    long micros = std::max(1L, microseconds);
    f << board_id << "," << scramble_num << "," << algo << "," << heuristic << "," << moves << "," << nodes_exp << "," << micros << endl;
}

int main()
{
    const int TOTAL_TRIALS = 1000;
    LinearConflictMD  lc;
    ManhattanDistance md;
    InversionDistance id;
    vector<Heuristic*> heuristics = { &md, &lc, &id };

    std::ofstream csv_file;
    string filename = "pa2-" + std::to_string(TOTAL_TRIALS) + ".csv";
    csv_file.open(filename);
    csv_write_headers(csv_file);
    int b_id = 0;

    for (int scramble_size = 10; scramble_size <= 50; scramble_size += 10) {
        cout << "Scramble size: " << scramble_size << " trial: ";
        for (int num_trials = 0; num_trials < TOTAL_TRIALS; num_trials++) {
            if (num_trials % (TOTAL_TRIALS / 10) == 0)
                cout << num_trials << " ";
            for (Heuristic* heuristic : heuristics) {
                Problem p_rbfs(*heuristic);
                Board start_rbfs = p_rbfs.scramble(scramble_size);
                ++b_id;

                // RBFS Algorithm
                int nodes_expanded = 0;
                chrono::time_point<chrono::high_resolution_clock> t0, t1;
                t0 = chrono::high_resolution_clock::now();
                vector<Board> solution_RBFS = RecursiveBestFirst(start_rbfs, p_rbfs, nodes_expanded);
                t1 = chrono::high_resolution_clock::now();
                chrono::microseconds duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
                //p.print(solution);
                csv_write_row(csv_file, b_id, scramble_size, "RBFS", heuristic->get_name(), solution_RBFS.size() - 1, nodes_expanded, duration.count());
            }
        }
        cout << endl;
    }

    for (int scramble_size = 10; scramble_size <= 50; scramble_size += 10) {
        cout << "Scramble size: " << scramble_size << " trial: ";
        for (int num_trials = 0; num_trials < TOTAL_TRIALS; num_trials++) {
            if (num_trials % (TOTAL_TRIALS / 10) == 0)
                cout << num_trials << " ";
            for (Heuristic* heuristic : heuristics) {
                Problem p_ida(*heuristic);
                Board start_ida = p_ida.scramble(scramble_size);
                ++b_id;

                // IDA* Algorithm
                int nodes_expanded = 0;
                chrono::time_point<chrono::high_resolution_clock> t0, t1;
                t0 = chrono::high_resolution_clock::now();
                vector<Board> solution_A_star = ID_A_star(start_ida, p_ida, nodes_expanded);
                t1 = chrono::high_resolution_clock::now();
                chrono::microseconds duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
                //p.print(solution);
                csv_write_row(csv_file, b_id, scramble_size, "IDA*", heuristic->get_name(), solution_A_star.size() - 1, nodes_expanded, duration.count());
            }
        }
        cout << endl;
    }



    csv_file.close();
}