package com.dynamo.cr.parted.curve;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Segmented Hermite spline representation
 * Given N control points N-1 Hermite spline segments are projected to
 * the range [xa, xb]
 * where xa and xb is in the value domain [0,1]
 * In order to keep derivative consistent(*) between segments the tangent is scaled by
 * dx where dx is defined as [xb - xa]
 *
 *
 * (*)
 * The mapping can be expressed parametrically as (x(t), y(t))
 *
 * where x(t) is the mapping function
 * and y(t) the Hermite spline function
 *
 * dy   y'(t)
 * -- = ----
 * dx   x'(t)
 *
 * if x(t) is a linear transformation: x(t) = at + c
 *
 * =>
 *
 * dy   y'(t)    y'(t)
 * -- = ----  =  ----
 * dx   x'(t)      a
 *
 * Hence, scaling every tangent with a yields consistent derivatives and C1 continuity
 * between segments
 *
 * @author chmu
 *
 */
public class HermiteSpline implements Serializable {

    private static final long serialVersionUID = 1L;

    public static final double MIN_POINT_X_DISTANCE = 0.01;

    private List<SplinePoint> points = new ArrayList<SplinePoint>(32);

    public HermiteSpline() {
        points.add(new SplinePoint(0, 0, 0.5, 0.5));
        points.add(new SplinePoint(1, 1, 0.5, 0.5));
    }

    public HermiteSpline(float[] data) {
        for (int i = 0; i < data.length; i+=4) {
            points.add(new SplinePoint(data[i], data[i+1], data[i+2], data[i+3]));
        }
    }

    private HermiteSpline(HermiteSpline spline, ArrayList<SplinePoint> l) {
        this.points = l;
    }

    private static double hermite(double x0, double x1, double t0, double t1, double t) {
        return (2 * t * t * t - 3 * t * t + 1) * x0 +
               (t * t * t - 2 * t * t + t) * t0 +
               (- 2 * t * t * t + 3 * t * t) * x1 +
               (t * t * t - t * t) * t1;
    }

    private static double hermiteD(double x0, double x1, double t0, double t1, double t) {
        return (6 * t * t - 6 * t) * x0 +
               (3 * t * t - 4 * t + 1) * t0 +
               (- 6 * t * t + 6 * t ) * x1 +
               (3 * t * t - 2 * t) * t1;
    }

    public int getCount() {
        return points.size();
    }

    public SplinePoint getPoint(int i) {
        return new SplinePoint(points.get(i));
    }

    public void getValue(int segment, double t, double[] value) {
        SplinePoint p0 = getPoint(segment);
        SplinePoint p1 = getPoint(segment + 1);
        double segT = t;
        double dx = p1.getX() - p0.getX();

        double py0 = p0.getY();
        double py1 = p1.getY();
        double pt0 = dx * p0.getTy() / p0.getTx();
        double pt1 = dx * p1.getTy() / p1.getTx();

        double x = p0.getX() * (1 - segT) + p1.getX() * segT;
        double y = hermite(py0, py1, pt0, pt1, segT);

        value[0] = x;
        value[1] = y;
    }

    public double getY(double x) {
        int segmentCount = getSegmentCount();
        int segment = 0;
        double t = 0;
        for (int s = 0; s < segmentCount; ++s) {
            SplinePoint p0 = getPoint(s);
            SplinePoint p1 = getPoint(s + 1);
            if (x >= p0.getX() && x < p1.getX()) {
                t = (x - p0.getX()) / (p1.getX() - p0.getX());
                segment = s;
                break;
            }
        }

        double[] value = new double[2];
        getValue(segment, t, value);
        return value[1];
    }

