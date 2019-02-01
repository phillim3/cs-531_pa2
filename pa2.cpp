#include <vector>
#include <cmath>
#include <unordered_set>

using namespace std;

class Heuristic {
public:
    virtual int operator()(Board b) = 0;
};

class ManhattanDistance {
public:
    virtual int operator()(Board b) {
        int MD = 0;
        for (int i = 0; i < 15; ++i) {
            int x = i / 4;
            int y = i % 4;
            int v = b.board[x][y] - 1;
            MD += abs(x - (v / 4)) + abs(y - (v % 4));
        }
        return MD;
    }
};

class {
public:
    virtual int operator()(Board b) {

    }
};

class Solution {
public:
    char movement = 'n';
    vector<Solution *> successors;
    Solution *parent;
    float f_value;
} *start;

class Board {
public:
    int board[4][4];
    int i_cord;
    int j_cord;
} problem;

bool goal_test(Board b, Heuristic &h) {
    return h(b) == 0;
}

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

void shuffle(int moves)
{
}

void init_board()
{
    int k = 1;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            problem.board[i][j] = k;
            k++;
        }
    }
    problem.board[3][3] = 0; //mark initial starting pos
    problem.i_cord = 3;
    problem.j_cord = 3;
}

int main()
{
    init_board();
    shuffle(10);
    recursive_best_first_search();
}