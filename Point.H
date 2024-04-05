#ifndef POINT_H_INCLUDED
#define POINT_H_INCLUDED

class Point {
public:
    // Constructors
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}

    // Getter methods
    int getX() const { return x; }
    int getY() const { return y; }

    // Setter methods
    void setX(int x_) { x = x_; }
    void setY(int y_) { y = y_; }

    // Case-insensitive access to X and Y properties
    int x, X;
    int y, Y;
};

#endif // POINT_H_INCLUDED
