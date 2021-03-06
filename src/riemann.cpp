/// \file
/// \brief Riemann solver.
///
/// Exact Riemann solver for the Euler equations in one dimension
/// Translated from the Fortran code er1pex.f and er1pex.ini
/// by Dr. E.F. Toro downloaded from
/// http://www.numeritek.com/numerica_software.html#freesample

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "riemann.h"

using namespace std;

/// \brief Calculate start guessed pressure value.
///
/// Purpose is to provide a guessed value for pressure
/// pm in the Star Region. The choice is made
/// according to adaptive Riemann solver using
/// the PVRS, TRRS and TSRS approximate
/// Riemann solvers. See Sect. 9.5 of Chapt. 9 of Ref. 1.
///
/// \param[in] dl - left side density
/// \param[in] ul - left side velocity
/// \param[in] pl - left side pressure
/// \param[in] cl - left side sound speed
/// \param[in] dr - right side density
/// \param[in] ur - right side velocity
/// \param[in] pr - right side pressure
/// \param[in] cr - right side sound speed
/// \param[out] pm - pressure
static void
guessp(float dl,
       float ul,
       float pl,
       float cl,
       float dr,
       float ur,
       float pr,
       float cr,
       float &pm)
{
    float cup, gel, ger, pmax, pmin, ppv, pq, ptl, ptr, qmax, quser, um;

    quser = 2.0;

    // Compute guess pressure from PVRS Riemann solver.
    cup = 0.25 * (dl + dr) * (cl + cr);
    ppv = 0.5 * (pl + pr) + 0.5 * (ul - ur) * cup;
    ppv = (ppv > 0.0) ? ppv : 0.0;
    pmin = (pl < pr) ? pl : pr;
    pmax = (pl > pr) ? pl : pr;
    qmax = pmax / pmin;

    if ((qmax <= quser) && (pmin <= ppv) && (ppv <= pmax))
    {
        // Select PVRS Riemann solver.
        pm = ppv;
    }
    else
    {
        if (ppv < pmin)
        {
            // Select Two-Rarefaction Riemann solver.
            pq = pow(pl / pr, G1);
            um = (pq * ul / cl + ur / cr + G4 * (pq - 1.0)) / (pq / cl + 1.0 / cr);
            ptl = 1.0 + G7 * (ul - um) / cl;
            ptr = 1.0 + G7 * (um - ur) / cr;
            pm = 0.5 * (pow(pl * ptl, G3) + pow(pr * ptr, G3));
        }
        else
        {
            // Select Two-Shock Riemann solver with PVRS as estimate.
            gel = sqrt((G5 / dl) / (G6 * pl + ppv));
            ger = sqrt((G5 / dr) / (G6 * pr + ppv));
            pm = (gel * pl + ger * pr - (ur - ul)) / (gel + ger);
        }
    }
}

/// \brief Calculate pressures and derivatives for left and right sides.
///
/// Purpose is to evaluate the pressure functions
/// fl and fr in exact Riemann solver
/// and their first derivatives.
///
/// \param[out] f - pressure function
/// \param[out] fd - pressure derivative
/// \param[in] p - old pressure
/// \param[in] dk - density
/// \param[in] pk - pressure
/// \param[in] ck - sound speed
static void
prefun(float &f,
       float &fd,
       float &p,
       float &dk,
       float &pk,
       float &ck)
{
    float ak, bk, pratio, qrt;

    if (p <= pk)
    {
        // Rarefaction wave.
        pratio = p / pk;
        f = G4 * ck * (pow(pratio, G1) - 1.0);
        fd = (1.0 / (dk * ck)) * pow(pratio, -G2);
    }
    else
    {
        // Shock wave.
        ak = G5 / dk;
        bk = G6 * pk;
        qrt = sqrt(ak / (bk + p));
        f = (p - pk) * qrt;
        fd = (1.0 - 0.5 * (p - pk) / (bk + p)) * qrt;
    }
}

/// \brief Pressure and speed calculation in star region.
///
/// Purpose is to compute the solution for pressure
/// and velocity in the Star Region.
///
/// \param[in] dl - left side density
/// \param[in] ul - left side speed
/// \param[in] pl - left side pressure
/// \param[in] cl - left side sound speed
/// \param[in] dr - right side density
/// \param[in] ur - right side speed
/// \param[in] pr - right side pressure
/// \param[in] cr - right side sound speed
/// \param[out] p - pressure in star region
/// \param[out] u - speed in star region
static void
starpu(float dl,
       float ul,
       float pl,
       float cl,
       float dr,
       float ur,
       float pr,
       float cr,
       float &p,
       float &u)
{
    const int nriter = 20;
    const float tolpre = 1.0e-6;
    float change, fl, fld, fr, frd, pold, pstart, udiff;

    // Guessed value pstart is computed.
    guessp(dl, ul, pl, cl, dr, ur, pr, cr, pstart);
    pold = pstart;
    udiff = ur - ul;

    int i = 1;

    for ( ; i <= nriter; i++)
    {
        prefun(fl, fld, pold, dl, pl, cl);
        prefun(fr, frd, pold, dr, pr, cr);
        p = pold - (fl + fr + udiff) / (fld + frd);
        change = 2.0 * abs((p - pold) / (p + pold));

        if (change <= tolpre)
        {
            break;
        }

        if (p < 0.0)
        {
            p = tolpre;
        }

        pold = p;
    }

    if (i > nriter)
    {
        cout << "divergence in Newton-Raphson iteration" << endl;

        exit(1);
    }

    // compute velocity in star region
    u = 0.5 * (ul + ur + fr - fl);
}