    public void getExtremValues(double[] v) {
        int segmentCount = getSegmentCount();
        double min = Double.MAX_VALUE;
        double max = -Double.MAX_VALUE;

        for (int s = 0; s < segmentCount; ++s) {
            SplinePoint p0 = getPoint(s);
            SplinePoint p1 = getPoint(s + 1);

            double dx = p1.getX() - p0.getX();

            double py0 = p0.getY();
            double py1 = p1.getY();
            min = Math.min(min, Math.min(py0, py1));
            max = Math.max(max, Math.max(py0, py1));
            double pt0 = dx * p0.getTy() / p0.getTx();
            double pt1 = dx * p1.getTy() / p1.getTx();

            //double x = p0.getX() * (1 - segT) + p1.getX() * segT;
            //double y = hermite(py0, py1, pt0, pt1, segT);

            /*
             * A, B and C are the coefficents of the derivative of the hermite spline
             * in power form, i.e. A * t^3 + B * t^2 ...
             */
            double A = 3 * (2 * py0  - 2 * py1 + pt0 + pt1);
            double B = 2 * (-3 * py0 + 3 * py1 - 2 * pt0 - pt1);
            double C = pt0;

            // Solve the equation for extreme points

            // Solution to the equation? (A straight line donsn't have a solution)
            if (Math.abs(A) > 0.0001) {
                double Q0 = B / (2 * A);
                double Q1 = B * B / (4 * A * A) - C / A;
                if (Q1 >= 0) {
                    double[] ts = new double[] {-Q0 + Math.sqrt(Q1), -Q0 - Math.sqrt(Q1)};
                    for (double t : ts) {
                        if (0.0 <= t && t <= 1) {
                            double y = hermite(py0, py1, pt0, pt1, t);
                            min = Math.min(min, y);
                            max = Math.max(max, y);
                        }
                    }
                }
            }
        }

        v[0] = min;
        v[1] = max;
    }


    public void getTangent(int segment, double t, double[] value) {
        SplinePoint p0 = getPoint(segment);
        SplinePoint p1 = getPoint(segment + 1);
        double segT = t;
        double dx = p1.getX() - p0.getX();

        double py0 = p0.getY();
        double py1 = p1.getY();
        double pt0 = dx * p0.getTy() / p0.getTx();
        double pt1 = dx * p1.getTy() / p1.getTx();

        double d = hermiteD(py0, py1, pt0, pt1, segT) / dx;
        double x = 1;
        double y = d;

        double l = Math.sqrt(x * x + y * y);
        value[0] = x / l;
        value[1] = y / l;
    }

    private HermiteSpline alterPoint(int i, double x, double y, double tx, double ty) {
        ArrayList<SplinePoint> l = new ArrayList<SplinePoint>(points);
        l.set(i, new SplinePoint(x, y, tx, ty));
        return new HermiteSpline(this, l);
    }

    public HermiteSpline setPosition(int i, double newX, double newY) {
        SplinePoint p = this.points.get(i);

        if (i == 0) {
            newX = 0;
        } else if (i == getCount() - 1) {
            newX = 1;
        }

        if (i > 0) {
            newX = Math.max(getPoint(i - 1).getX() + MIN_POINT_X_DISTANCE, newX);
        }
        if (i < getCount() - 1) {
            newX = Math.min(getPoint(i + 1).getX() - MIN_POINT_X_DISTANCE, newX);
        }

        return alterPoint(i, newX, newY, p.getTx(), p.getTy());
    }

    public HermiteSpline setTangent(int i, double tx, double ty) {
        SplinePoint p = this.points.get(i);
        tx = Math.max(tx, 0);

        return alterPoint(i, p.getX(), p.getY(), tx, ty);
    }

    public int getSegmentCount() {
        return getCount() - 1;
    }

    public HermiteSpline insertPoint(double x, double y) {
        int segmentCount = getSegmentCount();
        for (int s = 0; s < segmentCount; ++s) {
            SplinePoint p0 = getPoint(s);
            SplinePoint p1 = getPoint(s + 1);
            if (x >= p0.getX() && x < p1.getX()) {
                double[] pos = new double[2];
                double[] tan = new double[2];
                double t = (x - p0.getX()) / (p1.getX() - p0.getX());
                getValue(s, t, pos);
                getTangent(s, t, tan);

                ArrayList<SplinePoint> l = new ArrayList<SplinePoint>(points);
                l.add(s + 1, new SplinePoint(pos[0], pos[1], tan[0], tan[1]));
                return new HermiteSpline(this, l);
            }
        }
        return null;
    }

    public HermiteSpline removePoint(int index) {
        if (index > 0 && index < getCount() - 1) {
            ArrayList<SplinePoint> l = new ArrayList<SplinePoint>(points);
            l.remove(index);
            return new HermiteSpline(this, l);
        } else {
            return this;
        }
    }

}