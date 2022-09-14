#ifndef UTILS_H
#define UTILS_H
enum ConstraintType {
    Copy,
    AddressOf,
    Load,
    Store,
};

struct MyConstraint {

    MyConstraint(int dest, int src, ConstraintType type)
                    :dest(dest), src(src), type(type) {}
    int dest, src;
    ConstraintType type;

};
#endif
