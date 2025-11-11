#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
using namespace std;

void printMap(const vector<vector<int>>& map){
    int size = map.size();
    for(int r=0; r<size; r++){
        for(int c=0; c<size; c++){
            cout << map[r][c] << " ";
        }
        cout << endl;
    }
}

int main(){

    sf::RenderWindow window(sf::VideoMode(800, 600), "Tilemap Example");
    window.setFramerateLimit(60);

    //0 = empty space
    int empty = 0;

    //1 = wall
    int wall = 1;

    //2 = target
    int target = 2;

    //3 = player
    int player = 3;

    //mark current tile as visited
    //select up down left and right of the current
    //if it is a wall out of range or a wall return failure
    //if the tile has been visited return failure.
    //if it is target return success
    //check the distance from the target for all the values (f,g and h values) and select the closest
    //f = distance to player
    //g = distance to target
    //h = f + g
    //call the funcion again from the closer tile

    //functions needed
    //get distance function
    //get index of tile above below left and right, maybe dont need a function.
    //randomize walls, target, player?
    //place walls, than target, than player, if the player index is equal to the target, try again.
    //remember the random seeds of the indecies of player and target.
    //has been visited? remember in a hashset probobly.

    int size = 5;
    vector<vector<int>> map(size, vector<int>(size, 0));
    /*
      0  1  2  3
    [[0, 0, 0, 0],  0
     [0, 0, 0, 0],  1
     [0, 1, 0, 0],  2
     [0, 0, 0, 0]]  3
    */

    //map[y][x]
    //set value
    map[2][1] = 1;

    printMap(map);



    return 0;
}