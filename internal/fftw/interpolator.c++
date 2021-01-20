/*------------------------------------------------------------------------------
 *
 * interpolator.c++
 *
 * This functor is set up with a table of <x, y> values and returns the
 * linear interpolated values for any given x
 *
 * $Id: interpolator.c++,v 1.1 2010/12/02 14:17:07 schurek.p Exp $
 *
 *(c)2010 Peter Schurek
 *----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <tr1/memory>

#include "tcl.h"
#include "tclDecls.h"

#include "interpolator.h"


using namespace std;            // so we don't have to add the std:: prefix everywhere
using namespace std::tr1;


#ifdef EBUG
#define ENTER(fmt, ...) fprintf(stderr, "ENTER %s:" fmt "\n", __func__, ##__VA_ARGS__)
#define LEAVE(fmt, ...) fprintf(stderr, "LEAVE %s:" fmt "\n", __func__, ##__VA_ARGS__)
#define INFO(fmt, ...)  fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#else
#define ENTER(fmt, ...)
#define LEAVE(fmt, ...)
#define INFO(fmt, ...)
#endif


/**
 * copy ctor
 *
 * @param i Interpolator instance to clone
 */
Interpolator::Interpolator(const Interpolator &i) {
    ENTER();
    xyTable = i.xyTable;
    LEAVE();
}


/**
 * assignment operator
 *
 * @param i Interpolator instance to clone
 */
Interpolator &Interpolator::operator=(const Interpolator &i) {
    ENTER();
    xyTable = i.xyTable;
    return *this;
    LEAVE();
}


/**
 * ctor that takes list of x values, a list of y values and
 * builds a vector for searching
 *
 * @param vx vector of x values
 * @param vy vector of y values
 */
Interpolator::Interpolator(const vector<double> &vx, const vector<double> &vy) throw(Interpolator::Exception) {
    ENTER();

    if (vx.size() != vy.size())
        throw Exception("x and y vectors are of unequal length");

    if (vx.size() <= 0)
        throw Exception("need at least one point");

    /* prealloc xyTable */
    xyTable.reserve(vx.size());

    /* assign points */
    vector<double>::const_iterator j = vy.begin();
    for (vector<double>::const_iterator i = vx.begin(); i != vx.end(); i++, j++) {
        /* sanity check for ascending x */
        if ((i != vx.begin()) && (*i < xyTable.back()->first)) {
            fprintf(stderr, "last: %lf, current: %lf, at end: %s\n", xyTable.back()->first, *i, (i == vx.end() ? "t" : "f"));
            throw Exception("x values not monotonically increasing");
        }

        /* equal or increasing, add */
        xyTable.push_back(RefPoint(new Point(*i, *j)));
    }

    /* initialize range cache */
    pLower = pUpper = xyTable.begin();

    LEAVE();
}


/**
 * ctor that takes a Tcl list of x values, a Tcl list of y values and
 * builds a vector for searching
 *
 * @param lx list of x values
 * @param ly list of y values
 */
Interpolator::Interpolator(Tcl_Interp *interp, Tcl_Obj *lx, Tcl_Obj *ly) throw(Interpolator::Exception) {
    ENTER();

    int xLen = 0;
    int yLen = 0;

    if (Tcl_ListObjLength(interp, lx, &xLen) != TCL_OK)
        throw Exception("x not a list");

    if (Tcl_ListObjLength(interp, ly, &yLen) != TCL_OK)
        throw Exception("y not a list");

    if (xLen != yLen)
        throw Exception("x and y lists are of unequal length");

    if (xLen <= 0)
        throw Exception("need at least one point");

    /* prealloc xyTable */
    xyTable.reserve(xLen);

    /* assign points */
    Tcl_Obj *pX;
    Tcl_Obj *pY;

    for (int i = 0; i < xLen; i++) {
        double x;
        double y;

        if (Tcl_ListObjIndex(interp, lx, i, &pX) != TCL_OK)
            throw Exception("cannot access x element");

        if (Tcl_GetDoubleFromObj(interp, pX, &x) != TCL_OK)
            throw Exception("invalid x element");

        if (Tcl_ListObjIndex(interp, ly, i, &pY) != TCL_OK)
            throw Exception("cannot access y element");

        if (Tcl_GetDoubleFromObj(interp, pY, &y) != TCL_OK)
            throw Exception("invalid y element");

        /* sanity check for ascending x */
        if ((i > 0) && (x < xyTable.back()->first))
            throw Exception("x values not monotonically increasing");

        /* equal or increasing, add */
        xyTable.push_back(RefPoint(new Point(x, y)));
    }

    /* initialize range cache */
    pLower = pUpper = xyTable.begin();

    LEAVE();
}


