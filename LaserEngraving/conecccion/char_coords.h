// char_coords.h
#ifndef CHAR_COORDS_H
#define CHAR_COORDS_H

#include <vector>

struct Point {
    int x;
    int y;
};

const int MAX_X = 500;
const int MAX_Y = 500;

    std::vector<Point> getCharCoordinates(char c, int xOffset = 0) {
        std::vector<Point> coords;
        c = toupper(c);

        auto move = [&](int x, int y) {
            coords.push_back({x + xOffset, y});
        };

        auto lift = [&]() {
            coords.push_back({-1, -1});
        };

        switch (c) {
            case 'A':
                move(0, 0); move(MAX_X / 2, MAX_Y); lift();
                move(MAX_X, 0); move(MAX_X / 2, MAX_Y); lift();
                move(MAX_X / 4, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2);
                break;
            case 'B':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X * 3 / 4, MAX_Y); move(MAX_X, MAX_Y * 3 / 4); move(MAX_X * 3 / 4, MAX_Y / 2); move(0, MAX_Y / 2); lift();
                move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2); move(MAX_X, MAX_Y / 4); move(MAX_X * 3 / 4, 0); move(0, 0);
                break;
            case 'C':
                move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0);
                break;
            case 'D':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y * 3 / 4); move(MAX_X, MAX_Y / 4); move(0, 0);
                break;
            case 'E':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
                move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2); lift();
                move(0, 0); move(MAX_X, 0);
                break;
            case 'F':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
                move(0, MAX_Y / 2); move(MAX_X * 3 / 4, MAX_Y / 2);
                break;
            case 'G':
                move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y / 2); move(MAX_X / 2, MAX_Y / 2);
                break;
            case 'H':
                move(0, 0); move(0, MAX_Y); lift();
                move(MAX_X, 0); move(MAX_X, MAX_Y); lift();
                move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2);
                break;
            case 'I':
                move(MAX_X / 2, 0); move(MAX_X / 2, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
                move(0, 0); move(MAX_X, 0);
                break;
            case 'J':
                move(MAX_X, MAX_Y); move(MAX_X, 0); move(MAX_X / 2, 0); move(0, MAX_Y / 4);
                break;
            case 'K':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y / 2); move(MAX_X, MAX_Y); lift();
                move(0, MAX_Y / 2); move(MAX_X, 0);
                break;
            case 'L':
                move(0, MAX_Y); move(0, 0); lift();
                move(0, 0); move(MAX_X, 0);
                break;
            case 'M':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
                move(MAX_X / 2, MAX_Y / 2); move(MAX_X, MAX_Y); lift();
                move(MAX_X, MAX_Y); move(MAX_X, 0);
                break;
            case 'N':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, 0); lift();
                move(MAX_X, 0); move(MAX_X, MAX_Y);
                break;
            case 'O':
                move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
                break;
            case 'P':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2);
                break;
            case 'Q':
                move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0); lift();
                move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0);
                break;
            case 'R':
                move(0, 0); move(0, MAX_Y); lift();
                move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2); lift();
                move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0);
                break;
            case 'S':
                move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
                break;
            case 'T':
                move(0, MAX_Y); move(MAX_X, MAX_Y); lift();
                move(MAX_X / 2, MAX_Y); move(MAX_X / 2, 0);
                break;
            case 'U':
                move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y);
                break;
            case 'V':
                move(0, MAX_Y); move(MAX_X / 2, 0); move(MAX_X, MAX_Y);
                break;
            case 'W':
                move(0, MAX_Y); move(0, 0); move(MAX_X / 2, MAX_Y / 2); move(MAX_X, 0); move(MAX_X, MAX_Y);
                break;
            case 'X':
                move(0, MAX_Y); move(MAX_X, 0); lift();
                move(MAX_X, MAX_Y); move(0, 0);
                break;
            case 'Y':
                move(0, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
                move(MAX_X, MAX_Y); move(MAX_X / 2, MAX_Y / 2); lift();
                move(MAX_X / 2, MAX_Y / 2); move(MAX_X / 2, 0);
                break;
            case 'Z':
                move(0, MAX_Y); move(MAX_X, MAX_Y); move(0, 0); move(MAX_X, 0);
                break;
            case '3':
                move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2); lift();
                move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
                break;
            case '4':
                move(MAX_X, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); lift();
                move(MAX_X, MAX_Y / 2); move(MAX_X, 0);
                break;
            case '5':
                move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2); move(MAX_X, 0); move(0, 0);
                break;
            case '6':
                move(MAX_X, MAX_Y); move(0, MAX_Y); move(0, 0); move(MAX_X, 0); move(MAX_X, MAX_Y / 2); move(0, MAX_Y / 2);
                break;
            case '7':
                move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0);
                break;
            case '8':
                move(0, MAX_Y / 2); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0); move(0, MAX_Y / 2); move(MAX_X, MAX_Y / 2);
                break;
            case '9':
                move(MAX_X, MAX_Y / 2); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
                break;
            case '0':
                move(0, 0); move(0, MAX_Y); move(MAX_X, MAX_Y); move(MAX_X, 0); move(0, 0);
                break;
            case ' ': // Espacio
                move(0, 0); move(MAX_X, 0);
                break;
            default:
                break;
        }

        return coords;
    }

#endif
