#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <utility>
#include <unordered_set>
#include <climits>

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
    int board[4][4];
    int i_cord;
    int j_cord;
    int f_value;
    Board *parent;
    vector<Board> children;
    
    Board() : i_cord(3), j_cord(3) {
        for (int i = 0; i < 15; ++i)
            board[i / 4][i % 4] = i + 1;
        board[3][3] = 0;
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

    int best_child_f_value()
    {
        int best=10000;
        for(Board &c: children)
        {
            if(c.f_value<best)
            {
                best=c.f_value;
            }
        }
        return best;
    }

    int second_best_f_value()
    {
        int best=10000;
        int f_alt=10000;
        for(Board &c: children)
        {
            if(c.f_value-f_value>0 && c.f_value-f_value<best)
            {
                best=c.f_value-f_value;
                f_alt=c.f_value;
            }
        }
        return f_alt;
    }

    Board& best_child()
    {
        for(Board &c : children)
        {
            if(f_value==c.f_value)
            {
                return c;
            }
        }
        cout << "test1" << endl;
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
                string t;
                for (int i = 0; i < 16; ++i)
                    t += to_string(b.board[i / 4][i % 4]) + ",";
                return hash<string>()(t);
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

class LinearConflictMD : public ManhattanDistance {
public:
    virtual int operator()(Board &b) {
        return 0;
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
                if (x * y > 0) {                // ignore inversions with empty square
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
    int f_alternative;

    Problem(Heuristic &h) : h(h), randgen(mt19937(chrono::system_clock::now().time_since_epoch().count())) {}

    vector<Board> successors(Board &b) {
        vector<Board> succ;
        if (b.i_cord > 0) succ.emplace_back(Board(b).up());
        if (b.i_cord < 3) succ.emplace_back(Board(b).down());
        if (b.j_cord > 0) succ.emplace_back(Board(b).left());
        if (b.j_cord < 3) succ.emplace_back(Board(b).right());
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

    void create_solution(Board *final_board, vector<Board> &solution)
    {
        Board *temp=final_board;
        while(temp!=NULL)
        {
            solution.emplace(solution.begin(),(*temp));
            temp=temp->parent;
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

Board* rbfs(Board &node, Problem &p, int &nodes_expanded)
{
    nodes_expanded++;
    Board *ref;
    if(p.goal_test(node))
    {
        return &node;
    }
    if(node.children.size()==0)
    {
        node.children=p.successors(node); //add children then update their f-values
        for(Board &c : node.children)
        {
            c.f_value=p.h(c);
            c.parent=&node;
        }
    }
    while(true)
    {
        node.f_value=node.best_child_f_value();
        if(node.f_value>p.f_alternative)
        {
            return NULL;
        }
        if(node.second_best_f_value()<p.f_alternative)
        {
            p.f_alternative=node.second_best_f_value();
        }
        ref=rbfs(node.best_child(),p,nodes_expanded);
        if(ref!=NULL)
        {
            return ref;
        }
    }
}

Board* RecursiveBestFirst(Board &start, Problem &p, int &nodes_expanded)
{
    start.f_value = p.h(start);
    p.f_alternative=p.h(start);
    return rbfs(start,p,nodes_expanded);
}

int main() {
    ManhattanDistance md;
    LinearConflictMD  lc;
    InversionDistance id;

    vector<Heuristic*> heuristics;
    heuristics.push_back(&md);
    heuristics.push_back(&lc);
    heuristics.push_back(&id);
    for (int m = 10; m <= 50; m += 10) {
        for (int n = 0; n < 10; ++n) {
            for (Heuristic *h : heuristics) {
                    Problem p(*h);
                    Board start = p.scramble(m);
                    start.parent=NULL;
                    Board *final_board;
                    vector<Board> solution;
                    int nodes_expanded = 0;
                    chrono::time_point<chrono::high_resolution_clock> t0, t1;
                    t0 = chrono::high_resolution_clock::now();
                    final_board=RecursiveBestFirst(start, p, nodes_expanded);
                    t1 = chrono::high_resolution_clock::now();
                    chrono::microseconds duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
                    p.create_solution(final_board,solution);
                    p.print(solution);
                    cout << "Scramble number:\t" << m << endl;
                    cout << "Algorithm:\t\t\t\t" << "RBFS" << endl;
                    cout << "Heuristic:\t\t\t\t" << h->get_name() << endl;
                    cout << "Moves:\t\t\t\t\t\t" << solution.size() - 1 << endl;
                    cout << "Nodes expanded:\t\t" << nodes_expanded << endl;
                    cout << "Computation time:\t" << duration.count() << " microseconds" << endl;
                    cout << endl;
            }
        }
    }
}