/**
 * dtor, removes all allocated data
 */
Interpolator::~Interpolator() {
    ENTER();
    LEAVE();
};


/**
 * find the two nearest rows for a given x
 *
 * @param x value to find
 * @return pair of x1, x2 coordinates below and above the given x value
 */
pair<double, double> Interpolator::find_range(double x) {
    ENTER();
    VecRefPoint::const_iterator upper = upper_bound(xyTable.begin(), xyTable.end(), RefPoint(new Point(x, 0.)), less);

    if (upper == xyTable.begin()) {
        INFO("at left edge of table: %lf", (*upper)->first);
        LEAVE();
        return pair<double, double>((*upper)->first, (*upper)->first);
    } else if (upper == xyTable.end()) {
        INFO("at right edge of table: %lf", xyTable.back()->first);
        LEAVE();
        return pair<double, double>(xyTable.back()->first, xyTable.back()->first);
    } else {
        INFO("in middle of table: %lf %lf", (*(upper - 1))->first, (*upper)->first);
        LEAVE();
        return pair<double, double>((*(upper -1))->first, (*upper)->first);
    }
};


/**
 * return an interpolated y for a given x
 */
double Interpolator::operator()(const double x) {
    ENTER("%lf", x);

    if ((pLower == xyTable.end()) || (pUpper == xyTable.end()) || (x < (*pLower)->first) || (x > (*pUpper)->first)) {
        /* search new range */
        pUpper = upper_bound(xyTable.begin(), xyTable.end(), RefPoint(new Point(x, 0.)), less);

        /* if in middle of the table, compute left edge and proportion of the similar triangle */
        if ((pUpper != xyTable.end()) && (pUpper != xyTable.begin())) {
            pLower = pUpper - 1;
            prop   = ((*pUpper)->second - (*pLower)->second) / ((*pUpper)->first - (*pLower)->first);
        } else
            pLower = pUpper;
    } else {
        INFO("range cache hit %lf <= %lf <= %lf", (*pLower)->first, x, (*pUpper)->first);
    }

    if (pUpper == xyTable.begin()) {
        /* at left edge of table */
        LEAVE("%lf", (*pUpper)->second);
        return (*pUpper)->second;
    } else if (pUpper == xyTable.end()) {
        /* at right edge of table */
        LEAVE("%lf", xyTable.back()->second);
        return xyTable.back()->second;
    } else {
        /* in middle of table */
        LEAVE("%lf", (*pLower)->second + (x - (*pLower)->first) * prop);
        return (*pLower)->second + (x - (*pLower)->first) * prop;
    }
};


/**
 * for testing only
 *
 * @param argc argument count
 * @param argv argumetn vector
 *
 * @return 0
 */
int
main(int argc, char *argv[]) {
    printf("testing %s...\n", argv[0]);

    printf("setting compliant vectors...\n");
    vector<double> vx = vector<double>();
    vector<double> vy = vector<double>();

    for (int i = 0; i <= 10; i++) {
        vx.push_back(static_cast<double>(i));
        vy.push_back(i * 3.);
    }

    try {
        printf("creating Interpolator with compliant vectors...\n");
        Interpolator         ip1 = Interpolator(vx, vy);
        pair<double, double> p;

        printf("Search middle element 3.\n");
        p = ip1.find_range(3.);
        printf("y is %lf\n", ip1(3.));

        printf("Search middle element 3.5\n");
        p = ip1.find_range(3.5);
        printf("y is %lf\n", ip1(3.5));

        printf("Search middle element 3.6\n");
        p = ip1.find_range(3.6);
        printf("y is %lf\n", ip1(3.6));

        printf("Search beyond left edge -1.\n");
        p = ip1.find_range(-1.);
        printf("y is %lf\n", ip1(-1.));

        printf("Search beyond right edge 11.\n");
        p = ip1.find_range(11.);
        printf("y is %lf\n", ip1(11.));

        printf("Search middle element 3.6\n");
        p = ip1.find_range(3.6);
        printf("y is %lf\n", ip1(3.6));
    } catch (Interpolator::Exception &ex) {
        fprintf(stderr, "caught exception: %s\n", ex.error.c_str());
    }

    return 0;
}