/// \brief Final analyze of the configuration.
///
/// Purpose is to sample the solution throughout the wave
/// pattern. Pressure pm and velocity um in the
/// star region are known. Sampling is performed
/// in terms of the 'speed' s = x/t. Sampled
/// values are d, u, p.
///
/// \param[in] dl - left side density
/// \param[in] ul - left side speed
/// \param[in] pl - left side pressure
/// \param[in] cl - left side sound speed
/// \param[in] dr - right side density
/// \param[in] ur - right side speed
/// \param[in] pr - right side pressure
/// \param[in] cr - right side sound speed
/// \param[in] pm - pressure in star region
/// \param[in] um - speed in star region
/// \param[out] d - density
/// \param[out] u - speed
/// \param[out] p - pressure
static
void sample(float dl,
            float ul,
            float pl,
            float cl,
            float dr,
            float ur,
            float pr,
            float cr,
            const float pm,
            const float um,
            float &d,
            float &u,
            float &p)
{
    float c, cml, cmr, pml, pmr, shl, shr, sl, sr, stl, str;

    if (0.0 <= um)
    {
        // Sampling point lies to the left of the contact discontinuity.
        if (pm <= pl)
        {
            // Left rarefaction.
            shl = ul - cl;

            if (0.0 <= shl)
            {
                // Sampled point is left data state.
                d = dl;
                u = ul;
                p = pl;
            }
            else
            {
                cml = cl * pow(pm / pl, G1);
                stl = um - cml;

                if (0.0 > stl)
                {
                    // Sampled point is star left state.
                    d = dl * pow(pm / pl, 1.0 / GAMA);
                    u = um;
                    p = pm;
                }
                else
                {
                    // Sampled point is inside left fan.
                    u = G5 * (cl + G7 * ul);
                    c = G5 * (cl + G7 * ul);
                    d = dl * pow(c / cl, G4);
                    p = pl * pow(c / cl, G3);
                }
            }
        }
        else
        {
            // Left shock.
            pml = pm / pl;
            sl = ul - cl * sqrt(G2 * pml + G1);

            if (0.0 <= sl)
            {
                // Sampled point is left data state.
                d = dl;
                u = ul;
                p = pl;
            }
            else
            {
                // Sampled point is star left state.
                d = dl * (pml + G6) / (pml * G6 + 1.0);
                u = um;
                p = pm;
            }
        }
    }
    else
    {
        // Sampling point lies to the right of the contact discontinuity.
        if (pm > pr)
        {
            // Right shock.
            pmr = pm / pr;
            sr  = ur + cr * sqrt(G2 * pmr + G1);

            if (0.0 >= sr)
            {
                // Sampled point is right data state.
                d = dr;
                u = ur;
                p = pr;
            }
            else
            {
                // Sampled point is star right state.
                d = dr * (pmr + G6) / (pmr * G6 + 1.0);
                u = um;
                p = pm;
            }
        }
        else
        {
            // Right rarefaction.
            shr = ur + cr;
            if (0.0 >= shr)
            {
                // Sampled point is right data state.
                d = dr;
                u = ur;
                p = pr;
            }
            else
            {
                cmr = cr * pow(pm / pr, G1);
                str = um + cmr;

                if (0.0 <= str)
                {
                    // Sampled point is star right state.
                    d = dr * pow(pm / pr, 1.0 / GAMA);
                    u = um;
                    p = pm;
                }
                else
                {
                    // Sampled point is inside left fan.
                    u = G5 * (-cr + G7 * ur);
                    c = G5 * (cr - G7 * ur);
                    d = dr * pow(c / cr, G4);
                    p = pr * pow(c / cr, G3);
                }
            }
        }
    }
}

/// \brief Riemann solver.
///
/// \param[in] dl - left side density
/// \param[in] ul - left side velocity
/// \param[in] pl - left  side pressure
/// \param[in] dr - right side density
/// \param[in] ur - right side velocity
/// \param[in] pr - right side pressure
/// \param[out] d - result density reference
/// \param[out] u - result velocity reference
/// \param[out] p - result pressure reference
void
riemann(float dl,
        float ul,
        float pl,
        float dr,
        float ur,
        float pr,
        float &d,
        float &u,
        float &p)
{
    float pm, um, cl, cr;

    pm = 0.0;

    // Sound speeds.
    cl = sqrt(GAMA * pl / dl);
    cr = sqrt(GAMA * pr / dr);

    // Check for vacuum.
    if (G4 * (cl + cr) <= (ur - ul))
    {
        cerr << "VACUUM" << endl;
        exit(1);
    }

    // Exact solution.
    starpu(dl, ul, pl, cl, dr, ur, pr, cr, pm, um);
    sample(dl, ul, pl, cl, dr, ur, pr, cr, pm, um, d, u, p);
}

/// \brief Riemann solver.
///
/// \param[in] c - cases count
/// \param[in] dl - left side density
/// \param[in] ul - left side velocity
/// \param[in] pl - left  side pressure
/// \param[in] dr - right side density
/// \param[in] ur - right side velocity
/// \param[in] pr - right side pressure
/// \param[out] d - result density reference
/// \param[out] u - result velocity reference
/// \param[out] p - result pressure reference
void
riemann(int c,
        float *dl,
        float *ul,
        float *pl,
        float *dr,
        float *ur,
        float *pr,
        float *d,
        float *u,
        float *p)
{
    float d_, u_, p_;

    for (int i = 0; i < c; i++)
    {
        riemann(dl[i], ul[i], pl[i], dr[i], ur[i], pr[i], d_, u_, p_);
        d[i] = d_;
        u[i] = u_;
        p[i] = p_;
    }
}
