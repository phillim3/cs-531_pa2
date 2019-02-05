#include <iostream>
#include <vector>
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
    
    enum Action { UP, DOWN, LEFT, RIGHT };

    Board() : i_cord(3), j_cord(3) {
        for (int i = 0; i < 15; ++i)
            board[i / 4][i % 4] = i + 1;
        board[3][3] = 0;
    }

    Board(Board &b, Action a) : i_cord(b.i_cord), j_cord(b.j_cord) {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                board[i][j] = b.board[i][j];

        switch(a) {
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

	bool isValidForRow(int row, int x)
	{
		if (x >= solved.board[row][0] && x <= solved.board[row][Board::COLS-1])
			return true;
		return false;
	}

	int getRowCount(Board &b)
	{
		int count = 0;
		for (int row = 0; row < Board::ROWS; row++)
		{
			for (int column = 0; column < Board::COLS-2; column++)
			{				
				int left = b.board[row][column];
				int right = b.board[row][column+1];
				int correct_right = solved.board[row][column+1];
				if (isValidForRow(row, left) && isValidForRow(row, right))
				{
					if (right != correct_right)
						count++;
				}
			}
		}
		return count;
	}
	
	int getMD(Board &b)
	{
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

public:
    virtual int operator()(Board &b) {
		return getMD(b) + getRowCount(b)*2;
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
    vector<Board> path;
    unordered_set<Board> pathSet;
    path.push_back(start);
    pathSet.insert(start);
    while(1) {
        int f_min = DL_A_star(path, pathSet, p, 0, f_limit, nodes_expanded);
        if (f_min <= f_limit) return path;              // if goal is found, return path
        if (f_min == INT_MAX) return vector<Board>();   // if failure, return empty path
        f_limit = f_min;
    }
}


/*

class Solution {
public:
    char movement = 'n';
    vector<Solution *> path;
    Solution *parent;
    float f_value;
} *start;


bool add_successor(Solution *node)
{
    if (problem.i_cord == 0 || problem.i_cord == 1 || problem.i_cord == 2) //down
    {
        node->successors.push_back(new Solution);
        node->successors.back()->parent = node;
        node->successors.back()->movement = 'd';
    }
    if (problem.i_cord == 1 || problem.i_cord == 2 || problem.i_cord == 3) //up
    {
        node->successors.push_back(new Solution);
        node->successors.back()->parent = node;
        node->successors.back()->movement = 'u';
    }
    if (problem.j_cord == 0 || problem.j_cord == 1 || problem.i_cord == 2) //right
    {
        node->successors.push_back(new Solution);
        node->successors.back()->parent = node;
        node->successors.back()->movement = 'r';
    }
    if (problem.j_cord == 1 || problem.j_cord == 2 || problem.i_cord == 3) //left
    {
        node->successors.push_back(new Solution);
        node->successors.back()->parent = node;
        node->successors.back()->movement = 'l';
    }
}

void update_f_values(Solution *node) {
       
}

void best_f(Solution node)
{
}

bool best_f_gt_f_limit(Solution node, double f_limit)
{
}

void mark_alternative(Solution node)
{
}

struct solution *rbfs(Solution *node)
{
    if (goal_test())
    {
        return node;
    }
    if (add_successors(node))
    {
        update_f_values(node);
        while (true)
        {
            best_f(node);
            if (best_f_gt_f_limit(node, f_limit))
            {
                return node; //mark failure because best f-limit>current f-limit
            }
            else
            {
                mark_alternative(node); //second lowest f-value
                //mark best then call rbfs(problem, best, min(f_limit, alternative) and add to path
                //if resulting path != failure, return path solution
            }
        }
    }
    else
    {
        return node; //mark failure for no successors
    }
}

struct solution *recursive_best_first_search()
{
    return rbfs(start);
}

*/

int main() {

    ManhattanDistance md;
    LinearConflictMD  lc;
    InversionDistance id;
    vector<Heuristic*> heuristics = {&md, &lc, &id};	

    for (int m = 10; m <= 50; m += 10) {
        for (int n = 0; n < 10; ++n) {
            for (Heuristic* h : heuristics) {
                    Problem p(*h);
                    Board start = p.scramble(m);
                    int nodes_expanded = 0;
                    chrono::time_point<chrono::high_resolution_clock> t0, t1;
                    t0 = chrono::high_resolution_clock::now();
                    vector<Board> solution = ID_A_star(start, p, nodes_expanded);
                    t1 = chrono::high_resolution_clock::now();
                    chrono::microseconds duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
                    p.print(solution);
                    cout << "Scramble number:\t" << m << endl;
                    cout << "Algorithm:\t\t\t\t" << "IDA*" << endl;
                    cout << "Heuristic:\t\t\t\t" << h->get_name() << endl;
                    cout << "Moves:\t\t\t\t\t\t" << solution.size() - 1 << endl;
                    cout << "Nodes expanded:\t\t" << nodes_expanded << endl;
                    cout << "Computation time:\t" << duration.count() << " microseconds" << endl;
                    cout << endl;
            }
        }
    }
}