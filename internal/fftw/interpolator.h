/*------------------------------------------------------------------------------
 *
 * interpolator.h
 *
 * the public API for the interpolator class
 *
 * $Id: interpolator.h,v 1.1 2010/12/02 14:17:07 schurek.p Exp $
 *
 *(c)2010 Peter Schurek
 *----------------------------------------------------------------------------*/

#ifndef __INTERPOLATOR_H__
#define __INTERPOLATOR_H__


#include <string>
#include <vector>
#include <tr1/memory>
#include <functional>


using namespace std;            // so we don't have to add the std:: prefix everywhere
using namespace std::tr1;


class Interpolator {
public :

    /**
     * all errors are reported by throwing this exception
     */
    class Exception {
    public:
        const string error;

        Exception(string e) : error(e) {};
    };

    Interpolator(const vector<double> &vx, const vector<double> &vy) throw (Exception);
    Interpolator(Tcl_Interp *interp, Tcl_Obj *lx, Tcl_Obj *ly) throw (Exception);
    Interpolator(const Interpolator &i);
    Interpolator &operator=(const Interpolator &x);

    virtual ~Interpolator();

    pair<double, double> find_range(double x);
    double operator()(const double x);

private:
    /** concrete type for our x, y table rows */
    typedef pair<double, double> Point;
    typedef std::tr1::shared_ptr<Point>    RefPoint;
    typedef vector<RefPoint>     VecRefPoint;

    /** vector of pointers to <x, y> pairs, sorted along x for fast searching and interpolating */
    VecRefPoint xyTable;

    /** range cache: last lower boundary */
    VecRefPoint::const_iterator pLower;

    /** range cache: last upper boundary */
    VecRefPoint::const_iterator pUpper;

    /** range cache: pre-computed proportion of the similar triangle */
    double prop;

    /**
     * comparator for searching our interpolation table
     */
    class Less : public binary_function<const RefPoint &, const RefPoint &, bool> {
    public:
        bool operator()(const RefPoint &x, const RefPoint &y) {
            return (x->first < y->first);
        };
    };

    /** instantiate our comparator just once */
    Less less;
};

#endif
